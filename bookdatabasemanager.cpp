#include "bookdatabasemanager.h"
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>

BookDatabaseManager::BookDatabaseManager(QObject *parent) : QObject(parent)
{
}

bool BookDatabaseManager::initDatabase(const QString &dbPath)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        emit databaseError("Could not open SQLite database connection: " + db.lastError().text());
        return false;
    }

    QSqlQuery query;
    const QString createBooksTableSql =
        "CREATE TABLE IF NOT EXISTS books ("
        "  isbn TEXT PRIMARY KEY,"
        "  title TEXT NOT NULL,"
        "  authors TEXT,"
        "  engine_source TEXT,"
        "  cover_url TEXT,"
        "  publication_date TEXT,"
        "  publisher TEXT,"
        "  page_count INTEGER"
        ")";

    if (!query.exec(createBooksTableSql)) {
        emit databaseError("Failed to initialize database table layout: " + query.lastError().text());
        return false;
    }

    const QString createQueueTableSql =
        "CREATE TABLE IF NOT EXISTS sync_queue ("
        "  isbn TEXT PRIMARY KEY,"
        "  action_type TEXT NOT NULL"
        ")";
    if (!query.exec(createQueueTableSql)) {
        emit databaseError("Failed to initialize sync queue table: " + query.lastError().text());
        return false;
    }

    const QString createSyncStateTableSql =
        "CREATE TABLE IF NOT EXISTS sync_state ("
        "  username TEXT PRIMARY KEY, "
        "  checkpoint INTEGER NOT NULL"
        ")";
    if (!query.exec(createSyncStateTableSql)) {
        emit databaseError("Failed to initialize sync checkpoint table: " + query.lastError().text());
        return false;
    }

    return true;
}

void BookDatabaseManager::saveBookRecord(const BookInfo &info)
{
    if (writeBookRecord(info)) {
        emit bookSavedSuccessfully(info.isbn);
    }
}

void BookDatabaseManager::saveRemoteBookRecord(const BookInfo &info)
{
    writeBookRecord(info);
}

bool BookDatabaseManager::saveImportedBookRecord(const BookInfo &info)
{
    QSqlDatabase database = QSqlDatabase::database();
    if (!database.transaction()) {
        emit databaseError("Failed to begin book import transaction: "
                           + database.lastError().text());
        return false;
    }

    if (!writeBookRecord(info) || !queueSyncAction(info.isbn, "UPLOAD")) {
        database.rollback();
        return false;
    }

    if (!database.commit()) {
        database.rollback();
        emit databaseError("Failed to commit imported book: " + database.lastError().text());
        return false;
    }
    return true;
}

bool BookDatabaseManager::writeBookRecord(const BookInfo &info)
{
    if (!info.found) return false;

    QSqlQuery query;
    // Use an INSERT OR REPLACE clause so scanning a book a second time updates its entry
    // instead of throwing a duplicate primary key error constraint
    query.prepare("INSERT OR REPLACE INTO books "
                  "(isbn, title, authors, engine_source, cover_url, publication_date, publisher, page_count) "
                  "VALUES (?, ?, ?, ?, ?, ?, ?, ?)");

    query.addBindValue(info.isbn);
    query.addBindValue(info.title);
    query.addBindValue(info.authors);
    query.addBindValue(info.engineSource);
    query.addBindValue(info.coverUrl);
    query.addBindValue(info.publicationDate);
    query.addBindValue(info.publisher);
    query.addBindValue(info.pageCount);

    if (!query.exec()) {
        emit databaseError("Failed to save book record: " + query.lastError().text());
        return false;
    } else {
        return true;
    }
}

QList<BookInfo> BookDatabaseManager::getAllSavedBooks()
{
    QList<BookInfo> bookList;
    QSqlQuery query("SELECT isbn, title, authors, engine_source, cover_url, publication_date, publisher, page_count "
                    "FROM books ORDER BY isbn DESC");

    while (query.next()) {
        BookInfo info;
        info.found = true;
        info.isbn = query.value(0).toString();
        info.title = query.value(1).toString();
        info.authors = query.value(2).toString();
        info.engineSource = query.value(3).toString();
        info.coverUrl = query.value(4).toString();
        info.publicationDate = query.value(5).toString();
        info.publisher = query.value(6).toString();
        info.pageCount = query.value(7).toInt();
        bookList.append(info);
    }
    return bookList;
}

BookInfo BookDatabaseManager::getBookByIsbn(const QString &isbn)
{
    BookInfo info;
    QSqlQuery query;
    query.prepare("SELECT isbn, title, authors, engine_source, cover_url, publication_date, publisher, page_count "
                  "FROM books WHERE isbn = ?");
    query.addBindValue(isbn);

    if (query.exec() && query.next()) {
        info.found = true;
        info.isbn = query.value(0).toString();
        info.title = query.value(1).toString();
        info.authors = query.value(2).toString();
        info.engineSource = query.value(3).toString();
        info.coverUrl = query.value(4).toString();
        info.publicationDate = query.value(5).toString();
        info.publisher = query.value(6).toString();
        info.pageCount = query.value(7).toInt();
    }
    return info;
}

bool BookDatabaseManager::updateBookCoverUrl(const QString &isbn, const QString &coverUrl)
{
    QSqlQuery query;
    query.prepare("UPDATE books SET cover_url = ? WHERE isbn = ?");
    query.addBindValue(coverUrl);
    query.addBindValue(isbn);
    if (!query.exec()) {
        emit databaseError("Failed to update cover URL: " + query.lastError().text());
        return false;
    }
    if (query.numRowsAffected() == 0) {
        return false;
    }
    addPendingUpload(isbn);
    return true;
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

bool BookDatabaseManager::clearBooksAndQueueDeletes()
{
    QSqlDatabase database = QSqlDatabase::database();
    if (!database.transaction()) {
        emit databaseError("Failed to begin library clear transaction: " + database.lastError().text());
        return false;
    }

    QSqlQuery selectQuery("SELECT isbn FROM books");
    if (!selectQuery.exec()) {
        database.rollback();
        emit databaseError("Failed to list books for deletion: " + selectQuery.lastError().text());
        return false;
    }

    QStringList isbns;
    while (selectQuery.next()) {
        isbns.append(selectQuery.value(0).toString());
    }
    selectQuery.finish();

    for (const QString &isbn : isbns) {
        if (!queueSyncAction(isbn, "DELETE")) {
            database.rollback();
            return false;
        }
    }

    QSqlQuery clearQuery("DELETE FROM books");
    if (!clearQuery.exec()) {
        database.rollback();
        emit databaseError("Failed to clear local books: " + clearQuery.lastError().text());
        return false;
    }

    if (!database.commit()) {
        emit databaseError("Failed to commit library clear transaction: " + database.lastError().text());
        database.rollback();
        return false;
    }
    return true;
}

bool BookDatabaseManager::queueSyncAction(const QString &isbn, const QString &actionType)
{
    if (isbn.isEmpty()) {
        return false;
    }

    QSqlQuery clearConflictingAction;
    clearConflictingAction.prepare(
        "DELETE FROM sync_queue WHERE isbn = ? AND action_type <> ?");
    clearConflictingAction.addBindValue(isbn);
    clearConflictingAction.addBindValue(actionType);
    if (!clearConflictingAction.exec()) {
        emit databaseError("Failed to replace queued sync action: "
                           + clearConflictingAction.lastError().text());
        return false;
    }

    QSqlQuery query;
    query.prepare("INSERT OR REPLACE INTO sync_queue (isbn, action_type) VALUES (?, ?)");
    query.addBindValue(isbn);
    query.addBindValue(actionType);

    if (!query.exec()) {
        emit databaseError("Failed to queue " + actionType.toLower() + ": " + query.lastError().text());
        return false;
    }
    return true;
}

void BookDatabaseManager::addPendingUpload(const QString &isbn)
{
    queueSyncAction(isbn, "UPLOAD");
}

void BookDatabaseManager::addPendingDelete(const QString &isbn)
{
    queueSyncAction(isbn, "DELETE");
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

void BookDatabaseManager::removePendingAction(const QString &isbn, const QString &actionType)
{
    if (isbn.isEmpty() || actionType.isEmpty()) {
        return;
    }

    QSqlQuery query;
    query.prepare("DELETE FROM sync_queue WHERE isbn = ? AND action_type = ?");
    query.addBindValue(isbn);
    query.addBindValue(actionType);

    if (!query.exec()) {
        emit databaseError("Failed to clear sync queue item: " + query.lastError().text());
    }
}

bool BookDatabaseManager::hasPendingAction(const QString &isbn)
{
    QSqlQuery query;
    query.prepare("SELECT 1 FROM sync_queue WHERE isbn = ? LIMIT 1");
    query.addBindValue(isbn);
    return query.exec() && query.next();
}

bool BookDatabaseManager::hasSyncCheckpoint(const QString &username)
{
    QSqlQuery query;
    query.prepare("SELECT 1 FROM sync_state WHERE username = ? LIMIT 1");
    query.addBindValue(username);
    return query.exec() && query.next();
}

qint64 BookDatabaseManager::getSyncCheckpoint(const QString &username)
{
    QSqlQuery query;
    query.prepare("SELECT checkpoint FROM sync_state WHERE username = ?");
    query.addBindValue(username);
    if (query.exec() && query.next()) {
        return query.value(0).toLongLong();
    }
    return 0;
}

void BookDatabaseManager::setSyncCheckpoint(const QString &username, qint64 checkpoint)
{
    QSqlQuery query;
    query.prepare(
        "INSERT OR REPLACE INTO sync_state (username, checkpoint) VALUES (?, ?)");
    query.addBindValue(username);
    query.addBindValue(checkpoint);
    if (!query.exec()) {
        emit databaseError("Failed to save sync checkpoint: " + query.lastError().text());
    }
}
