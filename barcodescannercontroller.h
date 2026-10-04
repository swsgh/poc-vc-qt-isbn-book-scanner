#ifndef BARCODESCANNERCONTROLLER_H
#define BARCODESCANNERCONTROLLER_H

#include <QObject>
#include <QImage>
#include <memory>

class QCamera;
class QMediaCaptureSession;
class QVideoFrame;
class QVideoSink;

class BarcodeScannerController : public QObject
{
    Q_OBJECT

public:
    explicit BarcodeScannerController(QObject *parent = nullptr);
    ~BarcodeScannerController() override;

    void startCapture();
    void stopCapture();

signals:
    void frameReady(const QImage &image);
    void isbnScanned(const QString &isbn);
    void cameraUnavailable(const QString &message);

private slots:
    void processVideoFrame(const QVideoFrame &frame);

private:
    std::unique_ptr<QCamera> m_camera;
    std::unique_ptr<QMediaCaptureSession> m_captureSession;
    std::unique_ptr<QVideoSink> m_videoSink;
    bool m_isProcessingFrame = false;
};

#endif // BARCODESCANNERCONTROLLER_H