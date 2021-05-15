#ifndef DATALEGENDITEM_H
#define DATALEGENDITEM_H

#include <QGraphicsItem>
#include "receiver.h"

class DataLegendItem : public QGraphicsItem
{
public:
    DataLegendItem();

    QRectF boundingRect() const override;
    QPainterPath shape() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override;

public slots:
    void setDataRange(ResultType::ResultType type, double min, double max);

private:
    ResultType::ResultType m_result_type;
    int m_data_min;
    int m_data_max;

    QString m_data_start_str;
    QString m_data_mid_str;
    QString m_data_end_str;
};

#endif // DATALEGENDITEM_H
