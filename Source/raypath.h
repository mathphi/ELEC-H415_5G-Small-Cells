#ifndef RAYPATH_H
#define RAYPATH_H

#include "simulationitem.h"
#include "constants.h"

class Emitter;
class Receiver;

class RayPath : public SimulationItem
{
public:
    RayPath(Emitter *em, Receiver *rv, QList<QLineF> rays, vector<complex> En, double theta = M_PI_2, bool is_gnd = false);

    Emitter *getEmitter() const;
    Receiver *getReceiver() const;
    QList<QLineF> getRays() const;
    vector<complex> getElectricField() const;
    double getVerticalAngle() const;

    double computePower() const;

    bool isGround() const;

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override;

private:
    QLineF getScaledLine(QLineF r) const;

    Emitter *m_emitter;
    Receiver *m_receiver;
    QList<QLineF> m_rays;
    vector<complex> m_electric_field;
    double m_theta;

    bool m_is_ground;
};

#endif // RAYPATH_H
