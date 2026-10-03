#ifndef OPENLIBRARYPROVIDER_H
#define OPENLIBRARYPROVIDER_H

#include "abstractbookprovider.h"
#include <memory>

class QNetworkAccessManager;
class QNetworkReply;

class OpenLibraryProvider : public AbstractBookProvider
{
    Q_OBJECT
public:
    explicit OpenLibraryProvider(QObject *parent = nullptr);
    ~OpenLibraryProvider() override;

    void requestMetadata(const QString &isbn) override;

private slots:
    void handleReply(QNetworkReply *reply);

private:
    std::unique_ptr<QNetworkAccessManager> m_networkManager;
    QString m_activeIsbn;
};

#endif // OPENLIBRARYPROVIDER_H
