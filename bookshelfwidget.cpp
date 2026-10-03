#include "bookshelfwidget.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QLabel>
#include <QPixmap>
#include <QPainter>

BookshelfWidget::BookshelfWidget(QWidget *parent) : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);

    QLabel *titleLabel = new QLabel("📚 Your Virtual Bookshelf Archive", this);
    titleLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #2c3e50; padding: 2px;");
    mainLayout->addWidget(titleLabel);

    // Setup horizontal scrolling frame paths
    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFixedHeight(220);
    scrollArea->setStyleSheet("QScrollArea { border: 1px solid #dcdde1; background-color: #f5f6fa; border-radius: 6px; }");

    m_scrollContainer = new QWidget(scrollArea);
    m_shelfLayout = new QHBoxLayout(m_scrollContainer);
    m_shelfLayout->setContentsMargins(10, 10, 10, 10);
    m_shelfLayout->setSpacing(15);
    m_shelfLayout->setAlignment(Qt::AlignLeft);

    m_scrollContainer->setLayout(m_shelfLayout);
    scrollArea->setWidget(m_scrollContainer);
    mainLayout->addWidget(scrollArea);
}

void BookshelfWidget::addBookToShelf(const BookInfo &info, bool prepend)
{
    // 1. DYNAMIC REPLACEMENT CHECK: See if this book is already displayed on the shelf
    QWidget *existingCard = m_scrollContainer->findChild<QWidget*>("card_" + info.isbn);

    if (existingCard) {
        // If the card exists, extract its child elements to update them instantly
        QLabel *coverLabel = existingCard->findChild<QLabel*>("coverLabel");
        QLabel *titleLabel = existingCard->findChild<QLabel*>("titleLabel");
        QLabel *authorLabel = existingCard->findChild<QLabel*>("authorLabel");

        if (coverLabel && titleLabel && authorLabel) {
            // Update the data parameters in place
            titleLabel->setText(info.title);
            authorLabel->setText(info.authors);
            titleLabel->setToolTip(info.title);

            QPixmap coverPixmap;
            if (!info.coverData.isEmpty() && coverPixmap.loadFromData(info.coverData)) {
                coverLabel->setPixmap(coverPixmap.scaled(coverLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
            } else {
                coverLabel->setPixmap(generatePlaceholderCover(info.title));
            }
        }

        // Move the updated card to the very front of the horizontal visual layout line sequence
        if (prepend) {
            m_shelfLayout->removeWidget(existingCard);
            m_shelfLayout->insertWidget(0, existingCard);
        }
        return; // Stop processing right here; do not construct a duplicate card!
    }

    // 2. CONSTRUCT NEW CARD (Only executes if the book wasn't already on the shelf)
    QWidget *bookCard = new QWidget(m_scrollContainer);
    bookCard->setFixedSize(120, 190);
    bookCard->setStyleSheet("QWidget { background: white; border: 1px solid #dcdde1; border-radius: 4px; }");

    // Assign the unique tracking ID search key name property string
    bookCard->setObjectName("card_" + info.isbn);

    QVBoxLayout *cardLayout = new QVBoxLayout(bookCard);
    cardLayout->setContentsMargins(6, 6, 6, 6);
    cardLayout->setSpacing(4);

    QLabel *coverLabel = new QLabel(bookCard);
    coverLabel->setObjectName("coverLabel"); // Object name for tracking lookups
    coverLabel->setFixedSize(108, 130);
    coverLabel->setAlignment(Qt::AlignCenter);

    QPixmap coverPixmap;
    if (!info.coverData.isEmpty() && coverPixmap.loadFromData(info.coverData)) {
        coverLabel->setPixmap(coverPixmap.scaled(coverLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        coverLabel->setPixmap(generatePlaceholderCover(info.title));
    }
    cardLayout->addWidget(coverLabel);

    QLabel *titleLabel = new QLabel(info.title, bookCard);
    titleLabel->setObjectName("titleLabel"); // Object name for tracking lookups
    titleLabel->setStyleSheet("font-size: 11px; font-weight: bold; border: none; background: transparent;");
    titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    titleLabel->setToolTip(info.title);
    cardLayout->addWidget(titleLabel);

    QLabel *authorLabel = new QLabel(info.authors, bookCard);
    authorLabel->setObjectName("authorLabel"); // Object name for tracking lookups
    authorLabel->setStyleSheet("font-size: 10px; color: #7f8c8d; border: none; background: transparent;");
    authorLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    cardLayout->addWidget(authorLabel);

    if (prepend) {
        m_shelfLayout->insertWidget(0, bookCard);
    } else {
        m_shelfLayout->addWidget(bookCard);
    }
}

QPixmap BookshelfWidget::generatePlaceholderCover(const QString &title)
{
    QPixmap pixmap(108, 130);
    pixmap.fill(QColor("#34495e"));

    QPainter painter(&pixmap);
    painter.setPen(Qt::white);
    painter.drawRect(5, 5, 98, 120);

    QFont font = painter.font();
    font.setPointSize(9);
    font.setBold(true);
    painter.setFont(font);

    QRect textRect(10, 15, 88, 100);
    painter.drawText(textRect, Qt::AlignCenter | Qt::TextWordWrap, title.left(25) + (title.length() > 25 ? "..." : ""));

    return pixmap;
}

void BookshelfWidget::clearShelf()
{
    QLayoutItem *item;
    while ((item = m_shelfLayout->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }
}
