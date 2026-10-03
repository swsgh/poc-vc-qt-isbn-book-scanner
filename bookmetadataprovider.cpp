#include "bookmetadataprovider.h"
#include "openlibraryprovider.h"
#include "googlebooksprovider.h"
#include <QTimer>

BookMetadataProvider::BookMetadataProvider(QObject *parent)
    : QObject(parent)
    , m_isCooldownActive(false)
{
    m_openLibrary = new OpenLibraryProvider(this);
    m_googleBooks = new GoogleBooksProvider(this);

    // Open Library Signal Mappings
    connect(m_openLibrary, &AbstractBookProvider::lookupFinished, this, &BookMetadataProvider::handlePrimarySuccess);
    connect(m_openLibrary, &AbstractBookProvider::lookupFailed, this, &BookMetadataProvider::handlePrimaryFailure);

    // Google Books Fallback Signal Mappings
    connect(m_googleBooks, &AbstractBookProvider::lookupFinished, this, &BookMetadataProvider::handleFallbackSuccess);
    connect(m_googleBooks, &AbstractBookProvider::lookupFailed, this, &BookMetadataProvider::handleFallbackFailure);
}

void BookMetadataProvider::lookupIsbn(const QString &isbn)
{
    if (!isbn.startsWith("978") && !isbn.startsWith("979")) return;
    if (m_isCooldownActive && isbn == m_lastScannedIsbn) return;

    m_isCooldownActive = true;
    m_lastScannedIsbn = isbn;

    emit lookupStatusChanged(QString("Searching Open Library for ISBN: %1...").arg(isbn), false);

    // Execute Primary Provider Lookups
    m_openLibrary->requestMetadata(isbn);

    QTimer::singleShot(3000, this, &BookMetadataProvider::resetScannerCooldown);
}

void BookMetadataProvider::resetScannerCooldown() { m_isCooldownActive = false; }

void BookMetadataProvider::handlePrimarySuccess(const BookInfo &info)
{
    emit bookDataReady(info);
}

void BookMetadataProvider::handlePrimaryFailure(const QString &errorMsg)
{
    // If Open Library fails or returns empty metadata parameters, seamlessly fall back to Google Books!
    emit lookupStatusChanged("Not found on Open Library. Querying Google Books fallback engine...", false);
    m_googleBooks->requestMetadata(m_lastScannedIsbn);
}

void BookMetadataProvider::handleFallbackSuccess(const BookInfo &info)
{
    emit bookDataReady(info);
}

void BookMetadataProvider::handleFallbackFailure(const QString &errorMsg)
{
    emit lookupStatusChanged(errorMsg, true);
}
