#ifndef BOOKMETADATAPROVIDER_H
#define BOOKMETADATAPROVIDER_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <memory>
#include "bookinfo.h"

class AbstractBookProvider;
class QNetworkAccessManager;
class QNetworkReply;

class BookMetadataProvider : public QObject
{
    Q_OBJECT
public:
    explicit BookMetadataProvider(QObject *parent = nullptr);
    ~BookMetadataProvider() override;

    void lookupIsbn(const QString &isbn);

signals:
    void lookupStatusChanged(const QString &statusText, bool isError);
    void bookDataReady(const BookInfo &info);

private slots:
    void handlePrimarySuccess(const BookInfo &info, const QString &urlSmall, const QString &urlMedium);
    void handlePrimaryFailure(const QString &errorMsg);
    void handleFallbackSuccess(const BookInfo &info, const QString &urlSmall, const QString &urlMedium);
    void handleFallbackFailure(const QString &errorMsg);

    // Slots managing the dynamic image download hierarchy
    void handleMediumCoverFinished(QNetworkReply* reply);
    void handleSmallCoverFinished(QNetworkReply* reply);
    void resetScannerCooldown();

private:
    void downloadMediumCover(const QString &urlMedium);
    void downloadSmallCover(const QString &urlSmall);

    AbstractBookProvider* m_openLibrary;
    AbstractBookProvider* m_googleBooks;
    std::unique_ptr<QNetworkAccessManager> m_imageNetworkManager;

    QString m_lastScannedIsbn;
    bool m_isCooldownActive;

    // Cache tracking properties during the async cascade
    BookInfo m_pendingInfo;
    QString m_fallbackUrlSmall;
};

#endif // BOOKMETADATAPROVIDER_H
