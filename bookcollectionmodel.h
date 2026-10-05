#ifndef BOOKCOLLECTIONMODEL_H
#define BOOKCOLLECTIONMODEL_H

#include <QAbstractListModel>
#include <QList>

#include "bookinfo.h"

class BookCollectionModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Role {
        BookInfoRole = Qt::UserRole + 1,
        FoundRole,
        IsbnRole,
        TitleRole,
        AuthorsRole,
        CoverUrlRole,
        CoverSourceRole,
        PublicationDateRole,
        PublisherRole,
        PageCountRole,
        SearchTextRole
    };
    Q_ENUM(Role)

    explicit BookCollectionModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setBooks(const QList<BookInfo> &books);
    void addBook(const BookInfo &book, bool prepend = true);
    void removeBook(const QString &isbn);
    void clear();

    BookInfo bookAt(int row) const;

private:
    int rowForIsbn(const QString &isbn) const;

    QList<BookInfo> m_books;
};

#endif // BOOKCOLLECTIONMODEL_H