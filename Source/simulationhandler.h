#ifndef SIMULATIONHANDLER_H
#define SIMULATIONHANDLER_H

#include <QObject>
#include <QElapsedTimer>
#include <QThreadPool>

#include "simulationdata.h"
#include "simulationitem.h"
#include "simulationscene.h"
#include "constants.h"
#include "raypath.h"

class SimulationHandler : public QObject
{
    Q_OBJECT
public:
    SimulationHandler();

    SimulationData *simulationData();
    QList<RayPath*> getRayPathsList();

    bool isRunning();

    static QPointF mirror(QPointF source, QLineF wall);

private:
    SimulationData *m_simulation_data;
    QList<Receiver*> m_receivers_list;

    QElapsedTimer m_computation_timer;

    QThreadPool m_threadpool;
    QMutex m_mutex;

    bool m_sim_started;
    bool m_sim_cancelling;
    int m_init_cu_count;
};

#endif // SIMULATIONHANDLER_H
