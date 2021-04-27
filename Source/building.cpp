#include "building.h"
#include "constants.h"
#include "simulationscene.h"

#include <QPen>
#include <QPainter>


Building::Building(QRectF rect) : SimulationItem() {
    setPos(rect.topLeft());
    m_build_size = rect.size();
}

Building::~Building() {

}


QRectF Building::getRect() const {
    return QRectF(pos(), m_build_size);
}

QRectF Building::boundingRect() const {
    // Bounding rect contains the building rect
    return getRect();
}

QPainterPath Building::shape() const {
    QPainterPath path;
    path.addRect(getRect());

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
    painter->drawRect(QRectF(pos(), m_build_size));
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
