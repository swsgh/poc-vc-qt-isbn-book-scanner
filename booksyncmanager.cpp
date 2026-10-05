#include "booksyncmanager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QHttpMultiPart>
#include <QHttpPart>
#include <QDateTime>
#include <QDebug>
#include <QUrl>
#include <QTimer>

namespace {
QNetworkReply *postJsonRequest(QNetworkAccessManager *networkManager,
                               const QNetworkRequest &request,
                               const QJsonObject &payload)
{
    return networkManager->post(request, QJsonDocument(payload).toJson());
}

QString replyErrorMessage(QNetworkReply *reply, const QString &fallback)
{
    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (statusCode == 429) {
        return "Too many requests (HTTP 429). Wait a moment before trying again.";
    }

    const QJsonDocument document = QJsonDocument::fromJson(reply->readAll());
    const QString detail = document.object().value("detail").toString();
    if (statusCode > 0) {
        const QString statusMessage = QString("HTTP %1").arg(statusCode);
        return detail.isEmpty() ? statusMessage + ": " + reply->errorString()
                                : statusMessage + ": " + detail;
    }
    return detail.isEmpty() ? (reply->errorString().isEmpty() ? fallback : reply->errorString())
                            : detail;
}
}

BookSyncManager::BookSyncManager(const QString &serverUrl, QObject *parent)
    : QObject(parent), m_serverUrl(serverUrl), m_lastSyncTimestamp(0)
{
    m_networkManager = new QNetworkAccessManager(this);
    m_healthCheckTimer = new QTimer(this);
    m_healthCheckTimer->setInterval(30000);
    connect(m_healthCheckTimer, &QTimer::timeout,
            this, &BookSyncManager::checkServerConnection);
}

void BookSyncManager::setServerUrl(const QString &serverUrl)
{
    m_serverUrl = serverUrl.trimmed();
    while (m_serverUrl.endsWith('/')) {
        m_serverUrl.chop(1);
    }
    checkServerConnection();
}

void BookSyncManager::checkServerConnection()
{
    if (!isAuthenticated() || m_healthCheckInFlight) return;

    m_healthCheckInFlight = true;
    const QString checkedServerUrl = m_serverUrl;
    QNetworkReply *reply = m_networkManager->get(createJsonRequest("/health"));
    connect(reply, &QNetworkReply::finished, this, [this, reply, checkedServerUrl]() {
        m_healthCheckInFlight = false;
        const QJsonDocument response = QJsonDocument::fromJson(reply->readAll());
        const bool connected = reply->error() == QNetworkReply::NoError
            && response.isObject()
            && response.object().value("status").toString() == "ok";
        reply->deleteLater();

        if (!isAuthenticated()) return;
        if (checkedServerUrl != m_serverUrl) {
            checkServerConnection();
            return;
        }

        if (!m_hasServerConnectionResult || m_serverConnected != connected) {
            m_hasServerConnectionResult = true;
            m_serverConnected = connected;
            emit serverConnectionChanged(connected);
        }
    });
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
        m_healthCheckTimer->start();
        checkServerConnection();
        emit authStatusMessage(QString("Signed in to sync as %1.").arg(username), false);
        emit loginSuccess();
        triggerDifferentialSync();
    });
}

void BookSyncManager::logoutAccount()
{
    m_healthCheckTimer->stop();
    m_token.clear();
    m_username.clear();
    m_lastSyncTimestamp = 0;
    m_hasSyncCheckpoint = false;
    m_hasServerConnectionResult = false;
    m_serverConnected = false;
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

void BookSyncManager::lookupBookByIsbn(const QString &isbn)
{
    if (!isAuthenticated()) {
        emit bookLookupFailed("Sign in to the sync server before looking up books.");
        return;
    }

    QJsonObject payload;
    payload["isbn"] = isbn;
    const QNetworkRequest request = createAuthenticatedRequest("/api/books/lookup");
    QNetworkReply *reply = postJsonRequest(m_networkManager, request, payload);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        if (reply->error() != QNetworkReply::NoError) {
            const QString message = replyErrorMessage(reply, "Book lookup failed.");
            reply->deleteLater();
            emit bookLookupFailed(message);
            return;
        }

        const QJsonDocument document = QJsonDocument::fromJson(reply->readAll());
        reply->deleteLater();
        if (!document.isObject()) {
            emit bookLookupFailed("The server returned an invalid book response.");
            return;
        }

        const QJsonObject object = document.object();
        BookInfo info;
        info.found = true;
        info.isbn = object.value("isbn").toString();
        info.title = object.value("title").toString();
        info.authors = object.value("authors").toString();
        info.coverUrl = QUrl(m_serverUrl + "/").resolved(
            QUrl(object.value("coverUrl").toString())).toString();
        info.publicationDate = object.value("publicationDate").toString();
        info.publisher = object.value("publisher").toString();
        info.pageCount = object.value("pageCount").toInt();
        if (info.isbn.isEmpty() || info.title.isEmpty()) {
            emit bookLookupFailed("The server response is missing the book ISBN or title.");
            return;
        }

        const QJsonArray warnings = object.value("warnings").toArray();
        for (const QJsonValue &warning : warnings) {
            if (warning.isString() && !warning.toString().isEmpty()) {
                emit bookLookupWarning(warning.toString());
            }
        }
        emit bookLookupSucceeded(info);
    });
}

void BookSyncManager::uploadBookToServer(const BookInfo &info)
{
    if (!isAuthenticated()) return;

    // Keep the existing multipart form contract, but send cover URLs instead of image bytes.
    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    // Part A: Form text parameter wrapping stringified payload details
    QHttpPart metadataPart;
    metadataPart.setHeader(QNetworkRequest::ContentDispositionHeader, "form-data; name=\"metadata\"");

    QJsonObject metaJson;
    metaJson["isbn"] = info.isbn;
    metaJson["title"] = info.title;
    metaJson["authors"] = info.authors;
    metaJson["coverUrl"] = info.coverUrl;
    metaJson["publicationDate"] = info.publicationDate;
    metaJson["publisher"] = info.publisher;
    metaJson["pageCount"] = info.pageCount;
    metadataPart.setBody(QJsonDocument(metaJson).toJson(QJsonDocument::Compact));
    multiPart->append(metadataPart);

    QNetworkRequest request = createAuthenticatedRequest("/api/books/upload");
    request.setHeader(QNetworkRequest::ContentTypeHeader, QVariant());

    QNetworkReply *reply = m_networkManager->post(request, multiPart);
    multiPart->setParent(reply); // Memory safety lifecycle hook assignment

    connect(reply, &QNetworkReply::finished, this, [this, reply, info]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            emit uploadSucceeded(info.isbn); // Notify the coordinator to clear the queue entry.
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
            emit deleteSucceeded(isbn); // Notify the coordinator to clear the queue entry.
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
            info.coverUrl = bookObj.value("coverUrl").toString();
            info.publicationDate = bookObj.value("publicationDate").toString();
            info.publisher = bookObj.value("publisher").toString();
            info.pageCount = bookObj.value("pageCount").toInt();

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
