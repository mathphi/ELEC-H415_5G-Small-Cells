#include "building.h"
#include "constants.h"
#include "simulationscene.h"

#include <QPen>
#include <QPainter>


Building::Building(QPointF pos, QSizeF size) : SimulationItem() {
    m_build_size = size;
    setPos(pos);

    setTransformOriginPoint(m_build_size.width()/2, m_build_size.height()/2);
}

Building::Building(QSizeF size) : Building(QPoint(0,0), size)
{
    // Create a building at position (0,0)
}

Building::~Building() {

}

/**
 * @brief Building::clone
 * @return
 *
 * This function returns a new Building with the same properties
 */
Building* Building::clone() {
    return new Building(m_build_size);
}

/**
 * @brief Building::getSize
 * @return
 *
 * Return the dimensions of the building
 */
QSizeF Building::getSize() const {
    return m_build_size;
}

/**
 * @brief Building::getRect
 * @return
 *
 * Return the absolute rectangle corresponding to the building
 */
QRectF Building::getRect() const {
    return QRectF(
        pos() - QPointF(m_build_size.width()/2, m_build_size.height()/2),
        m_build_size
    );
}

/**
 * @brief Building::getRelativeRect
 * @return
 *
 * This function returns the building rectangle relative to the building
 * position (center of the building).
 */
QRectF Building::getRelativeRect() const {
    return QRectF(QPointF(-m_build_size.width()/2, -m_build_size.height()/2), m_build_size);
}

QRectF Building::boundingRect() const {
    // Bounding rect contains the building rect
    return getRelativeRect();
}

QPainterPath Building::shape() const {
    QPainterPath path;
    path.addRect(getRelativeRect());

    // Take care of the width of the pen
    QPainterPathStroker ps;
    ps.setWidth(1.0);

    QPainterPath p = ps.createStroke(path);
    p.addPath(path);
    return p;
}

void Building::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
    // Draw the shape of the gain in blue
    painter->setPen(QPen(QBrush(Qt::black), 1));
    painter->setBrush(Qt::gray);
    painter->drawRect(getRelativeRect());
}


QDataStream &operator>>(QDataStream &in, Building *&b) {
    QPointF pos;
    QSizeF size;
    in >> pos;
    in >> size;

    b = new Building(pos, size);

    return in;
}

QDataStream &operator<<(QDataStream &out, Building *b) {
    out << b->pos();
    out << b->getSize();

    return out;
}
