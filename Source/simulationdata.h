#ifndef SIMULATIONDATA_H
#define SIMULATIONDATA_H

#include <QObject>

#include "building.h"
#include "emitter.h"
#include "receiver.h"


namespace SimType {
enum SimType {
    PointReceiver = 0,
    AreaReceiver  = 1
};
}

class SimulationData : public QObject
{
    Q_OBJECT

public:
    SimulationData();
    SimulationData(QList<Building*> b_l, QList<Emitter*> e_l, QList<Receiver*> r_l);

    void setInitData(QList<Building*> b_l, QList<Emitter*> e_l, QList<Receiver*> r_l);

    static double convertPowerToWatts(double power_dbm);
    static double convertPowerTodBm(double power_watts);

    static QRgb ratioToColor(qreal ratio, bool light = false);

    void attachBuilding(Building *b);
    void attachEmitter(Emitter *e);
    void attachReceiver(Receiver *r);

    void detachBuilding(Building *b);
    void detachEmitter(Emitter *e);
    void detachReceiver(Receiver *r);

    void reset();

    QList<Building*> getBuildingsList();
    QList<Emitter*> getEmittersList();
    QList<Receiver*> getReceiverList();

    int maxReflectionsCount();
    SimType::SimType simulationType();

public slots:
    void setReflectionsCount(int cnt);
    void setSimulationType(SimType::SimType t);

private:
    // Lists of all buildings/emitters/recivers on the map
    QList<Building*> m_building_list;
    QList<Emitter*> m_emitter_list;
    QList<Receiver*> m_receiver_list;

    int m_reflections_count;
    SimType::SimType m_simulation_type;
};

// Operator overload to write the simulation data into a file
QDataStream &operator>>(QDataStream &in, SimulationData *sd);
QDataStream &operator<<(QDataStream &out, SimulationData *sd);

#endif // SIMULATIONDATA_H
