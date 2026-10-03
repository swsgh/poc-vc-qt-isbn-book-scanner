#include "bookshelfwidget.h"
#include <QGridLayout>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QLabel>
#include <QPixmap>
#include <QPainter>
#include <QMouseEvent> // NEW: Required to capture mouse click events

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
        // Cache the newly updated BookInfo into the existing card property fields
        existingCard->setProperty("bookData", QVariant::fromValue(info));

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
    // Added a visual hover/pointer effect so users intuitively know it is clickable
    bookCard->setStyleSheet("QWidget { background: white; border: 1px solid #dcdde1; border-radius: 6px; }"
                            "QWidget:hover { border: 1px solid #3498db; background: #fafafa; }");
    bookCard->setObjectName("card_" + info.isbn);
    bookCard->setCursor(Qt::PointingHandCursor);

    // Dynamic Binding: Inject the custom BookInfo struct data directly into the Qt Object metadata layer
    bookCard->setProperty("bookData", QVariant::fromValue(info));
    // Install the event filter directly on the card wrapper component container
    bookCard->installEventFilter(this);

    QVBoxLayout *cardLayout = new QVBoxLayout(bookCard);
    cardLayout->setContentsMargins(6, 6, 6, 6);
    cardLayout->setSpacing(4);

    QLabel *coverLabel = new QLabel(bookCard);
    coverLabel->setObjectName("coverLabel");
    coverLabel->setFixedSize(108, 130);
    coverLabel->setAlignment(Qt::AlignCenter);

    // Crucial: Child labels block mouse events by default; pass clicks through to the bookCard parent
    coverLabel->setAttribute(Qt::WA_TransparentForMouseEvents);

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
    titleLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    cardLayout->addWidget(titleLabel);

    QLabel *authorLabel = new QLabel(info.authors, bookCard);
    authorLabel->setObjectName("authorLabel");
    authorLabel->setStyleSheet("font-size: 10px; color: #7f8c8d; border: none; background: transparent;");
    authorLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    authorLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    cardLayout->addWidget(authorLabel);

    // Save references to track card arrays for line wrapping calculations
    if (prepend) {
        m_bookCards.prepend(bookCard);
    } else {
        m_bookCards.append(bookCard);
    }

    rearrangeGrid();
}

// NEW: Event Filter logic engine that catches click releases targeting any active book card
bool BookshelfWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonRelease) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            QWidget *clickedCard = qobject_cast<QWidget*>(watched);
            if (clickedCard) {
                QVariant prop = clickedCard->property("bookData");
                if (prop.isValid() && prop.canConvert<BookInfo>()) {
                    BookInfo selectedBook = prop.value<BookInfo>();
                    emit bookSelected(selectedBook); // Broadcast the event to MainWindow's sidebar!
                    return true; // Mark event handled cleanly
                }
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

// Dynamic grid allocation math calculation engine loop
void BookshelfWidget::rearrangeGrid()
{
    QLayoutItem *child;
    while ((child = m_shelfGridLayout->takeAt(0)) != nullptr) {
        delete child;
    }

    int shelfWidth = width() - 40;
    int cardWidth = 120 + 20;

    int maxColumns = qMax(1, shelfWidth / cardWidth);

    int row = 0;
    int col = 0;

    for (QWidget* card : m_bookCards) {
        m_shelfGridLayout->addWidget(card, row, col);
        col++;
        if (col >= maxColumns) {
            col = 0;
            row++;
        }
    }
}

void BookshelfWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    rearrangeGrid();
}

QPixmap BookshelfWidget::generatePlaceholderCover(const QString &title)
{
    QPixmap pixmap(108, 130);
    pixmap.fill(QColor("#34495e"));

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::white);

    painter.drawRect(5, 5, 98, 120);

    QFont font = painter.font();
    font.setPointSize(9);
    font.setBold(true);
    painter.setFont(font);

    QRect textRect(10, 15, 88, 100);
    painter.drawText(textRect, Qt::AlignCenter | Qt::TextWordWrap,
                     title.left(25) + (title.length() > 25 ? "..." : ""));

    return pixmap;
}

void BookshelfWidget::removeBookFromShelf(const QString &isbn)
{
    // Find the specific book card widget container by its object name format
    QWidget *cardToErase = m_scrollContainer->findChild<QWidget*>("card_" + isbn);

    if (cardToErase) {
        // Remove it from our structural list tracking array
        m_bookCards.removeOne(cardToErase);

        // Safely schedule the widget object and its child labels for deletion
        cardToErase->deleteLater();

        // Force layout engine recalculation and adjust wrapping columns
        rearrangeGrid();
    }
}

void BookshelfWidget::clearShelf()
{
    for (QWidget* card : m_bookCards) {
        card->deleteLater();
    }
    m_bookCards.clear();
    rearrangeGrid();
}
