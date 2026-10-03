#ifndef BARCODESCANNERVIEW_H
#define BARCODESCANNERVIEW_H

#include <QWidget>
#include <memory>

class QCamera;
class QMediaCaptureSession;
class QVideoWidget;
class QVideoFrame;

// --- Visual Viewfinder Overlay ---
class ViewfinderOverlay : public QWidget
{
    Q_OBJECT
public:
    explicit ViewfinderOverlay(QWidget *parent = nullptr);
protected:
    void paintEvent(QPaintEvent *event) override;
};

// --- Standalone Scanner View Component ---
class BarcodeScannerView : public QWidget
{
    Q_OBJECT
public:
    explicit BarcodeScannerView(QWidget *parent = nullptr);
    ~BarcodeScannerView() override;

    void startCapture();
    void stopCapture();

signals:
    // Emitted whenever a valid ISBN barcode code is decoded
    void isbnScanned(const QString &isbn);

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void processVideoFrame(const QVideoFrame &frame);

private:
    QVideoWidget* m_videoWidget;
    ViewfinderOverlay* m_overlay;

    std::unique_ptr<QCamera> m_camera;
    std::unique_ptr<QMediaCaptureSession> m_captureSession;

    bool m_isProcessingFrame;
};

#endif // BARCODESCANNERVIEW_H
