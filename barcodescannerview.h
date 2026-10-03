#ifndef BARCODESCANNERVIEW_H
#define BARCODESCANNERVIEW_H

#include <QWidget>
#include <QImage>
#include <memory>

class QCamera;
class QMediaCaptureSession;
class QVideoSink;
class QVideoFrame;

class BarcodeScannerView : public QWidget
{
    Q_OBJECT
public:
    explicit BarcodeScannerView(QWidget *parent = nullptr);
    ~BarcodeScannerView() override;

    void startCapture();
    void stopCapture();

signals:
    void isbnScanned(const QString &isbn);

protected:
    // We paint the camera frames and the laser overlay manually in the CPU pipeline
    void paintEvent(QPaintEvent *event) override;

private slots:
    void processVideoFrame(const QVideoFrame &frame);

private:
    std::unique_ptr<QCamera> m_camera;
    std::unique_ptr<QMediaCaptureSession> m_captureSession;
    std::unique_ptr<QVideoSink> m_videoSink; // Handles frame routing

    QImage m_currentFrame; // Locally caches the camera frame to draw
    bool m_isProcessingFrame;
};

#endif // BARCODESCANNERVIEW_H
