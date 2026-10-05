#ifndef BOOKMETADATAPROVIDER_H
#define BOOKMETADATAPROVIDER_H

#include <QObject>
#include <QString>
#include <QSet>
#include <memory>

class QNetworkAccessManager;

class BookMetadataProvider : public QObject
{
    Q_OBJECT
public:
    explicit BookMetadataProvider(QObject *parent = nullptr);
    ~BookMetadataProvider() override;

    void cacheCoverForBook(const QString &isbn, const QString &coverUrl,
                           const QString &authToken = {});

signals:
    void lookupStatusChanged(const QString &statusText, bool isError);
    void coverCached(const QString &isbn);

private:
    std::unique_ptr<QNetworkAccessManager> m_imageNetworkManager;

    QSet<QString> m_activeCoverDownloads;
};

#endif // BOOKMETADATAPROVIDER_H
