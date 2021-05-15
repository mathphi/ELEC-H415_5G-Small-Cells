#ifndef SIMULATIONSCENE_H
#define SIMULATIONSCENE_H

#include <QObject>
#include <QGraphicsScene>
#include <QGraphicsItem>

#include "simulationitem.h"
#include "receiver.h"

class ScaleRulerItem;
class DataLegendItem;

class SimulationScene : public QGraphicsScene
{
    Q_OBJECT

public:
    SimulationScene(QObject* parent = nullptr);

    static qreal simulationScale();

    QRectF simulationBoundingRect() const;

public slots:
    void viewRectChanged(const QRectF rect, const qreal scale);
    void showDataLegend(ResultType::ResultType type, double min, double max);
    void hideDataLegend();

protected:
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    virtual void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;

    virtual void wheelEvent(QGraphicsSceneWheelEvent *event) override;

    virtual void keyPressEvent(QKeyEvent *event) override;
    virtual void keyReleaseEvent(QKeyEvent *event) override;

signals:
    void mouseLeftPressed(QGraphicsSceneMouseEvent *event);
    void mouseLeftReleased(QGraphicsSceneMouseEvent *events);
    void mouseRightPressed(QGraphicsSceneMouseEvent *event);
    void mouseRightReleased(QGraphicsSceneMouseEvent *event);
    void mouseMoved(QGraphicsSceneMouseEvent *event);
    void mouseDoubleClicked(QGraphicsSceneMouseEvent *event);

    void mouseWheelEvent(QGraphicsSceneWheelEvent *event);

    void keyPressed(QKeyEvent *e);
    void keyReleased(QKeyEvent *e);

private:
    ScaleRulerItem *m_scale_legend;
    DataLegendItem *m_data_legend;
};

#endif // SIMULATIONSCENE_H
