#ifndef SCANNERFRAMEITEM_H
#define SCANNERFRAMEITEM_H

#include <QImage>
#include <QQuickPaintedItem>
#include <QtQml/qqmlregistration.h>

class ScannerFrameItem : public QQuickPaintedItem
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(bool hasFrame READ hasFrame NOTIFY frameChanged)

public:
    explicit ScannerFrameItem(QQuickItem *parent = nullptr);

    bool hasFrame() const;
    Q_INVOKABLE void setFrame(const QImage &frame);
    void paint(QPainter *painter) override;

signals:
    void frameChanged();

private:
    QImage m_frame;
};

#endif // SCANNERFRAMEITEM_H