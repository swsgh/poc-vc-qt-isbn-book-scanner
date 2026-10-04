#include "mainwindow.h"
#include "barcodescannerview.h"
#include "bookmetadataprovider.h"
#include "bookdatabasemanager.h"
#include "bookshelfwidget.h"
#include "bookdetailssidebar.h"
#include "booksyncmanager.h"
#include "booksynccoordinator.h"
#include "bookcollectionmodel.h"
#include "covercache.h"
#include "bookcsv.h"
#include "synccredentialsdialog.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QMessageBox>
#include <QLineEdit>
#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QEvent>
#include <QPalette>
#include <QSettings>
#include <QFrame>
#include <QPushButton>
#include <QToolButton>
#include <QStatusBar>
#include <QSizePolicy>
#include <QSortFilterProxyModel>
#include <QFileDialog>
#include <algorithm>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setPalette(QApplication::palette());
    setAutoFillBackground(true);
    setAttribute(Qt::WA_StyledBackground, true);
    initializeApplication();
}

void MainWindow::changeEvent(QEvent *event)
{
    QMainWindow::changeEvent(event);
    const bool paletteChanged = event->type() == QEvent::ApplicationPaletteChange
        || event->type() == QEvent::PaletteChange;
    if (paletteChanged && centralWidget() && !m_applyingPalette) {
        applyPaletteStyles(QApplication::palette());
    }
}

void MainWindow::initializeApplication()
{
    m_bookCollectionModel = new BookCollectionModel(this);
    m_bookFilterModel = new QSortFilterProxyModel(this);
    m_bookFilterModel->setSourceModel(m_bookCollectionModel);
    m_bookFilterModel->setFilterRole(BookCollectionModel::SearchTextRole);
    m_bookFilterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

    setupUi();
    applyPaletteStyles(QApplication::palette());
    setupDatabase();
    populateBookshelf();

    QSettings settings;
    const QString configuredUrl = qEnvironmentVariable("BOOKSHELF_SYNC_URL");
    const QString serverUrl = configuredUrl.isEmpty()
        ? settings.value("sync/server_url", "http://127.0.0.1:8000").toString()
        : configuredUrl;
    m_syncManager = new BookSyncManager(serverUrl, this);
    setupConnections();
    setupSync();
}

void MainWindow::setupUi()
{
    auto *centralWidget = new QWidget(this);
    centralWidget->setAutoFillBackground(true);

    auto *mainVerticalLayout = new QVBoxLayout(centralWidget);
    mainVerticalLayout->setContentsMargins(12, 12, 12, 12);
    mainVerticalLayout->setSpacing(10);

    auto *controlsLayout = new QHBoxLayout;
    m_cameraToggleButton = new QPushButton("📷 Show Camera Preview", centralWidget);
    m_cameraToggleButton->setObjectName("cameraToggleButton");
    m_cameraToggleButton->setToolTip("Open the scanner view and start camera capture");
    m_cameraToggleButton->setAccessibleName("Camera preview toggle");
    m_cameraToggleButton->setMinimumWidth(140);
    m_cameraToggleButton->setFixedHeight(36);
    connect(m_cameraToggleButton, &QPushButton::clicked,
            this, &MainWindow::toggleCameraView);
    controlsLayout->addWidget(m_cameraToggleButton);
    controlsLayout->addStretch();

    m_syncConnectionIndicator = new QLabel(centralWidget);
    m_syncConnectionIndicator->setObjectName("syncConnectionIndicator");
    m_syncConnectionIndicator->setFixedSize(12, 12);
    m_syncConnectionIndicator->setToolTip("Log in to check sync server");
    m_syncConnectionIndicator->setAccessibleName("Sync server connection status");
    m_syncConnectionIndicator->setStyleSheet(
        "QLabel { background-color: #8a929c; border-radius: 6px; }");
    controlsLayout->addWidget(m_syncConnectionIndicator, 0, Qt::AlignVCenter);

    m_settingsButton = new QToolButton(centralWidget);
    m_settingsButton->setObjectName("settingsButton");
    m_settingsButton->setText(QString::fromUtf8("⚙"));
    m_settingsButton->setPopupMode(QToolButton::InstantPopup);
    controlsLayout->addWidget(m_settingsButton);
    mainVerticalLayout->addLayout(controlsLayout);

    m_scannerPanel = new QWidget(centralWidget);
    m_scannerPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);
    auto *scannerLayout = new QVBoxLayout(m_scannerPanel);
    scannerLayout->setContentsMargins(0, 0, 0, 0);
    scannerLayout->setSpacing(6);

    m_scannerView = new BarcodeScannerView(m_scannerPanel);
    m_scannerView->setMaximumWidth(932);
    m_scannerView->setMaximumHeight(260);
    scannerLayout->addWidget(m_scannerView, 0, Qt::AlignHCenter);

    auto *manualLookupLayout = new QHBoxLayout;
    m_manualIsbnInput = new QLineEdit(m_scannerPanel);
    m_manualIsbnInput->setObjectName("manualIsbnInput");
    m_manualIsbnInput->setPlaceholderText("Type an ISBN code manually (e.g. 9781449392178)...");
    m_manualIsbnInput->setMaxLength(17);
    m_manualLookupButton = new QPushButton("🔍 Lookup", m_scannerPanel);
    m_manualLookupButton->setObjectName("manualLookupButton");
    m_manualLookupButton->setMinimumSize(110, 40);
    connect(m_manualIsbnInput, &QLineEdit::returnPressed,
            this, &MainWindow::submitManualIsbn);
    connect(m_manualLookupButton, &QPushButton::clicked,
            this, &MainWindow::submitManualIsbn);
    manualLookupLayout->addWidget(m_manualIsbnInput, 1);
    manualLookupLayout->addWidget(m_manualLookupButton);
    scannerLayout->addLayout(manualLookupLayout);

    m_statusLabel = new QLabel(m_scannerPanel);
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->setFrameShape(QFrame::NoFrame);
    m_statusLabel->setAutoFillBackground(false);
    applyStatusStyle("Center an ISBN barcode to add a book");
    scannerLayout->addWidget(m_statusLabel, 0);

    m_scannerPanel->hide();
    mainVerticalLayout->addWidget(m_scannerPanel);

    auto *bottomRowContainer = new QWidget(this);
    auto *bottomRowLayout = new QHBoxLayout(bottomRowContainer);
    bottomRowLayout->setContentsMargins(0, 5, 0, 0);

    auto *bookshelfAreaContainer = new QWidget(this);
    auto *bookshelfAreaLayout = new QVBoxLayout(bookshelfAreaContainer);
    bookshelfAreaLayout->setContentsMargins(0, 0, 0, 0);
    bookshelfAreaLayout->setSpacing(8);

    m_bookshelfWidget = new BookshelfWidget(this);
    m_bookshelfWidget->setModel(m_bookFilterModel);
    bookshelfAreaLayout->addWidget(m_bookshelfWidget, 1);

    m_searchBar = new QLineEdit(this);
    m_searchBar->setObjectName("bookSearchBar");
    m_searchBar->setPlaceholderText("🔎 Type to filter bookshelf by title or author name...");
    m_searchBar->setClearButtonEnabled(true);
    bookshelfAreaLayout->addWidget(m_searchBar, 0);

    bottomRowLayout->addWidget(bookshelfAreaContainer, 3);

    m_detailsSidebar = new BookDetailsSidebar(this);
    m_detailsSidebar->setVisible(false);
    bottomRowLayout->addWidget(m_detailsSidebar, 1);

    mainVerticalLayout->addWidget(bottomRowContainer, 1);

    setCentralWidget(centralWidget);
    setWindowTitle("ISBN Book Scanner");
    resize(950, 900);

    auto *syncMenu = new QMenu(this);
    m_loginAction = syncMenu->addAction("Log In to Sync...");
    m_syncAction = syncMenu->addAction("Sync Now");
    m_logoutAction = syncMenu->addAction("Log Out of Sync");
    m_loginAction->setEnabled(true);
    m_syncAction->setEnabled(false);
    m_logoutAction->setEnabled(false);
    syncMenu->addSeparator();
    m_registerAction = syncMenu->addAction("Register Sync Account...");
    syncMenu->addSeparator();
    QAction *importCsvAction = syncMenu->addAction("Import CSV...");
    QAction *exportCsvAction = syncMenu->addAction("Export CSV...");
    m_settingsButton->setMenu(syncMenu);

    connect(m_registerAction, &QAction::triggered, this, &MainWindow::promptRegisterAccount);
    connect(m_loginAction, &QAction::triggered, this, &MainWindow::promptLoginAccount);
    connect(m_syncAction, &QAction::triggered, this, &MainWindow::syncNow);
    connect(m_logoutAction, &QAction::triggered, this, &MainWindow::logoutSync);
    connect(importCsvAction, &QAction::triggered, this, &MainWindow::importBooksCsv);
    connect(exportCsvAction, &QAction::triggered, this, &MainWindow::exportBooksCsv);
}

void MainWindow::exportBooksCsv()
{
    const QList<BookInfo> books = m_dbManager->getAllSavedBooks();
    if (books.isEmpty()) {
        QMessageBox::information(this, "Export CSV", "There are no books to export.");
        return;
    }

    QString filePath = QFileDialog::getSaveFileName(
        this, "Export Books to CSV",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        "CSV files (*.csv)");
    if (filePath.isEmpty()) return;
    if (!filePath.endsWith(".csv", Qt::CaseInsensitive)) filePath += ".csv";

    QString error;
    if (!BookCsv::writeFile(filePath, books, error)) {
        QMessageBox::critical(this, "Export CSV Failed", error);
        return;
    }
    QMessageBox::information(this, "Export CSV",
                             QString("Exported %1 books to:\n%2").arg(books.size()).arg(filePath));
}

void MainWindow::importBooksCsv()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this, "Import Books from CSV",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation),
        "CSV files (*.csv)");
    if (filePath.isEmpty()) return;

    QList<BookInfo> importedBooks;
    int skippedCount = 0;
    QString error;
    if (!BookCsv::readFile(filePath, importedBooks, skippedCount, error)) {
        QMessageBox::warning(this, "Import CSV Failed", error);
        return;
    }

    int importedCount = 0;
    for (const BookInfo &info : importedBooks) {
        const BookInfo existing = m_dbManager->getBookByIsbn(info.isbn);
        if (existing.found && existing.coverUrl != info.coverUrl) {
            CoverCache::removeImage(info.isbn);
        }
        if (!m_dbManager->saveImportedBookRecord(info)) {
            ++skippedCount;
            continue;
        }

        m_bookCollectionModel->addBook(info, true);
        if (!info.coverUrl.isEmpty() && !CoverCache::contains(info.isbn)) {
            m_metadataProvider->cacheCoverForBook(info.isbn, info.coverUrl);
        }
        ++importedCount;
    }

    if (importedCount > 0 && m_syncManager->isAuthenticated()) {
        m_syncCoordinator->flushQueue();
    }
    QMessageBox::information(
        this, "Import CSV",
        QString("Imported %1 books; skipped %2 invalid rows.")
            .arg(importedCount).arg(skippedCount));
}

void MainWindow::applyPaletteStyles(const QPalette &palette)
{
    if (m_applyingPalette) return;
    m_applyingPalette = true;

    const QString windowColor = palette.color(QPalette::Window).name();
    const QString baseColor = palette.color(QPalette::Base).name();
    const QString textColor = palette.color(QPalette::WindowText).name();
    const QString disabledTextColor = palette.color(QPalette::Disabled, QPalette::WindowText).name();
    const QString borderColor = palette.color(QPalette::Mid).name();
    const QString buttonColor = palette.color(QPalette::Button).name();
    const QString buttonText = palette.color(QPalette::ButtonText).name();
    const QString highlightColor = palette.color(QPalette::Highlight).name();
    const QString highlightedText = palette.color(QPalette::HighlightedText).name();

    setPalette(palette);
    centralWidget()->setPalette(palette);
    centralWidget()->setAutoFillBackground(true);
    setStyleSheet(QString(
        "QMainWindow { background-color: %1; }"
        "QWidget { color: %2; font-family: 'Segoe UI', system-ui, sans-serif; font-size: 13px; }"
        "QFrame { border: 1px solid %3; border-radius: 8px; background-color: %4; }"
        "QLabel { background-color: transparent; border: none; padding: 0px; }"
        "QPushButton { background-color: %5; color: %6; border: 1px solid %3; "
        "border-radius: 6px; padding: 10px; font-weight: bold; }"
        "QPushButton:hover { background-color: %7; color: %8; }"
        "QMenu { background-color: %4; border: 1px solid %3; border-radius: 6px; padding: 5px; }"
        "QMenu::item { padding: 6px 25px 6px 20px; color: %2; }"
        "QMenu::item:selected { background-color: %7; color: %8; border-radius: 4px; }"
        "QMenu::item:disabled { color: %9; }"
        "QMenu::item:disabled:selected { background-color: %4; color: %9; }"
        "QLineEdit { background-color: %4; border: 1px solid %3; border-radius: 6px; "
        "padding: 10px; color: %2; font-size: 14px; }"
        "QLineEdit:focus { border: 1px solid %7; }"
        "QListWidget { background-color: %4; border: 1px solid %3; border-radius: 8px; }")
        .arg(windowColor, textColor, borderColor, baseColor,
               buttonColor, buttonText, highlightColor, highlightedText)
           .arg(disabledTextColor));

    m_cameraToggleButton->setStyleSheet(QString(
        "QPushButton { background-color: %1; color: %2; border: 1px solid %3; "
        "border-radius: 4px; font-size: 14px; padding: 0px 10px; }"
        "QPushButton:hover { background-color: %4; color: %5; }")
        .arg(buttonColor, buttonText, borderColor, highlightColor, highlightedText));
    m_settingsButton->setStyleSheet(QString(
        "QToolButton { background-color: %1; color: %2; border: 1px solid %3; "
        "border-radius: 4px; font-size: 20px; }"
        "QToolButton::menu-indicator { image: none; width: 0px; }"
        "QToolButton:hover { background-color: %4; color: %5; }")
        .arg(buttonColor, buttonText, borderColor, highlightColor, highlightedText));
    m_manualLookupButton->setStyleSheet(QString(
        "QPushButton { background-color: %1; color: %2; border: 1px solid %3; "
        "border-radius: 6px; padding: 10px; font-weight: bold; }"
        "QPushButton:hover { background-color: %4; color: %5; }")
        .arg(buttonColor, buttonText, borderColor, highlightColor, highlightedText));

    m_bookshelfWidget->applyPalette(palette);
    m_detailsSidebar->applyPalette(palette);
    applyStatusStyle(m_statusLabel->text(), m_statusTextColor);
    m_applyingPalette = false;
}

void MainWindow::toggleCameraView()
{
    const bool isVisible = !m_scannerPanel->isHidden();
    m_scannerPanel->setVisible(!isVisible);

    if (isVisible) {
        m_scannerView->stopCapture();
        m_cameraToggleButton->setText("📷 Show Camera Preview");
        m_cameraToggleButton->setToolTip("Open the scanner view and start camera capture");
    } else {
        m_cameraToggleButton->setText("🙈 Hide Camera Preview");
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

void MainWindow::setupDatabase()
{
    m_dbManager = new BookDatabaseManager(this);

    const QString appDataFolder = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(appDataFolder);
    if (!dir.exists()) {
        dir.mkpath(".");
    }

    const QString crossPlatformDbPath = QDir::cleanPath(appDataFolder + "/bookshelf.db");
    m_dbManager->initDatabase(crossPlatformDbPath);

    m_metadataProvider = new BookMetadataProvider(m_dbManager, this);
}

void MainWindow::setupConnections()
{
    connect(m_scannerView, &BarcodeScannerView::isbnScanned, this, [this](const QString &isbn) {
        if (isbn == "ERROR: Camera permission denied.") {
            applyStatusStyle("No camera detected. Use the manual ISBN field instead.", "#ffaa55");
            return;
        }
        m_metadataProvider->lookupIsbn(isbn);
    });
    connect(m_scannerView, &BarcodeScannerView::cameraUnavailable, this,
            [this](const QString &message) { applyStatusStyle(message, "#ffaa55"); });
    connect(m_metadataProvider, &BookMetadataProvider::lookupStatusChanged, this, &MainWindow::updateStatusLabel);
    connect(m_metadataProvider, &BookMetadataProvider::bookDataReady, this, &MainWindow::displayBookDetails);
    connect(m_metadataProvider, &BookMetadataProvider::bookDataReady, m_dbManager, &BookDatabaseManager::saveBookRecord);

    connect(m_dbManager, &BookDatabaseManager::databaseError, this, [this](const QString &err) {
        updateStatusLabel(err, true);
    });

    connect(m_bookshelfWidget, &BookshelfWidget::bookSelected, m_detailsSidebar, &BookDetailsSidebar::updateDetails);
    connect(m_detailsSidebar, &BookDetailsSidebar::deleteBookRequested, this, &MainWindow::removeBookRecord);
    connect(m_searchBar, &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);
    connect(m_metadataProvider, &BookMetadataProvider::coverCached, this,
            [this](const QString &isbn) {
                const BookInfo info = m_dbManager->getBookByIsbn(isbn);
                if (info.found) m_bookCollectionModel->addBook(info, false);
                m_detailsSidebar->refreshCover(isbn);
            });
}

void MainWindow::setupSync()
{
    m_syncCoordinator = new BookSyncCoordinator(
        m_dbManager, m_syncManager, m_metadataProvider, this);
    connect(m_syncCoordinator, &BookSyncCoordinator::statusMessage,
            this, &MainWindow::updateStatusLabel);
        connect(m_syncCoordinator, &BookSyncCoordinator::collectionBookAdded, this,
            [this](const BookInfo &book, bool prepend) {
            m_bookCollectionModel->addBook(book, prepend);
            });
        connect(m_syncCoordinator, &BookSyncCoordinator::collectionBookRemoved, this,
            [this](const QString &isbn) { m_bookCollectionModel->removeBook(isbn); });
        connect(m_syncCoordinator, &BookSyncCoordinator::bookDetailsCloseRequested,
            m_detailsSidebar, &BookDetailsSidebar::closeSidebar);
    connect(m_syncCoordinator, &BookSyncCoordinator::syncSummary, this,
            [this](const QString &message) { statusBar()->showMessage(message, 10000); });
    connect(m_syncManager, &BookSyncManager::serverConnectionChanged,
            this, &MainWindow::updateSyncConnectionIndicator);
    connect(m_syncManager, &BookSyncManager::loginSuccess, this, &MainWindow::handleLoginSuccess);
    connect(m_syncManager, &BookSyncManager::authStatusMessage,
            this, [this](const QString &message, bool isError) {
                statusBar()->showMessage(message, isError ? 15000 : 5000);
            });
    connect(m_syncManager, &BookSyncManager::networkErrorOccurred, this,
            [this](const QString &message) {
                statusBar()->showMessage("Sync failed: " + message, 15000);
            });
}

void MainWindow::updateSyncConnectionIndicator(bool connected)
{
    const QString color = connected ? "#2f9e62" : "#d64f4f";
    const QString description = connected
        ? "Sync server is reachable"
        : "Sync server is unreachable";
    m_syncConnectionIndicator->setStyleSheet(
        QString("QLabel { background-color: %1; border-radius: 6px; }").arg(color));
    m_syncConnectionIndicator->setToolTip(description);
    m_syncConnectionIndicator->setAccessibleDescription(description);
}

void MainWindow::promptRegisterAccount()
{
    SyncCredentialsDialog dialog(true, this);
    if (dialog.exec() != QDialog::Accepted) return;
    const QString serverUrl = dialog.serverUrl();
    const QString username = dialog.username();
    const QString password = dialog.password();

    m_syncManager->setSyncCheckpoint(
        m_dbManager->getSyncCheckpoint(username),
        m_dbManager->hasSyncCheckpoint(username));
    m_syncManager->setServerUrl(serverUrl);
    m_syncManager->registerAccount(username, password);
}

void MainWindow::promptLoginAccount()
{
    SyncCredentialsDialog dialog(false, this);
    if (dialog.exec() != QDialog::Accepted) return;
    const QString serverUrl = dialog.serverUrl();
    const QString username = dialog.username();
    const QString password = dialog.password();

    m_syncManager->setSyncCheckpoint(
        m_dbManager->getSyncCheckpoint(username),
        m_dbManager->hasSyncCheckpoint(username));
    m_syncManager->setServerUrl(serverUrl);
    m_syncManager->loginAccount(username, password);
}

void MainWindow::syncNow()
{
    if (!m_syncManager->isAuthenticated()) {
        statusBar()->showMessage("Log in before synchronizing.", 5000);
        return;
    }
    m_syncCoordinator->beginSync();
    statusBar()->showMessage("Synchronizing bookshelf...");
    m_syncManager->triggerDifferentialSync();
}

void MainWindow::logoutSync()
{
    if (m_syncManager->isSyncRequestInFlight()) {
        statusBar()->showMessage("Wait for the current sync to finish before logging out.", 5000);
        return;
    }
    m_syncManager->logoutAccount();
    m_syncConnectionIndicator->setStyleSheet(
        "QLabel { background-color: #8a929c; border-radius: 6px; }");
    m_syncConnectionIndicator->setToolTip("Log in to check sync server");
    m_syncConnectionIndicator->setAccessibleDescription("Log in to check sync server");
    m_registerAction->setEnabled(true);
    m_loginAction->setEnabled(true);
    m_syncAction->setEnabled(false);
    m_logoutAction->setEnabled(false);
    statusBar()->showMessage("Signed out of sync.", 5000);
}

void MainWindow::handleLoginSuccess()
{
    m_registerAction->setEnabled(false);
    m_loginAction->setEnabled(false);
    m_syncAction->setEnabled(true);
    m_logoutAction->setEnabled(true);
}

void MainWindow::populateBookshelf()
{
    const QList<BookInfo> historicalBooks = m_dbManager->getAllSavedBooks();
    m_bookCollectionModel->setBooks(historicalBooks);
    for (const BookInfo &book : historicalBooks) {
        if (!book.coverUrl.isEmpty() && !CoverCache::contains(book.isbn)) {
            m_metadataProvider->cacheCoverForBook(book.isbn, book.coverUrl);
        }
    }
}

void MainWindow::applyStatusStyle(const QString &text, const QString &textColor)
{
    m_statusTextColor = textColor;
    m_statusLabel->setText(text);
    const QString defaultTextColor =
        QApplication::palette().color(QPalette::WindowText).name();
    QString style = "font-weight: bold; font-size: 13px; padding: 3px; "
                    "color: " + defaultTextColor + "; background: transparent; border: none;";
    if (!textColor.isEmpty()) {
        style += " color: " + textColor + ";";
    }
    m_statusLabel->setStyleSheet(style);
}

void MainWindow::updateStatusLabel(const QString &text, bool isError)
{
    if (isError) {
        qCritical() << "[Scanner System Error Alert]:\n" << text;
        applyStatusStyle(text, "#ff6b6b");
        return;
    }

    if (text.startsWith("💡 ISBN ")) {
        applyStatusStyle(text, "#ffaa55");
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

    m_syncCoordinator->removeBook(isbn);
}

void MainWindow::onSearchTextChanged(const QString &text)
{
    m_bookFilterModel->setFilterFixedString(text.trimmed());
}
