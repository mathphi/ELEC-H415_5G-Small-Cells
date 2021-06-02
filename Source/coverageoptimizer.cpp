#include "coverageoptimizer.h"
#include "simulationhandler.h"
#include "corner.h"

#include <QApplication>
#include <QDebug>

#define CORNER_OFFSET_DIST  2   // Meters


CoverageOptimizer::CoverageOptimizer(
        SimulationHandler *sim_handler,
        SimulationArea *rcv_area,
        QObject *parent)
    : QObject(parent)
{
    // Store the pointer to the receivers area
    m_sim_area = rcv_area;

    // Store the pointer to the simulation handler
    m_simulation_handler = sim_handler;

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
    foreach (Wall *w, m_walls_list) {
        delete w;
    }

    foreach (Corner *c, m_corners_list) {
        delete c;
    }
}

/**
 * @brief CoverageOptimizer::optimizeEmitters
 * @return
 *
 * This function runs a complete optimization for emitters
 * placement.
 */
bool CoverageOptimizer::optimizeEmitters(
        double cover_thrld,
        double fade_margin,
        double emitter_freq,
        double emitter_eirp,
        double emitter_eff,
        AntennaType::AntennaType emitter_antenna)
{
    // Store the parameters
    m_fade_margin = fade_margin;
    m_cover_threshold = cover_thrld;
    m_emit_freq = emitter_freq;
    m_emit_eirp = emitter_eirp;
    m_emit_eff  = emitter_eff;
    m_emit_ant_type = emitter_antenna;

    // Remove the previously placed emitters (if one)
    m_sim_area->deletePlacedEmitters();

    // Initialize attributes
    m_optimized = false;
    m_finished = false;
    m_placed_emitters.clear();

    // Initially, all corners are availables
    m_available_corners = m_corners_list;

    // Reset the computation handler
    m_simulation_handler->resetComputedData();

    qDebug() << "OPTIMIZATION STARTED:";
    qDebug() << "Min. coverage:" << m_cover_threshold;
    qDebug() << "Coverage margin:" << m_fade_margin;
    qDebug() << "Receivers:" << m_receivers_map.size();
    qDebug() << "Walls:" << m_walls_list.size();
    qDebug() << "Corners:" << m_available_corners.size();

    // Restart the elapsed timer counter
    m_elapsed_time = 0;
    QElapsedTimer tmr;
    tmr.start();

    // If there is no corner in the map -> return an empty emitters list
    if (m_corners_list.size() == 0) {
        return true;
    }

    // Iterate until the coverage is optimal
    while (!m_finished) {
        runOptimizationIteration();
    }

    // Store the elapsed time
    m_elapsed_time = tmr.nsecsElapsed() / 1.0e9;

    qDebug() << "OPTIMIZATION FINISHED";
    qDebug() << "Total processing time:" << m_elapsed_time << "s";

    return m_optimized;
}

/**
 * @brief CoverageOptimizer::runOptimizationIteration
 *
 * This function runs an optimization iteration.
 * This consists in placing on emitter on a non-covered corner, close to the
 * first non-covered area found.
 */
void CoverageOptimizer::runOptimizationIteration() {
    // -> Kor each corner (not excluded), compute a score:
    //        sum 1/(1+dist) over all the uncovered receivers of the map
    //    where dist is the distance between the corner and each receiver.
    //
    // -> Keep the corner with highest score, place it in the
    //    exluded corners list, and place an emitter on it.
    //
    // -> If this emitter improved the coverage -> keep it
    //    Else -> discard it and its rays.
    //
    // -> Continue until the coverage is over the threshold or until there
    //    is no more available corners.

    // Compute the initial coverage ratio
    double cover_ratio = totalCoverageRatio(m_fade_margin);

    qDebug() << "Init coverage:" << cover_ratio << "/" << m_cover_threshold;

    // Buffer to get the maximum score
    double max_score     = 0;

    // Pointer to the corner position with maximum score
    Corner *max_score_corner;

    // Placeable corner position with maximum score
    QPointF max_score_pos;

    // Search the corner with highest score on a position
    foreach (Corner *c, m_available_corners) {
        // Get the placeable position at this corner
        QPointF c_place_pos = getPlaceableCornerPosition(c);

        // Get the relative position of this corner
        QPoint c_place_rel = QPointF(c_place_pos - m_real_sim_rect.topLeft()).toPoint();

        // Receiver at this positions
        Receiver *r = m_receivers_map.value(c_place_rel);

        // Check if there is a receiver at this position
        if (!r) {
            // This shouldn't happen
            qDebug() << "RCVPOS ERROR";
            continue;
        }

        // Get the score for this corner
        double score = getPositionScore(c_place_pos);

        // Keep the best corner
        if (max_score < score) {
            max_score        = score;
            max_score_pos    = c_place_pos;
            max_score_corner = c;
        }
    }

    qDebug() << "Best score:" << max_score << "@" << max_score_pos;


    // If no free corner was found -> stop
    if (max_score == 0) {
        m_optimized = true;
        m_finished = true;
        return;
    }

    // Set the best placement position found as occupied
    m_available_corners.removeAll(max_score_corner);

    qDebug() << "Emitter pos:" << max_score_pos;

    // Create a new test emitter
    Emitter *emit_test = new Emitter(m_emit_freq, m_emit_eirp, 1.0, m_emit_ant_type);
    emit_test->setPos(max_score_pos * SimulationScene::simulationScale());
    m_sim_area->addPlacedEmitter(emit_test);

    // Start the simulation for this emitter only, without resetting the previous results
    m_simulation_handler->startSimulationComputation(
                m_sim_area->getReceiversList(),
                m_sim_area->getArea(),
                false,
                QList<Emitter*>() << emit_test);

    // Elapsed timer to refresh the map regularily
    QElapsedTimer tmr;
    tmr.start();

    // Wait until the computation is done
    while (!m_simulation_handler->isDone()) {
        // If the simulation is not running anymore -> it was canceled
        if (!m_simulation_handler->isRunning() && !m_simulation_handler->isDone()) {
            m_finished = true;
            qDebug() << "OPTIMIZATION CANCELED";
            return;
        }

        qApp->processEvents();

        // Update all receivers at each second
        if (tmr.elapsed() > 1000.0) {
            foreach (Receiver *r, m_receivers_map) {
                r->update();
            }

            // Reset the timer counter
            tmr.restart();
        }
    }

    // Compute the new coverage ratio
    double new_coverage_ratio = totalCoverageRatio(m_fade_margin);

    qDebug() << "New coverage:" << new_coverage_ratio << "/" << m_cover_threshold;

    // Check if the coverage ratio has been improved
    if (new_coverage_ratio > cover_ratio) {
        // If this emitter improved the coverage
        //  -> Keep it in the placed emitters list;
        m_placed_emitters.append(emit_test);
    }
    else {
        // Else discard it and delete it and add this corner to banned list

        // Remove all raypaths from this emitter from all receivers...
        foreach(Receiver *r, m_receivers_map) {
            r->discardEmitter(emit_test);
        }

        // Remove it from the simulation area
        m_sim_area->removePlacedEmitter(emit_test);

        // Delete the tested emitter
        delete emit_test;
    }

    qDebug() << "Remaining corners:" << m_available_corners.size() << "/" << m_corners_list.size();

    // Finally, check if we have a full coverage or if all the corners have been banned
    if (new_coverage_ratio >= m_cover_threshold || m_available_corners.size() == 0) {
        m_optimized = true;
        m_finished = true;
    }
}

/**
 * @brief CoverageOptimizer::totalCoverageRatio
 * @return
 *
 * This function returns the coverage ratio (number of covered receivers over
 * total number or receivers).
 */
qreal CoverageOptimizer::totalCoverageRatio(double margin) {
    // Count the number of covered receivers
    int covered_count = 0;

    foreach(Receiver *r, m_sim_area->getReceiversList()) {
        if (r->isCovered(margin)) {
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

    return r->isCovered(m_fade_margin);
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
 * @brief CoverageOptimizer::getPositionScore
 * @param c
 * @return
 *
 * This function computes the score for the given position
 */
double CoverageOptimizer::getPositionScore(QPointF pos) {
    // Init score to zero
    double score = 0;

    // Temp vars
    double ampl_factor;
    double dist;
    QLineF direct_line;
    QLineF::IntersectionType intersect_type;

    // Loop over each receiver and take the uncovered ones into account
    foreach (Receiver *r, m_receivers_map) {
        // Check if it is uncovered
        if (r->isCovered(m_fade_margin))
            continue;

        // Get the direct line between given position and receiver
        direct_line = QLineF(r->getRealPos(), pos);

        // If the direct line is free of obstacle -> amplification factor = 100
        ampl_factor = 100.0;

        // Search for an obstacle for this line
        foreach (Wall *w, m_walls_list) {
            // Get the intersection type
            intersect_type = direct_line.intersects(w->getRealLine(), nullptr);

            // Amplification factor is 1 if the line is obstructed
            if (intersect_type == QLineF::BoundedIntersection) {
                ampl_factor = 1.0;
                break;
            }
        }

        // Get the distance to this receiver
        dist = direct_line.length();

        // Add this receiver to the score
        // The score function is based on a reciprocal function in order to give more importance to
        // the uncovered receivers which are close to the position.
        score += ampl_factor * 1 / (1 + dist);
    }

    return score;
}

/**
 * @brief CoverageOptimizer::getPlaceableCornerPosition
 * @param c
 * @return
 *
 * This function returns the "placeable" corner position.
 * It consists of a position at some distance from the given corner, outside the building.
 */
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


QList<Emitter*> CoverageOptimizer::getPlacedEmitters() {
    return m_placed_emitters;
}

int CoverageOptimizer::getNumPlacedEmitters() {
    return m_placed_emitters.size();
}

double CoverageOptimizer::getTotalCoverage() {
    return totalCoverageRatio(0);
}

double CoverageOptimizer::getTotalCoverageMargin() {
    return totalCoverageRatio(m_fade_margin);
}

double CoverageOptimizer::getTimeElapsed() {
    return m_elapsed_time;
}
