#include "bookmetadataprovider.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QTimer>

BookMetadataProvider::BookMetadataProvider(QObject *parent)
    : QObject(parent)
    , m_isCooldownActive(false)
{
    m_networkManager = std::make_unique<QNetworkAccessManager>(this);
    connect(m_networkManager.get(), &QNetworkAccessManager::finished,
            this, &BookMetadataProvider::handleNetworkReply);
}

BookMetadataProvider::~BookMetadataProvider() = default;

void BookMetadataProvider::lookupIsbn(const QString &isbn)
{
    if (!isbn.startsWith("978") && !isbn.startsWith("979")) {
        return;
    }

    if (m_isCooldownActive && isbn == m_lastScannedIsbn) {
        return;
    }

    m_isCooldownActive = true;
    m_lastScannedIsbn = isbn;

    emit lookupStatusChanged(QString("ISBN Detected: %1\nFetching data details...").arg(isbn), false);

    // 1. Target the legacy /api/books route using the exact requested parameter footprint
    QString urlString = QString("https://openlibrary.org/api/books?bibkeys=ISBN:%1&jscmd=data&format=json").arg(isbn);
    QUrl url(urlString);
    QNetworkRequest request(url);

    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setHeader(QNetworkRequest::UserAgentHeader, "Qt6ISBNBookScanner/1.0 (Local Dev Client)");
    request.setRawHeader("Accept", "application/json");

    m_networkManager->get(request);

    QTimer::singleShot(3000, this, &BookMetadataProvider::resetScannerCooldown);
}

void BookMetadataProvider::resetScannerCooldown()
{
    m_isCooldownActive = false;
}

void BookMetadataProvider::handleNetworkReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        emit lookupStatusChanged("Network Error:\n" + reply->errorString(), true);
        reply->deleteLater();
        return;
    }

    QByteArray responseData = reply->readAll();
    reply->deleteLater();

    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
    if (jsonDoc.isNull() || !jsonDoc.isObject()) {
        emit lookupStatusChanged("Failed to parse metadata payload JSON structure.", true);
        return;
    }

    QJsonObject rootObject = jsonDoc.object();

    // 2. Compute the lookup key wrapper required by the /api/books payload structure
    QString lookupKey = "ISBN:" + m_lastScannedIsbn;

    // If the key is completely missing or the object body is empty {}, it's a 404/Not Found scenario
    if (!rootObject.contains(lookupKey)) {
        emit lookupStatusChanged(QString("ISBN: %1\nNot found in Open Library database.").arg(m_lastScannedIsbn), true);
        return;
    }

    QJsonObject bookData = rootObject.value(lookupKey).toObject();

    BookInfo info;
    info.found = true;
    info.isbn = m_lastScannedIsbn;
    info.title = bookData.value("title").toString("Unknown Title");

    // 3. Extract author names out of the nested array object
    if (bookData.contains("authors") && bookData.value("authors").isArray()) {
        QJsonArray authorsArray = bookData.value("authors").toArray();
        QStringList authorsNames;

        for (const QJsonValueRef authorVal : authorsArray) {
            QJsonObject authorObj = authorVal.toObject();
            if (authorObj.contains("name")) {
                authorsNames.append(authorObj.value("name").toString());
            }
        }

        info.authors = authorsNames.isEmpty() ? "Unknown Author" : authorsNames.join(", ");
    }
    else if (bookData.contains("by_statement")) {
        info.authors = bookData.value("by_statement").toString();
    }
    else {
        info.authors = "Unknown Author";
    }

    emit bookDataReady(info);
}
