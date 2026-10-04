#ifndef BOOKINFO_H
#define BOOKINFO_H

#include <QString>
#include <QMetaType>

struct BookInfo {
    bool found = false;
    QString isbn;
    QString title;
    QString authors;
    QString engineSource;
    QString coverUrl;
    QString publicationDate;
    QString publisher;
    int pageCount = 0;
};
Q_DECLARE_METATYPE(BookInfo)

#endif // BOOKINFO_H
