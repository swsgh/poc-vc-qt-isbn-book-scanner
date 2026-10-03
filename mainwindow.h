#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <memory>

// Qt 6 Multimedia & Widget forward declarations
class QCamera;
class QMediaCaptureSession;
class QVideoWidget;
class QLabel;
class QVBoxLayout;
class QVideoFrame;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    // Slot connected to the QVideoSink to intercept camera frames
    void processVideoFrame(const QVideoFrame &frame);

private:
    // UI Layout Elements
    QVideoWidget* m_videoWidget;
    QLabel* m_isbnLabel;

    // Qt 6 Multimedia Pipeline Elements
    std::unique_ptr<QCamera> m_camera;
    std::unique_ptr<QMediaCaptureSession> m_captureSession;

    // Core processing guard flag
    bool m_isProcessingFrame;
};

#endif // MAINWINDOW_H
