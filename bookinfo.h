#ifndef BOOKINFO_H
#define BOOKINFO_H

#include <QString>
#include <QByteArray>

struct BookInfo {
    bool found = false;
    QString isbn;
    QString title;
    QString authors;
    QString engineSource;
    QByteArray coverData;
};

#endif // BOOKINFO_H
