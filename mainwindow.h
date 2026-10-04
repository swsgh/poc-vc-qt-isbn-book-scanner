#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "bookinfo.h"

class BarcodeScannerView;
class BookMetadataProvider;
class BookDatabaseManager;
class BookshelfWidget;
class BookDetailsSidebar;
class BookSyncManager;
class QLabel;
class QLineEdit;

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
    void onSearchTextChanged(const QString &text);

private:
    BarcodeScannerView* m_scannerView;
    BookMetadataProvider* m_metadataProvider;
    BookDatabaseManager* m_dbManager;
    BookshelfWidget* m_bookshelfWidget;
    BookDetailsSidebar* m_detailsSidebar;
    BookSyncManager* m_syncManager;
    QLabel* m_isbnLabel;
};

#endif // MAINWINDOW_H
