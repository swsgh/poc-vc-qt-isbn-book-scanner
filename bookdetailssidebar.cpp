#include "bookdetailssidebar.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QPixmap>
#include <QFont>

BookDetailsSidebar::BookDetailsSidebar(QWidget *parent) : QWidget(parent)
{
    setMinimumWidth(260);
    setMaximumWidth(320);

    auto *sidebarLayout = new QVBoxLayout(this);
    sidebarLayout->setContentsMargins(5, 0, 5, 0);

    auto *container = new QFrame(this);
    container->setStyleSheet(
        "QFrame { border: 1px solid rgba(255, 255, 255, 0.1); border-radius: 8px; }");
    auto *containerLayout = new QVBoxLayout(container);
    containerLayout->setContentsMargins(15, 15, 15, 15);

    auto *headerRowLayout = new QHBoxLayout;
    auto *sidebarHeader = new QLabel("Book Analytics", container);
    sidebarHeader->setFont(QFont("Segoe UI", 12, QFont::Bold));
    sidebarHeader->setStyleSheet("border: none;");

    auto *closeSidebarButton = new QPushButton("✕", container);
    closeSidebarButton->setFixedSize(24, 24);
    closeSidebarButton->setStyleSheet(
        "QPushButton { border-radius: 12px; padding: 0px; font-weight: bold; "
        "border: 1px solid rgba(255, 255, 255, 0.2); }"
        "QPushButton:hover { background-color: rgba(255, 85, 85, 0.2); color: #ff5555; }"
        );
    closeSidebarButton->setCursor(Qt::PointingHandCursor);

    headerRowLayout->addWidget(sidebarHeader, 1, Qt::AlignLeft);
    headerRowLayout->addWidget(closeSidebarButton, 0, Qt::AlignRight);
    containerLayout->addLayout(headerRowLayout);

    m_coverLabel = new QLabel(container);
    m_coverLabel->setFixedSize(160, 220);
    m_coverLabel->setAlignment(Qt::AlignCenter);
    m_coverLabel->setStyleSheet(
        "border: 1px solid rgba(255, 255, 255, 0.1); border-radius: 6px;");
    containerLayout->addWidget(m_coverLabel, 0, Qt::AlignCenter);

    m_detailTitleLabel = new QLabel("Select a book to inspect details", container);
    m_detailTitleLabel->setFont(QFont("Segoe UI", 11, QFont::Bold));
    m_detailTitleLabel->setWordWrap(true);
    m_detailTitleLabel->setAlignment(Qt::AlignCenter);
    m_detailTitleLabel->setStyleSheet("border: none; padding-top: 10px;");

    m_detailAuthorLabel = new QLabel("", container);
    m_detailAuthorLabel->setWordWrap(true);
    m_detailAuthorLabel->setAlignment(Qt::AlignCenter);
    m_detailAuthorLabel->setStyleSheet("border: none; font-style: italic; color: #cccccc;");

    m_detailIsbnLabel = new QLabel("", container);
    m_detailIsbnLabel->setAlignment(Qt::AlignCenter);
    m_detailIsbnLabel->setStyleSheet(
        "border: none; font-family: monospace; font-size: 12px; padding-top: 5px;");

    containerLayout->addWidget(m_detailTitleLabel);
    containerLayout->addWidget(m_detailAuthorLabel);
    containerLayout->addWidget(m_detailIsbnLabel);

    containerLayout->addStretch();

    m_deleteButton = new QPushButton("🗑️ Remove from Shelf", container);
    m_deleteButton->setMinimumHeight(40);
    m_deleteButton->setCursor(Qt::PointingHandCursor);
    m_deleteButton->setStyleSheet(
        "QPushButton { border-radius: 6px; padding: 10px; font-weight: bold; "
        "border: 1px solid #3498db; }"
        "QPushButton:hover { background-color: #3498db; color: #ffffff; }"
        );
    containerLayout->addWidget(m_deleteButton);
    sidebarLayout->addWidget(container);

    connect(closeSidebarButton, &QPushButton::clicked, this, &BookDetailsSidebar::closeSidebar);

    connect(m_deleteButton, &QPushButton::clicked, this, [this]() {
        if (!m_currentIsbn.isEmpty()) {
            emit deleteBookRequested(m_currentIsbn);
        }
    });
}

void BookDetailsSidebar::setCoverForBook(const BookInfo &info)
{
    QPixmap coverPixmap;
    if (!info.coverData.isEmpty() && coverPixmap.loadFromData(info.coverData)) {
        m_coverLabel->setPixmap(coverPixmap.scaled(m_coverLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        return;
    }

    QPixmap placeholder(160, 220);
    placeholder.fill(Qt::darkGray);
    m_coverLabel->setPixmap(placeholder);
}

void BookDetailsSidebar::clearDetailsText()
{
    m_detailTitleLabel->setText("Select a book to inspect details");
    m_detailAuthorLabel->clear();
    m_detailIsbnLabel->clear();
}

void BookDetailsSidebar::updateDetails(const BookInfo &info)
{
    setVisible(true);
    m_currentIsbn = info.isbn;
    setCoverForBook(info);

    m_detailTitleLabel->setText(info.title);
    m_detailAuthorLabel->setText(QString("by %1").arg(info.authors.isEmpty() ? "Unknown" : info.authors));
    m_detailIsbnLabel->setText("ISBN: " + info.isbn);
}

void BookDetailsSidebar::closeSidebar()
{
    setVisible(false);
    m_currentIsbn.clear();
    m_coverLabel->clear();
    clearDetailsText();
}
