#ifndef SIMULATIONDATA_H
#define SIMULATIONDATA_H

#include <QObject>

#include "building.h"
#include "emitter.h"
#include "receiver.h"
#include "walls.h"

class Corner;

namespace SimType {
enum SimType : int {
    PointReceiver   = 0,
    AreaReceiver    = 1,
    Analysis1D      = 2,
    CoverageOptim   = 3
};
}

class SimulationData : public QObject
{
    Q_OBJECT

public:
    SimulationData();
    
    static double convertPowerToWatts(double power_dbm);
    static double convertPowerTodBm(double power_watts);
    static double convertKelvinToCelsius(double T_k);
    static double convertCelsiusToKelvin(double T_c);
    static double delayToHumanReadable(double delay, QString *units, double *scale_factor = nullptr);

    static QRgb ratioToColor(qreal ratio, bool light = false);

    void attachBuilding(Building *b);
    void attachEmitter(Emitter *e);
    void attachReceiver(Receiver *r);

    void detachBuilding(Building *b);
    void detachEmitter(Emitter *e);
    void detachReceiver(Receiver *r);

    void reset();
    void resetDefaults();

    QList<Wall*> makeBuildingWallsFiltered(const QRectF boundary_rect) const;
    QList<Wall*> makeBuildingWallsList() const;

    QList<Corner*> makeWallsCorners(QList<Wall*> walls_list) const;

    QList<Building*> getBuildingsList();
    QList<Emitter*> getEmittersList();
    QList<Receiver*> getReceiverList();

    SimType::SimType simulationType();

    double getRelPermitivity() const;
    double getSimulationHeight() const;
    double getSimulationBandwidth() const;
    double getSimulationTemperature() const;
    double getSimulationNoiseFigure() const;
    double getSimulationTargetSNR() const;
    double getMinimumValidRadius() const;
    double getPruningRadius() const;

    double computeThermalNoise() const;

    int maxReflectionsCount() const;
    bool reflectionEnabledNLOS() const;

public slots:
    void setSimulationType(SimType::SimType t);
    void setRelPermitivity(double perm);
    void setReflectionEnabledNLOS(bool enabled);
    void setSimulationBandwidth(double bw);
    void setSimulationTemperature(double temp);
    void setSimulationNoiseFigure(double nf);
    void setSimulationTargetSNR(double snr);
    void setMinimumValidRadius(double radius);
    void setPruningRadius(double radius);

    void setSimulationHeight(double height);
    void setReflectionsCount(int cnt);

private:
    // Lists of all buildings/emitters/recivers on the map
    QList<Building*> m_building_list;
    QList<Emitter*> m_emitter_list;
    QList<Receiver*> m_receiver_list;


    // Simulation settings
    SimType::SimType m_simulation_type;
    int m_reflections_count;
    bool m_nlos_refl_en;

    // Simulation parameters
    double m_rel_permitivity;
    double m_simulation_height;
    double m_sim_bandwidth;
    double m_sim_temperature;
    double m_sim_noise_figure;
    double m_sim_target_SNR;
    double m_min_valid_radius;
    double m_pruning_radius;


    // Operator overload to write the simulation data into a file
    friend QDataStream &operator>>(QDataStream &in, SimulationData *sd);
    friend QDataStream &operator<<(QDataStream &out, SimulationData *sd);
};

#endif // SIMULATIONDATA_H
