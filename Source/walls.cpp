#include "walls.h"
#include "constants.h"
#include "simulationscene.h"

// Wall's relative permittivity
#define BUILDING_R_PERMITTIVITY     5


Wall::Wall(QLineF line) {
    m_line = line;
}

QLineF Wall::getLine() {
    return m_line;
}

void Wall::setLine(QLineF line) {
    m_line = line;
}

/**
 * @brief Wall::getRealLine
 * @return
 *
 * This function returns the real line of the wall (in meters)
 */
QLineF Wall::getRealLine() const {
    qreal scale = SimulationScene::simulationScale();
    return QLineF(m_line.p1() / scale, m_line.p2() / scale);
}

/**
 * @brief Wall::getNormalAngleTo
 * @param line
 * @return
 *
 * This function returns the angle made by the 'line' to the normal of the wall.
 * This angle is defined as 0 <= theta <= PI/2 (in radians)
 */
double Wall::getNormalAngleTo(QLineF line) const {
    double theta = fabs(M_PI_2 - m_line.angleTo(line) / 180.0 * M_PI);

    // If the angle is > PI/2 -> use the normal of the wall in the other direction
    if(theta > M_PI_2) {
       theta = fabs(theta - M_PI);
    }

    return theta;
}

double Wall::getPermitivity() const {
    return BUILDING_R_PERMITTIVITY;
}
