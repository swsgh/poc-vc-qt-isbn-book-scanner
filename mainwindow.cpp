#include "mainwindow.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QVideoFrame>
#include <QImage>
#include <QDebug>

// Qt 6 Multimedia Header Architectures
#include <QCamera>
#include <QMediaCaptureSession>
#include <QVideoWidget>
#include <QVideoSink>
#include <QMediaDevices>

// ZXing-C++ (v2.2.1) Native Decoding Contexts
#include <ReadBarcode.h>
#include <BarcodeFormat.h>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_isProcessingFrame(false)
{
    // 1. Enforce Programmatic Pure Widget Layout Architecture
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);

    // Setup Video Viewfinder Component (Top Placement)
    m_videoWidget = new QVideoWidget(this);
    m_videoWidget->setMinimumSize(640, 480);
    layout->addWidget(m_videoWidget, 1); // Expand factor 1 takes remaining vertical footprint

    // Setup ISBN Result View Component (Bottom Placement)
    m_isbnLabel = new QLabel("Position an ISBN Barcode in the frame...", this);
    m_isbnLabel->setAlignment(Qt::AlignCenter);
    m_isbnLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #2c3e50; padding: 15px; background: #ecf0f1;");
    layout->addWidget(m_isbnLabel, 0); // Expand factor 0 strictly uses required height footprint

    setCentralWidget(centralWidget);
    setWindowTitle("ISBN Book Scanner");

    // 2. Initialize and Construct the Multi-Media Pipeline
    m_camera = std::make_unique<QCamera>(QMediaDevices::defaultVideoInput(), this);
    m_captureSession = std::make_unique<QMediaCaptureSession>(this);

    m_captureSession->setCamera(m_camera.get());
    m_captureSession->setVideoOutput(m_videoWidget); // Routes feed to screen

    // 3. Intercept Viewfinder Sink to extract underlying raw image frames
    if (m_videoWidget->videoSink()) {
        connect(m_videoWidget->videoSink(), &QVideoSink::videoFrameChanged,
                this, &MainWindow::processVideoFrame);
    }

    // Begin standard capture runtime
    m_camera->start();
}

MainWindow::~MainWindow()
{
    m_camera->stop();
}

void MainWindow::processVideoFrame(const QVideoFrame &frame)
{
    // Drop execution frames if the loop engine thread execution path is blocked
    if (m_isProcessingFrame || !frame.isValid()) {
        return;
    }

    m_isProcessingFrame = true;

    // Clone and map video frame data safely out of GPU bounds into CPU memory
    QVideoFrame cloneFrame(frame);
    if (cloneFrame.map(QVideoFrame::ReadOnly)) {

        // Convert to a structural formats that ZXing understands natively
        QImage image = cloneFrame.toImage().convertToFormat(QImage::Format_RGB888);

        if (!image.isNull()) {
            // Instantiate ZXing Interop ImageView wrapping the QImage data block points
            ZXing::ImageView imageView(image.bits(), image.width(), image.height(), ZXing::ImageFormat::RGB);

            // Configure scanner rules matching explicit ISBN traits (EAN_13)
            ZXing::ReaderOptions options;
            options.setFormats(ZXing::BarcodeFormat::EAN13);
            options.setTryHarder(true);

            // Trigger the execution loop evaluation match step
            ZXing::Result result = ReadBarcode(imageView, options);

            if (result.isValid()) {
                QString scannedText = QString::fromStdString(result.text());

                // Safely marshal the string back to the Main GUI Thread
                QMetaObject::invokeMethod(this, [this, scannedText]() {
                    m_isbnLabel->setText("Scanned ISBN: " + scannedText);
                    m_isbnLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #27ae60; padding: 15px; background: #e8f8f5;");
                }, Qt::QueuedConnection);
            }
        }
        cloneFrame.unmap();
    }

    m_isProcessingFrame = false;
}
