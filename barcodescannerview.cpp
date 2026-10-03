#include "barcodescannerview.h"

#include <QPainter>
#include <QRegion>
#include <QVideoFrame>
#include <QImage>
#include <QCamera>
#include <QMediaCaptureSession>
#include <QVideoSink> // Direct frame sink access
#include <QMediaDevices>

#include <ReadBarcode.h>
#include <BarcodeFormat.h>

BarcodeScannerView::BarcodeScannerView(QWidget *parent)
    : QWidget(parent)
    , m_isProcessingFrame(false)
{
    // Ensure the widget can expand dynamically inside MainWindow's layout
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumSize(320, 240);

    m_camera = std::make_unique<QCamera>(QMediaDevices::defaultVideoInput(), this);
    m_captureSession = std::make_unique<QMediaCaptureSession>(this);
    m_videoSink = std::make_unique<QVideoSink>(this);

    m_captureSession->setCamera(m_camera.get());

    // Route the camera output stream straight into our custom data sink
    m_captureSession->setVideoOutput(m_videoSink.get());

    connect(m_videoSink.get(), &QVideoSink::videoFrameChanged,
            this, &BarcodeScannerView::processVideoFrame);
}

BarcodeScannerView::~BarcodeScannerView()
{
    stopCapture();
}

void BarcodeScannerView::startCapture() { m_camera->start(); }
void BarcodeScannerView::stopCapture() { m_camera->stop(); }

void BarcodeScannerView::processVideoFrame(const QVideoFrame &frame)
{
    if (!frame.isValid()) return;

    QVideoFrame cloneFrame(frame);
    if (cloneFrame.map(QVideoFrame::ReadOnly)) {
        // Convert the underlying hardware frame format cleanly into a CPU QImage
        QImage image = cloneFrame.toImage().convertToFormat(QImage::Format_RGB888);
        cloneFrame.unmap();

        if (!image.isNull()) {
            // Update the frame logic cache on the GUI Thread safely
            QMetaObject::invokeMethod(this, [this, image]() {
                m_currentFrame = image;
                update(); // Tells Qt to trigger paintEvent immediately
            }, Qt::QueuedConnection);

            // Double check execution path block flag before running heavy ZXing loops
            if (!m_isProcessingFrame) {
                m_isProcessingFrame = true;

                ZXing::ImageView imageView(image.bits(), image.width(), image.height(), ZXing::ImageFormat::RGB);
                ZXing::ReaderOptions options;
                options.setFormats(ZXing::BarcodeFormat::EAN13);
                options.setTryHarder(true);

                ZXing::Result result = ZXing::ReadBarcode(imageView, options);

                if (result.isValid()) {
                    QString scannedText = QString::fromStdString(result.text());
                    QMetaObject::invokeMethod(this, [this, scannedText]() {
                        emit isbnScanned(scannedText);
                    }, Qt::QueuedConnection);
                }
                m_isProcessingFrame = false;
            }
        }
    }
}

// Custom paint loop combining both camera frames and your visual overlays smoothly
void BarcodeScannerView::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 1. Draw the live camera feed if an image is available
    if (!m_currentFrame.isNull()) {
        // Automatically scales the camera frame to fill your window footprint smoothly
        QImage scaledFrame = m_currentFrame.scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        int frameX = (width() - scaledFrame.width()) / 2;
        int frameY = (height() - scaledFrame.height()) / 2;
        painter.drawImage(frameX, frameY, scaledFrame);
    } else {
        // Fallback layout screen background while the camera loads
        painter.fillRect(rect(), Qt::black);
    }

    // 2. Compute scanning target window metrics
    int boxWidth = width() * 0.7;
    int boxHeight = height() * 0.25;
    int x = (width() - boxWidth) / 2;
    int y = (height() - boxHeight) / 2;
    QRect targetRect(x, y, boxWidth, boxHeight);

    // 3. Darken target framing boundaries
    QRegion overlayRegion(rect());
    QRegion targetRegion(targetRect);
    QRegion dimmedRegion = overlayRegion.subtracted(targetRegion);

    painter.setClipRegion(dimmedRegion);
    painter.fillRect(rect(), QColor(0, 0, 0, 80));
    painter.setClipping(false);

    // 4. Draw modern corner targeting brackets
    painter.setPen(QPen(QColor("#27ae60"), 4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    int len = 20;

    // Top-Left
    painter.drawLine(x, y, x + len, y);
    painter.drawLine(x, y, x, y + len);
    // Top-Right
    painter.drawLine(x + boxWidth, y, x + boxWidth - len, y);
    painter.drawLine(x + boxWidth, y, x + boxWidth, y + len);
    // Bottom-Left
    painter.drawLine(x, y + boxHeight, x + len, y + boxHeight);
    painter.drawLine(x, y + boxHeight, x, y + boxHeight - len);
    // Bottom-Right
    painter.drawLine(x + boxWidth, y + boxHeight, x + boxWidth - len, y + boxHeight);
    painter.drawLine(x + boxWidth, y + boxHeight, x + boxWidth, y + boxHeight - len);

    // 5. Draw Red Horizontal Laser Line
    painter.setPen(QPen(QColor("#e74c3c"), 2, Qt::DashLine));
    int centerY = y + (boxHeight / 2);
    painter.drawLine(x + 5, centerY, x + boxWidth - 5, centerY);
}
