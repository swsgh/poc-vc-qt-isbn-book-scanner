#include "mainwindow.h"
#include "barcodescannerview.h"
#include "bookmetadataprovider.h"
#include "bookdatabasemanager.h"
#include "bookshelfwidget.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton> // NEW: Explicitly included for the close button
#include <QStandardPaths>
#include <QDir>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *mainVerticalLayout = new QVBoxLayout(centralWidget);

    // 1. TOP SECTION: Scanner View Component (Fixed Top Allocation bounds)
    m_scannerView = new BarcodeScannerView(this);
    m_scannerView->setMaximumSize(400, 220);
    mainVerticalLayout->addWidget(m_scannerView, 0, Qt::AlignHCenter); // Stretch factor 0 = keeps scanner small

    // 2. Descriptive Footer Feedback label (Now sits neatly right below the camera)
    m_isbnLabel = new QLabel("Align ISBN barcode with the red laser line...", this);
    m_isbnLabel->setAlignment(Qt::AlignCenter);
    m_isbnLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #2c3e50; padding: 8px; background: #ecf0f1;");
    mainVerticalLayout->addWidget(m_isbnLabel, 0); // Stretch factor 0 = minimal space

    // 3. BOTTOM SECTION: Bookshelf Row + Right Side Panel Container
    QWidget *bottomRowContainer = new QWidget(this);
    QHBoxLayout *bottomRowLayout = new QHBoxLayout(bottomRowContainer);
    bottomRowLayout->setContentsMargins(0, 5, 0, 0); // Clean, seamless alignment

    // Left side of bottom layout: Bookshelf Grid Layout View (Takes 3/4 layout footprint)
    m_bookshelfWidget = new BookshelfWidget(this);
    bottomRowLayout->addWidget(m_bookshelfWidget, 3);

    // Right side of bottom layout: Dedicated Book Details Sidebar panel
    // CHANGED: Instantiated into m_sidebarWidget so we can toggle visibility directly
    m_sidebarWidget = new QWidget(this);
    QVBoxLayout *sidebarLayout = new QVBoxLayout(m_sidebarWidget);
    m_sidebarWidget->setFixedWidth(280); // Locks sidebar to a clean, readable width
    m_sidebarWidget->setStyleSheet("background-color: #f8f9fa; border: 1px solid #dee2e6; border-radius: 4px;");

    // Hide the panel on application launch until an item is explicitly interactively chosen
    m_sidebarWidget->setVisible(false);

    // --- NEW: Interactive Top Header Row containing the Close Button ---
    QHBoxLayout *headerRowLayout = new QHBoxLayout();
    QLabel *sidebarHeader = new QLabel("<b>📚 BOOK DETAILS</b>", this);
    sidebarHeader->setStyleSheet("font-size: 13px; color: #7f8c8d; letter-spacing: 1px;");

    QPushButton *closeSidebarButton = new QPushButton("✕", this);
    closeSidebarButton->setFixedSize(24, 24);
    closeSidebarButton->setStyleSheet(
        "QPushButton { border: none; background: transparent; font-size: 14px; color: #95a5a6; font-weight: bold; }"
        "QPushButton:hover { color: #e74c3c; background-color: #f2f3f4; border-radius: 12px; }"
        );
    closeSidebarButton->setCursor(Qt::PointingHandCursor);

    headerRowLayout->addWidget(sidebarHeader, 1, Qt::AlignLeft);
    headerRowLayout->addWidget(closeSidebarButton, 0, Qt::AlignRight);
    sidebarLayout->addLayout(headerRowLayout);
    // --------------------------------------------------------------------

    m_detailTitleLabel = new QLabel("Select a book from your shelf...", this);
    m_detailTitleLabel->setWordWrap(true);
    m_detailTitleLabel->setStyleSheet("font-size: 14px; color: #2c3e50; font-weight: 500;");

    m_detailAuthorLabel = new QLabel("", this);
    m_detailAuthorLabel->setWordWrap(true);
    m_detailAuthorLabel->setStyleSheet("font-size: 13px; color: #566573;");

    m_detailIsbnLabel = new QLabel("", this);
    m_detailIsbnLabel->setStyleSheet("font-size: 12px; color: #95a5a6; font-family: monospace;");

    sidebarLayout->addWidget(m_detailTitleLabel);
    sidebarLayout->addWidget(m_detailAuthorLabel);
    sidebarLayout->addWidget(m_detailIsbnLabel);
    sidebarLayout->addStretch(); // Pushes all the text up to the top of the sidebar

    bottomRowLayout->addWidget(m_sidebarWidget, 1); // Sidebar takes up remaining 1/4 width

    // Connect the combined horizontal bottom shelf layout to the main layout frame
    mainVerticalLayout->addWidget(bottomRowContainer, 1); // Receives all primary structural scaling layout footprint!

    setCentralWidget(centralWidget);
    setWindowTitle("Dynamic Library grid tracker");

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

    // --- CONNECT BOOKSHELF SELECTION TO SIDEBAR UPDATE ---
    connect(m_bookshelfWidget, &BookshelfWidget::bookSelected, this, &MainWindow::updateDetailsSidebar);

    // --- NEW CONNECTION: Connect Close Button trigger to close slot ---
    connect(closeSidebarButton, &QPushButton::clicked, this, &MainWindow::closeDetailsSidebar);

    m_scannerView->startCapture();
}

void MainWindow::updateStatusLabel(const QString &text, bool isError)
{
    if (isError) {
        qCritical() << "[Scanner System Error Alert]:\n" << text;
        qCritical() << "==================================================";
        m_isbnLabel->setText("Ready for next scan...");
        m_isbnLabel->setStyleSheet("font-size: 15px; font-weight: bold; color: #7f8c8d; padding: 10px; background: #f2f4f4;");
    } else {
        m_isbnLabel->setText(text);
        m_isbnLabel->setStyleSheet("font-size: 15px; font-weight: bold; color: #d35400; padding: 10px; background: #fdf2e9;");
    }
}

// Triggered immediately when an active scanning cycle captures metadata
void MainWindow::displayBookDetails(const BookInfo &info)
{
    m_isbnLabel->setText(QString("📖 Successfully scanned: %1").arg(info.title));
    m_isbnLabel->setStyleSheet("font-size: 15px; font-weight: bold; color: #1e8449; padding: 15px; background: #e8f8f5;");

    updateDetailsSidebar(info);
}

// Triggered when clicking a book item inside the bookshelf layout row or when a live scan happens
void MainWindow::updateDetailsSidebar(const BookInfo &info)
{
    // Make sure the panel reveals itself dynamically if it was previously hidden away
    m_sidebarWidget->setVisible(true);

    m_detailTitleLabel->setText(QString("<b>Title:</b><br>%1").arg(info.title));
    m_detailAuthorLabel->setText(QString("<b>Author(s):</b><br>%1").arg(info.authors.isEmpty() ? "Unknown" : info.authors));
    m_detailIsbnLabel->setText(QString("<b>ISBN:</b> %1<br><small>Source: %2</small>").arg(info.isbn).arg(info.engineSource));
}

// NEW SLOT: Completely hides the panel layout container instantly
void MainWindow::closeDetailsSidebar()
{
    m_sidebarWidget->setVisible(false);

    // Reset standard fallback state metrics inside layout strings
    m_detailTitleLabel->setText("Select a book from your shelf...");
    m_detailAuthorLabel->clear();
    m_detailIsbnLabel->clear();
}

void MainWindow::handleDatabaseConfirmation(const QString &isbn)
{
    m_isbnLabel->setText("Saved securely into local archive database.");
}
