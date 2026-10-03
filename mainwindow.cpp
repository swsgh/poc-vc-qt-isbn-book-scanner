#include "mainwindow.h"
#include "barcodescannerview.h"

#include <QVBoxLayout>
#include <QLabel>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
// Remainder definitions...
{
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);

    // 1. Instance our separate view object block
    m_scannerView = new BarcodeScannerView(this);
    layout->addWidget(m_scannerView, 1);

    // 2. Setup standard descriptive interface labels
    m_isbnLabel = new QLabel("Align ISBN barcode with the red laser line...", this);
    m_isbnLabel->setAlignment(Qt::AlignCenter);
    m_isbnLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #2c3e50; padding: 15px; background: #ecf0f1;");
    layout->addWidget(m_isbnLabel, 0);

    setCentralWidget(centralWidget);
    setWindowTitle("Decoupled ISBN Scanner");

    // 3. Hook up the clean scanner event channel straight to the main window handler
    connect(m_scannerView, &BarcodeScannerView::isbnScanned, this, &MainWindow::handleIsbnScanned);

    m_scannerView->startCapture();
}

void MainWindow::handleIsbnScanned(const QString &isbn)
{
    m_isbnLabel->setText("Scanned ISBN: " + isbn);
    m_isbnLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #27ae60; padding: 15px; background: #e8f8f5;");
}
