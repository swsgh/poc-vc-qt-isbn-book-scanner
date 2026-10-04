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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QWidget *centralWidget = new QWidget(this);
    centralWidget->setStyleSheet("background-color: #121212;");
    QVBoxLayout *mainVerticalLayout = new QVBoxLayout(centralWidget);

    // 1. TOP SECTION: Scanner View Component (Fixed Top Allocation bounds)
    m_scannerView = new BarcodeScannerView(this);
    m_scannerView->setMaximumSize(400, 220);
    mainVerticalLayout->addWidget(m_scannerView, 0, Qt::AlignHCenter); // Stretch factor 0 = keeps scanner small

    // 2. Descriptive Footer Feedback label (Now sits neatly right below the camera)
    m_isbnLabel = new QLabel("Align ISBN barcode with the red laser line...", this);
    m_isbnLabel->setAlignment(Qt::AlignCenter);
    m_isbnLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #e0e0e0; padding: 8px; background: #1e1e1e; border-radius: 4px;");
    mainVerticalLayout->addWidget(m_isbnLabel, 0); // Stretch factor 0 = minimal space

    // 3. BOTTOM SECTION: Bookshelf Row + Right Side Panel Container
    QWidget *bottomRowContainer = new QWidget(this);
    QHBoxLayout *bottomRowLayout = new QHBoxLayout(bottomRowContainer);
    bottomRowLayout->setContentsMargins(0, 5, 0, 0); // Clean, seamless alignment

    // CHANGED: Wrapped the bookshelf in a vertical layout container to stack the search bar underneath it
    QWidget *bookshelfAreaContainer = new QWidget(this);
    QVBoxLayout *bookshelfAreaLayout = new QVBoxLayout(bookshelfAreaContainer);
    bookshelfAreaLayout->setContentsMargins(0, 0, 0, 0);
    bookshelfAreaLayout->setSpacing(8);

    // Left side item 1: Bookshelf Grid Layout View
    m_bookshelfWidget = new BookshelfWidget(this);
    bookshelfAreaLayout->addWidget(m_bookshelfWidget, 1); // Expand stretch factor to fill the layout footprint space

    // Left side item 2: NEW Search Input Field anchored at the bottom
    QLineEdit *searchBar = new QLineEdit(this);
    searchBar->setPlaceholderText("🔍 Search by title, author, or ISBN...");
    searchBar->setClearButtonEnabled(true); // Adds an interactive standard "✕" button to quickly clear filters
    searchBar->setStyleSheet(
        "QLineEdit { "
        "  background-color: #1e1e1e; "
        "  border: 1px solid #3a3a3a; "
        "  border-radius: 4px; "
        "  padding: 8px 12px; "
        "  font-size: 13px; "
        "  color: #ffffff; "
        "}"
        "QLineEdit:focus { "
        "  border: 1px solid #3498db; "
        "}"
        );
    bookshelfAreaLayout->addWidget(searchBar, 0); // Set stretch factor 0 to prevent vertical scaling expansion

    // Add your nested container area directly into the primary bottom row layout split
    bottomRowLayout->addWidget(bookshelfAreaContainer, 3); // Takes up 3/4 horizontal width

    // Right side of bottom layout: Dedicated Standalone Book Details Sidebar component file
    m_detailsSidebar = new BookDetailsSidebar(this);
    m_detailsSidebar->setVisible(false); // Hide panel on application launch until requested
    bottomRowLayout->addWidget(m_detailsSidebar, 1); // Sidebar takes up remaining 1/4 width

    // Connect the combined horizontal bottom shelf layout to the main layout frame
    mainVerticalLayout->addWidget(bottomRowContainer, 1); // Receives all primary structural scaling layout footprint!

    setCentralWidget(centralWidget);
    setWindowTitle("ISBN Book Scanner");

    // Enlarge default application launch sizing metrics to show beautiful rows out of the box
    resize(950, 750);

    m_dbManager = new BookDatabaseManager(this);
    QString appDataFolder = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(appDataFolder);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    QString crossPlatformDbPath = QDir::cleanPath(appDataFolder + "/scanned_books.db");
    qDebug() << "[Database Info] Absolute SQL Path written to hardware:";
    qDebug() << "   ->" << crossPlatformDbPath;
    m_dbManager->initDatabase(crossPlatformDbPath);

    m_metadataProvider = new BookMetadataProvider(m_dbManager, this);

    // --- POPULATE BOOKSHELF HISTORY ROW ON BOOT ---
    QList<BookInfo> historicalBooks = m_dbManager->getAllSavedBooks();
    for (const BookInfo &book : historicalBooks) {
        m_bookshelfWidget->addBookToShelf(book, false);
    }

    // Connect functional interaction pipelines across classes
    connect(m_scannerView, &BarcodeScannerView::isbnScanned, m_metadataProvider, &BookMetadataProvider::lookupIsbn);
    connect(m_metadataProvider, &BookMetadataProvider::lookupStatusChanged, this, &MainWindow::updateStatusLabel);
    connect(m_metadataProvider, &BookMetadataProvider::bookDataReady, this, &MainWindow::displayBookDetails);
    connect(m_metadataProvider, &BookMetadataProvider::bookDataReady, m_dbManager, &BookDatabaseManager::saveBookRecord);

    connect(m_dbManager, &BookDatabaseManager::bookSavedSuccessfully, this, [this](const QString &isbn) {
        BookInfo freshRecord = m_dbManager->getBookByIsbn(isbn);

        if (freshRecord.found) {
            m_bookshelfWidget->addBookToShelf(freshRecord, true);
        }

        m_isbnLabel->setText("Scan complete! Local database repository successfully updated.");
    });

    connect(m_dbManager, &BookDatabaseManager::databaseError, this, [this](const QString &err){
        updateStatusLabel(err, true);
    });

    // --- CONNECT BOOKSHELF SELECTION DIRECTLY TO SIDEBAR WIDGET SLOT ---
    connect(m_bookshelfWidget, &BookshelfWidget::bookSelected, m_detailsSidebar, &BookDetailsSidebar::updateDetails);
    connect(m_detailsSidebar, &BookDetailsSidebar::deleteBookRequested, this, &MainWindow::removeBookRecord);

    // --- NEW CONNECTION: Connect Search text events straight to your filter routing slot engine ---
    connect(searchBar, &QLineEdit::textChanged, this, &MainWindow::onSearchTextChanged);

    m_scannerView->startCapture();

    m_syncManager = new BookSyncManager("http://127.0.0.1:8000", this);

    // Automatically upload scans to the cloud backend after local DB confirmation updates
    connect(m_dbManager, &BookDatabaseManager::bookSavedSuccessfully, this, [this](const QString &isbn) {
        BookInfo freshRecord = m_dbManager->getBookByIsbn(isbn);
        if (freshRecord.found) {
            // Optimistically attempt an upload, but log it into the local queue beforehand
            m_dbManager->addPendingUpload(isbn);
            m_syncManager->uploadBookToServer(freshRecord);
        }
    });

    // Update cloud backend when a local book deletion occurs
    connect(m_detailsSidebar, &BookDetailsSidebar::deleteBookRequested, this, [this](const QString &isbn) {
        // Add deletion to queue before hitting the server resource route
        m_dbManager->addPendingDelete(isbn);
        m_syncManager->deleteBookFromServer(isbn);
    });

    connect(m_syncManager, &BookSyncManager::uploadSucceeded, m_dbManager, &BookDatabaseManager::removePendingAction);
    connect(m_syncManager, &BookSyncManager::deleteSucceeded, m_dbManager, &BookDatabaseManager::removePendingAction);

    // 4. NEW: Create a routine to flush the queue when internet connectivity is validated
    connect(m_syncManager, &BookSyncManager::loginSuccess, this, [this]() {
        qDebug() << "[Sync Engine] Connection validated. Processing offline pending queue backlog...";

        // Handle backlogged deletions first
        QStringList deletes = m_dbManager->getPendingDeletes();
        for (const QString &isbn : deletes) {
            m_syncManager->deleteBookFromServer(isbn);
        }

        // Handle backlogged uploads next
        QStringList uploads = m_dbManager->getPendingUploads();
        for (const QString &isbn : uploads) {
            BookInfo book = m_dbManager->getBookByIsbn(isbn);
            if (book.found) {
                m_syncManager->uploadBookToServer(book);
            } else {
                m_dbManager->removePendingAction(isbn); // Clean up if book no longer exists locally
            }
        }
    });

    // Capture remote download data packets to modify local caches seamlessly
    connect(m_syncManager, &BookSyncManager::remoteBookUpdatesDownloaded, this,
            [this](const QList<BookInfo> &booksToSave, const QStringList &isbnsToDelete) {

                // Clean out deleted books from local storage
                for (const QString &isbn : isbnsToDelete) {
                    m_dbManager->deleteBookRecord(isbn);
                    m_bookshelfWidget->removeBookFromShelf(isbn);
                }

                // Add or update remote entries locally
                for (const BookInfo &book : booksToSave) {
                    m_dbManager->saveBookRecord(book);
                    m_bookshelfWidget->addBookToShelf(book, true);
                }
            });

    m_syncManager->loginAccount("stefan", "secret");
}

void MainWindow::updateStatusLabel(const QString &text, bool isError)
{
    if (isError) {
        qCritical() << "[Scanner System Error Alert]:\n" << text;
        m_isbnLabel->setText("Ready for next scan...");
        m_isbnLabel->setStyleSheet("font-size: 15px; font-weight: bold; color: #ff6b6b; padding: 10px; background: #2c1515; border: 1px solid #e74c3c; border-radius: 4px;");
    } else {
        m_isbnLabel->setText(text);
        m_isbnLabel->setStyleSheet("font-size: 15px; font-weight: bold; color: #f39c12; padding: 10px; background: #2c2215; border: 1px solid #d35400; border-radius: 4px;");
    }
}

// Triggered immediately when an active scanning cycle captures metadata
void MainWindow::displayBookDetails(const BookInfo &info)
{
    m_isbnLabel->setText(QString("📖 Successfully scanned: %1").arg(info.title));
    m_isbnLabel->setStyleSheet("font-size: 15px; font-weight: bold; color: #2ecc71; padding: 15px; background: #152c1e; border: 1px solid #27ae60; border-radius: 4px;");

    m_detailsSidebar->updateDetails(info);
}

void MainWindow::removeBookRecord(const QString &isbn)
{
    BookInfo bookToPurge = m_dbManager->getBookByIsbn(isbn);
    QString bookTitle = bookToPurge.found ? bookToPurge.title : "this book";

    QMessageBox::StandardButton confirmation;
    confirmation = QMessageBox::question(this,
                                         "Confirm Deletion",
                                         QString("Are you sure you want to permanently remove \"%1\" from your library archive?").arg(bookTitle),
                                         QMessageBox::Yes | QMessageBox::No);

    if (confirmation == QMessageBox::No) {
        qDebug() << "[Archive Controller] Deletion sequence safely cancelled by user.";
        return;
    }

    qDebug() << "[Archive Controller] Initiating absolute purge sequence for ISBN:" << isbn;

    bool success = m_dbManager->deleteBookRecord(isbn);

    if (success) {
        m_bookshelfWidget->removeBookFromShelf(isbn);
        m_detailsSidebar->closeSidebar();

        m_isbnLabel->setText("Book record removed successfully from shelf archive.");
        m_isbnLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #7f8c8d; padding: 8px; background: #ecf0f1;");
    } else {
        updateStatusLabel("Failed to remove book from local database storage hierarchy.", true);
    }
}

// NEW SLOT: Routes search bar inputs straight through to the bookshelf filter engine
void MainWindow::onSearchTextChanged(const QString &text)
{
    m_bookshelfWidget->filterBooks(text);
}
