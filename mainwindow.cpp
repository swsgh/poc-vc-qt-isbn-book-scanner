#include "mainwindow.h"
#include "barcodescannerview.h"
#include "bookmetadataprovider.h"
#include "bookdatabasemanager.h"
#include "bookshelfwidget.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);

    // 1. Scanner View Component (Fixed Top Allocation bounds)
    m_scannerView = new BarcodeScannerView(this);
    m_scannerView->setMaximumSize(400, 220);
    layout->addWidget(m_scannerView, 0, Qt::AlignHCenter); // Stretch factor 0 = keeps scanner small

    // 2. Descriptive Footer Feedback label
    m_isbnLabel = new QLabel("Align ISBN barcode with the red laser line...", this);
    m_isbnLabel->setAlignment(Qt::AlignCenter);
    m_isbnLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #2c3e50; padding: 8px; background: #ecf0f1;");
    layout->addWidget(m_isbnLabel, 0); // Stretch factor 0 = minimal space

    // 3. Bookshelf Grid Layout View (Primary Central Component)
    m_bookshelfWidget = new BookshelfWidget(this);

    // CHANGED PARAMETER: Stretch factor 1 instructs layout engine to give ALL
    // vertical scaling footprint expansion space directly to this element panel!
    layout->addWidget(m_bookshelfWidget, 1);

    setCentralWidget(centralWidget);
    setWindowTitle("Dynamic Library grid tracker");

    // Enlarge default application launch sizing metrics to show beautiful rows out of the box
    resize(850, 750);

    m_dbManager = new BookDatabaseManager(this);
    // On Windows: Maps to C:/Users/<User>/AppData/Local/<AppName>
    // On Android: Maps to /data/user/0/<PackageName>/files
    QString appDataFolder = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    // Windows and Android will fail to write the .db file if the folder hasn't been created yet
    QDir dir(appDataFolder);
    if (!dir.exists()) {
        dir.mkpath("."); // Dynamically constructs the full nesting chain safely
    }
    QString crossPlatformDbPath = QDir::cleanPath(appDataFolder + "/scanned_books.db");
    qDebug() << "[Database Info] Absolute SQL Path written to hardware:";
    qDebug() << "   ->" << crossPlatformDbPath;
    m_dbManager->initDatabase(crossPlatformDbPath);

    m_metadataProvider = new BookMetadataProvider(m_dbManager, this);

    // --- POPULATE BOOKSHELF HISTORY ROW ON BOOT ---
    QList<BookInfo> historicalBooks = m_dbManager->getAllSavedBooks();
    for (const BookInfo &book : historicalBooks) {
        m_bookshelfWidget->addBookToShelf(book, false); // Append historically sorted data rows
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
    if (isError) {
        // =================================================================
        // REDIRECT TO QDEBUG TERMINAL (CRITICAL ERROR LOG CONTEXT)
        // =================================================================
        qCritical() << "[Scanner System Error Alert]:\n" << text;
        qCritical() << "==================================================";

        // (Optional) Reset the label to a calm standby status text on the UI
        m_isbnLabel->setText("Ready for next scan...");
        m_isbnLabel->setStyleSheet("font-size: 15px; font-weight: bold; color: #7f8c8d; padding: 10px; background: #f2f4f4;");
    } else {
        // Standard, non-error tracking feedback status flows normally on the interface
        m_isbnLabel->setText(text);
        m_isbnLabel->setStyleSheet("font-size: 15px; font-weight: bold; color: #d35400; padding: 10px; background: #fdf2e9;");
    }
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
