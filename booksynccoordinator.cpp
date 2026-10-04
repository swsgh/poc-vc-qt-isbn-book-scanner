#include "booksynccoordinator.h"

#include "bookdatabasemanager.h"
#include "bookdetailssidebar.h"
#include "bookmetadataprovider.h"
#include "bookshelfwidget.h"
#include "booksyncmanager.h"
#include "covercache.h"

#include <QSet>

BookSyncCoordinator::BookSyncCoordinator(BookDatabaseManager *database,
                                         BookSyncManager *syncManager,
                                         BookMetadataProvider *metadataProvider,
                                         BookshelfWidget *bookshelf,
                                         BookDetailsSidebar *details,
                                         QObject *parent)
    : QObject(parent),
      m_database(database),
      m_syncManager(syncManager),
      m_metadataProvider(metadataProvider),
      m_bookshelf(bookshelf),
      m_details(details)
{
    connect(m_database, &BookDatabaseManager::bookSavedSuccessfully,
            this, &BookSyncCoordinator::handleBookSaved);
    connect(m_syncManager, &BookSyncManager::uploadSucceeded, this,
            [this](const QString &isbn) {
                m_database->removePendingAction(isbn, "UPLOAD");
            });
    connect(m_syncManager, &BookSyncManager::deleteSucceeded, this,
            [this](const QString &isbn) {
                m_database->removePendingAction(isbn, "DELETE");
            });
    connect(m_syncManager, &BookSyncManager::loginSuccess,
            this, &BookSyncCoordinator::beginSync);
    connect(m_syncManager, &BookSyncManager::remoteBookUpdatesDownloaded,
            this, &BookSyncCoordinator::handleRemoteBookUpdates);
    connect(m_syncManager, &BookSyncManager::syncCompleted,
            this, &BookSyncCoordinator::handleSyncCompleted);
}

void BookSyncCoordinator::beginSync()
{
    m_downloadedCount = 0;
    m_removedCount = 0;
}

void BookSyncCoordinator::flushQueue()
{
    for (const QString &isbn : m_database->getPendingDeletes()) {
        m_syncManager->deleteBookFromServer(isbn);
    }

    for (const QString &isbn : m_database->getPendingUploads()) {
        const BookInfo book = m_database->getBookByIsbn(isbn);
        if (book.found) {
            m_syncManager->uploadBookToServer(book);
        } else {
            m_database->removePendingAction(isbn, "UPLOAD");
        }
    }
}

bool BookSyncCoordinator::removeBook(const QString &isbn)
{
    if (!m_database->deleteBookRecord(isbn)) {
        emit statusMessage("Failed to remove book from local database storage hierarchy.", true);
        return false;
    }

    m_bookshelf->removeBookFromShelf(isbn);
    m_details->closeSidebar();
    CoverCache::removeImage(isbn);
    m_database->addPendingDelete(isbn);
    m_syncManager->deleteBookFromServer(isbn);
    emit statusMessage("🗑️ Book removed from collection.", false);
    return true;
}

void BookSyncCoordinator::handleBookSaved(const QString &isbn)
{
    const BookInfo info = m_database->getBookByIsbn(isbn);
    if (!info.found) return;

    m_bookshelf->addBookToShelf(info, true);
    m_database->addPendingUpload(isbn);
    m_syncManager->uploadBookToServer(info);
    emit statusMessage(QString("✅ Logged: %1").arg(info.title), false);
}

void BookSyncCoordinator::handleRemoteBookUpdates(const QList<BookInfo> &booksToSave,
                                                  const QStringList &isbnsToDelete)
{
    QSet<QString> pendingLocalIsbns;
    for (const QString &isbn : m_database->getPendingUploads()) {
        pendingLocalIsbns.insert(isbn);
    }
    for (const QString &isbn : m_database->getPendingDeletes()) {
        pendingLocalIsbns.insert(isbn);
    }

    QSet<QString> tombstones;
    for (const QString &isbn : isbnsToDelete) {
        if (pendingLocalIsbns.contains(isbn)) continue;
        tombstones.insert(isbn);
        m_database->deleteBookRecord(isbn);
        m_bookshelf->removeBookFromShelf(isbn);
        CoverCache::removeImage(isbn);
        ++m_removedCount;
    }

    for (const BookInfo &book : booksToSave) {
        if (tombstones.contains(book.isbn) || pendingLocalIsbns.contains(book.isbn)) {
            continue;
        }
        m_database->saveRemoteBookRecord(book);
        m_bookshelf->addBookToShelf(book, true);
        ++m_downloadedCount;
        if (!book.coverUrl.isEmpty() && !CoverCache::contains(book.isbn)) {
            m_metadataProvider->cacheCoverForBook(book.isbn, book.coverUrl);
        }
    }
}

void BookSyncCoordinator::handleSyncCompleted(const QString &username, qint64 checkpoint,
                                              bool initialSync,
                                              const QStringList &remoteIsbns)
{
    m_database->setSyncCheckpoint(username, checkpoint);

    if (initialSync) {
        const QSet<QString> serverBooks(remoteIsbns.begin(), remoteIsbns.end());
        for (const BookInfo &book : m_database->getAllSavedBooks()) {
            if (!serverBooks.contains(book.isbn) && !m_database->hasPendingAction(book.isbn)) {
                m_database->addPendingUpload(book.isbn);
            }
        }
    }

    const int uploadCount = m_database->getPendingUploads().size();
    const int deleteCount = m_database->getPendingDeletes().size();
    flushQueue();
    emit syncSummary(
        QString("Sync complete: %1 downloaded, %2 removed, %3 uploaded, %4 deletes sent.")
            .arg(m_downloadedCount)
            .arg(m_removedCount)
            .arg(uploadCount)
            .arg(deleteCount));
}
