#ifndef BOOKSHELFWIDGET_H
#define BOOKSHELFWIDGET_H

#include <QWidget>
#include <QList>
#include "bookinfo.h"

// Correctly forward declare missing layout and widget classes
class QHBoxLayout;
class QVBoxLayout;
class QScrollArea;
class QLabel;

class BookshelfWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BookshelfWidget(QWidget *parent = nullptr);
    ~BookshelfWidget() override = default;

    // Appends a book card natively using raw database memory BLOB values
    void addBookToShelf(const BookInfo &info, bool prepend = true);
    void clearShelf();

private:
    QPixmap generatePlaceholderCover(const QString &title);

    QWidget* m_scrollContainer;
    QHBoxLayout* m_shelfLayout;
};

#endif // BOOKSHELFWIDGET_H
