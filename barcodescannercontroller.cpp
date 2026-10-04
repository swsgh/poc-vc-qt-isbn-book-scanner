#include "barcodescannercontroller.h"

#include <QCamera>
#include <QMediaCaptureSession>
#include <QMediaDevices>
#include <QMetaObject>
#include <QVideoFrame>
#include <QVideoSink>

#if defined(Q_OS_ANDROID)
#include <QCoreApplication>
#include <QPermissions>
#endif

#include <BarcodeFormat.h>
#include <ReadBarcode.h>

BarcodeScannerController::BarcodeScannerController(QObject *parent)
    : QObject(parent),
      m_camera(std::make_unique<QCamera>(QMediaDevices::defaultVideoInput(), this)),
      m_captureSession(std::make_unique<QMediaCaptureSession>(this)),
      m_videoSink(std::make_unique<QVideoSink>(this))
{
#if defined(Q_OS_ANDROID)
    if (m_camera->isFocusModeSupported(QCamera::FocusModeAutoNear)) {
        m_camera->setFocusMode(QCamera::FocusModeAutoNear);
    } else if (m_camera->isFocusModeSupported(QCamera::FocusModeAuto)) {
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
            this, &BarcodeScannerController::processVideoFrame);
}

BarcodeScannerController::~BarcodeScannerController()
{
    stopCapture();
}

void BarcodeScannerController::startCapture()
{
#if defined(Q_OS_ANDROID)
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
    m_camera->start();
#endif
}

void BarcodeScannerController::stopCapture()
{
    m_camera->stop();
}

void BarcodeScannerController::processVideoFrame(const QVideoFrame &frame)
{
    if (m_isProcessingFrame || !frame.isValid()) {
        return;
    }
    m_isProcessingFrame = true;

    QVideoFrame cloneFrame(frame);
    QImage image;

#if defined(Q_OS_ANDROID)
    if (cloneFrame.handleType() != QVideoFrame::NoHandle) {
        image = QImage(cloneFrame.size(), QImage::Format_RGB888);
        image.fill(Qt::black);

        QPainter painter(&image);
        QVideoFrame::PaintOptions options;
        cloneFrame.paint(&painter, QRect(0, 0, image.width(), image.height()), options);
        painter.end();
    } else if (cloneFrame.map(QVideoFrame::ReadOnly)) {
        image = cloneFrame.toImage().convertToFormat(QImage::Format_RGB888);
        cloneFrame.unmap();
    }
#else
    if (cloneFrame.map(QVideoFrame::ReadOnly)) {
        image = cloneFrame.toImage().convertToFormat(QImage::Format_RGB888);
        cloneFrame.unmap();
    }
#endif

    if (!image.isNull()) {
        QMetaObject::invokeMethod(this, [this, image]() {
            emit frameReady(image);
        }, Qt::QueuedConnection);

        const int cropWidth = static_cast<int>(image.width() * 0.7);
        const int cropHeight = static_cast<int>(image.height() * 0.25);
        const int cropX = (image.width() - cropWidth) / 2;
        const int cropY = (image.height() - cropHeight) / 2;
        const QImage croppedZone = image.copy(QRect(cropX, cropY, cropWidth, cropHeight));

        if (!croppedZone.isNull()) {
            ZXing::ImageView imageView(croppedZone.bits(), croppedZone.width(),
                                       croppedZone.height(), ZXing::ImageFormat::RGB);
            ZXing::ReaderOptions options;
            options.setFormats(ZXing::BarcodeFormat::EAN13);

            const ZXing::Result result = ZXing::ReadBarcode(imageView, options);
            if (result.isValid()) {
                const QString scannedText = QString::fromStdString(result.text());
                QMetaObject::invokeMethod(this, [this, scannedText]() {
                    emit isbnScanned(scannedText);
                }, Qt::QueuedConnection);
            }
        }
    }

    m_isProcessingFrame = false;
}