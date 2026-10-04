#include "scannerframeitem.h"

#include <QPainter>

ScannerFrameItem::ScannerFrameItem(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setOpaquePainting(true);
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
    painter->fillRect(boundingRect(), Qt::black);
    if (m_frame.isNull()) {
        return;
    }

    const QSize targetSize = boundingRect().size().toSize();
    const QImage scaledFrame = m_frame.scaled(
        targetSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    const QPointF frameOrigin((width() - scaledFrame.width()) / 2.0,
                              (height() - scaledFrame.height()) / 2.0);
    painter->save();
    painter->setClipRect(boundingRect());
    painter->drawImage(frameOrigin, scaledFrame);
    painter->restore();
}