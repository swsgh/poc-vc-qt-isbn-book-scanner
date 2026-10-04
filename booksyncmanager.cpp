#include "booksyncmanager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QDateTime>
#include <QDebug>
#include <QUrl>

namespace {
QNetworkReply *postJsonRequest(QNetworkAccessManager *networkManager,
                               const QNetworkRequest &request,
                               const QJsonObject &payload)
{
    return networkManager->post(request, QJsonDocument(payload).toJson());
}

QString replyErrorMessage(QNetworkReply *reply, const QString &fallback)
{
    const QJsonDocument document = QJsonDocument::fromJson(reply->readAll());
    const QString detail = document.object().value("detail").toString();
    return detail.isEmpty() ? (reply->errorString().isEmpty() ? fallback : reply->errorString())
                            : detail;
}
}

BookSyncManager::BookSyncManager(const QString &serverUrl, QObject *parent)
    : QObject(parent), m_serverUrl(serverUrl), m_lastSyncTimestamp(0)
{
    m_networkManager = new QNetworkAccessManager(this);
}

void BookSyncManager::setServerUrl(const QString &serverUrl)
{
    m_serverUrl = serverUrl.trimmed();
    while (m_serverUrl.endsWith('/')) {
        m_serverUrl.chop(1);
    }
}

QNetworkRequest BookSyncManager::createJsonRequest(const QString &endpointPath) const
{
    QNetworkRequest request(QUrl(m_serverUrl + endpointPath));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    return request;
}

QNetworkRequest BookSyncManager::createAuthenticatedRequest(const QString &endpointPath)
{
    QNetworkRequest request = createJsonRequest(endpointPath);
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

    QNetworkRequest request = createJsonRequest("/api/auth/register");

    QNetworkReply *reply = postJsonRequest(m_networkManager, request, json);
    connect(reply, &QNetworkReply::finished, this, [this, reply, username, password]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            loginAccount(username, password);
        } else {
            emit authStatusMessage(
                "Sync failed: " + replyErrorMessage(reply, "Registration failed."), true);
        }
    });
}

void BookSyncManager::loginAccount(const QString &username, const QString &password)
{
    QJsonObject json;
    json["username"] = username;
    json["password"] = password;

    QNetworkRequest request = createJsonRequest("/api/auth/login");

    QNetworkReply *reply = postJsonRequest(m_networkManager, request, json);
    connect(reply, &QNetworkReply::finished, this, [this, reply, username]() {
        reply->deleteLater();

        if (reply->error() != QNetworkReply::NoError) {
            emit authStatusMessage("Sync failed: " + replyErrorMessage(reply, "Login failed."), true);
            return;
        }

        const QByteArray response = reply->readAll();
        const QJsonDocument doc = QJsonDocument::fromJson(response);
        if (doc.isNull() || !doc.isObject()) {
            emit authStatusMessage("Sync failed: login response was malformed.", true);
            return;
        }

        m_token = doc.object().value("token").toString();
        if (m_token.isEmpty()) {
            emit authStatusMessage("Sync failed: the login response did not include a token.", true);
            return;
        }

        m_username = username;
        emit authStatusMessage(QString("Signed in to sync as %1.").arg(username), false);
        emit loginSuccess();
        triggerDifferentialSync();
    });
}

void BookSyncManager::logoutAccount()
{
    m_token.clear();
    m_username.clear();
    m_lastSyncTimestamp = 0;
    m_hasSyncCheckpoint = false;
    emit authStatusMessage("Signed out of sync.", false);
}

void BookSyncManager::setSyncCheckpoint(qint64 timestamp, bool hasCheckpoint)
{
    m_lastSyncTimestamp = timestamp;
    m_hasSyncCheckpoint = hasCheckpoint;
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
            emit networkErrorOccurred(replyErrorMessage(reply, "Book upload failed."));
        }
    });
}

void BookSyncManager::deleteBookFromServer(const QString &isbn)
{
    if (!isAuthenticated()) return;

    const QString encodedIsbn = QString::fromLatin1(QUrl::toPercentEncoding(isbn));
    QNetworkRequest request = createAuthenticatedRequest("/api/books/delete/" + encodedIsbn);
    QNetworkReply *reply = m_networkManager->deleteResource(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, isbn]() {
        reply->deleteLater();
        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (reply->error() == QNetworkReply::NoError || statusCode == 404) {
            emit deleteSucceeded(isbn); // Notify MainWindow to remove from queue
        } else {
            qWarning() << "[Sync Engine] Deletion failed, remaining in offline queue:" << reply->errorString();
            emit networkErrorOccurred(replyErrorMessage(reply, "Book deletion failed."));
        }
    });
}

void BookSyncManager::triggerDifferentialSync()
{
    if (!isAuthenticated() || m_syncRequestInFlight) return;

    const qint64 since = m_hasSyncCheckpoint ? m_lastSyncTimestamp : 0;
    const QString endpoint = QString("/api/books/sync?since=%1").arg(since);
    QNetworkRequest request = createAuthenticatedRequest(endpoint);

    m_syncRequestInFlight = true;
    QNetworkReply *reply = m_networkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        m_syncRequestInFlight = false;
        if (reply->error() == QNetworkReply::NoError) {
            handleSyncResponse(reply->readAll());
        } else {
            emit networkErrorOccurred(
                replyErrorMessage(reply, "Could not complete bookshelf synchronization."));
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
    if (!obj.value("serverTime").isDouble() || !obj.value("updates").isArray()) {
        emit networkErrorOccurred("Received an incomplete sync payload from the server.");
        return;
    }

    const qint64 serverTime = obj.value("serverTime").toVariant().toLongLong();
    const bool initialSync = !m_hasSyncCheckpoint;
    const qint64 checkpoint = qMax<qint64>(0, serverTime - 1);

    const QJsonArray updatesArray = obj.value("updates").toArray();
    QList<BookInfo> booksToSave;
    QStringList isbnsToDelete;
    QStringList remoteIsbns;

    for (const QJsonValue &val : updatesArray) {
        QJsonObject bookObj = val.toObject();
        QString isbn = bookObj.value("isbn").toString();
        if (!isbn.isEmpty()) {
            remoteIsbns.append(isbn);
        }
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

    m_lastSyncTimestamp = checkpoint;
    m_hasSyncCheckpoint = true;
    emit syncCompleted(m_username, checkpoint, initialSync, remoteIsbns);
}
