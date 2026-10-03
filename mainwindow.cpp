#include "mainwindow.h"
#include "barcodescannerview.h"
#include "bookmetadataprovider.h"
#include "bookdatabasemanager.h" // <-- Added

#include <QVBoxLayout>
#include <QLabel>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);

    m_scannerView = new BarcodeScannerView(this);
    layout->addWidget(m_scannerView, 1);

    m_isbnLabel = new QLabel("Align ISBN barcode with the red laser line...", this);
    m_isbnLabel->setAlignment(Qt::AlignCenter);
    m_isbnLabel->setWordWrap(true);
    m_isbnLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #2c3e50; padding: 15px; background: #ecf0f1;");
    layout->addWidget(m_isbnLabel, 0);

    setCentralWidget(centralWidget);
    setWindowTitle("ISBN Local Cataloger");

    // Initialize the standalone components
    m_metadataProvider = new BookMetadataProvider(this);
    m_dbManager = new BookDatabaseManager(this);

    // Start up the database file
    m_dbManager->initDatabase("scanned_books.db");

    // =================================================================
    // ORCHESTRATION LINKS: Inter-connecting the architectural modules
    // =================================================================
    // 1. Scanner View sends raw barcode text strings to the API layer
    connect(m_scannerView, &BarcodeScannerView::isbnScanned,
            m_metadataProvider, &BookMetadataProvider::lookupIsbn);

    // 2. Metadata Engine reports network statuses back to the main UI label
    connect(m_metadataProvider, &BookMetadataProvider::lookupStatusChanged,
            this, &MainWindow::updateStatusLabel);

    // 3. Metadata Engine routes successful payloads straight to the UI display
    connect(m_metadataProvider, &BookMetadataProvider::bookDataReady,
            this, &MainWindow::displayBookDetails);

    // 4. CRITICAL LINK: Automatically direct matched payloads down to SQLite database storage
    connect(m_metadataProvider, &BookMetadataProvider::bookDataReady,
            m_dbManager, &BookDatabaseManager::saveBookRecord);

    // 5. Database events reporting back up to UI confirmation pipes
    connect(m_dbManager, &BookDatabaseManager::bookSavedSuccessfully,
            this, &MainWindow::handleDatabaseConfirmation);
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
