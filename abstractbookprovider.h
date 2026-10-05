// Inside abstractbookprovider.h
#ifndef ABSTRACTBOOKPROVIDER_H
#define ABSTRACTBOOKPROVIDER_H

#include <QObject>
#include <QString>
#include "bookinfo.h"

class AbstractBookProvider : public QObject
{
    Q_OBJECT
public:
    explicit AbstractBookProvider(QObject *parent = nullptr) : QObject(parent) {}
    virtual ~AbstractBookProvider() override = default;

    virtual void requestMetadata(const QString &isbn) = 0;

signals:
    void lookupFinished(const BookInfo &info, const QString &urlLarge,
                        const QString &urlMedium, const QString &urlSmall);
    void lookupFailed(const QString &errorMsg);
};

#endif // ABSTRACTBOOKPROVIDER_H
