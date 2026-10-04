#ifndef BARCODESCANNERVIEW_H
#define BARCODESCANNERVIEW_H

#include <QWidget>
#include <QImage>

class BarcodeScannerController;

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
    void cameraUnavailable(const QString &message);

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void updateFrame(const QImage &image);

private:
    BarcodeScannerController *m_controller;
    QImage m_currentFrame;
};

#endif // BARCODESCANNERVIEW_H
