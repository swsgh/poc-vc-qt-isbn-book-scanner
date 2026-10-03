#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "bookinfo.h" // Ensure BookInfo structure is fully compiled for the slot parameter

class BarcodeScannerView;
class BookMetadataProvider;
class BookDatabaseManager;
class BookshelfWidget;
class QLabel;
class QWidget; // NEW: Forward declare QWidget for the sidebar pointer

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

    // Slot to update sidebar when a bookshelf book is clicked or a new book is scanned
    void updateDetailsSidebar(const BookInfo &info);

    // NEW: Slot triggered by the "✕" button to completely collapse the panel
    void closeDetailsSidebar();

private:
    BarcodeScannerView* m_scannerView;
    BookMetadataProvider* m_metadataProvider;
    BookDatabaseManager* m_dbManager;
    BookshelfWidget* m_bookshelfWidget;
    QLabel* m_isbnLabel;

    // NEW & UPDATED SIDEBAR MEMBERS
    QWidget* m_sidebarWidget;   // NEW: Primary container widget used to toggle panel visibility
    QLabel* m_detailTitleLabel;
    QLabel* m_detailAuthorLabel;
    QLabel* m_detailIsbnLabel;
};

#endif // MAINWINDOW_H
