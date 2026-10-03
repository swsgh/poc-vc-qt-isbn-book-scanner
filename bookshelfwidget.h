#ifndef BOOKSHELFWIDGET_H
#define BOOKSHELFWIDGET_H

#include <QWidget>
#include <QList>
#include "bookinfo.h"

class QGridLayout;
class QVBoxLayout;
class QScrollArea;
class QLabel;

class BookshelfWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BookshelfWidget(QWidget *parent = nullptr);
    ~BookshelfWidget() override = default;

    void addBookToShelf(const BookInfo &info, bool prepend = true);
    void clearShelf();

signals:
    // NEW: Emitted whenever an individual book card is interactively clicked
    void bookSelected(const BookInfo &info);

protected:
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void rearrangeGrid();
    QPixmap generatePlaceholderCover(const QString &title);

    QWidget* m_scrollContainer;
    QGridLayout* m_shelfGridLayout;
    QList<QWidget*> m_bookCards;
};

#endif // BOOKSHELFWIDGET_H
