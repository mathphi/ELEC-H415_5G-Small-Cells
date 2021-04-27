#ifndef SIMULATIONITEM_H
#define SIMULATIONITEM_H

#include <QGraphicsItem>

class SimulationScene;

class SimulationItem : public QGraphicsItem
{
public:
    SimulationItem();

    bool placingMode() const;
    void setPlacingMode(bool on);

    QPointF getRealPos();

    SimulationScene *simulationScene() const;

private:
    bool m_placing_mode;
};

#endif // SIMULATIONITEM_H
