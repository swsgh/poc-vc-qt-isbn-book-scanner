#include "barcodescannerview.h"
#include <QPainter>
#include <QRegion>
#include <QVideoFrame>
#include <QImage>
#include <QCamera>
#include <QMediaCaptureSession>
#include <QVideoSink>
#include <QMediaDevices>
#include <QPalette>

// Platform-specific conditional headers
#if defined(Q_OS_ANDROID)
#include <QCoreApplication>
#include <QPermissions>   // Required for mobile camera consent prompts
#endif

#include <ReadBarcode.h>
#include <BarcodeFormat.h>

BarcodeScannerView::BarcodeScannerView(QWidget *parent)
    : QWidget(parent)
    , m_isProcessingFrame(false)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumSize(320, 240);

    m_camera = std::make_unique<QCamera>(QMediaDevices::defaultVideoInput(), this);
    m_captureSession = std::make_unique<QMediaCaptureSession>(this);
    m_videoSink = std::make_unique<QVideoSink>(this);

#if defined(Q_OS_ANDROID)
    // 1. Try passing AutoNear first (ideal for tight tracking barcodes)
    if (m_camera->isFocusModeSupported(QCamera::FocusModeAutoNear)) {
        m_camera->setFocusMode(QCamera::FocusModeAutoNear);
    }
    // 2. Fall back to standard continuous auto focus if AutoNear is missing
    else if (m_camera->isFocusModeSupported(QCamera::FocusModeAuto)) {
        m_camera->setFocusMode(QCamera::FocusModeAuto);
    }
#endif

    m_captureSession->setCamera(m_camera.get());
    m_captureSession->setVideoOutput(m_videoSink.get());

        connect(m_camera.get(), &QCamera::errorOccurred, this,
            [this](QCamera::Error, const QString &) {
            emit cameraUnavailable(
                "No camera detected. Use the manual ISBN field instead.");
            });
    connect(m_videoSink.get(), &QVideoSink::videoFrameChanged,
            this, &BarcodeScannerView::processVideoFrame);
}

BarcodeScannerView::~BarcodeScannerView()
{
    stopCapture();
}

void BarcodeScannerView::startCapture()
{
#if defined(Q_OS_ANDROID)
    // Dynamic runtime authorization check required on Android
    QCameraPermission cameraPermission;
    if (qApp->checkPermission(cameraPermission) == Qt::PermissionStatus::Granted) {
        m_camera->start();
    } else {
        qApp->requestPermission(cameraPermission, this, [this](const QPermission &permission) {
            if (permission.status() == Qt::PermissionStatus::Granted) {
                m_camera->start();
            } else {
                emit isbnScanned("ERROR: Camera permission denied.");
            }
        });
    }
#else
    // Windows desktop platforms start up instantly without popups
    m_camera->start();
#endif
}

void BarcodeScannerView::stopCapture()
{
    m_camera->stop();
}

void BarcodeScannerView::processVideoFrame(const QVideoFrame &frame)
{
    if (m_isProcessingFrame || !frame.isValid()) return;
    m_isProcessingFrame = true;

    QVideoFrame cloneFrame(frame);
    QImage image;

#if defined(Q_OS_ANDROID)
    // 1. Android GPU texture optimization block
    if (cloneFrame.handleType() != QVideoFrame::NoHandle) {
        image = QImage(cloneFrame.size(), QImage::Format_RGB888);
        image.fill(palette().color(QPalette::Window));

        QPainter painter(&image);
        QVideoFrame::PaintOptions options;
        cloneFrame.paint(&painter, QRect(0, 0, image.width(), image.height()), options);
        painter.end();
    }
    else if (cloneFrame.map(QVideoFrame::ReadOnly)) {
        image = cloneFrame.toImage().convertToFormat(QImage::Format_RGB888);
        cloneFrame.unmap();
    }
#else
    // 2. Clear, simple Windows 11 memory mapping block
    if (cloneFrame.map(QVideoFrame::ReadOnly)) {
        image = cloneFrame.toImage().convertToFormat(QImage::Format_RGB888);
        cloneFrame.unmap();
    }
#endif

    if (!image.isNull()) {
        QMetaObject::invokeMethod(this, [this, image]() {
            m_currentFrame = image;
            update(); // Triggers cross-platform paintEvent overlay redraw
        }, Qt::QueuedConnection);

        // Core ZXing scanning regional crop calculations
        double targetBoxWidthPercent = 0.7;
        double targetBoxHeightPercent = 0.25;
        int cropWidth = static_cast<int>(image.width() * targetBoxWidthPercent);
        int cropHeight = static_cast<int>(image.height() * targetBoxHeightPercent);
        int cropX = (image.width() - cropWidth) / 2;
        int cropY = (image.height() - cropHeight) / 2;

        QImage croppedZone = image.copy(QRect(cropX, cropY, cropWidth, cropHeight));

        if (!croppedZone.isNull()) {
            ZXing::ImageView imageView(croppedZone.bits(), croppedZone.width(), croppedZone.height(), ZXing::ImageFormat::RGB);
            ZXing::ReaderOptions options;
            options.setFormats(ZXing::BarcodeFormat::EAN13);

            ZXing::Result result = ZXing::ReadBarcode(imageView, options);
            if (result.isValid()) {
                QString scannedText = QString::fromStdString(result.text());
                QMetaObject::invokeMethod(this, [this, scannedText]() {
                    emit isbnScanned(scannedText);
                }, Qt::QueuedConnection);
            }
        }
    }
    m_isProcessingFrame = false;
}

// Custom view graphics overlay (Remains identical to your current repo setup)
void BarcodeScannerView::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    if (!m_currentFrame.isNull()) {
        QImage scaledFrame = m_currentFrame.scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        int frameX = (width() - scaledFrame.width()) / 2;
        int frameY = (height() - scaledFrame.height()) / 2;
        painter.drawImage(frameX, frameY, scaledFrame);
    } else {
        painter.fillRect(rect(), palette().color(QPalette::Window));
        painter.setPen(palette().color(QPalette::WindowText));
        painter.drawText(rect(), Qt::AlignCenter, "Waiting for camera frame...");
    }

    int boxWidth = width() * 0.7;
    int boxHeight = height() * 0.25;
    int x = (width() - boxWidth) / 2;
    int y = (height() - boxHeight) / 2;
    QRect targetRect(x, y, boxWidth, boxHeight);

    QRegion overlayRegion(rect());
    QRegion targetRegion(targetRect);
    QRegion dimmedRegion = overlayRegion.subtracted(targetRegion);

    painter.setClipRegion(dimmedRegion);
    painter.fillRect(rect(), QColor(0, 0, 0, 80));
    painter.setClipping(false);

    painter.setPen(QPen(QColor("#27ae60"), 4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    int len = 20;

    painter.drawLine(x, y, x + len, y);
    painter.drawLine(x, y, x, y + len);
    painter.drawLine(x + boxWidth, y, x + boxWidth - len, y);
    painter.drawLine(x + boxWidth, y, x + boxWidth, y + len);
    painter.drawLine(x, y + boxHeight, x + len, y + boxHeight);
    painter.drawLine(x, y + boxHeight, x, y + boxHeight - len);
    painter.drawLine(x + boxWidth, y + boxHeight, x + boxWidth - len, y + boxHeight);
    painter.drawLine(x + boxWidth, y + boxHeight, x + boxWidth, y + boxHeight - len);

    painter.setPen(QPen(QColor("#e74c3c"), 2, Qt::DashLine));
    int centerY = y + (boxHeight / 2);
    painter.drawLine(x + 5, centerY, x + boxWidth - 5, centerY);
}
