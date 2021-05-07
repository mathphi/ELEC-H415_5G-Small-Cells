#include "corner.h"
#include "walls.h"
#include "simulationscene.h"

Corner::Corner(QPointF pos, QPointF wep1, QPointF wep2, Wall *w1, Wall *w2)
{
    m_position = pos;
    m_walls_end_points = {wep1, wep2};
    m_adjacent_walls = {w1, w2};
}

QPointF Corner::getPosition() {
    return m_position;
}

QPointF Corner::getRealPos() {
    qreal scale = SimulationScene::simulationScale();
    return m_position / scale;
}

std::array<QPointF,2> Corner::getWallsEndPoints() {
    return m_walls_end_points;
}

std::array<QPointF,2> Corner::getRealEndPoints() {
    qreal scale = SimulationScene::simulationScale();
    return {
        m_walls_end_points[0] / scale,
        m_walls_end_points[1] / scale
    };
}

std::array<QLineF,2> Corner::getAdjecentRealLines() {
    qreal scale = SimulationScene::simulationScale();
    return {
        QLineF(getRealPos(), m_walls_end_points[0] / scale),
        QLineF(getRealPos(), m_walls_end_points[1] / scale)
    };
}

std::array<Wall*,2> Corner::getAdjecentWalls() {
    return m_adjacent_walls;
}
