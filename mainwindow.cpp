#include "mainwindow.h"
#include "barcodescannerview.h"
#include "bookmetadataprovider.h"
#include "bookdatabasemanager.h"
#include "bookshelfwidget.h"
#include "bookdetailssidebar.h" // NEW: Added inclusion for decoupled side widget panel class

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QMessageBox>

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

    // Left side of bottom layout: Bookshelf Grid Layout View (Takes 3/4 layout footprint)
    m_bookshelfWidget = new BookshelfWidget(this);
    bottomRowLayout->addWidget(m_bookshelfWidget, 3);

    // Right side of bottom layout: Dedicated Standalone Book Details Sidebar component file
    // MOVED: All label allocations, buttons, and styles are now contained inside BookDetailsSidebar
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

    m_scannerView->startCapture();
}

void MainWindow::updateStatusLabel(const QString &text, bool isError)
{
    if (isError) {
        qCritical() << "[Scanner System Error Alert]:\n" << text;
        m_isbnLabel->setText("Ready for next scan...");
        // Crimson accent notice box for errors
        m_isbnLabel->setStyleSheet("font-size: 15px; font-weight: bold; color: #ff6b6b; padding: 10px; background: #2c1515; border: 1px solid #e74c3c; border-radius: 4px;");
    } else {
        m_isbnLabel->setText(text);
        // Amber accent notice box for actively processing lookups
        m_isbnLabel->setStyleSheet("font-size: 15px; font-weight: bold; color: #f39c12; padding: 10px; background: #2c2215; border: 1px solid #d35400; border-radius: 4px;");
    }
}

// Triggered immediately when an active scanning cycle captures metadata
void MainWindow::displayBookDetails(const BookInfo &info)
{
    m_isbnLabel->setText(QString("📖 Successfully scanned: %1").arg(info.title));
    // Emerald green accent notice box for a successful book match capture
    m_isbnLabel->setStyleSheet("font-size: 15px; font-weight: bold; color: #2ecc71; padding: 15px; background: #152c1e; border: 1px solid #27ae60; border-radius: 4px;");

    m_detailsSidebar->updateDetails(info);
}

void MainWindow::removeBookRecord(const QString &isbn)
{
    // 1. Fetch the book data first to display its title in the warning message box
    BookInfo bookToPurge = m_dbManager->getBookByIsbn(isbn);
    QString bookTitle = bookToPurge.found ? bookToPurge.title : "this book";

    // 2. Spawn a modal warning dialog box asking for verification
    QMessageBox::StandardButton confirmation;
    confirmation = QMessageBox::question(this,
                                         "Confirm Deletion",
                                         QString("Are you sure you want to permanently remove \"%1\" from your library archive?").arg(bookTitle),
                                         QMessageBox::Yes | QMessageBox::No);

    // If the user clicks "No" or closes the window, abort the delete operation instantly
    if (confirmation == QMessageBox::No) {
        qDebug() << "[Archive Controller] Deletion sequence safely cancelled by user.";
        return;
    }

    qDebug() << "[Archive Controller] Initiating absolute purge sequence for ISBN:" << isbn;

    // 3. Proceed with deletion since user clicked "Yes"
    bool success = m_dbManager->deleteBookRecord(isbn);

    if (success) {
        // Erase visual card node component from bookshelf layout view instantly
        m_bookshelfWidget->removeBookFromShelf(isbn);

        // Clear and hide the sidebar panel cleanly
        m_detailsSidebar->closeSidebar();

        m_isbnLabel->setText("Book record removed successfully from shelf archive.");
        m_isbnLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #7f8c8d; padding: 8px; background: #ecf0f1;");
    } else {
        updateStatusLabel("Failed to remove book from local database storage hierarchy.", true);
    }
}
