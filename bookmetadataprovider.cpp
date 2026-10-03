#include "bookmetadataprovider.h"
#include "openlibraryprovider.h"
#include "googlebooksprovider.h"
#include "bookdatabasemanager.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QTimer>

BookMetadataProvider::BookMetadataProvider(BookDatabaseManager* dbManager, QObject *parent)
    : QObject(parent)
    , m_isCooldownActive(false)
{
    m_openLibrary = new OpenLibraryProvider(this);
    m_googleBooks = new GoogleBooksProvider(this);
    m_imageNetworkManager = std::make_unique<QNetworkAccessManager>(this);

    connect(m_openLibrary, &AbstractBookProvider::lookupFinished, this, &BookMetadataProvider::handlePrimarySuccess);
    connect(m_openLibrary, &AbstractBookProvider::lookupFailed, this, &BookMetadataProvider::handlePrimaryFailure);
    connect(m_googleBooks, &AbstractBookProvider::lookupFinished, this, &BookMetadataProvider::handleFallbackSuccess);
    connect(m_googleBooks, &AbstractBookProvider::lookupFailed, this, &BookMetadataProvider::handleFallbackFailure);
}

BookMetadataProvider::~BookMetadataProvider() = default;

void BookMetadataProvider::lookupIsbn(const QString &isbn)
{
    if (!isbn.startsWith("978") && !isbn.startsWith("979")) return;
    if (m_isCooldownActive && isbn == m_lastScannedIsbn) return;

    m_isCooldownActive = true;
    m_lastScannedIsbn = isbn;
    m_fallbackUrlSmall.clear();
    m_pendingInfo = BookInfo();

    // =================================================================
    // LOCAL CACHE BYPASS INJECTION: Check SQLite before Web Lookup
    // =================================================================
    if (m_dbManager && m_dbManager->hasBookInLocalDatabase(isbn)) {
        emit lookupStatusChanged(QString("ISBN %1 matched locally. Loading from SQLite offline cache...").arg(isbn), false);

        // Extract the complete, fully formed data record directly from the database manager
        BookInfo cachedBook = m_dbManager->getBookByIsbn(isbn);

        if (cachedBook.found) {
            // Identify engine source source configuration markers dynamically
            cachedBook.engineSource += " (Local Offline Database Cache)";

            // Emit the complete book data and stop execution to block web network traffic completely
            emit bookDataReady(cachedBook);

            // Start the standard 3-second cooldown timer before returning
            QTimer::singleShot(3000, this, &BookMetadataProvider::resetScannerCooldown);
            return;
        }
    }
    // =================================================================

    // If the book is missing from the database rows, continue to web network fallback cascade routines
    emit lookupStatusChanged(QString("Searching Open Library for ISBN: %1...").arg(isbn), false);
    m_openLibrary->requestMetadata(isbn);

    QTimer::singleShot(3000, this, &BookMetadataProvider::resetScannerCooldown);
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
    emit lookupStatusChanged(errorMsg, true);
}

void BookMetadataProvider::downloadMediumCover(const QString &urlMedium)
{
    emit lookupStatusChanged("Attempting to download medium cover file...", false);

    QNetworkRequest req((QUrl(urlMedium)));
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    req.setHeader(QNetworkRequest::UserAgentHeader, "Qt6ISBNBookScanner/1.0");

    QNetworkReply* reply = m_imageNetworkManager->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { handleMediumCoverFinished(reply); });
}

void BookMetadataProvider::handleMediumCoverFinished(QNetworkReply* reply)
{
    reply->deleteLater();
    QByteArray data = reply->readAll();

    // Open Library serves a tiny blank 1x1 tracking pixel (~43 bytes) if no medium cover matches.
    // Check if the download succeeded and contains true image asset size parameters.
    if (reply->error() == QNetworkReply::NoError && data.size() > 100) {
        m_pendingInfo.coverData = data;
        emit bookDataReady(m_pendingInfo);
    } else {
        // Medium failed or is a blank pixel, cascade to small cover fallback execution
        if (!m_fallbackUrlSmall.isEmpty()) {
            downloadSmallCover(m_fallbackUrlSmall);
        } else {
            emit bookDataReady(m_pendingInfo);
        }
    }
}

void BookMetadataProvider::downloadSmallCover(const QString &urlSmall)
{
    emit lookupStatusChanged("Medium cover missing. Falling back to small cover layout...", false);

    QNetworkRequest req((QUrl(urlSmall)));
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply* reply = m_imageNetworkManager->get(req);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { handleSmallCoverFinished(reply); });
}

void BookMetadataProvider::handleSmallCoverFinished(QNetworkReply* reply)
{
    reply->deleteLater();
    QByteArray data = reply->readAll();

    if (reply->error() == QNetworkReply::NoError && data.size() > 100) {
        m_pendingInfo.coverData = data;
    }
    emit bookDataReady(m_pendingInfo);
}
