#ifndef BOOKINFO_H
#define BOOKINFO_H

#include <QObject>
#include <QString>
#include <QMetaType>

struct BookInfo {
    Q_GADGET
    Q_PROPERTY(bool found MEMBER found)
    Q_PROPERTY(QString isbn MEMBER isbn)
    Q_PROPERTY(QString title MEMBER title)
    Q_PROPERTY(QString authors MEMBER authors)
    Q_PROPERTY(QString engineSource MEMBER engineSource)
    Q_PROPERTY(QString coverUrl MEMBER coverUrl)
    Q_PROPERTY(QString publicationDate MEMBER publicationDate)
    Q_PROPERTY(QString publisher MEMBER publisher)
    Q_PROPERTY(int pageCount MEMBER pageCount)

public:
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
