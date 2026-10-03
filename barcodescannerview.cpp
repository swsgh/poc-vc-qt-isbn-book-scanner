#include "barcodescannerview.h"

#include <QVBoxLayout>
#include <QPainter>
#include <QRegion>
#include <QVideoFrame>
#include <QImage>

#include <QCamera>
#include <QMediaCaptureSession>
#include <QVideoWidget>
#include <QVideoSink>
#include <QMediaDevices>

#include <ReadBarcode.h>
#include <BarcodeFormat.h>

// --- ViewfinderOverlay Implementation ---
ViewfinderOverlay::ViewfinderOverlay(QWidget *parent) : QWidget(parent)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
}

void ViewfinderOverlay::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

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

    painter.setPen(QPen(QColor("#e74c3c"), 2, Qt::DashLine));
    int centerY = y + (boxHeight / 2);
    painter.drawLine(x + 5, centerY, x + boxWidth - 5, centerY);
}

// --- BarcodeScannerView Implementation ---
BarcodeScannerView::BarcodeScannerView(QWidget *parent)
    : QWidget(parent)
    , m_isProcessingFrame(false)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_videoWidget = new QVideoWidget(this);
    layout->addWidget(m_videoWidget);

    m_overlay = new ViewfinderOverlay(m_videoWidget);

    m_camera = std::make_unique<QCamera>(QMediaDevices::defaultVideoInput(), this);
    m_captureSession = std::make_unique<QMediaCaptureSession>(this);

    m_captureSession->setCamera(m_camera.get());
    m_captureSession->setVideoOutput(m_videoWidget);

    if (m_videoWidget->videoSink()) {
        connect(m_videoWidget->videoSink(), &QVideoSink::videoFrameChanged,
                this, &BarcodeScannerView::processVideoFrame);
    }
}

BarcodeScannerView::~BarcodeScannerView()
{
    stopCapture();
}

void BarcodeScannerView::startCapture() { m_camera->start(); }
void BarcodeScannerView::stopCapture() { m_camera->stop(); }

void BarcodeScannerView::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (m_overlay && m_videoWidget) {
        m_overlay->setGeometry(0, 0, m_videoWidget->width(), m_videoWidget->height());
    }
}

void BarcodeScannerView::processVideoFrame(const QVideoFrame &frame)
{
    if (m_isProcessingFrame || !frame.isValid()) {
        return;
    }

    m_isProcessingFrame = true;

    QVideoFrame cloneFrame(frame);
    if (cloneFrame.map(QVideoFrame::ReadOnly)) {
        QImage image = cloneFrame.toImage().convertToFormat(QImage::Format_RGB888);

        if (!image.isNull()) {
            ZXing::ImageView imageView(image.bits(), image.width(), image.height(), ZXing::ImageFormat::RGB);

            ZXing::ReaderOptions options;
            options.setFormats(ZXing::BarcodeFormat::EAN13);
            options.setTryHarder(true);

            ZXing::Result result = ZXing::ReadBarcode(imageView, options);

            if (result.isValid()) {
                QString scannedText = QString::fromStdString(result.text());

                // Emitting the signal safely using a QueuedConnection back onto the GUI Thread
                QMetaObject::invokeMethod(this, [this, scannedText]() {
                    emit isbnScanned(scannedText);
                }, Qt::QueuedConnection);
            }
        }
        cloneFrame.unmap();
    }

    m_isProcessingFrame = false;
}
