#ifndef BOOKINFO_H
#define BOOKINFO_H

#include <QString>

struct BookInfo {
    bool found = false;
    QString isbn;
    QString title;
    QString authors;
    QString engineSource; // Identifies which API successfully supplied the details
};

#endif // BOOKINFO_H
