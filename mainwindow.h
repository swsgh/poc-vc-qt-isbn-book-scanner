#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "bookinfo.h" // Ensure BookInfo structure is fully compiled for the slot parameter

class BarcodeScannerView;
class BookMetadataProvider;
class BookDatabaseManager;
class BookshelfWidget;
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

    // NEW: Slot to update sidebar when a bookshelf book is clicked or a new book is scanned
    void updateDetailsSidebar(const BookInfo &info);

private:
    BarcodeScannerView* m_scannerView;
    BookMetadataProvider* m_metadataProvider;
    BookDatabaseManager* m_dbManager;
    BookshelfWidget* m_bookshelfWidget;
    QLabel* m_isbnLabel;

    // NEW: UI components tracking individual sidebar text rows
    QLabel* m_detailTitleLabel;
    QLabel* m_detailAuthorLabel;
    QLabel* m_detailIsbnLabel;
};

#endif // MAINWINDOW_H
