#ifndef RECEIVER_H
#define RECEIVER_H

#include <QGraphicsItem>
#include <QMutex>
#include <QSet>

#include "simulationitem.h"
#include "raypath.h"
#include "antennas.h"
#include "emitter.h"

namespace ResultType {
enum ResultType {
    Power,
    SNR,
    DelaySpread,
    RiceFactor,
    CoverageMap
};
}

class Receiver : public SimulationItem
{
public:
    Receiver(Antenna *antenna);
    Receiver(AntennaType::AntennaType antenna_type, double efficiency = 1.0);

    ~Receiver();

    Receiver *clone();

    Antenna *getAntenna();
    void setAntenna(AntennaType::AntennaType type, double efficiency = 1.0);
    void setAntenna(Antenna *a);

    void setRotation(double angle);
    double getRotation() const;
    double getIncidentRayAngle(QLineF ray) const;

    double getEfficiency() const;
    double getResistance() const;
    vector<complex> getEffectiveHeight(double phi, double frequency) const;
    vector<complex> getEffectiveHeight(double theta, double phi, double frequency) const;
    double getGain(double phi) const;

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override;

    void setFlat(bool flat);
    void paintShaped(QPainter *painter);
    void paintFlat(QPainter *painter);

    void reset();
    void addRayPath(RayPath *rp);
    QList<RayPath*> getRayPaths();
    void discardEmitter(Emitter *e);

    void setOutOfModel(bool out, Emitter *e);
    bool outOfModel();

    double receivedPower();
    double userEndSNR();
    double delaySpread();
    double riceFactor();

    bool isCovered(double coverage_margin);

    void showResults(ResultType::ResultType type, double min, double max);

    void generateIdleTooltip();
    void generateResultsTooltip();

private:
    double m_rotation_angle;
    Antenna *m_antenna;

    QList<RayPath*> m_received_rays;
    QSet<Emitter*> m_attached_emitters;

    double m_received_power;
    double m_user_end_SNR;
    double m_delay_spread;
    double m_rice_factor;

    ResultType::ResultType m_result_type;
    double m_res_min;
    double m_res_max;

    bool m_flat;
    bool m_show_result;

    bool m_out_of_model;
    Emitter *m_oom_emitter; // The receiver is out of model w.r.t. this emitter

    QMutex m_mutex;
};

// Operator overload to write objects from the Receiver class into a files
QDataStream &operator>>(QDataStream &in, Receiver *&r);
QDataStream &operator<<(QDataStream &out, Receiver *r);

#endif // RECEIVER_H
