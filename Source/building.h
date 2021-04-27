#ifndef BUILDING_H
#define BUILDING_H

#include <QGraphicsItem>
#include <QPen>

#include "simulationitem.h"

// Building class
class Building : public SimulationItem
{
public:
    Building(QRectF rect);
    ~Building();

    QRectF getRect() const;

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override;

private:
    QSizeF m_build_size;
};

// Operator overload to write objects from the Building class into a files
QDataStream &operator>>(QDataStream &in, Building *&b);
QDataStream &operator<<(QDataStream &out, Building *b);

#endif // BUILDING_H
