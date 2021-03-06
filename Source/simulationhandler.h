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

class ComputationUnit;

class SimulationHandler : public QObject
{
    Q_OBJECT
public:
    SimulationHandler();

    static SimulationData *simulationData();

    QList<RayPath*> getRayPathsList() const;

    bool isDone() const;
    bool isRunning() const;
    bool isCancelling() const;

    static QPointF mirror(const QPointF source, Wall *wall);

    bool checkIntersections(QLineF ray, Wall *origin_wall, Wall *target_wall);
    vector<complex> reflectionCoefficient(Wall *w, QLineF in_ray);

    vector<complex> computeNominalElecField(
            Emitter *em,
            QLineF emitter_ray,
            QLineF receiver_ray,
            double dn,
            double theta = M_PI_2);

    RayPath *computeRayPath(
            Emitter *emitter,
            Receiver *receiver,
            QList<QPointF> images = QList<QPointF>(),
            QList<Wall*> walls = QList<Wall*>());

    void recursiveReflection(
            Emitter *emitter,
            Receiver *receiver,
            Wall *reflect_wall,
            QList<QPointF> images = QList<QPointF>(),
            QList<Wall*> walls = QList<Wall*>(),
            int level = 1);

    void computeDiffractedRay(Emitter *e, Receiver *r, Corner *c);
    void computeGroundReflection(Emitter *e, Receiver *r);

    void computeAllRays();
    void computeReceiverRays(Receiver *r);

    void receiverRaysThreaded(QList<Receiver*> r_lst);

    void startSimulationComputation(
            QList<Receiver *> rcv_list,
            QRectF sim_area,
            bool reset = true,
            QList<Emitter*> emit_list = QList<Emitter*>());
    void stopSimulationComputation();
    void resetComputedData();

signals:
    void simulationStarted();
    void simulationFinished();
    void simulationCancelled();
    void simulationProgress(double);

private slots:
    void computationUnitFinished();

private:
    QList<Emitter*> m_emitters_list;
    QList<Receiver*> m_receivers_list;
    QList<Wall*> m_wall_list;
    QList<Corner*> m_corners_list;

    QElapsedTimer m_computation_timer;

    QThreadPool m_threadpool;
    QList<ComputationUnit*> m_computation_units;
    QMutex m_mutex;

    int m_init_cu_count;
    bool m_sim_started;
    bool m_sim_cancelling;
    bool m_sim_done;

    QRectF m_sim_area;
};

#endif // SIMULATIONHANDLER_H
