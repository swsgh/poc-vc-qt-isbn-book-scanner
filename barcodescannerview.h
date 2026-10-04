#ifndef BARCODESCANNERVIEW_H
#define BARCODESCANNERVIEW_H

#include <QOpenGLWidget>
#include <QImage>
#include <memory>

class QCamera;
class QMediaCaptureSession;
class QVideoSink;
class QVideoFrame;

// CHANGED: Inherit from QOpenGLWidget to enforce GPU-accelerated rendering on Android
class BarcodeScannerView : public QOpenGLWidget
{
    Q_OBJECT
public:
    explicit BarcodeScannerView(QWidget *parent = nullptr);
    ~BarcodeScannerView() override;

    void startCapture();
    void stopCapture();

signals:
    void isbnScanned(const QString &isbn);
    void cameraUnavailable(const QString &message);

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void processVideoFrame(const QVideoFrame &frame);

private:
    std::unique_ptr<QCamera> m_camera;
    std::unique_ptr<QMediaCaptureSession> m_captureSession;
    std::unique_ptr<QVideoSink> m_videoSink;

    QImage m_currentFrame;
    bool m_isProcessingFrame;
};

#endif // BARCODESCANNERVIEW_H
