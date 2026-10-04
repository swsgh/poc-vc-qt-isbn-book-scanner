#include "booksyncmanager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QDateTime>
#include <QDebug>

BookSyncManager::BookSyncManager(const QString &serverUrl, QObject *parent)
    : QObject(parent), m_serverUrl(serverUrl), m_lastSyncTimestamp(0)
{
    m_networkManager = new QNetworkAccessManager(this);
}

QNetworkRequest BookSyncManager::createAuthenticatedRequest(const QString &endpointPath)
{
    QNetworkRequest request(QUrl(m_serverUrl + endpointPath));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!m_token.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + m_token.toUtf8());
    }
    return request;
}

// =================================================================
// 1. ACCOUNT INTEGRATION ROUTINES
// =================================================================

void BookSyncManager::registerAccount(const QString &username, const QString &password)
{
    QJsonObject json;
    json["username"] = username;
    json["password"] = password;

    QNetworkRequest request(QUrl(m_serverUrl + "/api/auth/register"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = m_networkManager->post(request, QJsonDocument(json).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            emit authStatusMessage("Account registered successfully! You can now log in.", false);
        } else {
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            QString detail = doc.object().value("detail").toString("Registration failed.");
            emit authStatusMessage(detail, true);
        }
    });
}

void BookSyncManager::loginAccount(const QString &username, const QString &password)
{
    QJsonObject json;
    json["username"] = username;
    json["password"] = password;

    QNetworkRequest request(QUrl(m_serverUrl + "/api/auth/login"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    auto *reply = m_networkManager->post(request, QJsonDocument(json).toJson());
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            emit authStatusMessage("Invalid credentials context verification error.", true);
            return;
        }

        const QByteArray response = reply->readAll();
        const QJsonDocument doc = QJsonDocument::fromJson(response);
        if (doc.isNull() || !doc.isObject()) {
            emit authStatusMessage("Login response was malformed.", true);
            return;
        }

        m_token = doc.object().value("token").toString();
        if (m_token.isEmpty()) {
            emit authStatusMessage("Login succeeded but no token was returned.", true);
            return;
        }

        emit authStatusMessage("Successfully connected to cloud workspace panel.", false);
        emit loginSuccess();
        triggerDifferentialSync();
    });
}

// =================================================================
// 2. DATA SYNCHRONIZATION PIPELINES
// =================================================================

void BookSyncManager::uploadBookToServer(const BookInfo &info)
{
    if (!isAuthenticated()) return;

    // Use MultiPart to handle both text string metadata configurations and binary blobs simultaneously
    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    // Part A: Form text parameter wrapping stringified payload details
    QHttpPart metadataPart;
    metadataPart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"metadata\"");

    QJsonObject metaJson;
    metaJson["isbn"] = info.isbn;
    metaJson["title"] = info.title;
    metaJson["authors"] = info.authors;
    metaJson["engineSource"] = info.engineSource;
    metadataPart.setBody(QJsonDocument(metaJson).toJson(QJsonDocument::Compact));
    multiPart->append(metadataPart);

    // Part B: Raw binary data streaming configuration mapping covers
    if (!info.coverData.isEmpty()) {
        QHttpPart coverPart;
        coverPart.setHeader(QNetworkRequest::ContentTypeHeader, "image/jpeg");
        coverPart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"cover\"; filename=\"cover.jpg\"");
        coverPart.setBody(info.coverData);
        multiPart->append(coverPart);
    }

    QNetworkRequest request = createAuthenticatedRequest("/api/books/upload");
    request.setHeader(QNetworkRequest::ContentTypeHeader, QVariant()); // Multer boundary maps this dynamically

    QNetworkReply *reply = m_networkManager->post(request, multiPart);
    multiPart->setParent(reply); // Memory safety lifecycle hook assignment

    connect(reply, &QNetworkReply::finished, this, [this, reply, info]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            emit uploadSucceeded(info.isbn); // Notify MainWindow to remove from queue
        } else {
            qWarning() << "[Sync Engine] Upload failed, remaining in offline queue:" << reply->errorString();
        }
    });
}

void BookSyncManager::deleteBookFromServer(const QString &isbn)
{
    if (!isAuthenticated()) return;

    QNetworkRequest request = createAuthenticatedRequest("/api/books/delete/" + isbn);
    QNetworkReply *reply = m_networkManager->deleteResource(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, isbn]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            emit deleteSucceeded(isbn); // Notify MainWindow to remove from queue
        } else {
            qWarning() << "[Sync Engine] Deletion failed, remaining in offline queue:" << reply->errorString();
        }
    });
}

void BookSyncManager::triggerDifferentialSync()
{
    if (!isAuthenticated()) return;

    // Build differential URL hook: /api/books/sync?since=17123456
    QString endpoint = QString("/api/books/sync?since=%1").arg(m_lastSyncTimestamp);
    QNetworkRequest request = createAuthenticatedRequest(endpoint);

    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            handleSyncResponse(reply->readAll());
        } else {
            emit networkErrorOccurred("Could not safely perform differential shelf tracking sync logic.");
        }
    });
}

void BookSyncManager::handleSyncResponse(const QByteArray &jsonResponse)
{
    const QJsonDocument doc = QJsonDocument::fromJson(jsonResponse);
    if (doc.isNull() || !doc.isObject()) {
        emit networkErrorOccurred("Received an invalid sync payload from the server.");
        return;
    }

    const QJsonObject obj = doc.object();
    m_lastSyncTimestamp = obj.value("serverTime").toInt(m_lastSyncTimestamp);

    const QJsonArray updatesArray = obj.value("updates").toArray();
    QList<BookInfo> booksToSave;
    QStringList isbnsToDelete;

    for (const QJsonValue &val : updatesArray) {
        QJsonObject bookObj = val.toObject();
        QString isbn = bookObj.value("isbn").toString();
        bool isDeleted = bookObj.value("isDeleted").toBool();

        if (isDeleted) {
            isbnsToDelete.append(isbn);
        } else {
            BookInfo info;
            info.found = true;
            info.isbn = isbn;
            info.title = bookObj.value("title").toString();
            info.authors = bookObj.value("authors").toString();
            info.engineSource = bookObj.value("engineSource").toString();

            // Re-translate the incoming safe Base64 server string field directly back into raw local image binaries
            QString base64Cover = bookObj.value("coverDataBase64").toString();
            info.coverData = QByteArray::fromBase64(base64Cover.toUtf8());

            booksToSave.append(info);
        }
    }

    // Broadcast findings downstream so local repositories synchronize
    if (!booksToSave.isEmpty() || !isbnsToDelete.isEmpty()) {
        emit remoteBookUpdatesDownloaded(booksToSave, isbnsToDelete);
    }
}
