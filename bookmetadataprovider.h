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
class BookDatabaseManager;

class BookMetadataProvider : public QObject
{
    Q_OBJECT
public:
    explicit BookMetadataProvider(BookDatabaseManager* dbManager, QObject *parent = nullptr);
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
    void handleMediumCoverFinished(QNetworkReply* reply);
    void handleSmallCoverFinished(QNetworkReply* reply);
    void resetScannerCooldown();

private:
    void downloadMediumCover(const QString &urlMedium);
    void downloadSmallCover(const QString &urlSmall);

    AbstractBookProvider* m_openLibrary;
    AbstractBookProvider* m_googleBooks;
    BookDatabaseManager* m_dbManager;
    std::unique_ptr<QNetworkAccessManager> m_imageNetworkManager;

    QString m_lastScannedIsbn;
    bool m_isCooldownActive;

    BookInfo m_pendingInfo;
    QString m_fallbackUrlSmall;
};

#endif // BOOKMETADATAPROVIDER_H
