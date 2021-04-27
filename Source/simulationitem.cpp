#include "simulationitem.h"
#include "simulationscene.h"

SimulationItem::SimulationItem() : QGraphicsItem()
{
    m_placing_mode = false;
}

bool SimulationItem::placingMode() const {
    return m_placing_mode;
}

void SimulationItem::setPlacingMode(bool on) {
    prepareGeometryChange();
    m_placing_mode = on;
    update();
}

/**
 * @brief SimulationItem::getRealPos
 * @return
 *
 * Returns the real position of the item (in meters)
 */
QPointF SimulationItem::getRealPos() {
    qreal scale = simulationScene()->simulationScale();
    return pos() / scale;
}

/**
 * @brief SimulationItem::simulationScene
 * @return
 *
 * Returns the current simulation scene for the item,
 * or nullptr if the item is not stored in a scene simulation scene
 */
SimulationScene *SimulationItem::simulationScene() const {
    return dynamic_cast<SimulationScene*>(scene());
}
