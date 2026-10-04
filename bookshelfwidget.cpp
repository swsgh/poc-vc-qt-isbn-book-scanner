#include "bookshelfwidget.h"
#include "covercache.h"
#include <QGridLayout>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QLabel>
#include <QPixmap>
#include <QPainter>
#include <QPalette>
#include <QMouseEvent> // NEW: Required to capture mouse click events

BookshelfWidget::BookshelfWidget(QWidget *parent) : QWidget(parent)
{
    const QPalette systemPalette = palette();
    const QString textColor = systemPalette.color(QPalette::WindowText).name();
    const QString baseColor = systemPalette.color(QPalette::Base).name();
    const QString borderColor = systemPalette.color(QPalette::Mid).name();

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(5, 5, 5, 5);

    m_titleLabel = new QLabel("Saved Books Shelf Grid", this);
    m_titleLabel->setStyleSheet(
        QString("font-size: 14px; font-weight: bold; color: %1; padding: 2px;")
            .arg(textColor));
    mainLayout->addWidget(m_titleLabel);

    // --- UPDATED CONFIGURATION: VERTICAL SCROLL AREA ---
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff); // Disable side-scrolling
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);     // Scroll vertically
    m_scrollArea->setWidgetResizable(true);

    m_scrollContainer = new QWidget(m_scrollArea);
    m_shelfGridLayout = new QGridLayout(m_scrollContainer);
    m_shelfGridLayout->setContentsMargins(15, 15, 15, 15);
    m_shelfGridLayout->setSpacing(20); // Balanced space between grid columns and rows
    m_shelfGridLayout->setAlignment(Qt::AlignLeft | Qt::AlignTop);

    m_scrollContainer->setLayout(m_shelfGridLayout);
    m_scrollArea->setWidget(m_scrollContainer);
    mainLayout->addWidget(m_scrollArea, 1); // Expand to fill parent bounds layout footprints
}

void BookshelfWidget::applyPalette(const QPalette &palette)
{
    const QString textColor = palette.color(QPalette::WindowText).name();
    const QString baseColor = palette.color(QPalette::Base).name();
    const QString borderColor = palette.color(QPalette::Mid).name();

    setPalette(palette);
    m_titleLabel->setStyleSheet(
        QString("font-size: 14px; font-weight: bold; color: %1; padding: 2px;")
            .arg(textColor));
    m_scrollArea->setStyleSheet(
        QString("QScrollArea { border: 1px solid %1; background-color: %2; border-radius: 6px; }")
            .arg(borderColor, baseColor));
    m_scrollContainer->setPalette(palette);

    for (QWidget *card : m_bookCards) {
        const QVariant bookData = card->property("bookData");
        if (bookData.isValid() && bookData.canConvert<BookInfo>()) {
            updateBookCardCover(card, bookData.value<BookInfo>());
        }
    }
}
void BookshelfWidget::updateBookCardCover(QWidget *card, const BookInfo &info)
{
    QLabel *coverLabel = card->findChild<QLabel*>("coverLabel");
    if (!coverLabel) {
        return;
    }

    QPixmap coverPixmap;
    if (!info.coverUrl.isEmpty() && CoverCache::contains(info.isbn)
        && coverPixmap.load(CoverCache::filePath(info.isbn))) {
        coverLabel->setPixmap(coverPixmap.scaled(coverLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
    } else {
        coverLabel->setPixmap(generatePlaceholderCover(info.title));
    }
}

QWidget *BookshelfWidget::createBookCard(const BookInfo &info)
{
    QWidget *bookCard = new QWidget(m_scrollContainer);
    bookCard->setFixedSize(120, 142);
    bookCard->setStyleSheet("QWidget { background: transparent; border: none; }");
    bookCard->setObjectName("card_" + info.isbn);
    bookCard->setCursor(Qt::PointingHandCursor);
    bookCard->setProperty("bookData", QVariant::fromValue(info));
    bookCard->installEventFilter(this);

    QVBoxLayout *cardLayout = new QVBoxLayout(bookCard);
    cardLayout->setContentsMargins(5, 5, 5, 5);
    cardLayout->setSpacing(0);

    QLabel *coverLabel = new QLabel(bookCard);
    coverLabel->setObjectName("coverLabel");
    coverLabel->setFixedSize(110, 132);
    coverLabel->setAlignment(Qt::AlignCenter);
    coverLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    coverLabel->setStyleSheet("QLabel { background: transparent; border: none; }");
    cardLayout->addWidget(coverLabel);

    updateBookCardCover(bookCard, info);
    return bookCard;
}

void BookshelfWidget::addBookToShelf(const BookInfo &info, bool prepend)
{
    QWidget *existingCard = m_scrollContainer->findChild<QWidget*>("card_" + info.isbn);

    if (existingCard) {
        existingCard->setProperty("bookData", QVariant::fromValue(info));
        updateBookCardCover(existingCard, info);

        if (prepend) {
            m_bookCards.removeOne(existingCard);
            m_bookCards.prepend(existingCard);
            rearrangeGrid();
        }
        return;
    }

    QWidget *bookCard = createBookCard(info);

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

void BookshelfWidget::rearrangeGrid()
{
    QLayoutItem *child;
    while ((child = m_shelfGridLayout->takeAt(0)) != nullptr) {
        delete child;
    }

    int shelfWidth = width() - 40;
    // CHANGED: Adjusted calculations to respect the newly optimized footprint sizing boundaries
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
    const QPalette systemPalette = palette();
    pixmap.fill(systemPalette.color(QPalette::AlternateBase));

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(systemPalette.color(QPalette::Mid));

    painter.drawRect(5, 5, 98, 120);

    QFont font = painter.font();
    font.setPointSize(9);
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(systemPalette.color(QPalette::WindowText));

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

void BookshelfWidget::filterBooks(const QString &searchText)
{
    QString cleanSearch = searchText.trimmed().toLower();

    for (QWidget* card : m_bookCards) {
        // Extract the stored BookInfo variant metadata layout from each card
        QVariant prop = card->property("bookData");
        if (prop.isValid() && prop.canConvert<BookInfo>()) {
            BookInfo info = prop.value<BookInfo>();

            // Match against Title, Author, or ISBN
            bool matchesTitle = info.title.toLower().contains(cleanSearch);
            bool matchesAuthor = info.authors.toLower().contains(cleanSearch);
            bool matchesIsbn = info.isbn.contains(cleanSearch);

            if (cleanSearch.isEmpty() || matchesTitle || matchesAuthor || matchesIsbn) {
                card->setVisible(true);
            } else {
                card->setVisible(false); // Hide the card if it doesn't match
            }
        }
    }

    // Force layout matrix recalculation so remaining visible cards rearrange cleanly
    rearrangeGrid();
}
