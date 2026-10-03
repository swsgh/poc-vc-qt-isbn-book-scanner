#ifndef BOOKMETADATAPRODUCER_H
#define BOOKMETADATAPRODUCER_H

#include <QObject>
#include <QString>
#include <memory>

class QNetworkAccessManager;
class QNetworkReply;

// Simple structure to pass parsed book details cleanly back to the UI
struct BookInfo {
    bool found = false;
    QString isbn;
    QString title;
    QString authors;
    QString errorString;
};

class BookMetadataProvider : public QObject
{
    Q_OBJECT
public:
    explicit BookMetadataProvider(QObject *parent = nullptr);
    ~BookMetadataProvider() override;

    // Checks the debounce rules and triggers the asynchronous API download loop
    void lookupIsbn(const QString &isbn);

signals:
    // Emitted when a book lookup status updates (loading, success, or error)
    void lookupStatusChanged(const QString &statusText, bool isError);
    // Emitted when book data is successfully fetched and fully parsed
    void bookDataReady(const BookInfo &info);

private slots:
    void handleNetworkReply(QNetworkReply *reply);
    void resetScannerCooldown();

private:
    std::unique_ptr<QNetworkAccessManager> m_networkManager;

    QString m_lastScannedIsbn;
    bool m_isCooldownActive;
};

#endif // BOOKMETADATAPRODUCER_H
