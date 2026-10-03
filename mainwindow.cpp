#include "mainwindow.h"
#include "barcodescannerview.h"
#include "bookmetadataprovider.h"
#include "bookdatabasemanager.h"
#include "bookshelfwidget.h"

#include <QVBoxLayout>
#include <QLabel>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);

    // 1. Scanner View Component (Top Allocation)
    m_scannerView = new BarcodeScannerView(this);
    layout->addWidget(m_scannerView, 3); // Stretch factor 3 keeps camera viewfinder tall

    // 2. Active Metadata Tracking Footer Label View (Center Allocation)
    m_isbnLabel = new QLabel("Align ISBN barcode with the red laser line...", this);
    m_isbnLabel->setAlignment(Qt::AlignCenter);
    m_isbnLabel->setWordWrap(true);
    m_isbnLabel->setStyleSheet("font-size: 15px; font-weight: bold; color: #2c3e50; padding: 10px; background: #ecf0f1;");
    layout->addWidget(m_isbnLabel, 0);

    // 3. Bookshelf Widget Component View (Bottom Allocation)
    m_bookshelfWidget = new BookshelfWidget(this);
    layout->addWidget(m_bookshelfWidget, 2); // Stretch factor 2 allocates proper room for covers layout row

    setCentralWidget(centralWidget);
    setWindowTitle("Virtual Bookshelf Tracker");
    resize(850, 750); // Provide extra vertical window bounds canvas space for the new bookshelf rows

    // Initialize individual application component controllers
    m_metadataProvider = new BookMetadataProvider(this);
    m_dbManager = new BookDatabaseManager(this);

    if (m_dbManager->initDatabase("scanned_books.db")) {
        // --- POPULATE BOOKSHELF HISTORY ROW ON BOOT ---
        QList<BookInfo> historicalBooks = m_dbManager->getAllSavedBooks();
        for (const BookInfo &book : historicalBooks) {
            m_bookshelfWidget->addBookToShelf(book, false); // Append historically sorted data rows
        }
    }

    // Connect functional interaction pipelines across classes
    connect(m_scannerView, &BarcodeScannerView::isbnScanned, m_metadataProvider, &BookMetadataProvider::lookupIsbn);
    connect(m_metadataProvider, &BookMetadataProvider::lookupStatusChanged, this, &MainWindow::updateStatusLabel);
    connect(m_metadataProvider, &BookMetadataProvider::bookDataReady, this, &MainWindow::displayBookDetails);
    connect(m_metadataProvider, &BookMetadataProvider::bookDataReady, m_dbManager, &BookDatabaseManager::saveBookRecord);

    connect(m_dbManager, &BookDatabaseManager::bookSavedSuccessfully, this, [this](const QString &isbn) {
        // Extract the specific book record that was just written to the database
        BookInfo freshRecord = m_dbManager->getBookByIsbn(isbn);

        if (freshRecord.found) {
            // Send it to the shelf; our new logic will replace or prepend it without duplicates!
            m_bookshelfWidget->addBookToShelf(freshRecord, true);
        }

        m_isbnLabel->setText(m_isbnLabel->text() + "\nLocal database cache repository successfully updated.");
    });

    connect(m_dbManager, &BookDatabaseManager::databaseError, this, [this](const QString &err){
        updateStatusLabel(err, true);
    });

    m_scannerView->startCapture();
}

void MainWindow::updateStatusLabel(const QString &text, bool isError)
{
    m_isbnLabel->setText(text);
    m_isbnLabel->setStyleSheet(isError ? "font-size: 14px; font-weight: bold; color: #c0392b; padding: 15px; background: #f9ebea;"
                                       : "font-size: 16px; font-weight: bold; color: #d35400; padding: 15px; background: #fdf2e9;");
}

void MainWindow::displayBookDetails(const BookInfo &info)
{
    QString displayTemplate = QString("📖 Title: %1\n✍️ Author(s): %2\n🔢 Code: %3\n⚙️ Source: %4")
                                  .arg(info.title)
                                  .arg(info.authors)
                                  .arg(info.isbn)
                                  .arg(info.engineSource);

    m_isbnLabel->setText(displayTemplate);
    m_isbnLabel->setStyleSheet("font-size: 15px; font-weight: bold; color: #1e8449; padding: 15px; background: #e8f8f5;");
}

void MainWindow::handleDatabaseConfirmation(const QString &isbn)
{
    // Append a quick secondary layout confirmation message
    m_isbnLabel->setText(m_isbnLabel->text() + "\n💾 Saved securely into local archive database.");
}
