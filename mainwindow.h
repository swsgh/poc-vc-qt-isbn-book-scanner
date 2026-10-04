#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QUrl>
#include "bookinfo.h"

class BarcodeScannerController;
class BookMetadataProvider;
class BookDatabaseManager;
class BookSyncManager;
class BookSyncCoordinator;
class BookCollectionModel;
class QSortFilterProxyModel;
class QQuickWidget;
class QEvent;
class QPalette;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

    Q_INVOKABLE void selectBook(const QString &isbn);
    Q_INVOKABLE void clearSelectedBook();
    Q_INVOKABLE void removeSelectedBook();
    Q_INVOKABLE bool submitManualIsbn(const QString &input);
    Q_INVOKABLE void toggleScannerPanel();
    Q_INVOKABLE void syncNow();
    Q_INVOKABLE void logoutSync();
    Q_INVOKABLE QString submitSyncCredentials(bool registering, const QString &serverUrl,
                                              const QString &username, const QString &password,
                                              const QString &confirmation,
                                              bool rememberUsername);
    Q_INVOKABLE QVariantMap importBooksCsv(const QUrl &fileUrl);
    Q_INVOKABLE QVariantMap exportBooksCsv(const QUrl &fileUrl);
    Q_INVOKABLE QString defaultSyncServerUrl() const;
    Q_INVOKABLE QString rememberedSyncUsername() const;
    Q_INVOKABLE bool shouldRememberSyncUsername() const;
    Q_INVOKABLE void setBookSearchText(const QString &text);

    Q_PROPERTY(bool selectedBookVisible READ selectedBookVisible NOTIFY selectedBookChanged)
    Q_PROPERTY(QString selectedBookTitle READ selectedBookTitle NOTIFY selectedBookChanged)
    Q_PROPERTY(QString selectedBookAuthors READ selectedBookAuthors NOTIFY selectedBookChanged)
    Q_PROPERTY(QString selectedBookIsbn READ selectedBookIsbn NOTIFY selectedBookChanged)
    Q_PROPERTY(QString selectedBookMetadata READ selectedBookMetadata NOTIFY selectedBookChanged)
    Q_PROPERTY(QUrl selectedBookCoverSource READ selectedBookCoverSource NOTIFY selectedBookChanged)
    Q_PROPERTY(bool scannerVisible READ scannerVisible NOTIFY scannerVisibilityChanged)
    Q_PROPERTY(QString scannerStatusText READ scannerStatusText NOTIFY scannerStatusChanged)
    Q_PROPERTY(QString scannerStatusColor READ scannerStatusColor NOTIFY scannerStatusChanged)
    Q_PROPERTY(bool syncAuthenticated READ syncAuthenticated NOTIFY syncStateChanged)
    Q_PROPERTY(bool syncServerReachable READ syncServerReachable NOTIFY syncConnectionChanged)

    bool selectedBookVisible() const;
    QString selectedBookTitle() const;
    QString selectedBookAuthors() const;
    QString selectedBookIsbn() const;
    QString selectedBookMetadata() const;
    QUrl selectedBookCoverSource() const;
    bool scannerVisible() const;
    QString scannerStatusText() const;
    QString scannerStatusColor() const;
    bool syncAuthenticated() const;
    bool syncServerReachable() const;

signals:
    void selectedBookChanged();
    void scannerVisibilityChanged();
    void scannerStatusChanged();
    void syncStateChanged();
    void syncConnectionChanged();

private slots:
    void updateStatusLabel(const QString &text, bool isError);
    void displayBookDetails(const BookInfo &info);

protected:
    void changeEvent(QEvent *event) override;

private:
    void initializeApplication();
    void setupUi();
    void setupDatabase();
    void setupConnections();
    void setupSync();
    void populateBookshelf();
    void applyPaletteStyles(const QPalette &palette);
    void applyStatusStyle(const QString &text, const QString &textColor = {});
    void updateSyncConnectionIndicator(bool connected);
    void handleLoginSuccess();

    BarcodeScannerController* m_scannerController = nullptr;
    BookMetadataProvider* m_metadataProvider = nullptr;
    BookDatabaseManager* m_dbManager = nullptr;
    BookSyncManager* m_syncManager = nullptr;
    BookSyncCoordinator* m_syncCoordinator = nullptr;
    BookCollectionModel* m_bookCollectionModel = nullptr;
    QSortFilterProxyModel* m_bookFilterModel = nullptr;
    QQuickWidget* m_mainQuickWidget = nullptr;
    BookInfo m_selectedBook;
    QString m_scannerStatusText = "Center an ISBN barcode to add a book";
    QString m_statusTextColor;
    bool m_scannerVisible = false;
    bool m_syncServerReachable = false;
    bool m_applyingPalette = false;
};

#endif // MAINWINDOW_H
