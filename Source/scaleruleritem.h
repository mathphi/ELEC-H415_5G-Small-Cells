#ifndef SCALERULERITEM_H
#define SCALERULERITEM_H

#include <QGraphicsItem>

class ScaleRulerItem : public QGraphicsItem
{
public:
    ScaleRulerItem();

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override;

public slots:
    void viewScaleChanged(qreal scale);

private:
    qreal getSimulationScale();

    qreal m_view_scale;
};

#endif // SCALERULERITEM_H
