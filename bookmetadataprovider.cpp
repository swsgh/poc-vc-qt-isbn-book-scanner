#include "bookmetadataprovider.h"
#include "openlibraryprovider.h"
#include "googlebooksprovider.h"
#include "bookdatabasemanager.h"
#include "covercache.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QTimer>
#include <QDebug>
#include <algorithm>

BookMetadataProvider::BookMetadataProvider(BookDatabaseManager* dbManager, QObject *parent)
    : QObject(parent)
    , m_isCooldownActive(false)
{
    m_openLibrary = new OpenLibraryProvider(this);
    m_googleBooks = new GoogleBooksProvider(this);
    m_imageNetworkManager = std::make_unique<QNetworkAccessManager>(this);

    connect(m_imageNetworkManager.get(), &QNetworkAccessManager::finished, this, [](QNetworkReply* reply) {
        if (!reply) return;

        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "   -> Error String details:" << reply->errorString();
        }
    });

    connect(m_openLibrary, &AbstractBookProvider::lookupFinished, this, &BookMetadataProvider::handlePrimarySuccess);
    connect(m_openLibrary, &AbstractBookProvider::lookupFailed, this, &BookMetadataProvider::handlePrimaryFailure);
    connect(m_googleBooks, &AbstractBookProvider::lookupFinished, this, &BookMetadataProvider::handleFallbackSuccess);
    connect(m_googleBooks, &AbstractBookProvider::lookupFailed, this, &BookMetadataProvider::handleFallbackFailure);
}

BookMetadataProvider::~BookMetadataProvider() = default;

void BookMetadataProvider::lookupIsbn(const QString &isbn)
{
    if (m_isRefreshingCovers) return;

    const bool isIsbn10 = isbn.length() == 10;
    const bool isIsbn13 = isbn.length() == 13
        && (isbn.startsWith("978") || isbn.startsWith("979"));
    const bool containsOnlyDigits = std::all_of(
        isbn.cbegin(), isbn.cend(), [](QChar character) {
            return character >= QLatin1Char('0') && character <= QLatin1Char('9');
        });
    if ((!isIsbn10 && !isIsbn13) || !containsOnlyDigits) return;
    if (m_isCooldownActive && isbn == m_lastScannedIsbn) return;

    m_isCooldownActive = true;
    m_lastScannedIsbn = isbn;
    m_fallbackUsingGoogle = false;
    m_fallbackUrlSmall.clear();
    m_pendingInfo = BookInfo();

    // =================================================================
    // LOCAL CACHE BYPASS INJECTION: Check SQLite before Web Lookup
    // =================================================================
    if (m_dbManager && m_dbManager->hasBookInLocalDatabase(isbn)) {
        emit lookupStatusChanged(QString("💡 ISBN %1 already exists on shelf.").arg(isbn), false);
        QTimer::singleShot(3000, this, &BookMetadataProvider::resetScannerCooldown);
        return;
    }
    // =================================================================

    // If the book is missing from the database rows, continue to web network fallback cascade routines
    emit lookupStatusChanged(QString("🔍 Digging up metadata for ISBN: %1...").arg(isbn), false);
    m_openLibrary->requestMetadata(isbn);

    QTimer::singleShot(3000, this, &BookMetadataProvider::resetScannerCooldown);
}

void BookMetadataProvider::cacheCoverForBook(const QString &isbn, const QString &coverUrl)
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
    QNetworkReply *reply = m_imageNetworkManager->get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, isbn]() {
        const QByteArray data = reply->readAll();
        const bool cached = reply->error() == QNetworkReply::NoError
            && CoverCache::saveImage(isbn, data);
        reply->deleteLater();
        m_activeCoverDownloads.remove(isbn);
        if (cached) emit coverCached(isbn);
    });
}

void BookMetadataProvider::refreshCoverImages()
{
    if (m_isRefreshingCovers) {
        emit lookupStatusChanged("Cover image refresh is already running.", false);
        return;
    }
    if (m_isCooldownActive) {
        emit lookupStatusChanged("Wait for the current ISBN lookup to finish before refreshing covers.", true);
        return;
    }

    m_coverRefreshIsbns.clear();
    for (const BookInfo &book : m_dbManager->getAllSavedBooks()) {
        if (!book.isbn.isEmpty()) {
            m_coverRefreshIsbns.append(book.isbn);
        }
    }
    m_coverRefreshIndex = 0;
    m_coverRefreshCount = 0;
    m_isRefreshingCovers = true;
    processNextCoverRefresh();
}

void BookMetadataProvider::processNextCoverRefresh()
{
    if (m_coverRefreshIndex >= m_coverRefreshIsbns.size()) {
        m_isRefreshingCovers = false;
        emit lookupStatusChanged(
            QString("Refreshed %1 of %2 cover images.")
                .arg(m_coverRefreshCount).arg(m_coverRefreshIsbns.size()), false);
        emit coverRefreshFinished(m_coverRefreshCount, m_coverRefreshIsbns.size());
        return;
    }

    m_refreshIsbn = m_coverRefreshIsbns.at(m_coverRefreshIndex++);
    m_refreshUsingGoogle = false;
    m_pendingInfo = m_dbManager->getBookByIsbn(m_refreshIsbn);
    if (!m_pendingInfo.found) {
        processNextCoverRefresh();
        return;
    }

    m_fallbackUrlMedium.clear();
    m_fallbackUrlSmall.clear();
    emit lookupStatusChanged(
        QString("Refreshing cover %1 of %2...")
            .arg(m_coverRefreshIndex).arg(m_coverRefreshIsbns.size()), false);
    m_openLibrary->requestMetadata(m_refreshIsbn);
}

void BookMetadataProvider::completeCoverRefresh(bool refreshed)
{
    if (refreshed) {
        ++m_coverRefreshCount;
    }
    QTimer::singleShot(0, this, &BookMetadataProvider::processNextCoverRefresh);
}

void BookMetadataProvider::resetScannerCooldown() { m_isCooldownActive = false; }

void BookMetadataProvider::handlePrimarySuccess(const BookInfo &info, const QString &urlLarge,
                                                const QString &urlMedium, const QString &urlSmall)
{
    if (m_isRefreshingCovers) {
        const BookInfo existing = m_dbManager->getBookByIsbn(m_refreshIsbn);
        if (!existing.found) {
            completeCoverRefresh(false);
            return;
        }
        beginCoverDownload(existing, urlLarge, urlMedium, urlSmall);
    } else {
        beginCoverDownload(info, urlLarge, urlMedium, urlSmall);
    }
}

void BookMetadataProvider::handlePrimaryFailure(const QString &errorMsg)
{
    emit lookupStatusChanged("Not found on Open Library. Querying Google Books fallback engine...", false);
    m_fallbackUsingGoogle = true;
    if (m_isRefreshingCovers) {
        m_refreshUsingGoogle = true;
    }
    m_googleBooks->requestMetadata(m_isRefreshingCovers ? m_refreshIsbn : m_lastScannedIsbn);
}

void BookMetadataProvider::handleFallbackSuccess(const BookInfo &info, const QString &urlLarge,
                                                 const QString &urlMedium, const QString &urlSmall)
{
    if (m_isRefreshingCovers) {
        const BookInfo existing = m_dbManager->getBookByIsbn(m_refreshIsbn);
        if (!existing.found) {
            completeCoverRefresh(false);
            return;
        }
        beginCoverDownload(existing, urlLarge, urlMedium, urlSmall);
    } else {
        beginCoverDownload(info, urlLarge, urlMedium, urlSmall);
    }
}

void BookMetadataProvider::handleFallbackFailure(const QString &errorMsg)
{
    if (m_isRefreshingCovers) {
        completeCoverRefresh(false);
        return;
    }
    // Pass to true so the application controller forwards it to the status area.
    emit lookupStatusChanged(errorMsg, true);
}

void BookMetadataProvider::beginCoverDownload(const BookInfo &info, const QString &urlLarge,
                                              const QString &urlMedium, const QString &urlSmall)
{
    m_pendingInfo = info;
    m_fallbackUrlMedium = urlMedium;
    m_fallbackUrlSmall = urlSmall;

    if (!urlLarge.isEmpty()) {
        downloadCoverImage(urlLarge, "Downloading large cover...", CoverSize::Large);
    } else if (!urlMedium.isEmpty()) {
        downloadCoverImage(urlMedium, "Downloading medium cover...", CoverSize::Medium);
    } else if (!urlSmall.isEmpty()) {
        downloadCoverImage(urlSmall, "Downloading small cover...", CoverSize::Small);
    } else if (m_isRefreshingCovers) {
        if (!m_refreshUsingGoogle) {
            m_refreshUsingGoogle = true;
            m_googleBooks->requestMetadata(m_refreshIsbn);
        } else {
            completeCoverRefresh(false);
        }
    } else if (!m_fallbackUsingGoogle) {
        m_fallbackUsingGoogle = true;
        m_googleBooks->requestMetadata(m_lastScannedIsbn);
    } else {
        emit bookDataReady(m_pendingInfo);
    }
}

void BookMetadataProvider::downloadCoverImage(const QString &url, const QString &statusText,
                                              CoverSize size)
{
    emit lookupStatusChanged(statusText, false);

    QNetworkRequest req((QUrl(url)));
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    req.setHeader(QNetworkRequest::UserAgentHeader, "Qt6ISBNBookScanner/1.0");

    QNetworkReply* reply = m_imageNetworkManager->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, size]() {
        handleCoverDownloadFinished(reply, size);
    });
}

void BookMetadataProvider::handleCoverDownloadFinished(QNetworkReply *reply, CoverSize size)
{
    const QString downloadedUrl = reply->url().toString();
    reply->deleteLater();
    QByteArray data = reply->readAll();

    if (reply->error() == QNetworkReply::NoError
        && CoverCache::saveImage(m_pendingInfo.isbn, data)) {
        m_pendingInfo.coverUrl = downloadedUrl;
        if (m_isRefreshingCovers) {
            const bool updated = m_dbManager->updateBookCoverUrl(m_pendingInfo.isbn, downloadedUrl);
            if (updated) emit coverCached(m_pendingInfo.isbn);
            completeCoverRefresh(updated);
        } else {
            emit bookDataReady(m_pendingInfo);
        }
        return;
    }

    if (size == CoverSize::Large && !m_fallbackUrlMedium.isEmpty()) {
        downloadCoverImage(m_fallbackUrlMedium, "Large cover unavailable. Trying medium cover...",
                           CoverSize::Medium);
        return;
    }
    if (size != CoverSize::Small && !m_fallbackUrlSmall.isEmpty()) {
        downloadCoverImage(m_fallbackUrlSmall, "Medium cover unavailable. Trying small cover...",
                           CoverSize::Small);
        return;
    }

    if (m_isRefreshingCovers) {
        completeCoverRefresh(false);
    } else {
        if (!m_fallbackUsingGoogle) {
            m_fallbackUsingGoogle = true;
            m_googleBooks->requestMetadata(m_lastScannedIsbn);
        } else {
            m_pendingInfo.coverUrl.clear();
            emit bookDataReady(m_pendingInfo);
        }
    }
}

