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
#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QCheckBox>
#include <QMenu>
#include <QGridLayout>
#include <QAction>
#include <QApplication>
#include <QEvent>
#include <QPalette>
#include <QSettings>
#include <QUrl>
#include <QFrame>
#include <QPushButton>
#include <QToolButton>
#include <QStatusBar>
#include <QSizePolicy>
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
    m_cameraToggleButton = new QPushButton("📷 Open Scanner Suite", centralWidget);
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
    m_syncConnectionIndicator->setToolTip("Checking sync server connection...");
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
    applyStatusStyle("Center an ISBN barcode to log a book");
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
    m_settingsButton->setMenu(syncMenu);

    connect(m_registerAction, &QAction::triggered, this, &MainWindow::promptRegisterAccount);
    connect(m_loginAction, &QAction::triggered, this, &MainWindow::promptLoginAccount);
    connect(m_syncAction, &QAction::triggered, this, &MainWindow::syncNow);
    connect(m_logoutAction, &QAction::triggered, this, &MainWindow::logoutSync);
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
    connect(m_syncManager, &BookSyncManager::serverConnectionChanged,
            this, &MainWindow::updateSyncConnectionIndicator);
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
    m_syncManager->checkServerConnection();
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
    QString serverUrl;
    QString username;
    QString password;
    if (!promptSyncCredentials(true, serverUrl, username, password)) return;

    m_syncManager->setSyncCheckpoint(
        m_dbManager->getSyncCheckpoint(username),
        m_dbManager->hasSyncCheckpoint(username));
    m_syncManager->setServerUrl(serverUrl);
    m_syncManager->registerAccount(username, password);
}

void MainWindow::promptLoginAccount()
{
    QString serverUrl;
    QString username;
    QString password;
    if (!promptSyncCredentials(false, serverUrl, username, password)) return;

    m_syncManager->setSyncCheckpoint(
        m_dbManager->getSyncCheckpoint(username),
        m_dbManager->hasSyncCheckpoint(username));
    m_syncManager->setServerUrl(serverUrl);
    m_syncManager->loginAccount(username, password);
}

bool MainWindow::promptSyncCredentials(bool registering, QString &serverUrl,
                                       QString &username, QString &password)
{
    QSettings settings;
    const QString environmentUrl = qEnvironmentVariable("BOOKSHELF_SYNC_URL");
    const QString defaultUrl = !environmentUrl.isEmpty()
        ? environmentUrl
        : settings.value("sync/server_url", "http://127.0.0.1:8000").toString();
    const QString rememberedUsername = settings.value("sync/username").toString();

    QDialog dialog(this);
    dialog.setWindowTitle(registering ? "Register Sync Account" : "Log In to Sync");
    dialog.setMinimumWidth(560);
    dialog.setStyleSheet("QDialog QLabel { background-color: transparent; border: none; }");

    QLineEdit serverUrlInput(&dialog);
    serverUrlInput.setText(defaultUrl);
    QLineEdit usernameInput(&dialog);
    if (!registering) usernameInput.setText(rememberedUsername);
    QLineEdit passwordInput(&dialog);
    passwordInput.setEchoMode(QLineEdit::Password);
    QLineEdit *confirmationInput = nullptr;
    if (registering) {
        confirmationInput = new QLineEdit(&dialog);
        confirmationInput->setEchoMode(QLineEdit::Password);
    }
    QCheckBox *rememberUsername = nullptr;
    if (!registering) {
        rememberUsername = new QCheckBox("Remember username", &dialog);
        rememberUsername->setChecked(!rememberedUsername.isEmpty());
    }

    auto *form = new QGridLayout;
    form->setColumnMinimumWidth(0, 130);
    form->setColumnStretch(1, 1);
    const auto addField = [form, &dialog](int row, const QString &labelText,
                                          QLineEdit *field) {
        auto *label = new QLabel(labelText, &dialog);
        label->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        label->setFrameShape(QFrame::NoFrame);
        label->setAutoFillBackground(false);
        label->setStyleSheet(
            "QLabel { background-color: transparent; border: none; padding: 0px; }");
        form->addWidget(label, row, 0);
        form->addWidget(field, row, 1);
    };
    addField(0, "Server URL:", &serverUrlInput);
    addField(1, "Username:", &usernameInput);
    addField(2, "Password:", &passwordInput);
    int nextRow = 3;
    if (registering) {
        addField(nextRow++, "Confirm password:", confirmationInput);
    }
    if (rememberUsername) form->addWidget(rememberUsername, nextRow, 1);

    auto *layout = new QVBoxLayout(&dialog);
    layout->addLayout(form);
    auto *buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, &dialog, [&]() {
        const QString enteredUrl = serverUrlInput.text().trimmed();
        const QUrl url(enteredUrl);
        const QString scheme = url.scheme().toLower();
        if (!url.isValid() || url.host().isEmpty()
            || (scheme != "http" && scheme != "https")) {
            QMessageBox::warning(
                &dialog, "Invalid Server URL",
                "Enter an absolute http:// or https:// server URL.");
            return;
        }
        if (usernameInput.text().trimmed().isEmpty() || passwordInput.text().isEmpty()) {
            QMessageBox::warning(&dialog, dialog.windowTitle(), "Enter a username and password.");
            return;
        }
        if (registering && passwordInput.text() != confirmationInput->text()) {
            QMessageBox::warning(&dialog, dialog.windowTitle(), "The passwords do not match.");
            return;
        }
        dialog.accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() != QDialog::Accepted) return false;

    serverUrl = serverUrlInput.text().trimmed();
    while (serverUrl.endsWith('/')) serverUrl.chop(1);
    username = usernameInput.text().trimmed();
    password = passwordInput.text();
    settings.setValue("sync/server_url", serverUrl);
    if (rememberUsername) {
        if (rememberUsername->isChecked()) {
            settings.setValue("sync/username", username);
        } else {
            settings.remove("sync/username");
        }
    }
    return true;
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
