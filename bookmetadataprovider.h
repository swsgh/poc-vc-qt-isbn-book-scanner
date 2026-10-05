#ifndef BOOKMETADATAPROVIDER_H
#define BOOKMETADATAPROVIDER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QSet>
#include <memory>

class QNetworkAccessManager;
class BookDatabaseManager;

class BookMetadataProvider : public QObject
{
    Q_OBJECT
public:
    explicit BookMetadataProvider(BookDatabaseManager* dbManager, QObject *parent = nullptr);
    ~BookMetadataProvider() override;

    void cacheCoverForBook(const QString &isbn, const QString &coverUrl,
                           const QString &authToken = {});
    void refreshCoverImages(const QString &serverUrl, const QString &authToken);

signals:
    void lookupStatusChanged(const QString &statusText, bool isError);
    void coverCached(const QString &isbn);
    void coverRefreshFinished(int refreshed, int total);

private:
    void processNextCoverRefresh();
    void finishCoverRefreshItem(const QString &isbn, bool refreshed);

    BookDatabaseManager* m_dbManager;
    std::unique_ptr<QNetworkAccessManager> m_imageNetworkManager;

    QSet<QString> m_activeCoverDownloads;
    QStringList m_coverRefreshIsbns;
    QString m_currentRefreshIsbn;
    QString m_coverRefreshAuthToken;
    int m_coverRefreshIndex = 0;
    int m_coverRefreshCount = 0;
    bool m_isRefreshingCovers = false;
};

#endif // BOOKMETADATAPROVIDER_H
