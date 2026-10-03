#include "bookdetailssidebar.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

BookDetailsSidebar::BookDetailsSidebar(QWidget *parent) : QWidget(parent)
{
    setFixedWidth(280); // Locks sidebar to a clean, readable width
    setStyleSheet("background-color: #f8f9fa; border: 1px solid #dee2e6; border-radius: 4px;");

    QVBoxLayout *sidebarLayout = new QVBoxLayout(this);

    // --- Interactive Top Header Row containing the Close Button ---
    QHBoxLayout *headerRowLayout = new QHBoxLayout();
    QLabel *sidebarHeader = new QLabel("<b>📚 BOOK DETAILS</b>", this);
    sidebarHeader->setStyleSheet("font-size: 13px; color: #7f8c8d; letter-spacing: 1px;");

    QPushButton *closeSidebarButton = new QPushButton("✕", this);
    closeSidebarButton->setFixedSize(24, 24);
    closeSidebarButton->setStyleSheet(
        "QPushButton { border: none; background: transparent; font-size: 14px; color: #95a5a6; font-weight: bold; }"
        "QPushButton:hover { color: #e74c3c; background-color: #f2f3f4; border-radius: 12px; }"
        );
    closeSidebarButton->setCursor(Qt::PointingHandCursor);

    headerRowLayout->addWidget(sidebarHeader, 1, Qt::AlignLeft);
    headerRowLayout->addWidget(closeSidebarButton, 0, Qt::AlignRight);
    sidebarLayout->addLayout(headerRowLayout);

    // --- Detail fields tracking text items ---
    m_detailTitleLabel = new QLabel("Select a book from your shelf...", this);
    m_detailTitleLabel->setWordWrap(true);
    m_detailTitleLabel->setStyleSheet("font-size: 14px; color: #2c3e50; font-weight: 500;");

    m_detailAuthorLabel = new QLabel("", this);
    m_detailAuthorLabel->setWordWrap(true);
    m_detailAuthorLabel->setStyleSheet("font-size: 13px; color: #566573;");

    m_detailIsbnLabel = new QLabel("", this);
    m_detailIsbnLabel->setStyleSheet("font-size: 12px; color: #95a5a6; font-family: monospace;");

    sidebarLayout->addWidget(m_detailTitleLabel);
    sidebarLayout->addWidget(m_detailAuthorLabel);
    sidebarLayout->addWidget(m_detailIsbnLabel);
    sidebarLayout->addStretch(); // Pushes all layout text elements to the top

    // Connect the Close Button directly to our localized hiding slot engine
    connect(closeSidebarButton, &QPushButton::clicked, this, &BookDetailsSidebar::closeSidebar);
}

void BookDetailsSidebar::updateDetails(const BookInfo &info)
{
    setVisible(true); // Reveal panel container layout bounds instantly
    m_detailTitleLabel->setText(QString("<b>Title:</b><br>%1").arg(info.title));
    m_detailAuthorLabel->setText(QString("<b>Author(s):</b><br>%1").arg(info.authors.isEmpty() ? "Unknown" : info.authors));
    m_detailIsbnLabel->setText(QString("<b>ISBN:</b> %1<br><small>Source: %2</small>").arg(info.isbn).arg(info.engineSource));
}

void BookDetailsSidebar::closeSidebar()
{
    setVisible(false); // Cleanly collapse the horizontal layout allocation footprint
    m_detailTitleLabel->setText("Select a book from your shelf...");
    m_detailAuthorLabel->clear();
    m_detailIsbnLabel->clear();
}
