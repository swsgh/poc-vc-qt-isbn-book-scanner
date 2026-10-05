#ifndef BOOKMETADATAPROVIDER_H
#define BOOKMETADATAPROVIDER_H

#include <QObject>
#include <QString>
#include <QStringList>
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
    void refreshCoverImages();

signals:
    void lookupStatusChanged(const QString &statusText, bool isError);
    void bookDataReady(const BookInfo &info);
    void coverCached(const QString &isbn);
    void coverRefreshFinished(int refreshed, int total);

private slots:
    void handlePrimarySuccess(const BookInfo &info, const QString &urlLarge,
                              const QString &urlMedium, const QString &urlSmall);
    void handlePrimaryFailure([[maybe_unused]] const QString &errorMsg);
    void handleFallbackSuccess(const BookInfo &info, const QString &urlLarge,
                               const QString &urlMedium, const QString &urlSmall);
    void handleFallbackFailure([[maybe_unused]] const QString &errorMsg);
    void resetScannerCooldown();

private:
    enum class CoverSize { Large, Medium, Small };
    void beginCoverDownload(const BookInfo &info, const QString &urlLarge,
                            const QString &urlMedium, const QString &urlSmall);
    void downloadCoverImage(const QString &url, const QString &statusText, CoverSize size);
    void handleCoverDownloadFinished(QNetworkReply *reply, CoverSize size);
    void processNextCoverRefresh();
    void completeCoverRefresh(bool refreshed);

    AbstractBookProvider* m_openLibrary;
    AbstractBookProvider* m_googleBooks;
    BookDatabaseManager* m_dbManager;
    std::unique_ptr<QNetworkAccessManager> m_imageNetworkManager;

    QString m_lastScannedIsbn;
    bool m_isCooldownActive;

    BookInfo m_pendingInfo;
    QString m_fallbackUrlMedium;
    QString m_fallbackUrlSmall;
    QSet<QString> m_activeCoverDownloads;
    QStringList m_coverRefreshIsbns;
    QString m_refreshIsbn;
    int m_coverRefreshIndex = 0;
    int m_coverRefreshCount = 0;
    bool m_isRefreshingCovers = false;
    bool m_refreshUsingGoogle = false;
    bool m_fallbackUsingGoogle = false;
};

#endif // BOOKMETADATAPROVIDER_H
