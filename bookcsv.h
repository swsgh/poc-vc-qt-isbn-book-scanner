#ifndef BOOKCSV_H
#define BOOKCSV_H

#include <QList>
#include <QString>
#include <QStringList>

#include "bookinfo.h"

namespace BookCsv {
QStringList headers();
bool writeFile(const QString &filePath, const QList<BookInfo> &books, QString &error);
bool readFile(const QString &filePath, QList<BookInfo> &books,
              int &skippedRows, QString &error);
}

#endif // BOOKCSV_H
