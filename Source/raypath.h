#ifndef RAYPATH_H
#define RAYPATH_H

#include "simulationitem.h"
#include "constants.h"

class Emitter;

class RayPath : public SimulationItem
{
public:
    RayPath(Emitter *em, QList<QLineF> rays, double power);

    Emitter *getEmitter();
    QList<QLineF> getRays();
    double getPower();

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override;

private:
    QLineF getScaledLine(QLineF r) const;

    Emitter *m_emitter;
    QList<QLineF> m_rays;
    double m_power;
};

#endif // RAYPATH_H
