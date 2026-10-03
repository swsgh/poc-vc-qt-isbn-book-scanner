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

public slots:
    // Slot designed to directly consume the BookInfo packet emitted by the provider
    void saveBookRecord(const BookInfo &info);

signals:
    void databaseError(const QString &errorMessage);
    void bookSavedSuccessfully(const QString &isbn);
};

#endif // BOOKDATABASEMANAGER_H
