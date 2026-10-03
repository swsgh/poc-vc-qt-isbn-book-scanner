#ifndef BOOKDETAILSSIDEBAR_H
#define BOOKDETAILSSIDEBAR_H

#include <QWidget>
#include "bookinfo.h"

class QLabel;

class BookDetailsSidebar : public QWidget
{
    Q_OBJECT

public:
    explicit BookDetailsSidebar(QWidget *parent = nullptr);
    ~BookDetailsSidebar() override = default;

public slots:
    // Updates the fields and forces the panel into visibility
    void updateDetails(const BookInfo &info);

    // Completely collapses the panel and clears data fields out of memory footprint
    void closeSidebar();

private:
    QLabel* m_detailTitleLabel;
    QLabel* m_detailAuthorLabel;
    QLabel* m_detailIsbnLabel;
};

#endif // BOOKDETAILSSIDEBAR_H
