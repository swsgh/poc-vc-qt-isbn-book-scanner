#include "mainwindow.h"
#include "barcodescannerview.h"
#include "bookmetadataprovider.h"

#include <QVBoxLayout>
#include <QLabel>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);

    // 1. Mount components onto layout tree
    m_scannerView = new BarcodeScannerView(this);
    layout->addWidget(m_scannerView, 1);

    m_isbnLabel = new QLabel("Align ISBN barcode with the red laser line...", this);
    m_isbnLabel->setAlignment(Qt::AlignCenter);
    m_isbnLabel->setWordWrap(true);
    m_isbnLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #2c3e50; padding: 15px; background: #ecf0f1;");
    layout->addWidget(m_isbnLabel, 0);

    setCentralWidget(centralWidget);
    setWindowTitle("Decoupled Architecture Scanner");

    // 2. Initialize the backend data processing component
    m_metadataProvider = new BookMetadataProvider(this);

    // 3. Connect signals across components (Plugging components together)
    connect(m_scannerView, &BarcodeScannerView::isbnScanned,
            m_metadataProvider, &BookMetadataProvider::lookupIsbn);

    connect(m_metadataProvider, &BookMetadataProvider::lookupStatusChanged,
            this, &MainWindow::updateStatusLabel);

    connect(m_metadataProvider, &BookMetadataProvider::bookDataReady,
            this, &MainWindow::displayBookDetails);

    m_scannerView->startCapture();
}

void MainWindow::updateStatusLabel(const QString &text, bool isError)
{
    m_isbnLabel->setText(text);
    if (isError) {
        m_isbnLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #c0392b; padding: 15px; background: #f9ebea;");
    } else {
        m_isbnLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #d35400; padding: 15px; background: #fdf2e9;");
    }
}

void MainWindow::displayBookDetails(const BookInfo &info)
{
    QString displayTemplate = QString("📖 Title: %1\n✍️ Author(s): %2\n🔢 Code: %3\n⚙️ Source: %4")
                                  .arg(info.title)
                                  .arg(info.authors)
                                  .arg(info.isbn)
                                  .arg(info.engineSource); // Shows "Open Library" or "Google Books"

    m_isbnLabel->setText(displayTemplate);
    m_isbnLabel->setStyleSheet("font-size: 15px; font-weight: bold; color: #1e8449; padding: 15px; background: #e8f8f5;");
}
