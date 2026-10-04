#ifndef BOOKSYNCCOORDINATOR_H
#define BOOKSYNCCOORDINATOR_H

#include <QObject>
#include <QString>
#include <QStringList>

#include "bookinfo.h"

class BookDatabaseManager;
class BookDetailsSidebar;
class BookMetadataProvider;
class BookSyncManager;
class BookshelfWidget;

class BookSyncCoordinator : public QObject
{
    Q_OBJECT

public:
    BookSyncCoordinator(BookDatabaseManager *database,
                        BookSyncManager *syncManager,
                        BookMetadataProvider *metadataProvider,
                        BookshelfWidget *bookshelf,
                        BookDetailsSidebar *details,
                        QObject *parent = nullptr);

    void beginSync();
    void flushQueue();
    bool removeBook(const QString &isbn);

signals:
    void statusMessage(const QString &message, bool isError);
    void syncSummary(const QString &message);

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
    BookshelfWidget *m_bookshelf;
    BookDetailsSidebar *m_details;
    int m_downloadedCount = 0;
    int m_removedCount = 0;
};

#endif // BOOKSYNCCOORDINATOR_H
