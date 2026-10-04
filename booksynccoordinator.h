#ifndef BOOKSYNCCOORDINATOR_H
#define BOOKSYNCCOORDINATOR_H

#include <QObject>
#include <QString>
#include <QStringList>

#include "bookinfo.h"

class BookDatabaseManager;
class BookMetadataProvider;
class BookSyncManager;

class BookSyncCoordinator : public QObject
{
    Q_OBJECT

public:
    BookSyncCoordinator(BookDatabaseManager *database,
                        BookSyncManager *syncManager,
                        BookMetadataProvider *metadataProvider,
                        QObject *parent = nullptr);

    void beginSync();
    void flushQueue();
    bool removeBook(const QString &isbn);

signals:
    void statusMessage(const QString &message, bool isError);
    void syncSummary(const QString &message);
    void collectionBookAdded(const BookInfo &book, bool prepend);
    void collectionBookRemoved(const QString &isbn);
    void bookDetailsCloseRequested();

private slots:
    void handleBookSaved(const QString &isbn);
    void handleRemoteBookUpdates(const QList<BookInfo> &booksToSave,
                                 const QStringList &isbnsToDelete);
    void handleSyncCompleted(const QString &username, qint64 checkpoint,
                             bool initialSync, const QStringList &remoteIsbns);

private:
    BookDatabaseManager *m_database;
    BookSyncManager *m_syncManager;
    BookMetadataProvider *m_metadataProvider;
    int m_downloadedCount = 0;
    int m_removedCount = 0;
};

#endif // BOOKSYNCCOORDINATOR_H
