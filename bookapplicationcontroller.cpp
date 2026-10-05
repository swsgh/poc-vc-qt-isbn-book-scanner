#include "bookapplicationcontroller.h"
#include "barcodescannercontroller.h"
#include "bookmetadataprovider.h"
#include "bookdatabasemanager.h"
#include "booksyncmanager.h"
#include "booksynccoordinator.h"
#include "bookcollectionmodel.h"
#include "covercache.h"
#include "bookcsv.h"

#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QQmlApplicationEngine>
#include <QSettings>
#include <QTimer>
#include <QSortFilterProxyModel>
#include <QUrl>
#include <algorithm>

#if defined(Q_OS_ANDROID)
#include <QJniObject>
#include <QMetaObject>
#include <QPointer>
#include <QtCore/qcoreapplication_platform.h>
#endif

#if defined(Q_OS_ANDROID)
namespace {
QPointer<BookApplicationController> permissionController;
}

extern "C" JNIEXPORT void JNICALL
Java_org_bookshelf_NearbyPermissionActivity_nativeNearbyPermissionResult(
    JNIEnv *, jclass, jint requestCode, jboolean granted)
{
    if (!permissionController) {
        return;
    }
    QMetaObject::invokeMethod(permissionController.data(), "handleNearbyPermissionResult",
                              Qt::QueuedConnection, Q_ARG(int, requestCode),
                              Q_ARG(bool, granted == JNI_TRUE));
}
#endif

BookApplicationController::BookApplicationController(QQmlApplicationEngine &engine, QObject *parent)
    : QObject(parent)
{
#if defined(Q_OS_ANDROID)
    permissionController = this;
#endif
    initializeApplication(engine);
}

BookApplicationController::~BookApplicationController()
{
#if defined(Q_OS_ANDROID)
    if (permissionController == this) {
        permissionController.clear();
    }
#endif
}

void BookApplicationController::initializeApplication(QQmlApplicationEngine &engine)
{
    m_scannerController = new BarcodeScannerController(this);
    m_bookCollectionModel = new BookCollectionModel(this);
    m_bookFilterModel = new QSortFilterProxyModel(this);
    m_bookFilterModel->setSourceModel(m_bookCollectionModel);
    m_bookFilterModel->setFilterRole(BookCollectionModel::SearchTextRole);
    m_bookFilterModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

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

    engine.loadFromModule("ISBNBookScanner", "MainView");
    if (engine.rootObjects().isEmpty()) {
        qCritical() << "Failed to load the ISBN Book Scanner QML application window.";
        return;
    }
    QObject *root = engine.rootObjects().constFirst();
    root->setProperty("appController", QVariant::fromValue(static_cast<QObject *>(this)));
    root->setProperty("bookCollection", QVariant::fromValue(static_cast<QObject *>(m_bookFilterModel)));
    root->setProperty("scannerController", QVariant::fromValue(static_cast<QObject *>(m_scannerController)));
}

QVariantMap BookApplicationController::exportBooksCsv(const QUrl &fileUrl)
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

QVariantMap BookApplicationController::importBooksCsv(const QUrl &fileUrl)
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

void BookApplicationController::toggleScannerPanel()
{
    m_scannerVisible = !m_scannerVisible;
    if (m_scannerVisible) {
        m_scannerController->startCapture();
    } else {
        m_scannerController->stopCapture();
    }
    emit scannerVisibilityChanged();
}

bool BookApplicationController::submitManualIsbn(const QString &input)
{
    QString isbn = input.trimmed();
    isbn.remove(QLatin1Char('-'));

    const bool validLength = isbn.length() == 10 || isbn.length() == 13;
    const bool containsOnlyDigits = std::all_of(
        isbn.cbegin(), isbn.cend(), [](QChar character) {
            return character >= QLatin1Char('0') && character <= QLatin1Char('9');
        });
    if (!validLength || !containsOnlyDigits) {
        setScannerStatus("ISBN must be a string of 10 or 13 numbers.");
        return false;
    }

    m_metadataProvider->lookupIsbn(isbn);
    return true;
}

void BookApplicationController::setupDatabase()
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

void BookApplicationController::setupConnections()
{
    connect(m_scannerController, &BarcodeScannerController::isbnScanned, this, [this](const QString &isbn) {
        if (isbn == "ERROR: Camera permission denied.") {
            setScannerStatus("No camera detected. Use the manual ISBN field instead.");
            return;
        }
        m_metadataProvider->lookupIsbn(isbn);
    });
    connect(m_scannerController, &BarcodeScannerController::cameraUnavailable, this,
            [this](const QString &message) { setScannerStatus(message); });
    connect(m_metadataProvider, &BookMetadataProvider::lookupStatusChanged, this, &BookApplicationController::updateStatusLabel);
    connect(m_metadataProvider, &BookMetadataProvider::bookDataReady, this, &BookApplicationController::displayBookDetails);
    connect(m_metadataProvider, &BookMetadataProvider::bookDataReady, m_dbManager, &BookDatabaseManager::saveBookRecord);

    connect(m_dbManager, &BookDatabaseManager::databaseError, this, [this](const QString &err) {
        updateStatusLabel(err, true);
    });

    connect(m_metadataProvider, &BookMetadataProvider::coverCached, this,
            [this](const QString &isbn) {
                const BookInfo info = m_dbManager->getBookByIsbn(isbn);
                if (info.found) m_bookCollectionModel->addBook(info, false);
                if (m_selectedBook.isbn == isbn) {
                    emit selectedBookChanged();
                }
            });
}

void BookApplicationController::setupSync()
{
    m_syncCoordinator = new BookSyncCoordinator(
        m_dbManager, m_syncManager, m_metadataProvider, this);
    connect(m_syncCoordinator, &BookSyncCoordinator::statusMessage,
            this, &BookApplicationController::updateStatusLabel);
        connect(m_syncCoordinator, &BookSyncCoordinator::collectionBookAdded, this,
            [this](const BookInfo &book, bool prepend) {
            m_bookCollectionModel->addBook(book, prepend);
            });
        connect(m_syncCoordinator, &BookSyncCoordinator::collectionBookRemoved, this,
            [this](const QString &isbn) { m_bookCollectionModel->removeBook(isbn); });
        connect(m_syncCoordinator, &BookSyncCoordinator::bookDetailsCloseRequested,
                this, &BookApplicationController::clearSelectedBook);
        connect(m_syncCoordinator, &BookSyncCoordinator::syncSummary, this,
                [this](const QString &message) { setApplicationStatus(message, 10000); });
    connect(m_syncManager, &BookSyncManager::serverConnectionChanged,
            this, &BookApplicationController::updateSyncConnectionIndicator);
    connect(m_syncManager, &BookSyncManager::loginSuccess, this, &BookApplicationController::handleLoginSuccess);
    connect(m_syncManager, &BookSyncManager::authStatusMessage,
            this, [this](const QString &message, bool isError) {
                setApplicationStatus(message, isError ? 15000 : 5000);
            });
    connect(m_syncManager, &BookSyncManager::networkErrorOccurred, this,
            [this](const QString &message) {
                setApplicationStatus("Sync failed: " + message, 15000);
            });
}

void BookApplicationController::updateSyncConnectionIndicator(bool connected)
{
    m_syncServerReachable = connected;
    emit syncConnectionChanged();
}

QVariantMap BookApplicationController::submitSyncCredentials(bool registering,
                                                              const QString &serverUrl,
                                                              const QString &username,
                                                              const QString &password,
                                                              const QString &confirmation,
                                                              bool rememberUsername)
{
    QString normalizedUrl = serverUrl.trimmed();
    while (normalizedUrl.endsWith('/')) normalizedUrl.chop(1);
    const QUrl parsedUrl(normalizedUrl);
    const QString scheme = parsedUrl.scheme().toLower();
    if (!parsedUrl.isValid() || parsedUrl.host().isEmpty()
        || (scheme != "http" && scheme != "https")) {
        return credentialSubmissionResult("error",
                                          "Enter an absolute http:// or https:// server URL.");
    }
    if (username.trimmed().isEmpty() || password.isEmpty()) {
        return credentialSubmissionResult("error", "Enter a username and password.");
    }
    if (registering && password != confirmation) {
        return credentialSubmissionResult("error", "The passwords do not match.");
    }

    PendingCredentials credentials;
    credentials.registering = registering;
    credentials.rememberUsername = rememberUsername;
    credentials.serverUrl = normalizedUrl;
    credentials.username = username.trimmed();
    credentials.password = password;

#if defined(Q_OS_ANDROID)
    const int sdkVersion = QNativeInterface::QAndroidApplication::sdkVersion();
    if (sdkVersion >= 33) {
        if (m_pendingCredentials.active) {
            return credentialSubmissionResult("error", "A permission request is already in progress.");
        }
        if (!QNativeInterface::QAndroidApplication::isActivityContext()) {
            return credentialSubmissionResult("error", "Android activity is unavailable.");
        }

        const QString permission = sdkVersion >= 37
            ? QStringLiteral("android.permission.ACCESS_LOCAL_NETWORK")
            : QStringLiteral("android.permission.NEARBY_WIFI_DEVICES");
        const auto androidContext = QNativeInterface::QAndroidApplication::context();
        const QJniObject activity(androidContext.object());
        const QJniObject javaPermission = QJniObject::fromString(permission);
        const jint permissionStatus = activity.callMethod<jint>(
            "checkSelfPermission", "(Ljava/lang/String;)I",
            javaPermission.object<jstring>());

        if (permissionStatus != 0) {
            credentials.active = true;
            credentials.requestCode = m_nextPermissionRequestCode++;
            m_pendingCredentials = credentials;
            QNativeInterface::QAndroidApplication::runOnAndroidMainThread(
                [activity, javaPermission, requestCode = credentials.requestCode]() {
                    activity.callMethod<void>("requestNearbyNetworkPermission",
                                              "(Ljava/lang/String;I)V",
                                              javaPermission.object<jstring>(), requestCode);
                });
            return credentialSubmissionResult("pending");
        }
    }
#endif

    startSyncWithCredentials(credentials);
    return credentialSubmissionResult("accepted");
}

QVariantMap BookApplicationController::credentialSubmissionResult(const QString &status,
                                                                   const QString &message) const
{
    return {{"status", status}, {"message", message}};
}

void BookApplicationController::startSyncWithCredentials(const PendingCredentials &credentials)
{
    QSettings settings;
    settings.setValue("sync/server_url", credentials.serverUrl);
    if (!credentials.registering) {
        if (credentials.rememberUsername) {
            settings.setValue("sync/username", credentials.username);
        } else {
            settings.remove("sync/username");
        }
    }

    m_syncManager->setSyncCheckpoint(
        m_dbManager->getSyncCheckpoint(credentials.username),
        m_dbManager->hasSyncCheckpoint(credentials.username));
    m_syncManager->setServerUrl(credentials.serverUrl);
    if (credentials.registering) {
        m_syncManager->registerAccount(credentials.username, credentials.password);
    } else {
        m_syncManager->loginAccount(credentials.username, credentials.password);
    }
}

void BookApplicationController::cancelSyncCredentialsSubmission()
{
    m_pendingCredentials = PendingCredentials{};
}

void BookApplicationController::handleNearbyPermissionResult(int requestCode, bool granted)
{
    if (!m_pendingCredentials.active || m_pendingCredentials.requestCode != requestCode) {
        return;
    }

    const PendingCredentials credentials = m_pendingCredentials;
    m_pendingCredentials = PendingCredentials{};
    if (!granted) {
        emit syncCredentialsSubmissionFinished(
            false,
            "Nearby network permission is needed for sync. Allow it in Android app settings, then try again.");
        return;
    }

    startSyncWithCredentials(credentials);
    emit syncCredentialsSubmissionFinished(true, {});
}

void BookApplicationController::syncNow()
{
    if (!m_syncManager->isAuthenticated()) {
        setApplicationStatus("Log in before synchronizing.");
        return;
    }
    m_syncCoordinator->beginSync();
    setApplicationStatus("Synchronizing bookshelf...", 0);
    m_syncManager->triggerDifferentialSync();
}

void BookApplicationController::logoutSync()
{
    if (m_syncManager->isSyncRequestInFlight()) {
        setApplicationStatus("Wait for the current sync to finish before logging out.");
        return;
    }
    m_syncManager->logoutAccount();
    m_syncServerReachable = false;
    emit syncConnectionChanged();
    emit syncStateChanged();
    setApplicationStatus("Signed out of sync.");
}

void BookApplicationController::handleLoginSuccess()
{
    emit syncStateChanged();
}

QString BookApplicationController::defaultSyncServerUrl() const
{
    const QString configuredUrl = qEnvironmentVariable("BOOKSHELF_SYNC_URL");
    if (!configuredUrl.isEmpty()) {
        return configuredUrl;
    }
    return QSettings().value("sync/server_url", "http://127.0.0.1:8000").toString();
}

QString BookApplicationController::rememberedSyncUsername() const
{
    return QSettings().value("sync/username").toString();
}

bool BookApplicationController::shouldRememberSyncUsername() const
{
    return !rememberedSyncUsername().isEmpty();
}

bool BookApplicationController::syncAuthenticated() const
{
    return m_syncManager && m_syncManager->isAuthenticated();
}

bool BookApplicationController::syncServerReachable() const
{
    return m_syncServerReachable;
}

void BookApplicationController::populateBookshelf()
{
    const QList<BookInfo> historicalBooks = m_dbManager->getAllSavedBooks();
    m_bookCollectionModel->setBooks(historicalBooks);
    for (const BookInfo &book : historicalBooks) {
        if (!book.coverUrl.isEmpty() && !CoverCache::contains(book.isbn)) {
            m_metadataProvider->cacheCoverForBook(book.isbn, book.coverUrl);
        }
    }
}

void BookApplicationController::setScannerStatus(const QString &text)
{
    m_scannerStatusText = text;
    emit scannerStatusChanged();
}

void BookApplicationController::updateStatusLabel(const QString &text, bool isError)
{
    if (isError) {
        qCritical() << "[Scanner System Error Alert]:\n" << text;
    }
    setScannerStatus(text);
}

void BookApplicationController::displayBookDetails(const BookInfo &info)
{
    setScannerStatus(QString("📖 Successfully scanned: %1").arg(info.title));
    m_selectedBook = info;
    emit selectedBookChanged();
}

bool BookApplicationController::scannerVisible() const
{
    return m_scannerVisible;
}

QString BookApplicationController::scannerStatusText() const
{
    return m_scannerStatusText;
}

QString BookApplicationController::applicationStatusText() const
{
    return m_applicationStatusText;
}

void BookApplicationController::setApplicationStatus(const QString &text, int durationMs)
{
    m_applicationStatusText = text;
    const quint64 generation = ++m_applicationStatusGeneration;
    emit applicationStatusChanged();

    if (durationMs > 0) {
        QTimer::singleShot(durationMs, this, [this, generation]() {
            if (generation == m_applicationStatusGeneration) {
                m_applicationStatusText.clear();
                emit applicationStatusChanged();
            }
        });
    }
}

void BookApplicationController::setBookSearchText(const QString &text)
{
    m_bookFilterModel->setFilterFixedString(text.trimmed());
}

void BookApplicationController::selectBook(const QString &isbn)
{
    const BookInfo info = m_dbManager->getBookByIsbn(isbn);
    if (info.found) {
        m_selectedBook = info;
        emit selectedBookChanged();
    }
}

void BookApplicationController::clearSelectedBook()
{
    if (!m_selectedBook.found) {
        return;
    }
    m_selectedBook = BookInfo{};
    emit selectedBookChanged();
}

void BookApplicationController::removeSelectedBook()
{
    if (m_selectedBook.found) {
        m_syncCoordinator->removeBook(m_selectedBook.isbn);
    }
}

bool BookApplicationController::selectedBookVisible() const
{
    return m_selectedBook.found;
}

QString BookApplicationController::selectedBookTitle() const
{
    return m_selectedBook.title;
}

QString BookApplicationController::selectedBookAuthors() const
{
    return m_selectedBook.authors;
}

QString BookApplicationController::selectedBookIsbn() const
{
    return m_selectedBook.isbn;
}

QString BookApplicationController::selectedBookPublicationDate() const
{
    return m_selectedBook.publicationDate;
}

QString BookApplicationController::selectedBookPublisher() const
{
    return m_selectedBook.publisher;
}

int BookApplicationController::selectedBookPageCount() const
{
    return m_selectedBook.pageCount;
}

QUrl BookApplicationController::selectedBookCoverSource() const
{
    if (!m_selectedBook.found || m_selectedBook.coverUrl.isEmpty()
        || !CoverCache::contains(m_selectedBook.isbn)) {
        return {};
    }
    return QUrl::fromLocalFile(CoverCache::filePath(m_selectedBook.isbn));
}
