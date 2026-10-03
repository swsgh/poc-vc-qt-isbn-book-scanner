#include "bookdetailssidebar.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QPixmap>
#include <QPainter> // Required to draw the local placeholder backup

BookDetailsSidebar::BookDetailsSidebar(QWidget *parent) : QWidget(parent)
{
    setFixedWidth(280);
    setStyleSheet("background-color: #1e1e1e; border: 1px solid #2d2d2d; border-radius: 4px;");

    QVBoxLayout *sidebarLayout = new QVBoxLayout(this);
    sidebarLayout->setSpacing(12);

    QHBoxLayout *headerRowLayout = new QHBoxLayout();
    QLabel *sidebarHeader = new QLabel("<b>📚 BOOK DETAILS</b>", this);
    sidebarHeader->setStyleSheet("font-size: 13px; color: #aaaaaa; letter-spacing: 1px;"); // Dim silver title

    QPushButton *closeSidebarButton = new QPushButton("✕", this);
    closeSidebarButton->setFixedSize(24, 24);
    closeSidebarButton->setStyleSheet(
        "QPushButton { border: none; background: transparent; font-size: 14px; color: #777777; font-weight: bold; }"
        "QPushButton:hover { color: #ff6b6b; background-color: #2a2a2a; border-radius: 12px; }"
        );
    closeSidebarButton->setCursor(Qt::PointingHandCursor);

    headerRowLayout->addWidget(sidebarHeader, 1, Qt::AlignLeft);
    headerRowLayout->addWidget(closeSidebarButton, 0, Qt::AlignRight);
    sidebarLayout->addLayout(headerRowLayout);

    m_coverLabel = new QLabel(this);
    m_coverLabel->setFixedSize(140, 180);
    m_coverLabel->setAlignment(Qt::AlignCenter);
    m_coverLabel->setStyleSheet("border: 1px solid #2d2d2d; background: #121212; border-radius: 4px;"); // Deep image tray
    sidebarLayout->addWidget(m_coverLabel, 0, Qt::AlignHCenter);

    m_detailTitleLabel = new QLabel("Select a book from your shelf...", this);
    m_detailTitleLabel->setWordWrap(true);
    m_detailTitleLabel->setStyleSheet("font-size: 14px; color: #ffffff; font-weight: 500;"); // Crisp white

    m_detailAuthorLabel = new QLabel("", this);
    m_detailAuthorLabel->setWordWrap(true);
    m_detailAuthorLabel->setStyleSheet("font-size: 13px; color: #cccccc;"); // Mid gray

    m_detailIsbnLabel = new QLabel("", this);
    m_detailIsbnLabel->setStyleSheet("font-size: 12px; color: #888888; font-family: monospace;"); // Muted identifier text

    sidebarLayout->addWidget(m_detailTitleLabel);
    sidebarLayout->addWidget(m_detailAuthorLabel);
    sidebarLayout->addWidget(m_detailIsbnLabel);

    sidebarLayout->addStretch();

    m_deleteButton = new QPushButton("🗑️ Remove From Shelf", this);
    m_deleteButton->setMinimumHeight(35);
    m_deleteButton->setCursor(Qt::PointingHandCursor);
    // Flat dark design with neon red frame lines matching dark modern layout themes
    m_deleteButton->setStyleSheet(
        "QPushButton { background-color: #1e1e1e; border: 1px solid #ff4d4d; color: #ff4d4d; font-weight: bold; border-radius: 4px; font-size: 12px; }"
        "QPushButton:hover { background-color: #ff4d4d; color: #ffffff; }"
        "QPushButton:pressed { background-color: #cc3333; border-color: #cc3333; }"
        );
    sidebarLayout->addWidget(m_deleteButton);

    connect(closeSidebarButton, &QPushButton::clicked, this, &BookDetailsSidebar::closeSidebar);

    connect(m_deleteButton, &QPushButton::clicked, this, [this]() {
        if (!m_currentIsbn.isEmpty()) {
            emit deleteBookRequested(m_currentIsbn);
        }
    });
}

void BookDetailsSidebar::updateDetails(const BookInfo &info)
{
    setVisible(true);
    m_currentIsbn = info.isbn;

    // --- Parse Cover Image Bytes ---
    QPixmap coverPixmap;
    if (!info.coverData.isEmpty() && coverPixmap.loadFromData(info.coverData)) {
        m_coverLabel->setPixmap(coverPixmap.scaled(m_coverLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        // Fallback layout generation tool directly mimicking BookshelfWidget placeholder logic
        QPixmap placeholder(140, 180);
        placeholder.fill(QColor("#34495e"));
        QPainter painter(&placeholder);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::white);
        painter.drawRect(5, 5, 130, 170);

        QFont font = painter.font();
        font.setPointSize(9);
        font.setBold(true);
        painter.setFont(font);

        QRect textRect(10, 20, 120, 140);
        painter.drawText(textRect, Qt::AlignCenter | Qt::TextWordWrap,
                         info.title.left(30) + (info.title.length() > 30 ? "..." : ""));
        painter.end();

        m_coverLabel->setPixmap(placeholder);
    }

    m_detailTitleLabel->setText(QString("<b>Title:</b><br>%1").arg(info.title));
    m_detailAuthorLabel->setText(QString("<b>Author(s):</b><br>%1").arg(info.authors.isEmpty() ? "Unknown" : info.authors));
    m_detailIsbnLabel->setText(QString("<b>ISBN:</b> %1<br><small>Source: %2</small>").arg(info.isbn).arg(info.engineSource));
}

void BookDetailsSidebar::closeSidebar()
{
    setVisible(false);
    m_currentIsbn.clear();
    m_coverLabel->clear(); // Drop old pixel memory buffers cleanly
    m_detailTitleLabel->setText("Select a book from your shelf...");
    m_detailAuthorLabel->clear();
    m_detailIsbnLabel->clear();
}
