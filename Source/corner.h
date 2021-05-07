#ifndef CORNER_H
#define CORNER_H

#include <stdlib.h>
#include <array>
#include <QPointF>
#include <QLineF>

class Wall;

class Corner
{
public:
    Corner(QPointF pos, QPointF wep1, QPointF wep2, Wall *w1, Wall *w2);

    QPointF getPosition();
    QPointF getRealPos();
    std::array<QPointF,2> getWallsEndPoints();
    std::array<QPointF,2> getRealEndPoints();
    std::array<QLineF,2> getAdjecentRealLines();
    std::array<Wall*,2> getAdjecentWalls();

private:
    QPointF m_position;
    std::array<QPointF,2> m_walls_end_points;
    std::array<Wall*,2> m_adjacent_walls;
};

#endif // CORNER_H
