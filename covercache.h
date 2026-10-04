#ifndef COVERCACHE_H
#define COVERCACHE_H

#include <QByteArray>
#include <QString>

namespace CoverCache {
QString filePath(const QString &isbn);
bool contains(const QString &isbn);
bool saveImage(const QString &isbn, const QByteArray &imageData);
void removeImage(const QString &isbn);
}

#endif // COVERCACHE_H
