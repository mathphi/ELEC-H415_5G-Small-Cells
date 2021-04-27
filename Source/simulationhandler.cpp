#include "simulationhandler.h"

#include <QDebug>

SimulationHandler::SimulationHandler()
{
    m_simulation_data = new SimulationData();
    m_sim_started = false;
    m_sim_cancelling = false;
    m_init_cu_count = 0;
}

SimulationData *SimulationHandler::simulationData() {
    return m_simulation_data;
}

/**
 * @brief SimulationHandler::getRayPathsList
 * @return
 *
 * This function returns all the computed ray paths in the scene
 */
QList<RayPath*> SimulationHandler::getRayPathsList() {
    QList<RayPath*> ray_paths;

    // Append the ray paths of each receivers to the ray paths list to return
    foreach(Receiver *re, m_receivers_list) {
        ray_paths.append(re->getRayPaths());
    }

    return ray_paths;
}

/**
 * @brief SimulationHandler::isRunning
 * @return
 *
 * This function returns true if a simulation computation is running
 */
bool SimulationHandler::isRunning() {
    return m_sim_started;
}


/**************************************************************************************************/
// --------------------------------- COMPUTATION FUNCTIONS -------------------------------------- //
/**************************************************************************************************/

/**
 * @brief SimulationHandler::mirror
 *
 * This function returns the coordinates of the image of the 'source' point
 * after an axial symmetry through the wall.
 *
 * WARNING: the y axis is upside down in the graphics scene !
 *
 * @param source : The position of the source whose image is calculated
 * @param wall   : The wall over which compute the image (line)
 * @return       : The coordinates of the image
 */
QPointF SimulationHandler::mirror(QPointF source, QLineF wall) {
    // Get the angle of the wall to the horizontal axis
    double theta = wall.angle() * M_PI / 180.0 - M_PI / 2.0;

    // Translate the origin of the coordinates system on the base of the wall
    double x = source.x() - wall.p1().x();
    double y = source.y() - wall.p1().y();

    // We use a rotation matrix :
    //   x' =  x*cos(θ) - y*sin(θ)
    //   y' = -x*sin(θ) - y*cos(θ)
    // to set the y' axis along the wall, so the image is at the opposite of the x' coordinate
    // in the new coordinates system.
    double x_p =  x*cos(theta) - y*sin(theta);
    double y_p = -x*sin(theta) - y*cos(theta);

    // Rotate back to the original coordinates system
    QPointF rel_pos = QPointF(-x_p*cos(theta) - y_p*sin(theta), x_p*sin(theta) - y_p*cos(theta));

    // Translate back to the original coordinates system
    return rel_pos + wall.p1();
}

