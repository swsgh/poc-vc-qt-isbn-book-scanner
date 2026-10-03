#ifndef BOOKINFO_H
#define BOOKINFO_H

#include <QString>
#include <QByteArray>
#include <QMetaType>

struct BookInfo {
    bool found = false;
    QString isbn;
    QString title;
    QString authors;
    QString engineSource;
    QByteArray coverData;
};
Q_DECLARE_METATYPE(BookInfo)

#endif // BOOKINFO_H
