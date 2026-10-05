#include "bookmetadataprovider.h"
#include "covercache.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QDebug>

BookMetadataProvider::BookMetadataProvider(QObject *parent)
    : QObject(parent)
{
    m_imageNetworkManager = std::make_unique<QNetworkAccessManager>(this);
}

BookMetadataProvider::~BookMetadataProvider() = default;

void BookMetadataProvider::cacheCoverForBook(const QString &isbn, const QString &coverUrl,
                                             const QString &authToken)
{
    if (isbn.isEmpty() || coverUrl.isEmpty() || CoverCache::contains(isbn)
        || m_activeCoverDownloads.contains(isbn)) {
        return;
    }

    const QUrl url(coverUrl);
    const QString scheme = url.scheme().toLower();
    if (!url.isValid() || url.host().isEmpty()
        || (scheme != "http" && scheme != "https")) {
        return;
    }

    m_activeCoverDownloads.insert(isbn);
    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setHeader(QNetworkRequest::UserAgentHeader, "Qt6ISBNBookScanner/1.0");
    if (!authToken.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + authToken.toUtf8());
    }
    QNetworkReply *reply = m_imageNetworkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, isbn]() {
        const QByteArray data = reply->readAll();
        const bool cached = reply->error() == QNetworkReply::NoError
            && CoverCache::saveImage(isbn, data);
        if (!cached) {
            const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            const QString message = statusCode == 429
                ? "Cover image rate limit reached (HTTP 429)."
                : statusCode > 0
                    ? QString("Cover image request failed (HTTP %1): %2")
                          .arg(statusCode).arg(reply->errorString())
                    : "Cover image download failed: " + reply->errorString();
            emit lookupStatusChanged(message, true);
        }
        reply->deleteLater();
        m_activeCoverDownloads.remove(isbn);
        if (cached) emit coverCached(isbn);
    });
}


