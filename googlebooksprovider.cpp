#include "googlebooksprovider.h"
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

GoogleBooksProvider::GoogleBooksProvider(QObject *parent) : AbstractBookProvider(parent)
{
    m_networkManager = std::make_unique<QNetworkAccessManager>(this);
    connect(m_networkManager.get(), &QNetworkAccessManager::finished, this, &GoogleBooksProvider::handleReply);
}

GoogleBooksProvider::~GoogleBooksProvider() = default;

void GoogleBooksProvider::requestMetadata(const QString &isbn)
{
    m_activeIsbn = isbn;
    // Public Google Books standard ISBN search syntax endpoint
    QString urlString = QString("https://www.googleapis.com/books/v1/volumes?q=isbn:%1").arg(isbn);
    QNetworkRequest request((QUrl(urlString)));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setHeader(QNetworkRequest::UserAgentHeader, "Qt6ISBNBookScanner/1.0 (GoogleBooks Module)");
    m_networkManager->get(request);
}

void GoogleBooksProvider::handleReply(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const QString message = statusCode == 429
            ? "Google Books rate limit reached (HTTP 429). Try again shortly."
            : statusCode > 0
                ? QString("Google Books request failed (HTTP %1): %2")
                      .arg(statusCode).arg(reply->errorString())
                : "Google Books network request failed: " + reply->errorString();
        qWarning() << "[GoogleBooks System Trace]:" << message;
        emit lookupFailed(message);
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    reply->deleteLater();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isNull() || !doc.isObject()) {
        emit lookupFailed("Malformed JSON response from Google Books.");
        return;
    }

    QJsonObject root = doc.object();
    // Google Books provides hits inside an array block key matching "items"
    if (!root.contains("items") || !root.value("items").isArray()) {
        emit lookupFailed(QString("ISBN %1 not found in Google Books database either.").arg(m_activeIsbn));
        return;
    }

    QJsonObject firstItem = root.value("items").toArray().at(0).toObject();
    QJsonObject volumeInfo = firstItem.value("volumeInfo").toObject();

    BookInfo info;
    info.found = true;
    info.isbn = m_activeIsbn;
    info.title = volumeInfo.value("title").toString("Unknown Title");
    info.engineSource = "Google Books";
    info.publicationDate = volumeInfo.value("publishedDate").toString();
    info.publisher = volumeInfo.value("publisher").toString();
    info.pageCount = volumeInfo.value("pageCount").toInt();

    // Extract temporary string assets locally
    QString urlLarge;
    QString urlMedium;
    QString urlSmall;
    if (volumeInfo.contains("imageLinks") && volumeInfo.value("imageLinks").isObject()) {
        QJsonObject imageLinks = volumeInfo.value("imageLinks").toObject();
        urlLarge = imageLinks.value("extraLarge").toString(
            imageLinks.value("large").toString());
        urlMedium = imageLinks.value("medium").toString(
            imageLinks.value("thumbnail").toString());
        urlSmall = imageLinks.value("small").toString(
            imageLinks.value("smallThumbnail").toString());

        if (urlLarge.startsWith("http://")) urlLarge.replace(0, 7, "https://");
        if (urlMedium.startsWith("http://")) urlMedium.replace(0, 7, "https://");
        if (urlSmall.startsWith("http://")) urlSmall.replace(0, 7, "https://");
    }

    // Google Books parses author tokens as a flat string array list directly inside volumeInfo
    if (volumeInfo.contains("authors") && volumeInfo.value("authors").isArray()) {
        QJsonArray authorsArr = volumeInfo.value("authors").toArray();
        QStringList names;
        for (const QJsonValueRef val : authorsArr) {
            names.append(val.toString());
        }
        info.authors = names.join(", ");
    } else {
        info.authors = "Unknown Author";
    }

    emit lookupFinished(info, urlLarge, urlMedium, urlSmall);
}
