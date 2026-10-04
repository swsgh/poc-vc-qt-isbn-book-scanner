#ifndef BOOKDATABASEMANAGER_H
#define BOOKDATABASEMANAGER_H

#include <QObject>
#include <QString>
#include "bookinfo.h"

class BookDatabaseManager : public QObject
{
    Q_OBJECT
public:
    explicit BookDatabaseManager(QObject *parent = nullptr);
    ~BookDatabaseManager() override = default;

    // Opens the database file and creates the table layout if it doesn't exist
    bool initDatabase(const QString &dbPath = "scanned_books.db");
    // Retrieves all saved items sorted by the newest scan entry sequence timestamp
    QList<BookInfo> getAllSavedBooks();
    BookInfo getBookByIsbn(const QString &isbn);
    // Returns true if the ISBN primary key exists in the database table rows
    bool hasBookInLocalDatabase(const QString &isbn);
    bool deleteBookRecord(const QString &isbn);
    void addPendingUpload(const QString &isbn);
    void addPendingDelete(const QString &isbn);
    QStringList getPendingUploads();
    QStringList getPendingDeletes();
    void removePendingAction(const QString &isbn, const QString &actionType);
    bool hasPendingAction(const QString &isbn);
    bool hasSyncCheckpoint(const QString &username);
    qint64 getSyncCheckpoint(const QString &username);
    void setSyncCheckpoint(const QString &username, qint64 checkpoint);

private:
    void queueSyncAction(const QString &isbn, const QString &actionType);
    bool writeBookRecord(const BookInfo &info);

public slots:
    // Slot designed to directly consume the BookInfo packet emitted by the provider
    void saveBookRecord(const BookInfo &info);
    void saveRemoteBookRecord(const BookInfo &info);

signals:
    void databaseError(const QString &errorMessage);
    void bookSavedSuccessfully(const QString &isbn);
};

#endif // BOOKDATABASEMANAGER_H
