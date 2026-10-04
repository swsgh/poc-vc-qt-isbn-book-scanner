#include "mainwindow.h"
#include "barcodescannerview.h"
#include "bookmetadataprovider.h"
#include "bookdatabasemanager.h"
#include "bookshelfwidget.h"
#include "bookdetailssidebar.h"
#include "booksyncmanager.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QMessageBox>
#include <QLineEdit>
#include <QSet>
#include <QInputDialog>
#include <QMenu>
#include <QAction>
#include <QPushButton>
#include <QToolButton>
#include <QStatusBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    initializeApplication();
}

void MainWindow::initializeApplication()
{
    setupUi();
    setupDatabase();
    populateBookshelf();

    m_syncManager = new BookSyncManager(
        qEnvironmentVariable("BOOKSHELF_SYNC_URL", "http://127.0.0.1:8000"), this);
    setupConnections();
    setupSync();
}

void MainWindow::setupUi()
{
    auto *centralWidget = new QWidget(this);
    centralWidget->setStyleSheet("background-color: #121212;");

    auto *mainVerticalLayout = new QVBoxLayout(centralWidget);
    mainVerticalLayout->setContentsMargins(12, 12, 12, 12);
    mainVerticalLayout->setSpacing(10);

    auto *controlsLayout = new QHBoxLayout;
    m_cameraToggleButton = new QPushButton("📷 Show Camera", centralWidget);
    m_cameraToggleButton->setObjectName("cameraToggleButton");
    m_cameraToggleButton->setToolTip("Show the camera preview and start capture");
    m_cameraToggleButton->setAccessibleName("Camera preview toggle");
    m_cameraToggleButton->setMinimumWidth(140);
    m_cameraToggleButton->setFixedHeight(36);
    m_cameraToggleButton->setStyleSheet(
        "QPushButton { background-color: #1e1e1e; border: 1px solid #3a3a3a; "
        "border-radius: 4px; font-size: 14px; padding: 0px 10px; }"
        "QPushButton:hover { border-color: #3498db; }");
    connect(m_cameraToggleButton, &QPushButton::clicked,
            this, &MainWindow::toggleCameraView);
    controlsLayout->addWidget(m_cameraToggleButton);
    controlsLayout->addStretch();

    m_settingsButton = new QToolButton(centralWidget);
    m_settingsButton->setText(QString::fromUtf8("⚙"));
    m_settingsButton->setToolTip("Account and synchronization options");
    m_settingsButton->setFixedSize(40, 36);
    m_settingsButton->setPopupMode(QToolButton::InstantPopup);
    m_settingsButton->setStyleSheet(
        "QToolButton { background-color: #1e1e1e; border: 1px solid #3a3a3a; "
        "border-radius: 4px; font-size: 20px; }"
        "QToolButton::menu-indicator { image: none; width: 0px; }"
        "QToolButton:hover { border-color: #3498db; }");
    controlsLayout->addWidget(m_settingsButton);
    mainVerticalLayout->addLayout(controlsLayout);

    m_scannerView = new BarcodeScannerView(this);
    m_scannerView->setMaximumSize(400, 220);
    m_scannerView->hide();
    mainVerticalLayout->addWidget(m_scannerView, 0, Qt::AlignHCenter);

    m_isbnLabel = new QLabel(this);
    m_isbnLabel->setAlignment(Qt::AlignCenter);
    applyStatusStyle("Align ISBN barcode with the red laser line...",
                     "#e0e0e0",
                     "#1e1e1e",
                     "#1e1e1e",
                     14,
                     8);
    m_isbnLabel->hide();
    mainVerticalLayout->addWidget(m_isbnLabel, 0);

    auto *bottomRowContainer = new QWidget(this);
    auto *bottomRowLayout = new QHBoxLayout(bottomRowContainer);
    bottomRowLayout->setContentsMargins(0, 5, 0, 0);

    auto *bookshelfAreaContainer = new QWidget(this);
    auto *bookshelfAreaLayout = new QVBoxLayout(bookshelfAreaContainer);
    bookshelfAreaLayout->setContentsMargins(0, 0, 0, 0);
    bookshelfAreaLayout->setSpacing(8);

    m_bookshelfWidget = new BookshelfWidget(this);
    bookshelfAreaLayout->addWidget(m_bookshelfWidget, 1);

    m_searchBar = new QLineEdit(this);
    m_searchBar->setPlaceholderText("🔍 Search by title, author, or ISBN...");
    m_searchBar->setClearButtonEnabled(true);
    m_searchBar->setStyleSheet(
        "QLineEdit {"
        "  background-color: #1e1e1e;"
        "  border: 1px solid #3a3a3a;"
        "  border-radius: 4px;"
        "  padding: 8px 12px;"
        "  font-size: 13px;"
        "  color: #ffffff;"
        "}"
        "QLineEdit:focus {"
        "  border: 1px solid #3498db;"
        "}"
        );
    bookshelfAreaLayout->addWidget(m_searchBar, 0);

    bottomRowLayout->addWidget(bookshelfAreaContainer, 3);

    m_detailsSidebar = new BookDetailsSidebar(this);
    m_detailsSidebar->setVisible(false);
    bottomRowLayout->addWidget(m_detailsSidebar, 1);

    mainVerticalLayout->addWidget(bottomRowContainer, 1);

    setCentralWidget(centralWidget);
    setWindowTitle("ISBN Book Scanner");
    resize(950, 750);

    auto *syncMenu = new QMenu(this);
    m_registerAction = syncMenu->addAction("Register account...");
    m_loginAction = syncMenu->addAction("Log in...");
    m_syncAction = syncMenu->addAction("Sync now");
    m_logoutAction = syncMenu->addAction("Log out");
    m_syncAction->setEnabled(false);
    m_logoutAction->setEnabled(false);
    syncMenu->addSeparator();
    m_clearLibraryAction = syncMenu->addAction("Clear Library Database...");
    m_settingsButton->setMenu(syncMenu);

    connect(m_registerAction, &QAction::triggered, this, &MainWindow::promptRegisterAccount);
    connect(m_loginAction, &QAction::triggered, this, &MainWindow::promptLoginAccount);
    connect(m_syncAction, &QAction::triggered, this, &MainWindow::syncNow);
    connect(m_logoutAction, &QAction::triggered, this, &MainWindow::logoutSync);
    connect(m_clearLibraryAction, &QAction::triggered,
            this, &MainWindow::clearLibraryDatabase);
}

void MainWindow::toggleCameraView()
{
    const bool isVisible = m_scannerView->isVisible();
    m_scannerView->setVisible(!isVisible);
    m_isbnLabel->setVisible(!isVisible);

    if (isVisible) {
        m_scannerView->stopCapture();
        m_cameraToggleButton->setText("📷 Show Camera");
        m_cameraToggleButton->setToolTip("Show the camera preview and start capture");
    } else {
        m_cameraToggleButton->setText("📷 Hide Camera");
        m_cameraToggleButton->setToolTip("Hide the camera preview and stop capture");
        m_scannerView->startCapture();
    }
}

void MainWindow::clearLibraryDatabase()
{
    const auto confirmation = QMessageBox::question(
        this,
        "Clear Library Database",
        "Delete all books from this library? Pending server deletions will be synchronized when connected.",
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No);
    if (confirmation != QMessageBox::Yes) return;

    if (!m_dbManager->clearBooksAndQueueDeletes()) return;

    m_bookshelfWidget->clearShelf();
    m_detailsSidebar->closeSidebar();
    m_searchBar->clear();

    if (m_syncManager->isAuthenticated()) {
        handleSyncQueueFlush();
    }

    statusBar()->showMessage("Library cleared; pending deletions are queued for synchronization.", 8000);
}

void MainWindow::setupDatabase()
{
    m_dbManager = new BookDatabaseManager(this);

    const QString appDataFolder = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(appDataFolder);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    const QString crossPlatformDbPath = QDir::cleanPath(appDataFolder + "/scanned_books.db");
    qDebug() << "[Database Info] Absolute SQL Path written to hardware:";
    qDebug() << "   ->" << crossPlatformDbPath;
    m_dbManager->initDatabase(crossPlatformDbPath);

    m_metadataProvider = new BookMetadataProvider(m_dbManager, this);
}

void MainWindow::setupConnections()
{
    connect(m_scannerView, &BarcodeScannerView::isbnScanned, m_metadataProvider, &BookMetadataProvider::lookupIsbn);
    connect(m_metadataProvider, &BookMetadataProvider::lookupStatusChanged, this, &MainWindow::updateStatusLabel);
    connect(m_metadataProvider, &BookMetadataProvider::bookDataReady, this, &MainWindow::displayBookDetails);
    connect(m_metadataProvider, &BookMetadataProvider::bookDataReady, m_dbManager, &BookDatabaseManager::saveBookRecord);

    connect(m_dbManager, &BookDatabaseManager::bookSavedSuccessfully, this, &MainWindow::handleBookSaved);
    connect(m_dbManager, &BookDatabaseManager::databaseError, this, [this](const QString &err) {
        updateStatusLabel(err, true);
    });

    connect(m_bookshelfWidget, &BookshelfWidget::bookSelected, m_detailsSidebar, &BookDetailsSidebar::updateDetails);
    connect(m_detailsSidebar, &BookDetailsSidebar::deleteBookRequested, this, &MainWindow::removeBookRecord);
    connect(m_searchBar, &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);
}

void MainWindow::setupSync()
{
    connect(m_syncManager, &BookSyncManager::uploadSucceeded, this, [this](const QString &isbn) {
        m_dbManager->removePendingAction(isbn, "UPLOAD");
    });
    connect(m_syncManager, &BookSyncManager::deleteSucceeded, this, [this](const QString &isbn) {
        m_dbManager->removePendingAction(isbn, "DELETE");
    });
    connect(m_syncManager, &BookSyncManager::loginSuccess, this, &MainWindow::handleLoginSuccess);
    connect(m_syncManager, &BookSyncManager::remoteBookUpdatesDownloaded,
            this, &MainWindow::handleRemoteBookUpdates);
    connect(m_syncManager, &BookSyncManager::syncCompleted,
            this, &MainWindow::handleSyncCompleted);
    connect(m_syncManager, &BookSyncManager::authStatusMessage,
            this, &MainWindow::updateStatusLabel);
    connect(m_syncManager, &BookSyncManager::networkErrorOccurred, this,
            [this](const QString &message) { updateStatusLabel(message, true); });
}

void MainWindow::promptRegisterAccount()
{
    bool accepted = false;
    const QString username = QInputDialog::getText(
        this, "Register Sync Account", "Username:", QLineEdit::Normal, {}, &accepted);
    if (!accepted || username.trimmed().isEmpty()) return;

    const QString password = QInputDialog::getText(
        this, "Register Sync Account", "Password:", QLineEdit::Password, {}, &accepted);
    if (!accepted || password.isEmpty()) return;

    m_syncManager->setSyncCheckpoint(
        m_dbManager->getSyncCheckpoint(username.trimmed()),
        m_dbManager->hasSyncCheckpoint(username.trimmed()));
    m_syncManager->registerAccount(username.trimmed(), password);
}

void MainWindow::promptLoginAccount()
{
    bool accepted = false;
    const QString username = QInputDialog::getText(
        this, "Log In to Sync", "Username:", QLineEdit::Normal, {}, &accepted);
    if (!accepted || username.trimmed().isEmpty()) return;

    const QString password = QInputDialog::getText(
        this, "Log In to Sync", "Password:", QLineEdit::Password, {}, &accepted);
    if (!accepted || password.isEmpty()) return;

    m_syncManager->setSyncCheckpoint(
        m_dbManager->getSyncCheckpoint(username.trimmed()),
        m_dbManager->hasSyncCheckpoint(username.trimmed()));
    m_syncManager->loginAccount(username.trimmed(), password);
}

void MainWindow::syncNow()
{
    if (!m_syncManager->isAuthenticated()) {
        updateStatusLabel("Log in before synchronizing.", true);
        return;
    }
    m_syncManager->triggerDifferentialSync();
}

void MainWindow::logoutSync()
{
    if (m_syncManager->isSyncRequestInFlight()) {
        updateStatusLabel("Wait for synchronization to finish before logging out.", true);
        return;
    }
    m_syncManager->logoutAccount();
    m_registerAction->setEnabled(true);
    m_loginAction->setEnabled(true);
    m_syncAction->setEnabled(false);
    m_logoutAction->setEnabled(false);
}

void MainWindow::handleLoginSuccess()
{
    m_registerAction->setEnabled(false);
    m_loginAction->setEnabled(false);
    m_syncAction->setEnabled(true);
    m_logoutAction->setEnabled(true);
}

void MainWindow::handleSyncCompleted(const QString &username, qint64 checkpoint,
                                     bool initialSync, const QStringList &remoteIsbns)
{
    m_dbManager->setSyncCheckpoint(username, checkpoint);

    if (initialSync) {
        const QSet<QString> serverBooks(remoteIsbns.begin(), remoteIsbns.end());
        for (const BookInfo &book : m_dbManager->getAllSavedBooks()) {
            if (!serverBooks.contains(book.isbn) && !m_dbManager->hasPendingAction(book.isbn)) {
                m_dbManager->addPendingUpload(book.isbn);
            }
        }
    }

    handleSyncQueueFlush();
    applyStatusStyle("Bookshelf synchronization complete.",
                     "#2ecc71", "#152c1e", "#27ae60", 14, 8);
}

void MainWindow::populateBookshelf()
{
    const QList<BookInfo> historicalBooks = m_dbManager->getAllSavedBooks();
    for (const BookInfo &book : historicalBooks) {
        m_bookshelfWidget->addBookToShelf(book, false);
    }
}

void MainWindow::applyStatusStyle(const QString &text,
                                 const QString &textColor,
                                 const QString &backgroundColor,
                                 const QString &borderColor,
                                 int fontSize,
                                 int padding)
{
    m_isbnLabel->setText(text);
    const QString style = QString(
        "font-size: %1px; font-weight: bold; color: %2; padding: %3px; background: %4; border: 1px solid %5; border-radius: 4px;"
    ).arg(fontSize).arg(textColor).arg(padding).arg(backgroundColor).arg(borderColor);
    m_isbnLabel->setStyleSheet(style);
}

void MainWindow::handleBookSaved(const QString &isbn)
{
    const BookInfo freshRecord = m_dbManager->getBookByIsbn(isbn);
    if (freshRecord.found) {
        m_bookshelfWidget->addBookToShelf(freshRecord, true);

        if (m_syncManager) {
            m_dbManager->addPendingUpload(isbn);
            m_syncManager->uploadBookToServer(freshRecord);
        }
    }

    applyStatusStyle("Scan complete! Local database repository successfully updated.",
                     "#2ecc71",
                     "#152c1e",
                     "#27ae60",
                     15,
                     15);
}

void MainWindow::handleSyncQueueFlush()
{
    qDebug() << "[Sync Engine] Connection validated. Processing offline pending queue backlog...";

    const QStringList deletes = m_dbManager->getPendingDeletes();
    for (const QString &isbn : deletes) {
        m_syncManager->deleteBookFromServer(isbn);
    }

    const QStringList uploads = m_dbManager->getPendingUploads();
    for (const QString &isbn : uploads) {
        const BookInfo book = m_dbManager->getBookByIsbn(isbn);
        if (book.found) {
            m_syncManager->uploadBookToServer(book);
        } else {
            m_dbManager->removePendingAction(isbn, "UPLOAD");
        }
    }
}

void MainWindow::handleRemoteBookUpdates(const QList<BookInfo> &booksToSave,
                                         const QStringList &isbnsToDelete)
{
    QSet<QString> pendingLocalIsbns;
    for (const QString &isbn : m_dbManager->getPendingUploads()) {
        pendingLocalIsbns.insert(isbn);
    }
    for (const QString &isbn : m_dbManager->getPendingDeletes()) {
        pendingLocalIsbns.insert(isbn);
    }

    QSet<QString> tombstones;
    for (const QString &isbn : isbnsToDelete) {
        if (pendingLocalIsbns.contains(isbn)) continue;
        tombstones.insert(isbn);
        m_dbManager->deleteBookRecord(isbn);
        m_bookshelfWidget->removeBookFromShelf(isbn);
    }

    for (const BookInfo &book : booksToSave) {
        if (tombstones.contains(book.isbn) || pendingLocalIsbns.contains(book.isbn)) {
            continue;
        }
        m_dbManager->saveRemoteBookRecord(book);
        m_bookshelfWidget->addBookToShelf(book, true);
    }
}

void MainWindow::updateStatusLabel(const QString &text, bool isError)
{
    if (isError) {
        qCritical() << "[Scanner System Error Alert]:\n" << text;
        applyStatusStyle(text,
                         "#ff6b6b",
                         "#2c1515",
                         "#e74c3c",
                         15,
                         10);
        return;
    }

    applyStatusStyle(text, "#f39c12", "#2c2215", "#d35400", 15, 10);
}

void MainWindow::displayBookDetails(const BookInfo &info)
{
    applyStatusStyle(QString("📖 Successfully scanned: %1").arg(info.title),
                     "#2ecc71",
                     "#152c1e",
                     "#27ae60",
                     15,
                     15);
    m_detailsSidebar->updateDetails(info);
}

void MainWindow::removeBookRecord(const QString &isbn)
{
    const BookInfo bookToPurge = m_dbManager->getBookByIsbn(isbn);
    const QString bookTitle = bookToPurge.found ? bookToPurge.title : "this book";

    const auto confirmation = QMessageBox::question(
        this,
        "Confirm Deletion",
        QString("Are you sure you want to permanently remove \"%1\" from your library archive?")
            .arg(bookTitle),
        QMessageBox::Yes | QMessageBox::No);

    if (confirmation == QMessageBox::No) {
        qDebug() << "[Archive Controller] Deletion sequence safely cancelled by user.";
        return;
    }

    qDebug() << "[Archive Controller] Initiating absolute purge sequence for ISBN:" << isbn;

    const bool success = m_dbManager->deleteBookRecord(isbn);
    if (!success) {
        updateStatusLabel("Failed to remove book from local database storage hierarchy.", true);
        return;
    }

    m_bookshelfWidget->removeBookFromShelf(isbn);
    m_detailsSidebar->closeSidebar();

    if (m_syncManager) {
        m_dbManager->addPendingDelete(isbn);
        m_syncManager->deleteBookFromServer(isbn);
    }

    applyStatusStyle("Book record removed successfully from shelf archive.",
                     "#7f8c8d",
                     "#ecf0f1",
                     "#7f8c8d",
                     14,
                     8);
}

void MainWindow::onSearchTextChanged(const QString &text)
{
    m_bookshelfWidget->filterBooks(text);
}
