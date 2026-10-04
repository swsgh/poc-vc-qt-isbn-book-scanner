#include "bookdatabasemanager.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

BookDatabaseManager::BookDatabaseManager(QObject *parent) : QObject(parent)
{
}

bool BookDatabaseManager::initDatabase(const QString &dbPath)
{
    // Initialize the SQLite native driver connection
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        emit databaseError("Could not open SQLite database connection: " + db.lastError().text());
        return false;
    }

    // Build the books archive table schema
    QSqlQuery query;
    QString createTableSql =
        "CREATE TABLE IF NOT EXISTS books ("
        "  isbn TEXT PRIMARY KEY,"
        "  title TEXT NOT NULL,"
        "  authors TEXT,"
        "  engine_source TEXT,"
        "  cover_blob BLOB"
        ")";

    if (!query.exec(createTableSql)) {
        emit databaseError("Failed to initialize database table layout: " + query.lastError().text());
        return false;
    }

    QString createQueueTableSql =
        "CREATE TABLE IF NOT EXISTS sync_queue ("
        "  isbn TEXT PRIMARY KEY,"
        "  action_type TEXT NOT NULL" // Tracks whether we need to "UPLOAD" or "DELETE" this book online
        ")";

    if (!query.exec(createQueueTableSql)) {
        emit databaseError("Failed to initialize sync queue table: " + query.lastError().text());
        return false;
    }

    return true;
}

void BookDatabaseManager::saveBookRecord(const BookInfo &info)
{
    if (!info.found) return;

    QSqlQuery query;
    // Use an INSERT OR REPLACE clause so scanning a book a second time updates its entry
    // instead of throwing a duplicate primary key error constraint
    query.prepare("INSERT OR REPLACE INTO books (isbn, title, authors, engine_source, cover_blob) "
                  "VALUES (?, ?, ?, ?, ?)");

    query.addBindValue(info.isbn);
    query.addBindValue(info.title);
    query.addBindValue(info.authors);
    query.addBindValue(info.engineSource);
    // SQLite driver automatically converts QByteArray objects straight into local BLOB field entries
    query.addBindValue(info.coverData);

    if (!query.exec()) {
        emit databaseError("Failed to save book record: " + query.lastError().text());
    } else {
        emit bookSavedSuccessfully(info.isbn);
    }
}

QList<BookInfo> BookDatabaseManager::getAllSavedBooks()
{
    QList<BookInfo> bookList;
    QSqlQuery query("SELECT isbn, title, authors, engine_source, cover_blob FROM books ORDER BY isbn DESC");

    while (query.next()) {
        BookInfo info;
        info.found = true;
        info.isbn = query.value(0).toString();
        info.title = query.value(1).toString();
        info.authors = query.value(2).toString();
        info.engineSource = query.value(3).toString();
        info.coverData = query.value(4).toByteArray();
        bookList.append(info);
    }
    return bookList;
}

BookInfo BookDatabaseManager::getBookByIsbn(const QString &isbn)
{
    BookInfo info;
    QSqlQuery query;
    query.prepare("SELECT isbn, title, authors, engine_source, cover_blob FROM books WHERE isbn = ?");
    query.addBindValue(isbn);

    if (query.exec() && query.next()) {
        info.found = true;
        info.isbn = query.value(0).toString();
        info.title = query.value(1).toString();
        info.authors = query.value(2).toString();
        info.engineSource = query.value(3).toString();
        info.coverData = query.value(4).toByteArray();
    }
    return info;
}

bool BookDatabaseManager::hasBookInLocalDatabase(const QString &isbn)
{
    QSqlQuery query;
    query.prepare("SELECT COUNT(*) FROM books WHERE isbn = ?");
    query.addBindValue(isbn);

    if (query.exec() && query.next()) {
        return query.value(0).toInt() > 0;
    }
    return false;
}

// NEW: Safely handles removing the target entry matching the chosen primary key tracking string
bool BookDatabaseManager::deleteBookRecord(const QString &isbn)
{
    if (isbn.isEmpty()) return false;

    QSqlQuery query;
    query.prepare("DELETE FROM books WHERE isbn = ?");
    query.addBindValue(isbn);

    if (!query.exec()) {
        emit databaseError("Failed to purge book from database: " + query.lastError().text());
        return false;
    }

    // Verify a row was actually affected by checking the database engine footprint response
    return query.numRowsAffected() > 0;
}

void BookDatabaseManager::addPendingUpload(const QString &isbn)
{
    if (isbn.isEmpty()) {
        return;
    }

    QSqlQuery query;
    query.prepare("INSERT OR REPLACE INTO sync_queue (isbn, action_type) VALUES (?, 'UPLOAD')");
    query.addBindValue(isbn);

    if (!query.exec()) {
        emit databaseError("Failed to queue upload: " + query.lastError().text());
    }
}

void BookDatabaseManager::addPendingDelete(const QString &isbn)
{
    if (isbn.isEmpty()) {
        return;
    }

    QSqlQuery query;
    query.prepare("INSERT OR REPLACE INTO sync_queue (isbn, action_type) VALUES (?, 'DELETE')");
    query.addBindValue(isbn);

    if (!query.exec()) {
        emit databaseError("Failed to queue delete: " + query.lastError().text());
    }
}

QStringList BookDatabaseManager::getPendingUploads()
{
    QStringList list;
    QSqlQuery query("SELECT isbn FROM sync_queue WHERE action_type = 'UPLOAD'");
    while (query.next()) {
        list.append(query.value(0).toString());
    }
    return list;
}

QStringList BookDatabaseManager::getPendingDeletes()
{
    QStringList list;
    QSqlQuery query("SELECT isbn FROM sync_queue WHERE action_type = 'DELETE'");
    while (query.next()) {
        list.append(query.value(0).toString());
    }
    return list;
}

void BookDatabaseManager::removePendingAction(const QString &isbn)
{
    if (isbn.isEmpty()) {
        return;
    }

    QSqlQuery query;
    query.prepare("DELETE FROM sync_queue WHERE isbn = ?");
    query.addBindValue(isbn);

    if (!query.exec()) {
        emit databaseError("Failed to clear sync queue item: " + query.lastError().text());
    }
}
