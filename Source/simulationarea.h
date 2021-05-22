#ifndef SIMULATIONAREA_H
#define SIMULATIONAREA_H

#include "simulationitem.h"
#include "raypath.h"
#include "antennas.h"
#include "receiver.h"


/*
 * This function allows "orderable positions", mendatory to use a position
 * as a QMap key.
 */
bool operator<(const QPoint& p1, const QPoint& p2);


class SimulationArea : public QGraphicsRectItem, public SimulationItem
{
public:
    SimulationArea();
    ~SimulationArea();

    void getReceivedDataBounds(ResultType::ResultType type, double *min, double *max) const;

    QList<Receiver*> getReceiversList() const;
    QMap<QPoint,Receiver*> getReceiversMap() const;
    void setArea(AntennaType::AntennaType type, QRectF area);
    QRectF getArea();
    QRectF getRealArea();

    QList<Emitter*> getPlacedEmitters();
    void addPlacedEmitter(Emitter *e);
    void removePlacedEmitter(Emitter *e);

    bool ignoreInBound() { return true; }

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter *p, const QStyleOptionGraphicsItem *s, QWidget *w) override;

    void deletePlacedEmitters();

private:
    void createReceivers(AntennaType::AntennaType type, QRectF area);
    void deleteReceivers();

    QMap<QPoint,Receiver*> m_receivers_map;
    QRectF m_area;

    QList<Emitter*> m_placed_emitters;
};

#endif // SIMULATIONAREA_H
