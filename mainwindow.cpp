#include "mainwindow.h"
#include "barcodescannercontroller.h"
#include "bookmetadataprovider.h"
#include "bookdatabasemanager.h"
#include "booksyncmanager.h"
#include "booksynccoordinator.h"
#include "bookcollectionmodel.h"
#include "covercache.h"
#include "bookcsv.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QMessageBox>
#include <QLineEdit>
#include <QApplication>
#include <QEvent>
#include <QPalette>
#include <QSettings>
#include <QStatusBar>
#include <QSortFilterProxyModel>
#include <QQmlError>
#include <QQmlContext>
#include <QQuickWidget>
#include <QQuickItem>
#include <QUrl>
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
    m_scannerController = new BarcodeScannerController(this);
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
    auto *scannerToggleWidget = new QQuickWidget(centralWidget);
    scannerToggleWidget->setFixedSize(176, 36);
    scannerToggleWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    connect(scannerToggleWidget, &QQuickWidget::statusChanged, this,
            [this, scannerToggleWidget](QQuickWidget::Status status) {
                if (status == QQuickWidget::Error) {
                    for (const QQmlError &error : scannerToggleWidget->errors()) {
                        qWarning().noquote() << error.toString();
                    }
                } else if (status == QQuickWidget::Ready && scannerToggleWidget->rootObject()) {
                    scannerToggleWidget->rootObject()->setProperty(
                        "scannerActions", QVariant::fromValue(static_cast<QObject *>(this)));
                }
            });
    scannerToggleWidget->setSource(
        QUrl(QStringLiteral("qrc:/qt/qml/ISBNBookScanner/ScannerToggle.qml")));
    controlsLayout->addWidget(scannerToggleWidget);
    controlsLayout->addStretch();

    m_syncConnectionIndicator = new QLabel(centralWidget);
    m_syncConnectionIndicator->setObjectName("syncConnectionIndicator");
    m_syncConnectionIndicator->setFixedSize(12, 12);
    m_syncConnectionIndicator->setToolTip("Log in to check sync server");
    m_syncConnectionIndicator->setAccessibleName("Sync server connection status");
    m_syncConnectionIndicator->setStyleSheet(
        "QLabel { background-color: #8a929c; border-radius: 6px; }");
    controlsLayout->addWidget(m_syncConnectionIndicator, 0, Qt::AlignVCenter);

    m_settingsQuickWidget = new QQuickWidget(centralWidget);
    m_settingsQuickWidget->setFixedSize(42, 36);
    m_settingsQuickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    connect(m_settingsQuickWidget, &QQuickWidget::statusChanged, this,
            [this](QQuickWidget::Status status) {
                if (status == QQuickWidget::Error) {
                    for (const QQmlError &error : m_settingsQuickWidget->errors()) {
                        qWarning().noquote() << error.toString();
                    }
                } else if (status == QQuickWidget::Ready && m_settingsQuickWidget->rootObject()) {
                    m_settingsQuickWidget->rootObject()->setProperty(
                        "mainWindow", QVariant::fromValue(static_cast<QObject *>(this)));
                }
            });
    m_settingsQuickWidget->setSource(
        QUrl(QStringLiteral("qrc:/qt/qml/ISBNBookScanner/SettingsMenu.qml")));
    controlsLayout->addWidget(m_settingsQuickWidget);
    mainVerticalLayout->addLayout(controlsLayout);

    m_scannerPanel = new QWidget(centralWidget);
    m_scannerPanel->setMaximumHeight(340);
    auto *scannerLayout = new QVBoxLayout(m_scannerPanel);
    scannerLayout->setContentsMargins(0, 0, 0, 0);
    scannerLayout->setSpacing(0);

    auto *scannerQuickWidget = new QQuickWidget(m_scannerPanel);
    scannerQuickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    scannerQuickWidget->setMinimumSize(320, 320);
    connect(scannerQuickWidget, &QQuickWidget::statusChanged, this,
            [this, scannerQuickWidget](QQuickWidget::Status status) {
                if (status == QQuickWidget::Error) {
                    for (const QQmlError &error : scannerQuickWidget->errors()) {
                        qWarning().noquote() << error.toString();
                    }
                } else if (status == QQuickWidget::Ready && scannerQuickWidget->rootObject()) {
                    QQuickItem *root = scannerQuickWidget->rootObject();
                    root->setProperty("scannerController",
                                      QVariant::fromValue(static_cast<QObject *>(m_scannerController)));
                    root->setProperty("scannerActions",
                                      QVariant::fromValue(static_cast<QObject *>(this)));
                }
            });
    scannerQuickWidget->setSource(
        QUrl(QStringLiteral("qrc:/qt/qml/ISBNBookScanner/ScannerControls.qml")));
    scannerLayout->addWidget(scannerQuickWidget);

    m_scannerPanel->hide();
    mainVerticalLayout->addWidget(m_scannerPanel);

    auto *bottomRowContainer = new QWidget(this);
    auto *bottomRowLayout = new QHBoxLayout(bottomRowContainer);
    bottomRowLayout->setContentsMargins(0, 5, 0, 0);

    auto *bookshelfAreaContainer = new QWidget(this);
    auto *bookshelfAreaLayout = new QVBoxLayout(bookshelfAreaContainer);
    bookshelfAreaLayout->setContentsMargins(0, 0, 0, 0);
    bookshelfAreaLayout->setSpacing(8);

    m_bookshelfQuickWidget = new QQuickWidget(bookshelfAreaContainer);
    m_bookshelfQuickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_bookshelfQuickWidget->rootContext()->setContextProperty(
        "bookCollection", m_bookFilterModel);
    m_bookshelfQuickWidget->rootContext()->setContextProperty("bookSelection", this);
    connect(m_bookshelfQuickWidget, &QQuickWidget::statusChanged, this,
            [this](QQuickWidget::Status status) {
                if (status == QQuickWidget::Error) {
                    for (const QQmlError &error : m_bookshelfQuickWidget->errors()) {
                        qWarning().noquote() << error.toString();
                    }
                }
            });
    m_bookshelfQuickWidget->setSource(
        QUrl(QStringLiteral("qrc:/qt/qml/ISBNBookScanner/BookshelfView.qml")));
    bookshelfAreaLayout->addWidget(m_bookshelfQuickWidget, 1);

    m_searchBar = new QLineEdit(this);
    m_searchBar->setObjectName("bookSearchBar");
    m_searchBar->setPlaceholderText("🔎 Type to filter bookshelf by title or author name...");
    m_searchBar->setClearButtonEnabled(true);
    bookshelfAreaLayout->addWidget(m_searchBar, 0);

    bottomRowLayout->addWidget(bookshelfAreaContainer, 3);

    m_bookDetailsQuickWidget = new QQuickWidget(bottomRowContainer);
    m_bookDetailsQuickWidget->setMinimumWidth(260);
    m_bookDetailsQuickWidget->setMaximumWidth(320);
    m_bookDetailsQuickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    connect(m_bookDetailsQuickWidget, &QQuickWidget::statusChanged, this,
            [this](QQuickWidget::Status status) {
                if (status == QQuickWidget::Error) {
                    for (const QQmlError &error : m_bookDetailsQuickWidget->errors()) {
                        qWarning().noquote() << error.toString();
                    }
                } else if (status == QQuickWidget::Ready
                           && m_bookDetailsQuickWidget->rootObject()) {
                    m_bookDetailsQuickWidget->rootObject()->setProperty(
                        "selection", QVariant::fromValue(static_cast<QObject *>(this)));
                }
            });
    m_bookDetailsQuickWidget->setSource(
        QUrl(QStringLiteral("qrc:/qt/qml/ISBNBookScanner/BookDetailsView.qml")));
    m_bookDetailsQuickWidget->setVisible(false);
    bottomRowLayout->addWidget(m_bookDetailsQuickWidget, 1);

    mainVerticalLayout->addWidget(bottomRowContainer, 1);

    setCentralWidget(centralWidget);
    setWindowTitle("ISBN Book Scanner");
    resize(950, 900);

}

QVariantMap MainWindow::exportBooksCsv(const QUrl &fileUrl)
{
    QVariantMap result;
    const QList<BookInfo> books = m_dbManager->getAllSavedBooks();
    if (books.isEmpty()) {
        result.insert("title", "Export CSV");
        result.insert("message", "There are no books to export.");
        result.insert("success", false);
        return result;
    }

    QString filePath = fileUrl.toLocalFile();
    if (filePath.isEmpty()) {
        result.insert("title", "Export CSV Failed");
        result.insert("message", "Choose a local file to export.");
        result.insert("success", false);
        return result;
    }
    if (!filePath.endsWith(".csv", Qt::CaseInsensitive)) filePath += ".csv";

    QString error;
    if (!BookCsv::writeFile(filePath, books, error)) {
        result.insert("title", "Export CSV Failed");
        result.insert("message", error);
        result.insert("success", false);
        return result;
    }
    result.insert("title", "Export CSV");
    result.insert("message", QString("Exported %1 books to:\n%2")
                                  .arg(books.size()).arg(filePath));
    result.insert("success", true);
    return result;
}

QVariantMap MainWindow::importBooksCsv(const QUrl &fileUrl)
{
    QVariantMap result;
    const QString filePath = fileUrl.toLocalFile();
    if (filePath.isEmpty()) {
        result.insert("title", "Import CSV Failed");
        result.insert("message", "Choose a local CSV file to import.");
        result.insert("success", false);
        return result;
    }

    QList<BookInfo> importedBooks;
    int skippedCount = 0;
    QString error;
    if (!BookCsv::readFile(filePath, importedBooks, skippedCount, error)) {
        result.insert("title", "Import CSV Failed");
        result.insert("message", error);
        result.insert("success", false);
        return result;
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
    result.insert("title", "Import CSV");
    result.insert("message", QString("Imported %1 books; skipped %2 invalid rows.")
                                  .arg(importedCount).arg(skippedCount));
    result.insert("success", true);
    return result;
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
        "QLineEdit { background-color: %4; border: 1px solid %3; border-radius: 6px; "
        "padding: 10px; color: %2; font-size: 14px; }"
        "QLineEdit:focus { border: 1px solid %7; }"
        "QListWidget { background-color: %4; border: 1px solid %3; border-radius: 8px; }")
        .arg(windowColor, textColor, borderColor, baseColor,
               buttonColor, buttonText, highlightColor, highlightedText)
           .arg(disabledTextColor));

    applyStatusStyle(m_scannerStatusText, m_statusTextColor);
    m_applyingPalette = false;
}

void MainWindow::toggleScannerPanel()
{
    const bool showPanel = m_scannerPanel->isHidden();
    m_scannerPanel->setVisible(showPanel);
    if (showPanel) {
        m_scannerController->startCapture();
    } else {
        m_scannerController->stopCapture();
    }
    emit scannerVisibilityChanged();
}

bool MainWindow::submitManualIsbn(const QString &input)
{
    QString isbn = input.trimmed();
    isbn.remove(QLatin1Char('-'));

    const bool validLength = isbn.length() == 10 || isbn.length() == 13;
    const bool containsOnlyDigits = std::all_of(
        isbn.cbegin(), isbn.cend(), [](QChar character) {
            return character >= QLatin1Char('0') && character <= QLatin1Char('9');
        });
    if (!validLength || !containsOnlyDigits) {
        QMessageBox::warning(this, "Invalid Input",
                             "ISBN must be a string of 10 or 13 numbers.");
        return false;
    }

    m_metadataProvider->lookupIsbn(isbn);
    return true;
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
    connect(m_scannerController, &BarcodeScannerController::isbnScanned, this, [this](const QString &isbn) {
        if (isbn == "ERROR: Camera permission denied.") {
            applyStatusStyle("No camera detected. Use the manual ISBN field instead.", "#ffaa55");
            return;
        }
        m_metadataProvider->lookupIsbn(isbn);
    });
    connect(m_scannerController, &BarcodeScannerController::cameraUnavailable, this,
            [this](const QString &message) { applyStatusStyle(message, "#ffaa55"); });
    connect(m_metadataProvider, &BookMetadataProvider::lookupStatusChanged, this, &MainWindow::updateStatusLabel);
    connect(m_metadataProvider, &BookMetadataProvider::bookDataReady, this, &MainWindow::displayBookDetails);
    connect(m_metadataProvider, &BookMetadataProvider::bookDataReady, m_dbManager, &BookDatabaseManager::saveBookRecord);

    connect(m_dbManager, &BookDatabaseManager::databaseError, this, [this](const QString &err) {
        updateStatusLabel(err, true);
    });

    connect(m_searchBar, &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);
    connect(m_metadataProvider, &BookMetadataProvider::coverCached, this,
            [this](const QString &isbn) {
                const BookInfo info = m_dbManager->getBookByIsbn(isbn);
                if (info.found) m_bookCollectionModel->addBook(info, false);
                if (m_selectedBook.isbn == isbn) {
                    emit selectedBookChanged();
                }
            });
    connect(this, &MainWindow::selectedBookChanged, m_bookDetailsQuickWidget,
            [this]() { m_bookDetailsQuickWidget->setVisible(selectedBookVisible()); });
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
                this, &MainWindow::clearSelectedBook);
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

QString MainWindow::submitSyncCredentials(bool registering, const QString &serverUrl,
                                          const QString &username, const QString &password,
                                          const QString &confirmation, bool rememberUsername)
{
    QString normalizedUrl = serverUrl.trimmed();
    while (normalizedUrl.endsWith('/')) normalizedUrl.chop(1);
    const QUrl parsedUrl(normalizedUrl);
    const QString scheme = parsedUrl.scheme().toLower();
    if (!parsedUrl.isValid() || parsedUrl.host().isEmpty()
        || (scheme != "http" && scheme != "https")) {
        return "Enter an absolute http:// or https:// server URL.";
    }
    if (username.trimmed().isEmpty() || password.isEmpty()) {
        return "Enter a username and password.";
    }
    if (registering && password != confirmation) {
        return "The passwords do not match.";
    }

    QSettings settings;
    settings.setValue("sync/server_url", normalizedUrl);
    if (!registering) {
        if (rememberUsername) {
            settings.setValue("sync/username", username.trimmed());
        } else {
            settings.remove("sync/username");
        }
    }

    m_syncManager->setSyncCheckpoint(
        m_dbManager->getSyncCheckpoint(username.trimmed()),
        m_dbManager->hasSyncCheckpoint(username.trimmed()));
    m_syncManager->setServerUrl(normalizedUrl);
    if (registering) {
        m_syncManager->registerAccount(username.trimmed(), password);
    } else {
        m_syncManager->loginAccount(username.trimmed(), password);
    }
    return {};
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
    emit syncStateChanged();
    statusBar()->showMessage("Signed out of sync.", 5000);
}

void MainWindow::handleLoginSuccess()
{
    emit syncStateChanged();
}

QString MainWindow::defaultSyncServerUrl() const
{
    const QString configuredUrl = qEnvironmentVariable("BOOKSHELF_SYNC_URL");
    if (!configuredUrl.isEmpty()) {
        return configuredUrl;
    }
    return QSettings().value("sync/server_url", "http://127.0.0.1:8000").toString();
}

QString MainWindow::rememberedSyncUsername() const
{
    return QSettings().value("sync/username").toString();
}

bool MainWindow::shouldRememberSyncUsername() const
{
    return !rememberedSyncUsername().isEmpty();
}

bool MainWindow::syncAuthenticated() const
{
    return m_syncManager && m_syncManager->isAuthenticated();
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
    m_scannerStatusText = text;
    m_statusTextColor = textColor;
    emit scannerStatusChanged();
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
    m_selectedBook = info;
    emit selectedBookChanged();
}

bool MainWindow::scannerVisible() const
{
    return m_scannerPanel && !m_scannerPanel->isHidden();
}

QString MainWindow::scannerStatusText() const
{
    return m_scannerStatusText;
}

QString MainWindow::scannerStatusColor() const
{
    return m_statusTextColor.isEmpty()
        ? QApplication::palette().color(QPalette::WindowText).name()
        : m_statusTextColor;
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

void MainWindow::selectBook(const QString &isbn)
{
    const BookInfo info = m_dbManager->getBookByIsbn(isbn);
    if (info.found) {
        m_selectedBook = info;
        emit selectedBookChanged();
    }
}

void MainWindow::clearSelectedBook()
{
    if (!m_selectedBook.found) {
        return;
    }
    m_selectedBook = BookInfo{};
    emit selectedBookChanged();
}

void MainWindow::removeSelectedBook()
{
    if (m_selectedBook.found) {
        removeBookRecord(m_selectedBook.isbn);
    }
}

bool MainWindow::selectedBookVisible() const
{
    return m_selectedBook.found;
}

QString MainWindow::selectedBookTitle() const
{
    return m_selectedBook.title;
}

QString MainWindow::selectedBookAuthors() const
{
    return m_selectedBook.authors;
}

QString MainWindow::selectedBookIsbn() const
{
    return m_selectedBook.isbn;
}

QString MainWindow::selectedBookMetadata() const
{
    QStringList metadata;
    if (!m_selectedBook.publicationDate.isEmpty()) {
        metadata.append("First published: " + m_selectedBook.publicationDate);
    }
    if (!m_selectedBook.publisher.isEmpty()) {
        metadata.append("Publisher: " + m_selectedBook.publisher);
    }
    if (m_selectedBook.pageCount > 0) {
        metadata.append(QString("Pages: %1").arg(m_selectedBook.pageCount));
    }
    return metadata.join('\n');
}

QUrl MainWindow::selectedBookCoverSource() const
{
    if (!m_selectedBook.found || m_selectedBook.coverUrl.isEmpty()
        || !CoverCache::contains(m_selectedBook.isbn)) {
        return {};
    }
    return QUrl::fromLocalFile(CoverCache::filePath(m_selectedBook.isbn));
}
