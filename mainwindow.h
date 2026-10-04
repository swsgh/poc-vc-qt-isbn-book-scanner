#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QUrl>
#include "bookinfo.h"

class BarcodeScannerView;
class BookMetadataProvider;
class BookDatabaseManager;
class BookSyncManager;
class BookSyncCoordinator;
class BookCollectionModel;
class QSortFilterProxyModel;
class QQuickWidget;
class QAction;
class QEvent;
class QLabel;
class QLineEdit;
class QPalette;
class QPushButton;
class QToolButton;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

    Q_INVOKABLE void selectBook(const QString &isbn);
    Q_INVOKABLE void clearSelectedBook();
    Q_INVOKABLE void removeSelectedBook();

    Q_PROPERTY(bool selectedBookVisible READ selectedBookVisible NOTIFY selectedBookChanged)
    Q_PROPERTY(QString selectedBookTitle READ selectedBookTitle NOTIFY selectedBookChanged)
    Q_PROPERTY(QString selectedBookAuthors READ selectedBookAuthors NOTIFY selectedBookChanged)
    Q_PROPERTY(QString selectedBookIsbn READ selectedBookIsbn NOTIFY selectedBookChanged)
    Q_PROPERTY(QString selectedBookMetadata READ selectedBookMetadata NOTIFY selectedBookChanged)
    Q_PROPERTY(QUrl selectedBookCoverSource READ selectedBookCoverSource NOTIFY selectedBookChanged)

    bool selectedBookVisible() const;
    QString selectedBookTitle() const;
    QString selectedBookAuthors() const;
    QString selectedBookIsbn() const;
    QString selectedBookMetadata() const;
    QUrl selectedBookCoverSource() const;

signals:
    void selectedBookChanged();

private slots:
    void updateStatusLabel(const QString &text, bool isError);
    void displayBookDetails(const BookInfo &info);
    void removeBookRecord(const QString &isbn);
    void onSearchTextChanged(const QString &text);
    void submitManualIsbn();
    void promptRegisterAccount();
    void promptLoginAccount();
    void syncNow();
    void logoutSync();
    void toggleCameraView();
    void importBooksCsv();
    void exportBooksCsv();

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

    BarcodeScannerView* m_scannerView = nullptr;
    BookMetadataProvider* m_metadataProvider = nullptr;
    BookDatabaseManager* m_dbManager = nullptr;
    BookSyncManager* m_syncManager = nullptr;
    BookSyncCoordinator* m_syncCoordinator = nullptr;
    BookCollectionModel* m_bookCollectionModel = nullptr;
    QSortFilterProxyModel* m_bookFilterModel = nullptr;
    QQuickWidget* m_bookshelfQuickWidget = nullptr;
    QQuickWidget* m_bookDetailsQuickWidget = nullptr;
    BookInfo m_selectedBook;
    QWidget* m_scannerPanel = nullptr;
    QLabel* m_statusLabel = nullptr;
    QLabel* m_syncConnectionIndicator = nullptr;
    QLineEdit* m_searchBar = nullptr;
    QLineEdit* m_manualIsbnInput = nullptr;
    QPushButton* m_cameraToggleButton = nullptr;
    QPushButton* m_manualLookupButton = nullptr;
    QToolButton* m_settingsButton = nullptr;
    QAction* m_registerAction = nullptr;
    QAction* m_loginAction = nullptr;
    QAction* m_syncAction = nullptr;
    QAction* m_logoutAction = nullptr;
    QString m_statusTextColor;
    bool m_applyingPalette = false;
};

#endif // MAINWINDOW_H
