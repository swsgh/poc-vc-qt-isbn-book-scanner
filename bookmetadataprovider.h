#ifndef BOOKMETADATAPROVIDER_H
#define BOOKMETADATAPROVIDER_H

#include <QObject>
#include <QString>
#include "bookinfo.h"

class AbstractBookProvider;

class BookMetadataProvider : public QObject
{
    Q_OBJECT
public:
    explicit BookMetadataProvider(QObject *parent = nullptr);
    ~BookMetadataProvider() override = default;

    void lookupIsbn(const QString &isbn);

signals:
    void lookupStatusChanged(const QString &statusText, bool isError);
    void bookDataReady(const BookInfo &info);

private slots:
    void handlePrimarySuccess(const BookInfo &info);
    void handlePrimaryFailure(const QString &errorMsg);
    void handleFallbackSuccess(const BookInfo &info);
    void handleFallbackFailure(const QString &errorMsg);
    void resetScannerCooldown();

private:
    AbstractBookProvider* m_openLibrary;
    AbstractBookProvider* m_googleBooks;

    QString m_lastScannedIsbn;
    bool m_isCooldownActive;
};

#endif // BOOKMETADATAPROVIDER_H
