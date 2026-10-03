#include "bookshelfwidget.h"
#include <QGridLayout>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QLabel>
#include <QPixmap>
#include <QPainter>

BookshelfWidget::BookshelfWidget(QWidget *parent) : QWidget(parent)
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);

    QLabel *titleLabel = new QLabel("📚 Your Library Bookshelf Archive", this);
    titleLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #2c3e50; padding: 2px;");
    mainLayout->addWidget(titleLabel);

    // --- UPDATED CONFIGURATION: VERTICAL SCROLL AREA ---
    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // Disable side-scrolling
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);     // Scroll vertically
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet("QScrollArea { border: 1px solid #dcdde1; background-color: #f5f6fa; border-radius: 6px; }");

    m_scrollContainer = new QWidget(scrollArea);
    m_shelfGridLayout = new QGridLayout(m_scrollContainer);
    m_shelfGridLayout->setContentsMargins(15, 15, 15, 15);
    m_shelfGridLayout->setSpacing(20); // Balanced space between grid columns and rows
    m_shelfGridLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    m_scrollContainer->setLayout(m_shelfGridLayout);
    scrollArea->setWidget(m_scrollContainer);
    mainLayout->addWidget(scrollArea, 1); // Expand to fill parent bounds layout footprints
}

void BookshelfWidget::addBookToShelf(const BookInfo &info, bool prepend)
{
    QWidget *existingCard = m_scrollContainer->findChild<QWidget*>("card_" + info.isbn);

    if (existingCard) {
        QLabel *coverLabel = existingCard->findChild<QLabel*>("coverLabel");
        QLabel *titleLabel = existingCard->findChild<QLabel*>("titleLabel");
        QLabel *authorLabel = existingCard->findChild<QLabel*>("authorLabel");

        if (coverLabel && titleLabel && authorLabel) {
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

        if (prepend) {
            m_bookCards.removeOne(existingCard);
            m_bookCards.prepend(existingCard);
            rearrangeGrid();
        }
        return;
    }

    // Build standard card elements
    QWidget *bookCard = new QWidget(m_scrollContainer);
    bookCard->setFixedSize(120, 190);
    bookCard->setStyleSheet("QWidget { background: white; border: 1px solid #dcdde1; border-radius: 6px; }");
    bookCard->setObjectName("card_" + info.isbn);

    QVBoxLayout *cardLayout = new QVBoxLayout(bookCard);
    cardLayout->setContentsMargins(6, 6, 6, 6);
    cardLayout->setSpacing(4);

    QLabel *coverLabel = new QLabel(bookCard);
    coverLabel->setObjectName("coverLabel");
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
    titleLabel->setObjectName("titleLabel");
    titleLabel->setStyleSheet("font-size: 11px; font-weight: bold; border: none; background: transparent;");
    titleLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    titleLabel->setToolTip(info.title);
    cardLayout->addWidget(titleLabel);

    QLabel *authorLabel = new QLabel(info.authors, bookCard);
    authorLabel->setObjectName("authorLabel");
    authorLabel->setStyleSheet("font-size: 10px; color: #7f8c8d; border: none; background: transparent;");
    authorLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    cardLayout->addWidget(authorLabel);

    // Save references to track card arrays for line wrapping calculations
    if (prepend) {
        m_bookCards.prepend(bookCard);
    } else {
        m_bookCards.append(bookCard);
    }

    rearrangeGrid();
}

// Dynamic grid allocation math calculation engine loop
void BookshelfWidget::rearrangeGrid()
{
    // Clear out old positional links out of the grid layout map array
    QLayoutItem *child;
    while ((child = m_shelfGridLayout->takeAt(0)) != nullptr) {
        // We only detach the links, do NOT delete the underlying card objects!
        delete child;
    }

    int shelfWidth = width() - 40; // account for layout boundary padding margins
    int cardWidth = 120 + 20;      // card size dimension + layout item spacing widths

    // Determine how many items fit horizontally on one row safely before wrapping
    int maxColumns = qMax(1, shelfWidth / cardWidth);

    int row = 0;
    int col = 0;

    for (QWidget* card : m_bookCards) {
        m_shelfGridLayout->addWidget(card, row, col);
        col++;
        if (col >= maxColumns) {
            col = 0;
            row++; // Wrap line down to the next row grid layer!
        }
    }
}

void BookshelfWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    rearrangeGrid(); // Force row recalculation when window stretches or shrinks
}

QPixmap BookshelfWidget::generatePlaceholderCover(const QString &title)
{
    QPixmap pixmap(108, 130);
    pixmap.fill(QColor("#34495e")); // Dark elegant book spine color

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::white);

    // Draw a decorative framing inner border line box
    painter.drawRect(5, 5, 98, 120);

    QFont font = painter.font();
    font.setPointSize(9);
    font.setBold(true);
    painter.setFont(font);

    // Securely wrap long titles so they don't leak out of the card bounds
    QRect textRect(10, 15, 88, 100);
    painter.drawText(textRect, Qt::AlignCenter | Qt::TextWordWrap,
                     title.left(25) + (title.length() > 25 ? "..." : ""));

    return pixmap;
}

void BookshelfWidget::clearShelf()
{
    for (QWidget* card : m_bookCards) {
        card->deleteLater();
    }
    m_bookCards.clear();
    rearrangeGrid();
}
