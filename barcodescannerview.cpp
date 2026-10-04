#include "barcodescannerview.h"
#include "barcodescannercontroller.h"

#include <QPainter>
#include <QRegion>
#include <QPalette>

BarcodeScannerView::BarcodeScannerView(QWidget *parent)
    : QWidget(parent)
{
    setAttribute(Qt::WA_OpaquePaintEvent);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMinimumSize(320, 240);

    m_controller = new BarcodeScannerController(this);
    connect(m_controller, &BarcodeScannerController::frameReady,
            this, &BarcodeScannerView::updateFrame);
    connect(m_controller, &BarcodeScannerController::isbnScanned,
            this, &BarcodeScannerView::isbnScanned);
    connect(m_controller, &BarcodeScannerController::cameraUnavailable,
            this, &BarcodeScannerView::cameraUnavailable);
}

BarcodeScannerView::~BarcodeScannerView()
{
    stopCapture();
}

void BarcodeScannerView::startCapture()
{
    m_controller->startCapture();
}

void BarcodeScannerView::stopCapture()
{
    m_controller->stopCapture();
}

void BarcodeScannerView::updateFrame(const QImage &image)
{
    m_currentFrame = image;
    update();
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
