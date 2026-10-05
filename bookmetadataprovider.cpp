#include "bookmetadataprovider.h"
#include "bookdatabasemanager.h"
#include "covercache.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QTimer>
#include <QDebug>

BookMetadataProvider::BookMetadataProvider(BookDatabaseManager* dbManager, QObject *parent)
    : QObject(parent)
    , m_dbManager(dbManager)
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
        const bool refreshing = m_isRefreshingCovers && m_currentRefreshIsbn == isbn;
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
        if (refreshing) finishCoverRefreshItem(isbn, cached);
    });
}

void BookMetadataProvider::refreshCoverImages(const QString &serverUrl, const QString &authToken)
{
    if (m_isRefreshingCovers) {
        emit lookupStatusChanged("Local cover cache refresh is already running.", false);
        return;
    }

    m_coverRefreshIsbns.clear();
    for (const BookInfo &book : m_dbManager->getAllSavedBooks()) {
        const QUrl coverUrl(book.coverUrl);
        const QString scheme = coverUrl.scheme().toLower();
        const QString expectedUrl = serverUrl + "/api/books/cover/"
            + QString::fromLatin1(QUrl::toPercentEncoding(book.isbn));
        if (!book.isbn.isEmpty() && book.coverUrl == expectedUrl
            && !m_activeCoverDownloads.contains(book.isbn)
            && coverUrl.isValid() && !coverUrl.host().isEmpty()
            && (scheme == "http" || scheme == "https")) {
            m_coverRefreshIsbns.append(book.isbn);
        }
    }
    m_coverRefreshIndex = 0;
    m_coverRefreshCount = 0;
    m_coverRefreshAuthToken = authToken;
    m_isRefreshingCovers = true;
    if (m_coverRefreshIsbns.isEmpty()) {
        m_isRefreshingCovers = false;
        emit lookupStatusChanged("No server-cached covers are available to refresh.", false);
        emit coverRefreshFinished(0, 0);
        return;
    }
    processNextCoverRefresh();
}

void BookMetadataProvider::processNextCoverRefresh()
{
    if (m_coverRefreshIndex >= m_coverRefreshIsbns.size()) {
        m_isRefreshingCovers = false;
        emit lookupStatusChanged(
            QString("Refreshed %1 of %2 local cover images.")
                .arg(m_coverRefreshCount).arg(m_coverRefreshIsbns.size()), false);
        emit coverRefreshFinished(m_coverRefreshCount, m_coverRefreshIsbns.size());
        return;
    }

    m_currentRefreshIsbn = m_coverRefreshIsbns.at(m_coverRefreshIndex++);
    const BookInfo book = m_dbManager->getBookByIsbn(m_currentRefreshIsbn);
    if (!book.found || book.coverUrl.isEmpty()) {
        finishCoverRefreshItem(m_currentRefreshIsbn, false);
        return;
    }

    CoverCache::removeImage(m_currentRefreshIsbn);
    emit lookupStatusChanged(
        QString("Refreshing local cover %1 of %2...")
            .arg(m_coverRefreshIndex).arg(m_coverRefreshIsbns.size()), false);
    cacheCoverForBook(book.isbn, book.coverUrl, m_coverRefreshAuthToken);
}

void BookMetadataProvider::finishCoverRefreshItem(const QString &isbn, bool refreshed)
{
    if (m_currentRefreshIsbn != isbn) return;
    if (refreshed) ++m_coverRefreshCount;
    m_currentRefreshIsbn.clear();
    QTimer::singleShot(0, this, &BookMetadataProvider::processNextCoverRefresh);
}


