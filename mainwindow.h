#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "bookinfo.h"

class BarcodeScannerView;
class BookMetadataProvider;
class BookDatabaseManager;
class BookshelfWidget;
class BookDetailsSidebar;
class BookSyncManager;
class QAction;
class QLabel;
class QLineEdit;
class QPushButton;
class QToolButton;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void updateStatusLabel(const QString &text, bool isError);
    void displayBookDetails(const BookInfo &info);
    void removeBookRecord(const QString &isbn);
    void onSearchTextChanged(const QString &text);
    void promptRegisterAccount();
    void promptLoginAccount();
    void syncNow();
    void logoutSync();
    void toggleCameraView();
    void handleSyncCompleted(const QString &username, qint64 checkpoint,
                             bool initialSync, const QStringList &remoteIsbns);

private:
    void initializeApplication();
    void setupUi();
    void setupDatabase();
    void setupConnections();
    void setupSync();
    void populateBookshelf();
    void applyStatusStyle(const QString &text,
                          const QString &textColor,
                          const QString &backgroundColor,
                          const QString &borderColor,
                          int fontSize,
                          int padding);
    void handleBookSaved(const QString &isbn);
    void handleSyncQueueFlush();
    void handleLoginSuccess();
    void handleRemoteBookUpdates(const QList<BookInfo> &booksToSave,
                                 const QStringList &isbnsToDelete);

    BarcodeScannerView* m_scannerView = nullptr;
    BookMetadataProvider* m_metadataProvider = nullptr;
    BookDatabaseManager* m_dbManager = nullptr;
    BookshelfWidget* m_bookshelfWidget = nullptr;
    BookDetailsSidebar* m_detailsSidebar = nullptr;
    BookSyncManager* m_syncManager = nullptr;
    QLabel* m_isbnLabel = nullptr;
    QLineEdit* m_searchBar = nullptr;
    QPushButton* m_cameraToggleButton = nullptr;
    QToolButton* m_settingsButton = nullptr;
    QAction* m_registerAction = nullptr;
    QAction* m_loginAction = nullptr;
    QAction* m_syncAction = nullptr;
    QAction* m_logoutAction = nullptr;
};

#endif // MAINWINDOW_H
