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

    // Pure virtual method that each sub-service must implement
    virtual void requestMetadata(const QString &isbn) = 0;

signals:
    void lookupFinished(const BookInfo &info);
    void lookupFailed(const QString &errorMsg);
};

#endif // ABSTRACTBOOKPROVIDER_H
