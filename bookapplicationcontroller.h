#ifndef BOOKAPPLICATIONCONTROLLER_H
#define BOOKAPPLICATIONCONTROLLER_H

#include <QObject>
#include <QUrl>
#include <QVariantMap>
#include "bookinfo.h"

class BarcodeScannerController;
class BookMetadataProvider;
class BookDatabaseManager;
class BookSyncManager;
class BookSyncCoordinator;
class BookCollectionModel;
class QSortFilterProxyModel;
class QQmlApplicationEngine;

class BookApplicationController : public QObject
{
    Q_OBJECT

public:
    explicit BookApplicationController(QQmlApplicationEngine &engine, QObject *parent = nullptr);
    ~BookApplicationController() override;

    Q_INVOKABLE void selectBook(const QString &isbn);
    Q_INVOKABLE void clearSelectedBook();
    Q_INVOKABLE void removeSelectedBook();
    Q_INVOKABLE bool submitManualIsbn(const QString &input);
    Q_INVOKABLE void toggleScannerPanel();
    Q_INVOKABLE void syncNow();
    Q_INVOKABLE void logoutSync();
    Q_INVOKABLE QVariantMap submitSyncCredentials(bool registering, const QString &serverUrl,
                                                  const QString &username, const QString &password,
                                                  const QString &confirmation,
                                                  bool rememberUsername);
    Q_INVOKABLE void cancelSyncCredentialsSubmission();
    Q_INVOKABLE QVariantMap importBooksCsv(const QUrl &fileUrl);
    Q_INVOKABLE QVariantMap exportBooksCsv(const QUrl &fileUrl);
    Q_INVOKABLE QString defaultSyncServerUrl() const;
    Q_INVOKABLE QString rememberedSyncUsername() const;
    Q_INVOKABLE bool shouldRememberSyncUsername() const;
    Q_INVOKABLE void setBookSearchText(const QString &text);
    Q_INVOKABLE bool addManualBook(const QString &isbn, const QString &title,
                                   const QString &authors);

    Q_PROPERTY(bool selectedBookVisible READ selectedBookVisible NOTIFY selectedBookChanged)
    Q_PROPERTY(QString selectedBookTitle READ selectedBookTitle NOTIFY selectedBookChanged)
    Q_PROPERTY(QString selectedBookAuthors READ selectedBookAuthors NOTIFY selectedBookChanged)
    Q_PROPERTY(QString selectedBookIsbn READ selectedBookIsbn NOTIFY selectedBookChanged)
    Q_PROPERTY(QString selectedBookPublicationDate READ selectedBookPublicationDate NOTIFY selectedBookChanged)
    Q_PROPERTY(QString selectedBookPublisher READ selectedBookPublisher NOTIFY selectedBookChanged)
    Q_PROPERTY(int selectedBookPageCount READ selectedBookPageCount NOTIFY selectedBookChanged)
    Q_PROPERTY(QUrl selectedBookCoverSource READ selectedBookCoverSource NOTIFY selectedBookChanged)
    Q_PROPERTY(bool scannerVisible READ scannerVisible NOTIFY scannerVisibilityChanged)
    Q_PROPERTY(QString scannerStatusText READ scannerStatusText NOTIFY scannerStatusChanged)
    Q_PROPERTY(bool syncAuthenticated READ syncAuthenticated NOTIFY syncStateChanged)
    Q_PROPERTY(bool syncServerReachable READ syncServerReachable NOTIFY syncConnectionChanged)
    Q_PROPERTY(QString applicationStatusText READ applicationStatusText NOTIFY applicationStatusChanged)

    bool selectedBookVisible() const;
    QString selectedBookTitle() const;
    QString selectedBookAuthors() const;
    QString selectedBookIsbn() const;
    QString selectedBookPublicationDate() const;
    QString selectedBookPublisher() const;
    int selectedBookPageCount() const;
    QUrl selectedBookCoverSource() const;
    bool scannerVisible() const;
    QString scannerStatusText() const;
    bool syncAuthenticated() const;
    bool syncServerReachable() const;
    QString applicationStatusText() const;

signals:
    void selectedBookChanged();
    void scannerVisibilityChanged();
    void scannerStatusChanged();
    void syncStateChanged();
    void syncConnectionChanged();
    void applicationStatusChanged();
    void bookLookupNotFound(const QString &isbn);
    void syncCredentialsSubmissionFinished(bool success, const QString &message);

private slots:
    void updateStatusLabel(const QString &text, bool isError);
    void displayBookDetails(const BookInfo &info);
    void handleNearbyPermissionResult(int requestCode, bool granted);

private:
    struct PendingCredentials {
        bool active = false;
        bool registering = false;
        bool rememberUsername = false;
        int requestCode = 0;
        QString serverUrl;
        QString username;
        QString password;
    };

    void initializeApplication(QQmlApplicationEngine &engine);
    void startSyncWithCredentials(const PendingCredentials &credentials);
    QVariantMap credentialSubmissionResult(const QString &status,
                                           const QString &message = {}) const;
    void setupDatabase();
    void setupConnections();
    void setupSync();
    void populateBookshelf();
    void setScannerStatus(const QString &text);
    void setApplicationStatus(const QString &text, int durationMs = 5000);
    void lookupIsbnOnServer(const QString &isbn);
    void updateSyncConnectionIndicator(bool connected);
    void handleLoginSuccess();

    BarcodeScannerController* m_scannerController = nullptr;
    BookMetadataProvider* m_metadataProvider = nullptr;
    BookDatabaseManager* m_dbManager = nullptr;
    BookSyncManager* m_syncManager = nullptr;
    BookSyncCoordinator* m_syncCoordinator = nullptr;
    BookCollectionModel* m_bookCollectionModel = nullptr;
    QSortFilterProxyModel* m_bookFilterModel = nullptr;
    BookInfo m_selectedBook;
    QString m_scannerStatusText = "Center an ISBN barcode to add a book";
    bool m_scannerVisible = false;
    bool m_syncServerReachable = false;
    QString m_applicationStatusText;
    quint64 m_applicationStatusGeneration = 0;
    PendingCredentials m_pendingCredentials;
    int m_nextPermissionRequestCode = 0x51A;
};

#endif // BOOKAPPLICATIONCONTROLLER_H
