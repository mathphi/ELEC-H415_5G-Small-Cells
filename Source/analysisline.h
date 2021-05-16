#ifndef ANALYSISLINE_H
#define ANALYSISLINE_H

#include "simulationitem.h"
#include "antennas.h"

class Receiver;

class AnalysisLine : public SimulationItem
{
public:
    AnalysisLine(QLineF line);
    AnalysisLine(QPointF start_point);
    ~AnalysisLine();

    void setEndPoint(QPointF end_point);

    void deleteReceivers();
    void createReceivers(AntennaType::AntennaType ant_type);
    QList<Receiver*> getReceiversList();

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override;


private:
    QLineF m_analysis_line;

    QList<Receiver*> m_receivers_list;
};

#endif // ANALYSISLINE_H
