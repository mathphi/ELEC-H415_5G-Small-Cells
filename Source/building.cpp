#include "building.h"
#include "constants.h"
#include "simulationscene.h"

#include <QPen>
#include <QPainter>


// Building border width (px)
#define BUILDING_BORDER 1.0


Building::Building(QRectF rect) : SimulationItem() {
    m_build_size = rect.size();
    setPos(rect.topLeft());

    setTransformOriginPoint(m_build_size.width()/2, m_build_size.height()/2);
}

Building::Building(QSizeF size) : Building(QRectF(QPoint(0,0), size))
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
        pos(),
        m_build_size
    );
}

QRectF Building::boundingRect() const {
    // Bounding rect contains the building rect
    return QRectF(QPoint(0,0), m_build_size);
}

QPainterPath Building::shape() const {
    QPainterPath path;
    path.addRect(QRectF(QPoint(0,0), m_build_size));

    // Take care of the width of the pen
    QPainterPathStroker ps;
    ps.setWidth(BUILDING_BORDER);

    QPainterPath p = ps.createStroke(path);
    p.addPath(path);
    return p;
}

void Building::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
    // Draw the shape of the gain in blue
    painter->setPen(QPen(QBrush(Qt::black), BUILDING_BORDER));
    painter->setBrush(Qt::gray);
    painter->drawRect(QRectF(QPoint(0,0), m_build_size));
}


QDataStream &operator>>(QDataStream &in, Building *&b) {
    QRectF rect;

    in >> rect;

    b = new Building(rect);

    return in;
}

QDataStream &operator<<(QDataStream &out, Building *b) {
    out << b->getRect();

    return out;
}
