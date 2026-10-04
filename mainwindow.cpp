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
#include <algorithm>

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
    m_cameraToggleButton = new QPushButton("📷 Open Scanner Suite", centralWidget);
    m_cameraToggleButton->setObjectName("cameraToggleButton");
    m_cameraToggleButton->setToolTip("Open the scanner view and start camera capture");
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

    auto *manualLookupLayout = new QHBoxLayout;
    m_manualIsbnInput = new QLineEdit(centralWidget);
    m_manualIsbnInput->setPlaceholderText("Type an ISBN code manually (e.g. 9781449392178)...");
    m_manualIsbnInput->setMaxLength(17);
    m_manualIsbnInput->setStyleSheet(
        "QLineEdit { background-color: #121212; border: 1px solid #121212; "
        "border-radius: 6px; padding: 10px; color: #ffffff; font-size: 14px; }"
        "QLineEdit:focus { border: 1px solid #3498db; }");
    m_manualLookupButton = new QPushButton("🔍 Lookup", centralWidget);
    m_manualLookupButton->setMinimumSize(110, 40);
    m_manualLookupButton->setStyleSheet(
        "QPushButton { background-color: #121212; color: #ffffff; "
        "border: 1px solid #3498db; border-radius: 6px; padding: 10px; "
        "font-weight: bold; }"
        "QPushButton:hover { background-color: #3498db; color: #ffffff; }");
    connect(m_manualIsbnInput, &QLineEdit::returnPressed,
            this, &MainWindow::submitManualIsbn);
    connect(m_manualLookupButton, &QPushButton::clicked,
            this, &MainWindow::submitManualIsbn);
    manualLookupLayout->addWidget(m_manualIsbnInput, 1);
    manualLookupLayout->addWidget(m_manualLookupButton);
    m_manualIsbnInput->hide();
    m_manualLookupButton->hide();
    mainVerticalLayout->addLayout(manualLookupLayout);

    m_statusLabel = new QLabel(this);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    applyStatusStyle("Center an ISBN barcode to log a book");
    m_statusLabel->hide();
    mainVerticalLayout->addWidget(m_statusLabel, 0);

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
    m_searchBar->setPlaceholderText("🔎 Type to filter bookshelf by title or author name...");
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
    m_registerAction = syncMenu->addAction("Register Sync Account...");
    m_loginAction = syncMenu->addAction("Log In to Sync...");
    m_syncAction = syncMenu->addAction("Sync Now");
    m_logoutAction = syncMenu->addAction("Log Out of Sync");
    m_syncAction->setEnabled(false);
    m_logoutAction->setEnabled(false);
    syncMenu->addSeparator();
    m_clearLibraryAction = syncMenu->addAction("🗑️ Clear Library Database");
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
    m_statusLabel->setVisible(!isVisible);
    m_manualIsbnInput->setVisible(!isVisible);
    m_manualLookupButton->setVisible(!isVisible);

    if (isVisible) {
        m_scannerView->stopCapture();
        m_cameraToggleButton->setText("📷 Open Scanner Suite");
        m_cameraToggleButton->setToolTip("Open the scanner view and start camera capture");
    } else {
        m_cameraToggleButton->setText("🙈 Hide Scanner Suite");
        m_cameraToggleButton->setToolTip("Hide the scanner view and stop camera capture");
        m_scannerView->startCapture();
    }
}

void MainWindow::submitManualIsbn()
{
    QString isbn = m_manualIsbnInput->text().trimmed();
    isbn.remove(QLatin1Char('-'));

    const bool validLength = isbn.length() == 10 || isbn.length() == 13;
    const bool containsOnlyDigits = std::all_of(
        isbn.cbegin(), isbn.cend(), [](QChar character) {
            return character >= QLatin1Char('0') && character <= QLatin1Char('9');
        });
    if (!validLength || !containsOnlyDigits) {
        QMessageBox::warning(this, "Invalid Input",
                             "ISBN must be a string of 10 or 13 numbers.");
        return;
    }

    m_manualIsbnInput->clear();
    m_metadataProvider->lookupIsbn(isbn);
}

void MainWindow::clearLibraryDatabase()
{
    const auto confirmation = QMessageBox::question(
        this,
        "Clear Entire Library?",
        "Are you completely sure you want to purge all books from the database and UI grid view?",
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

    applyStatusStyle("🧹 Library database completely wiped.");
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
            [this](const QString &message) {
                updateStatusLabel("Sync failed: " + message, true);
            });
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
        updateStatusLabel("Wait for the current sync to finish before logging out.", true);
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
    applyStatusStyle("Bookshelf synchronization complete.");
}

void MainWindow::populateBookshelf()
{
    const QList<BookInfo> historicalBooks = m_dbManager->getAllSavedBooks();
    for (const BookInfo &book : historicalBooks) {
        m_bookshelfWidget->addBookToShelf(book, false);
    }
}

void MainWindow::applyStatusStyle(const QString &text, const QString &textColor)
{
    m_statusLabel->setText(text);
    QString style = "font-weight: bold; font-size: 13px; padding: 3px; "
                    "color: #e0e0e0; background: transparent; border: none;";
    if (!textColor.isEmpty()) {
        style += " color: " + textColor + ";";
    }
    m_statusLabel->setStyleSheet(style);
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

    applyStatusStyle(QString("✅ Logged: %1").arg(freshRecord.title));
}

void MainWindow::handleSyncQueueFlush()
{
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
        applyStatusStyle(text, "#ff6b6b");
        return;
    }

    applyStatusStyle(text);
}

void MainWindow::displayBookDetails(const BookInfo &info)
{
    applyStatusStyle(QString("📖 Successfully scanned: %1").arg(info.title));
    m_detailsSidebar->updateDetails(info);
}

void MainWindow::removeBookRecord(const QString &isbn)
{
    const auto confirmation = QMessageBox::question(
        this,
        "Remove Book",
        "Are you sure you want to remove this book from your collection?",
        QMessageBox::Yes | QMessageBox::No);

    if (confirmation == QMessageBox::No) {
        return;
    }

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

    applyStatusStyle("🗑️ Book removed from collection.");
}

void MainWindow::onSearchTextChanged(const QString &text)
{
    m_bookshelfWidget->filterBooks(text);
}
