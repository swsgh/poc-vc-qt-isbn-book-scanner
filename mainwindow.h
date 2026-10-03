#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class BarcodeScannerView;
class QLabel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private slots:
    // Slot acting upon target signal events emitted by our standalone component
    void handleIsbnScanned(const QString &isbn);

private:
    BarcodeScannerView* m_scannerView;
    QLabel* m_isbnLabel;
};

#endif // MAINWINDOW_H
