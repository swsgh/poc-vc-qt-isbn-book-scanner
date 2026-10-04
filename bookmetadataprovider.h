#ifndef BOOKMETADATAPROVIDER_H
#define BOOKMETADATAPROVIDER_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QSet>
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
    void cacheCoverForBook(const QString &isbn, const QString &coverUrl);

signals:
    void lookupStatusChanged(const QString &statusText, bool isError);
    void bookDataReady(const BookInfo &info);
    void coverCached(const QString &isbn);

private slots:
    void handlePrimarySuccess(const BookInfo &info, const QString &urlSmall, const QString &urlMedium);
    void handlePrimaryFailure([[maybe_unused]] const QString &errorMsg);
    void handleFallbackSuccess(const BookInfo &info, const QString &urlSmall, const QString &urlMedium);
    void handleFallbackFailure([[maybe_unused]] const QString &errorMsg);
    void resetScannerCooldown();

private:
    void downloadCoverImage(const QString &url, const QString &statusText, bool isMedium);
    void downloadMediumCover(const QString &urlMedium);
    void downloadSmallCover(const QString &urlSmall);
    void handleCoverDownloadFinished(QNetworkReply* reply, bool isMedium);

    AbstractBookProvider* m_openLibrary;
    AbstractBookProvider* m_googleBooks;
    BookDatabaseManager* m_dbManager;
    std::unique_ptr<QNetworkAccessManager> m_imageNetworkManager;

    QString m_lastScannedIsbn;
    bool m_isCooldownActive;

    BookInfo m_pendingInfo;
    QString m_fallbackUrlSmall;
    QSet<QString> m_activeCoverDownloads;
};

#endif // BOOKMETADATAPROVIDER_H
