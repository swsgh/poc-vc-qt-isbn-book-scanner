#include "covercache.h"

#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QSaveFile>
#include <QStandardPaths>

namespace {
QString coverDirectory()
{
    const QString path = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
        + "/covers";
    QDir().mkpath(path);
    return path;
}
}

QString CoverCache::filePath(const QString &isbn)
{
    const QByteArray filename = QCryptographicHash::hash(
        isbn.toUtf8(), QCryptographicHash::Sha256).toHex();
    return coverDirectory() + "/" + QString::fromLatin1(filename) + ".img";
}

bool CoverCache::contains(const QString &isbn)
{
    return QFile::exists(filePath(isbn));
}

bool CoverCache::saveImage(const QString &isbn, const QByteArray &imageData)
{
    if (isbn.isEmpty() || imageData.isEmpty() || QImage::fromData(imageData).isNull()) {
        return false;
    }

    QSaveFile file(filePath(isbn));
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    if (file.write(imageData) != imageData.size()) {
        file.cancelWriting();
        return false;
    }
    return file.commit();
}

void CoverCache::removeImage(const QString &isbn)
{
    QFile::remove(filePath(isbn));
}
