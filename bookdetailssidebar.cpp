#include "bookdetailssidebar.h"
#include "covercache.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QFrame>
#include <QPixmap>
#include <QFont>
#include <QPalette>

BookDetailsSidebar::BookDetailsSidebar(QWidget *parent) : QWidget(parent)
{
    setMinimumWidth(260);
    setMaximumWidth(320);
    const QPalette systemPalette = palette();

    auto *sidebarLayout = new QVBoxLayout(this);
    sidebarLayout->setContentsMargins(5, 0, 5, 0);

    m_container = new QFrame(this);
    auto *containerLayout = new QVBoxLayout(m_container);
    containerLayout->setContentsMargins(15, 15, 15, 15);

    auto *headerRowLayout = new QHBoxLayout;
    auto *sidebarHeader = new QLabel("Book Analytics", m_container);
    sidebarHeader->setFont(QFont("Segoe UI", 12, QFont::Bold));
    sidebarHeader->setStyleSheet("border: none;");

    m_closeButton = new QPushButton("✕", m_container);
    m_closeButton->setFixedSize(24, 24);
    m_closeButton->setCursor(Qt::PointingHandCursor);

    headerRowLayout->addWidget(sidebarHeader, 1, Qt::AlignLeft);
    headerRowLayout->addWidget(m_closeButton, 0, Qt::AlignRight);
    containerLayout->addLayout(headerRowLayout);

    m_coverLabel = new QLabel(m_container);
    m_coverLabel->setFixedSize(160, 220);
    m_coverLabel->setAlignment(Qt::AlignCenter);
    containerLayout->addWidget(m_coverLabel, 0, Qt::AlignCenter);

    m_detailTitleLabel = new QLabel("Select a book to inspect details", m_container);
    m_detailTitleLabel->setFont(QFont("Segoe UI", 11, QFont::Bold));
    m_detailTitleLabel->setWordWrap(true);
    m_detailTitleLabel->setAlignment(Qt::AlignCenter);
    m_detailTitleLabel->setTextInteractionFlags(
        Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    m_detailTitleLabel->setStyleSheet("border: none; padding-top: 10px;");

    m_detailAuthorLabel = new QLabel("", m_container);
    m_detailAuthorLabel->setWordWrap(true);
    m_detailAuthorLabel->setAlignment(Qt::AlignCenter);
    m_detailAuthorLabel->setTextInteractionFlags(
        Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    m_detailAuthorLabel->setStyleSheet("border: none; font-style: italic;");

    m_detailIsbnLabel = new QLabel("", m_container);
    m_detailIsbnLabel->setAlignment(Qt::AlignCenter);
    m_detailIsbnLabel->setTextInteractionFlags(
        Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    m_detailIsbnLabel->setStyleSheet(
        "border: none; font-family: monospace; font-size: 12px; padding-top: 5px;");

    containerLayout->addWidget(m_detailTitleLabel);
    containerLayout->addWidget(m_detailAuthorLabel);
    containerLayout->addWidget(m_detailIsbnLabel);

    containerLayout->addStretch();

    m_deleteButton = new QPushButton("🗑️ Remove from Shelf", m_container);
    m_deleteButton->setMinimumHeight(40);
    m_deleteButton->setCursor(Qt::PointingHandCursor);
    containerLayout->addWidget(m_deleteButton);
    sidebarLayout->addWidget(m_container);

    connect(m_closeButton, &QPushButton::clicked, this, &BookDetailsSidebar::closeSidebar);

    connect(m_deleteButton, &QPushButton::clicked, this, [this]() {
        if (!m_currentIsbn.isEmpty()) {
            emit deleteBookRequested(m_currentIsbn);
        }
    });

    applyPalette(systemPalette);
}

void BookDetailsSidebar::applyPalette(const QPalette &palette)
{
    const QString windowText = palette.color(QPalette::WindowText).name();
    const QString baseColor = palette.color(QPalette::Base).name();
    const QString borderColor = palette.color(QPalette::Mid).name();
    const QString buttonColor = palette.color(QPalette::Button).name();
    const QString buttonText = palette.color(QPalette::ButtonText).name();
    const QString highlightColor = palette.color(QPalette::Highlight).name();
    const QString highlightedText = palette.color(QPalette::HighlightedText).name();

    setPalette(palette);
    m_container->setStyleSheet(QString(
        "QFrame { background-color: %1; border: 1px solid %2; border-radius: 8px; }")
        .arg(baseColor, borderColor));
    m_closeButton->setStyleSheet(QString(
        "QPushButton { color: %1; border-radius: 12px; padding: 0px; "
        "font-weight: bold; border: 1px solid %2; }"
        "QPushButton:hover { background-color: %3; color: %4; }")
        .arg(windowText, borderColor, highlightColor, highlightedText));
    m_coverLabel->setStyleSheet(QString(
        "background-color: %1; border: 1px solid %2; border-radius: 6px;")
        .arg(baseColor, borderColor));
    m_deleteButton->setStyleSheet(QString(
        "QPushButton { background-color: %1; color: %2; border-radius: 6px; "
        "padding: 10px; font-weight: bold; border: 1px solid %3; }"
        "QPushButton:hover { background-color: %4; color: %5; }")
        .arg(buttonColor, buttonText, borderColor, highlightColor, highlightedText));

    if (m_currentBook.found && !CoverCache::contains(m_currentBook.isbn)) {
        setCoverForBook(m_currentBook);
    }
}

void BookDetailsSidebar::setCoverForBook(const BookInfo &info)
{
    QPixmap coverPixmap;
    if (!info.coverUrl.isEmpty() && CoverCache::contains(info.isbn)
        && coverPixmap.load(CoverCache::filePath(info.isbn))) {
        m_coverLabel->setPixmap(coverPixmap.scaled(m_coverLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        return;
    }

    QPixmap placeholder(160, 220);
    placeholder.fill(palette().color(QPalette::AlternateBase));
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
    m_currentBook = info;
    m_currentIsbn = info.isbn;
    setCoverForBook(info);

    m_detailTitleLabel->setText(info.title);
    m_detailAuthorLabel->setText(info.authors.isEmpty() ? "Unknown" : info.authors);
    m_detailIsbnLabel->setText("ISBN: " + info.isbn);
}

void BookDetailsSidebar::refreshCover(const QString &isbn)
{
    if (m_currentBook.found && m_currentBook.isbn == isbn) {
        setCoverForBook(m_currentBook);
    }
}

void BookDetailsSidebar::closeSidebar()
{
    setVisible(false);
    m_currentIsbn.clear();
    m_currentBook = BookInfo();
    m_coverLabel->clear();
    clearDetailsText();
}
