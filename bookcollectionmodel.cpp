#include "bookcollectionmodel.h"

#include "covercache.h"

#include <QUrl>

BookCollectionModel::BookCollectionModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int BookCollectionModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_books.size();
}

QVariant BookCollectionModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.column() != 0
        || index.row() < 0 || index.row() >= m_books.size()) {
        return {};
    }

    const BookInfo &book = m_books.at(index.row());
    switch (role) {
    case Qt::DisplayRole:
    case TitleRole:
        return book.title;
    case BookInfoRole:
        return QVariant::fromValue(book);
    case FoundRole:
        return book.found;
    case IsbnRole:
        return book.isbn;
    case AuthorsRole:
        return book.authors;
    case EngineSourceRole:
        return book.engineSource;
    case CoverUrlRole:
        return book.coverUrl;
    case CoverSourceRole:
        if (!book.coverUrl.isEmpty() && CoverCache::contains(book.isbn)) {
            return QUrl::fromLocalFile(CoverCache::filePath(book.isbn));
        }
        return {};
    case PublicationDateRole:
        return book.publicationDate;
    case PublisherRole:
        return book.publisher;
    case PageCountRole:
        return book.pageCount;
    case SearchTextRole:
        return QStringLiteral("%1 %2 %3").arg(book.title, book.authors, book.isbn);
    default:
        return {};
    }
}

QHash<int, QByteArray> BookCollectionModel::roleNames() const
{
    return {
        {BookInfoRole, "bookInfo"},
        {FoundRole, "found"},
        {IsbnRole, "isbn"},
        {TitleRole, "title"},
        {AuthorsRole, "authors"},
        {EngineSourceRole, "engineSource"},
        {CoverUrlRole, "coverUrl"},
        {CoverSourceRole, "coverSource"},
        {PublicationDateRole, "publicationDate"},
        {PublisherRole, "publisher"},
        {PageCountRole, "pageCount"},
        {SearchTextRole, "searchText"}
    };
}

void BookCollectionModel::setBooks(const QList<BookInfo> &books)
{
    beginResetModel();
    m_books = books;
    endResetModel();
}

void BookCollectionModel::addBook(const BookInfo &book, bool prepend)
{
    if (!book.found || book.isbn.isEmpty()) {
        return;
    }

    int row = rowForIsbn(book.isbn);
    if (row >= 0) {
        if (prepend && row > 0
            && beginMoveRows(QModelIndex(), row, row, QModelIndex(), 0)) {
            const BookInfo existingBook = m_books.takeAt(row);
            m_books.prepend(existingBook);
            endMoveRows();
            row = 0;
        }
        m_books[row] = book;
        emit dataChanged(index(row, 0), index(row, 0));
        return;
    }

    const int insertRow = prepend ? 0 : m_books.size();
    beginInsertRows(QModelIndex(), insertRow, insertRow);
    m_books.insert(insertRow, book);
    endInsertRows();
}

void BookCollectionModel::removeBook(const QString &isbn)
{
    const int row = rowForIsbn(isbn);
    if (row < 0) {
        return;
    }

    beginRemoveRows(QModelIndex(), row, row);
    m_books.removeAt(row);
    endRemoveRows();
}

void BookCollectionModel::clear()
{
    if (m_books.isEmpty()) {
        return;
    }

    beginResetModel();
    m_books.clear();
    endResetModel();
}

BookInfo BookCollectionModel::bookAt(int row) const
{
    return row >= 0 && row < m_books.size() ? m_books.at(row) : BookInfo{};
}

int BookCollectionModel::rowForIsbn(const QString &isbn) const
{
    for (int row = 0; row < m_books.size(); ++row) {
        if (m_books.at(row).isbn == isbn) {
            return row;
        }
    }
    return -1;
}