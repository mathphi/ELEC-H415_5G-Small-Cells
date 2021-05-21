#ifndef COVERAGEOPTIMIZER_H
#define COVERAGEOPTIMIZER_H

#include <QObject>

#include "emitter.h"
#include "simulationarea.h"

class Wall;
class Corner;
class SimulationHandler;

class CoverageOptimizer : public QObject
{
    Q_OBJECT
public:
    explicit CoverageOptimizer(
            SimulationArea *rcv_area,
            double emitter_freq,
            double emitter_eirp,
            AntennaType::AntennaType emitter_antenna,
            QObject *parent = nullptr);
    ~CoverageOptimizer();

    void optimizeEmitters();

private:
    void deletePlacedEmitters();
    void runOptimizationIteration();

    qreal totalCoverageRatio();
    bool isCoveredAt(QPoint pos);

    Receiver *getReceiverAt(QPoint pos);
    Corner *getClosestFreeCorner(QPoint pos);
    QPointF getPlaceableCornerPosition(Corner *c);


    SimulationArea *m_sim_area;
    double m_emit_freq;
    double m_emit_eirp;
    AntennaType::AntennaType m_emit_ant_type;

    SimulationHandler *m_simulation_handler;

    bool m_optimized;
    QRectF m_real_sim_rect;

    QMap<QPoint,Receiver*> m_receivers_map;

    QList<Wall*> m_walls_list;
    QList<Corner*> m_corners_list;

    QList<Corner*> m_excluded_corners;

    QList<Emitter*> m_placed_emitters;
};

#endif // COVERAGEOPTIMIZER_H
