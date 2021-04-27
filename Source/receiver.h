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
    Bitrate
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
    void setAntenna(AntennaType::AntennaType type, double efficiency);
    void setAntenna(Antenna *a);

    void setRotation(double angle);
    double getRotation();
    double getIncidentRayAngle(QLineF ray);

    double getEfficiency() const;
    double getResistance() const;
    vector<complex> getEffectiveHeight(double phi, double frequency) const;
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

    double receivedPower();
    double getBitRate();

    void showResults(ResultType::ResultType type, int min, int max);

    void generateIdleTooltip();
    void generateResultsTooltip();

private:
    double m_rotation_angle;
    Antenna *m_antenna;

    QList<RayPath*> m_received_rays;
    double m_received_power;

    ResultType::ResultType m_res_type;
    int m_res_min;
    int m_res_max;

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

    QList<Receiver*> getReceiversList();
    void setArea(AntennaType::AntennaType type, QRectF area);

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter *p, const QStyleOptionGraphicsItem *s, QWidget *w) override;

private:
    void createReceivers(AntennaType::AntennaType type, QRectF area);
    void deleteReceivers();

    QList<Receiver*> m_receivers_list;
};

#endif // RECEIVER_H
