#ifndef BOOKSYNCMANAGER_H
#define BOOKSYNCMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include "bookinfo.h"

class BookSyncManager : public QObject
{
    Q_OBJECT

public:
    explicit BookSyncManager(const QString &serverUrl, QObject *parent = nullptr);
    ~BookSyncManager() override = default;

    // Session Management API
    void registerAccount(const QString &username, const QString &password);
    void loginAccount(const QString &username, const QString &password);
    bool isAuthenticated() const { return !m_token.isEmpty(); }

    // Sync API
    void uploadBookToServer(const BookInfo &info);
    void deleteBookFromServer(const QString &isbn);
    void triggerDifferentialSync();

signals:
    // Routing pipelines targeting UI components and local SQLite databases
    void authStatusMessage(const QString &message, bool isError);
    void loginSuccess();
    void remoteBookUpdatesDownloaded(const QList<BookInfo> &booksToSave, const QStringList &isbnsToDelete);
    void networkErrorOccurred(const QString &errorMsg);
    void uploadSucceeded(const QString &isbn);
    void deleteSucceeded(const QString &isbn);

private:
    QString m_serverUrl;
    QString m_token;
    qint64 m_lastSyncTimestamp; // Maps down to SQLite Unix tracked epochs
    QNetworkAccessManager *m_networkManager;

    // Private helpers to build injection-safe header requirements
    QNetworkRequest createJsonRequest(const QString &endpointPath) const;
    QNetworkRequest createAuthenticatedRequest(const QString &endpointPath);
    void handleSyncResponse(const QByteArray &jsonResponse);
};

#endif // BOOKSYNCMANAGER_H
