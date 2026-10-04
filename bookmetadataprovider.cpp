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

void BookMetadataProvider::resetScannerCooldown() { m_isCooldownActive = false; }

void BookMetadataProvider::handlePrimarySuccess(const BookInfo &info, const QString &urlSmall, const QString &urlMedium)
{
    m_pendingInfo = info;
    m_fallbackUrlSmall = urlSmall; // Store in case medium asset download fails
    downloadMediumCover(urlMedium);
}

void BookMetadataProvider::handlePrimaryFailure(const QString &errorMsg)
{
    emit lookupStatusChanged("Not found on Open Library. Querying Google Books fallback engine...", false);
    m_googleBooks->requestMetadata(m_lastScannedIsbn);
}

void BookMetadataProvider::handleFallbackSuccess(const BookInfo &info, const QString &urlSmall, const QString &urlMedium)
{
    m_pendingInfo = info;
    m_fallbackUrlSmall = urlSmall;

    if (urlMedium.isEmpty()) {
        if (urlSmall.isEmpty()) {
            emit bookDataReady(m_pendingInfo); // No covers available, emit immediately
        } else {
            downloadSmallCover(urlSmall);
        }
    } else {
        downloadMediumCover(urlMedium);
    }
}

void BookMetadataProvider::handleFallbackFailure(const QString &errorMsg)
{
    // Pass to true so the application controller forwards it to the status area.
    emit lookupStatusChanged(errorMsg, true);
}

void BookMetadataProvider::downloadCoverImage(const QString &url, const QString &statusText, bool isMedium)
{
    emit lookupStatusChanged(statusText, false);
    m_pendingInfo.coverUrl = url;

    QNetworkRequest req((QUrl(url)));
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    req.setHeader(QNetworkRequest::UserAgentHeader, "Qt6ISBNBookScanner/1.0");

    QNetworkReply* reply = m_imageNetworkManager->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply, isMedium]() {
        handleCoverDownloadFinished(reply, isMedium);
    });
}

void BookMetadataProvider::handleCoverDownloadFinished(QNetworkReply* reply, bool isMedium)
{
    reply->deleteLater();
    QByteArray data = reply->readAll();

    if (reply->error() == QNetworkReply::NoError
        && CoverCache::saveImage(m_pendingInfo.isbn, data)) {
        if (isMedium) {
            emit bookDataReady(m_pendingInfo);
            return;
        }
        emit bookDataReady(m_pendingInfo);
        return;
    }

    if (isMedium && !m_fallbackUrlSmall.isEmpty()) {
        downloadCoverImage(m_fallbackUrlSmall, "Medium cover missing. Falling back to small cover layout...", false);
        return;
    }

    m_pendingInfo.coverUrl.clear();
    emit bookDataReady(m_pendingInfo);
}

void BookMetadataProvider::downloadMediumCover(const QString &urlMedium)
{
    downloadCoverImage(urlMedium, "Attempting to download medium cover file...", true);
}

void BookMetadataProvider::downloadSmallCover(const QString &urlSmall)
{
    downloadCoverImage(urlSmall, "Medium cover missing. Falling back to small cover layout...", false);
}
