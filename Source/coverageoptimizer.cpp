#include "coverageoptimizer.h"
#include "simulationhandler.h"
#include "corner.h"

#include <QApplication>
#include <QDebug>

#define CORNER_OFFSET_DIST      2.5     // Meters
#define MINIMUM_EMIT_DISTANCE   20.0    // Meters
#define COVERAGE_THRESHOLD      0.99    // Coverage ratio


CoverageOptimizer::CoverageOptimizer(
        SimulationArea *rcv_area,
        double emitter_freq,
        double emitter_eirp,
        AntennaType::AntennaType emitter_antenna,
        QObject *parent)
    : QObject(parent)
{
    // Store the pointer to the receivers area
    m_sim_area = rcv_area;
    m_emit_freq = emitter_freq;
    m_emit_eirp = emitter_eirp;
    m_emit_ant_type = emitter_antenna;

    // Init the dedicated simulation handler
    m_simulation_handler = new SimulationHandler;

    // Initialize attributes
    m_optimized = false;

    // Get simulation area
    m_real_sim_rect = m_sim_area->getRealArea();

    // Store the receivers map
    m_receivers_map = m_sim_area->getReceiversMap();

    // Make the walls and building list
    m_walls_list = SimulationHandler::simulationData()->makeBuildingWallsFiltered(m_sim_area->getArea());
    m_corners_list = SimulationHandler::simulationData()->makeWallsCorners(m_walls_list);
}

CoverageOptimizer::~CoverageOptimizer() {
    delete m_simulation_handler;
}

void CoverageOptimizer::deletePlacedEmitters() {
    foreach (Emitter *e, m_placed_emitters) {
        delete e;
    }

    m_placed_emitters.clear();
}

/**
 * @brief CoverageOptimizer::optimizeEmitters
 * @return
 *
 * This function runs a complete optimization for emitters
 * placement.
 */
void CoverageOptimizer::optimizeEmitters() {
    // Delete the placed emitters (if one)
    deletePlacedEmitters();

    // Initialize attributes
    m_optimized = false;
    m_excluded_corners.clear();
    m_placed_emitters.clear();

    // Reset the computation handler
    m_simulation_handler->resetComputedData();

    // If there is no corner in the map -> return an empty emitters list
    if (m_corners_list.size() == 0) {
        return;
    }

    // Iterate until the coverage is optimal
    while (!m_optimized) {
        runOptimizationIteration();


        double min, max;
        m_sim_area->getReceivedDataBounds(ResultType::CoverageMap, &min, &max);

        // Loop over every receiver and show its results
        foreach(Receiver *re, m_sim_area->getReceiversList())
        {
            // Show the results of each receiver
            re->showResults(ResultType::CoverageMap, min, max);
        }
        m_sim_area->simulationScene()->showDataLegend(ResultType::CoverageMap, min, max);
    }
}

/**
 * @brief CoverageOptimizer::runOptimizationIteration
 *
 * This function runs an optimization iteration.
 * This consists in placing on emitter on a non-covered corner, close to the
 * first non-covered area found.
 */
void CoverageOptimizer::runOptimizationIteration() {
    // Compute the initial coverage ratio
    double cover_ratio = totalCoverageRatio();

    // Pointer to the found corner
    Corner *found_corner = nullptr;

    // Search for a non-covered receiver
    foreach (QPoint pos, m_receivers_map.keys()) {
        // Receiver at this positions
        Receiver *r = m_receivers_map.value(pos);

        // Skip this position if already covered
        if (r->isCovered())
            continue;

        // If this receiver is not covered, place an emitter on the closest corner
        Corner *c = getClosestFreeCorner(pos);

        // Check if this corner was already excluded
        if (m_excluded_corners.contains(c)) {
            continue;
        }

        // Cancel the loop if we found a corner
        found_corner = c;
        break;
    }

    // If no free corner was found -> stop
    if (!found_corner) {
        m_optimized = true;
        return;
    }

    // Get the placement position for the tested emitter
    QPointF em_pos = getPlaceableCornerPosition(found_corner);

    qDebug() << "Emitter pos:" << em_pos;

    // Create a new test emitter
    Emitter *emit_test = new Emitter(m_emit_freq, m_emit_eirp, 1.0, m_emit_ant_type);
    emit_test->setPos(em_pos * SimulationScene::simulationScale());


    // Start the simulation for this emitter only, without resetting the previous results
    m_simulation_handler->startSimulationComputation(
                m_sim_area->getReceiversList(),
                m_sim_area->getArea(),
                false,
                QList<Emitter*>() << emit_test);

    // Wait until the computation is done
    while (!m_simulation_handler->isDone()) {
        qApp->processEvents();
    }

    // Compute the new coverage ratio
    double new_coverage_ratio = totalCoverageRatio();

    qDebug() << "Coverage: " << new_coverage_ratio;

    // Check if the coverage ratio has been improved
    if (new_coverage_ratio > cover_ratio) {
        // If this emitter improved the coverage
        //  -> Keep it in the placed emitters list;
        m_placed_emitters.append(emit_test);
        m_sim_area->addPlacedEmitter(emit_test);
    }
    else {
        // Else discard it and delete it and add this corner to banned list

        // Remove all raypaths from this emitter from all receivers...
        foreach(Receiver *r, m_receivers_map) {
            r->discardEmitter(emit_test);
        }

        // Delete the tested emitter
        delete emit_test;
    }

    // Exclude this corner (it has no effect or it is occupied by an emitter)
    m_excluded_corners.append(found_corner);

    qDebug() << "post exclude" << m_excluded_corners.size() << m_corners_list.size();

    // Finally, check if we have a full coverage or if all the corners have been banned
    if (new_coverage_ratio >= COVERAGE_THRESHOLD || m_excluded_corners.size() == m_corners_list.size()) {
        m_optimized = true;
    }
}

/**
 * @brief CoverageOptimizer::totalCoverageRatio
 * @return
 *
 * This function returns the coverage ratio (number of covered receivers over
 * total number or receivers).
 */
qreal CoverageOptimizer::totalCoverageRatio() {
    // Count the number of covered receivers
    int covered_count = 0;

    foreach(Receiver *r, m_sim_area->getReceiversList()) {
        if (r->isCovered()) {
            covered_count++;
        }
    }

    return qreal(covered_count) / qreal(m_sim_area->getReceiversList().size());
}

/**
 * @brief CoverageOptimizer::isCoveredAt
 * @param pos
 * @return
 *
 * This function returns true if this position is covered
 */
bool CoverageOptimizer::isCoveredAt(QPoint pos) {
    Receiver *r = getReceiverAt(pos);

    if (!r) {
        qDebug() << "PROBLEM" << pos;
        return false;
    }

    return r->isCovered();
}

/**
 * @brief CoverageOptimizer::getReceiverAt
 * @param pos
 * @return
 *
 * This function returns the receiver at the given position
 */
Receiver *CoverageOptimizer::getReceiverAt(QPoint pos) {
    return m_receivers_map.value(pos, nullptr);
}

/**
 * @brief CoverageOptimizer::getClosestFreeCorner
 * @param pos
 * @return
 *
 * This function returns the free corner that is the clostest to the given pos.
 */
Corner *CoverageOptimizer::getClosestFreeCorner(QPoint pos) {
    // Get the closest corner
    double min_dist = INFINITY;
    Corner *best_corner = nullptr;

    // Search for a corner which is not covered
    foreach(Corner *c, m_corners_list) {
        // Get the corner position relative to the simulation area
        QPointF corner_pos = getPlaceableCornerPosition(c);
        QPoint test_pos = QPointF(corner_pos - m_real_sim_rect.topLeft()).toPoint();
        double dist = QLineF(test_pos, pos).length();

        bool min_dist_valid = true;
        // Ignore this corner if it is too close to another placed emitter
        foreach(Emitter *e, m_placed_emitters) {
            // Get the distance between the emitter and the corner
            double dist = QLineF(e->getRealPos(), c->getRealPos()).length();

            // Ignore this corner if too close to a placed emitter
            if (dist < MINIMUM_EMIT_DISTANCE) {
                min_dist_valid = false;
            }
        }

        // Keep the closest non-covered corner
        if (dist < min_dist && !isCoveredAt(test_pos) && !m_excluded_corners.contains(c) && min_dist_valid) {
            min_dist = dist;
            best_corner = c;

            qDebug() << "UNCOVERED" << test_pos;
        }
    }

    // If we found a corner -> return it
    if (best_corner) {
        return best_corner;
    }

    // If we didn't found a corner here, this means that they are all covered
    //  -> now get the closest (covered) corner

    // Reset the min distance
    min_dist = INFINITY;

    // Search for a corner which is not covered
    foreach(Corner *c, m_corners_list) {
        // Get the corner position relative to the simulation area
        QPoint c_pos = QPointF(c->getRealPos() - m_real_sim_rect.topLeft()).toPoint();
        double dist = QLineF(c_pos, pos).length();

        // Keep the closest corner
        if (dist < min_dist && !m_excluded_corners.contains(c)) {
            min_dist = dist;
            best_corner = c;
        }
    }

    // Here we should have found a corner
    return best_corner;
}

QPointF CoverageOptimizer::getPlaceableCornerPosition(Corner *c) {
    // Get the unit vectors of the two walls (length = 1m, direction = inside the building)
    QLineF unit_v1 = c->getAdjecentRealLines().at(0).unitVector();
    QLineF unit_v2 = c->getAdjecentRealLines().at(1).unitVector();

    // Place a point at the corner position
    QPointF pos = c->getRealPos();

    // Move the point at a certain distance in the direction outside the building
    pos -= CORNER_OFFSET_DIST * QPointF(unit_v1.dx() + unit_v2.dx(), unit_v1.dy() + unit_v2.dy());

    return pos;
}
