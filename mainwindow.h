#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class BarcodeScannerView;
class BookMetadataProvider;
struct BookInfo;
class QLabel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void updateStatusLabel(const QString &text, bool isError);
    void displayBookDetails(const BookInfo &info);

private:
    BarcodeScannerView* m_scannerView;
    BookMetadataProvider* m_metadataProvider;
    QLabel* m_isbnLabel;
};

#endif // MAINWINDOW_H
