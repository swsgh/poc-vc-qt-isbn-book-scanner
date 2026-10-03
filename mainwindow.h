#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class BarcodeScannerView;
class BookMetadataProvider;
class BookDatabaseManager;
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
    void handleDatabaseConfirmation(const QString &isbn);

private:
    BarcodeScannerView* m_scannerView;
    BookMetadataProvider* m_metadataProvider;
    BookDatabaseManager* m_dbManager;
    QLabel* m_isbnLabel;
};

#endif // MAINWINDOW_H
