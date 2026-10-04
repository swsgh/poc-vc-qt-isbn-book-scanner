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
    void setServerUrl(const QString &serverUrl);
    void logoutAccount();
    void setSyncCheckpoint(qint64 timestamp, bool hasCheckpoint);
    bool isAuthenticated() const { return !m_token.isEmpty(); }
    bool isSyncRequestInFlight() const { return m_syncRequestInFlight; }
    QString currentUsername() const { return m_username; }

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
    void syncCompleted(const QString &username, qint64 checkpoint, bool initialSync,
                       const QStringList &remoteIsbns);

private:
    QString m_serverUrl;
    QString m_token;
    QString m_username;
    qint64 m_lastSyncTimestamp; // Maps down to SQLite Unix tracked epochs
    bool m_hasSyncCheckpoint = false;
    bool m_syncRequestInFlight = false;
    QNetworkAccessManager *m_networkManager;

    // Private helpers to build injection-safe header requirements
    QNetworkRequest createJsonRequest(const QString &endpointPath) const;
    QNetworkRequest createAuthenticatedRequest(const QString &endpointPath);
    void handleSyncResponse(const QByteArray &jsonResponse);
};

#endif // BOOKSYNCMANAGER_H
