#ifndef BOOKSHELFWIDGET_H
#define BOOKSHELFWIDGET_H

#include <QWidget>
#include <QList>
#include "bookinfo.h"

class QGridLayout; // <-- CHANGED from QHBoxLayout
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

protected:
    // Automatically recalculates wrapping columns whenever the application window scales
    void resizeEvent(QResizeEvent *event) override;

private:
    void rearrangeGrid();
    QPixmap generatePlaceholderCover(const QString &title);

    QWidget* m_scrollContainer;
    QGridLayout* m_shelfGridLayout; // <-- Grid controller
    QList<QWidget*> m_bookCards;     // Keep a tracking list of elements to handle line wrapping calculations
};

#endif // BOOKSHELFWIDGET_H
