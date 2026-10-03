#ifndef BOOKDETAILSSIDEBAR_H
#define BOOKDETAILSSIDEBAR_H

#include <QWidget>
#include "bookinfo.h"

class QLabel;
class QPushButton; // Forward declare the button class

class BookDetailsSidebar : public QWidget
{
    Q_OBJECT

public:
    explicit BookDetailsSidebar(QWidget *parent = nullptr);
    ~BookDetailsSidebar() override = default;

public slots:
    void updateDetails(const BookInfo &info);
    void closeSidebar();

signals:
    // NEW: Emitted when the user wants to delete the active book from the archive
    void deleteBookRequested(const QString &isbn);

private:
    QLabel* m_coverLabel;
    QLabel* m_detailTitleLabel;
    QLabel* m_detailAuthorLabel;
    QLabel* m_detailIsbnLabel;
    QPushButton* m_deleteButton; // NEW: Pointer to toggle button context safely
    QString m_currentIsbn;       // NEW: Tracks active ISBN being shown
};

#endif // BOOKDETAILSSIDEBAR_H
