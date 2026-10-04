#ifndef BOOKSHELFWIDGET_H
#define BOOKSHELFWIDGET_H

#include <QWidget>
#include <QList>
#include "bookinfo.h"

class QGridLayout;
class QVBoxLayout;
class QScrollArea;
class QLabel;
class QPalette;

class BookshelfWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BookshelfWidget(QWidget *parent = nullptr);
    ~BookshelfWidget() override = default;

    void addBookToShelf(const BookInfo &info, bool prepend = true);
    void removeBookFromShelf(const QString &isbn);
    void clearShelf();
    void filterBooks(const QString &searchText);
    void applyPalette(const QPalette &palette);

signals:
    // NEW: Emitted whenever an individual book card is interactively clicked
    void bookSelected(const BookInfo &info);

protected:
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void rearrangeGrid();
    QWidget *createBookCard(const BookInfo &info);
    void updateBookCardCover(QWidget *card, const BookInfo &info);
    QPixmap generatePlaceholderCover(const QString &title);

    QWidget* m_scrollContainer;
    QGridLayout* m_shelfGridLayout;
    QList<QWidget*> m_bookCards;
    QScrollArea* m_scrollArea;
    QLabel* m_titleLabel;
};

#endif // BOOKSHELFWIDGET_H
