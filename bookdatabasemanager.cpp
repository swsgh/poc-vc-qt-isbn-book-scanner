#include "bookdatabasemanager.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDateTime>
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
        "  scanned_at TEXT"
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
    query.prepare("INSERT OR REPLACE INTO books (isbn, title, authors, engine_source, scanned_at) "
                  "VALUES (?, ?, ?, ?, ?)");

    query.addBindValue(info.isbn);
    query.addBindValue(info.title);
    query.addBindValue(info.authors);
    query.addBindValue(info.engineSource);

    // Stamp the record with the current date and time
    QString currentTimestamp = QDateTime::currentDateTime().toString(Qt::ISODate);
    query.addBindValue(currentTimestamp);

    if (!query.exec()) {
        emit databaseError("Failed to save book record: " + query.lastError().text());
    } else {
        emit bookSavedSuccessfully(info.isbn);
    }
}
