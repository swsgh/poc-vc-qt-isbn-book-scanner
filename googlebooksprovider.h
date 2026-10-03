#ifndef GOOGLEBOOKSPROVIDER_H
#define GOOGLEBOOKSPROVIDER_H

#include "abstractbookprovider.h"
#include <memory>

class QNetworkAccessManager;
class QNetworkReply;

class GoogleBooksProvider : public AbstractBookProvider
{
    Q_OBJECT
public:
    explicit GoogleBooksProvider(QObject *parent = nullptr);
    ~GoogleBooksProvider() override;

    void requestMetadata(const QString &isbn) override;

private slots:
    void handleReply(QNetworkReply *reply);

private:
    std::unique_ptr<QNetworkAccessManager> m_networkManager;
    QString m_activeIsbn;
};

#endif // GOOGLEBOOKSPROVIDER_H
