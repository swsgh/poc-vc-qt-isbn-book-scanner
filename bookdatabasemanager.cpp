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
