#include "scannerframeitem.h"

#include <QPainter>
#include <algorithm>

ScannerFrameItem::ScannerFrameItem(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
}

bool ScannerFrameItem::hasFrame() const
{
    return !m_frame.isNull();
}

void ScannerFrameItem::setFrame(const QImage &frame)
{
    if (m_frame.cacheKey() == frame.cacheKey()) {
        return;
    }
    m_frame = frame;
    update();
    emit frameChanged();
}

void ScannerFrameItem::paint(QPainter *painter)
{
    if (m_frame.isNull()) {
        return;
    }

    const qreal targetWidth = boundingRect().width();
    const qreal targetHeight = boundingRect().height();
    if (targetWidth <= 0 || targetHeight <= 0) {
        return;
    }

    const qreal scale = std::max(targetWidth / m_frame.width(),
                                 targetHeight / m_frame.height());
    const qreal scaledWidth = m_frame.width() * scale;
    const qreal scaledHeight = m_frame.height() * scale;
    const QRectF targetRect((targetWidth - scaledWidth) / 2.0,
                            (targetHeight - scaledHeight) / 2.0,
                            scaledWidth, scaledHeight);
    painter->save();
    painter->setClipRect(boundingRect());
    painter->setRenderHint(QPainter::SmoothPixmapTransform, false);
    painter->drawImage(targetRect, m_frame);
    painter->restore();
}