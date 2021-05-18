#ifndef RECEIVER_H
#define RECEIVER_H

#include <QGraphicsItem>
#include <QMutex>

#include "simulationitem.h"
#include "raypath.h"
#include "antennas.h"

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

    void setOutOfModel(bool out);
    bool outOfModel();

    double receivedPower();
    double userEndSNR();
    double delaySpread();
    double riceFactor();

    void showResults(ResultType::ResultType type, double min, double max);

    void generateIdleTooltip();
    void generateResultsTooltip();

private:
    double m_rotation_angle;
    Antenna *m_antenna;

    QList<RayPath*> m_received_rays;

    double m_received_power;
    double m_user_end_SNR;
    double m_delay_spread;
    double m_rice_factor;

    ResultType::ResultType m_result_type;
    double m_res_min;
    double m_res_max;

    bool m_out_of_model;
    bool m_flat;
    bool m_show_result;

    QMutex m_mutex;
};

// Operator overload to write objects from the Receiver class into a files
QDataStream &operator>>(QDataStream &in, Receiver *&r);
QDataStream &operator<<(QDataStream &out, Receiver *r);


class ReceiversArea : public QGraphicsRectItem, public SimulationItem
{
public:
    ReceiversArea();
    ~ReceiversArea();

    void getReceivedDataBounds(ResultType::ResultType type, double *min, double *max) const;

    QList<Receiver*> getReceiversList() const;
    void setArea(AntennaType::AntennaType type, QRectF area);
    QRectF getArea();

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter *p, const QStyleOptionGraphicsItem *s, QWidget *w) override;

private:
    void createReceivers(AntennaType::AntennaType type, QRectF area);
    void deleteReceivers();

    QList<Receiver*> m_receivers_list;
    QRectF m_area;
};

#endif // RECEIVER_H
