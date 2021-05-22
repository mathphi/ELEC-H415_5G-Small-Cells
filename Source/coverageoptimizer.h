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
            SimulationHandler *sim_handler,
            SimulationArea *rcv_area,
            QObject *parent = nullptr);
    ~CoverageOptimizer();

    bool optimizeEmitters(
            double cover_thrld,
            double fade_margin,
            double emitter_freq,
            double emitter_eirp,
            double emitter_eff,
            AntennaType::AntennaType emitter_antenna);

    QList<Emitter*> getPlacedEmitters();

    int getNumPlacedEmitters();
    double getTotalCoverage();
    double getTotalCoverageMargin();
    double getTimeElapsed();

private:
    void runOptimizationIteration();

    qreal totalCoverageRatio(double margin);
    bool isCoveredAt(QPoint pos);

    Receiver *getReceiverAt(QPoint pos);
    double getPositionScore(QPointF pos);
    QPointF getPlaceableCornerPosition(Corner *c);


    SimulationArea *m_sim_area;

    double m_fade_margin;
    double m_cover_threshold;
    double m_emit_freq;
    double m_emit_eirp;
    double m_emit_eff;
    AntennaType::AntennaType m_emit_ant_type;

    SimulationHandler *m_simulation_handler;

    bool m_optimized;
    bool m_finished;
    QRectF m_real_sim_rect;

    QMap<QPoint,Receiver*> m_receivers_map;

    QList<Wall*> m_walls_list;
    QList<Corner*> m_corners_list;

    QList<Corner*> m_available_corners;
    QList<Emitter*> m_placed_emitters;

    double m_elapsed_time;
};

#endif // COVERAGEOPTIMIZER_H
