#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "bookinfo.h"

class BarcodeScannerView;
class BookMetadataProvider;
class BookDatabaseManager;
class BookshelfWidget;
class BookDetailsSidebar; // Forward declare the new class
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
    void removeBookRecord(const QString &isbn);

private:
    BarcodeScannerView* m_scannerView;
    BookMetadataProvider* m_metadataProvider;
    BookDatabaseManager* m_dbManager;
    BookshelfWidget* m_bookshelfWidget;
    QLabel* m_isbnLabel;

    BookDetailsSidebar* m_detailsSidebar;
};

#endif // MAINWINDOW_H
