#include "datalegenditem.h"
#include "simulationscene.h"
#include "simulationdata.h"

#include <QPainter>

#define LEGEND_WIDTH 300
#define LEGEND_HEIGHT 40

const QSizeF TEXT_RECT_SIZE(100, 20);

DataLegendItem::DataLegendItem() : QGraphicsItem()
{
    // This item must not scale with the view scale
    setFlag(QGraphicsItem::ItemIgnoresTransformations);

    // This item is above all others
    setZValue(9999999);

    // Default content
    setDataRange(ResultType::Power, -100, 0);
}

void DataLegendItem::setDataRange(ResultType::ResultType type, double min, double max) {
    m_result_type = type;

    double mid = (max+min)/2;

    // Prepare the data strings
    QString units_max, units_min, units_mid;

    switch (type) {
    case ResultType::Power: {
        units_min = "dBm";
        units_mid = "dBm";
        units_max = "dBm";
        min = SimulationData::convertPowerTodBm(min);
        max = SimulationData::convertPowerTodBm(max);

        // Recompute the mid-value in dBm
        mid = (max+min)/2;
        break;
    }
    case ResultType::SNR:
    case ResultType::RiceFactor: {
        units_min = "dB";
        units_mid = "dB";
        units_max = "dB";
        break;
    }
    case ResultType::DelaySpread: {
        min = SimulationData::delayToHumanReadable(min, &units_min);
        mid = SimulationData::delayToHumanReadable(mid, &units_mid);
        max = SimulationData::delayToHumanReadable(max, &units_max);
        break;
    }
    }

    // Round the results
    min = floor(min);
    mid = round(mid);
    max = ceil(max);

    // Store min and max
    m_data_min = min;
    m_data_max = max;

    m_data_start_str = QString("%1 %2").arg(max, 0, 'f', 0).arg(units_max);
    m_data_mid_str = QString("%1 %2").arg(mid, 0, 'f', 0).arg(units_mid);
    m_data_end_str = QString("%1 %2").arg(min, 0, 'f', 0).arg(units_min);

    prepareGeometryChange();
    update();
}

QRectF DataLegendItem::boundingRect() const {
    // Return the rectangle containing the line
    return shape().controlPointRect();
}

QPainterPath DataLegendItem::shape() const {
    QPainterPath path;
    path.addRect(0, -LEGEND_HEIGHT-5, LEGEND_WIDTH+10, 5);
    return path;
}

void DataLegendItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
    // Scale the painter if we are printing in an image (exportation)
    if (dynamic_cast<QImage*>(painter->device())) {
        // Scale by 2 (resolution scale)
        painter->scale(2, 2);
    }

    // Translate the painter (origin on the bottom left)
    painter->translate(0, -LEGEND_HEIGHT-10);

    // Set pen and brush for the background
    painter->setPen(QPen(QBrush(Qt::transparent), 1));
    painter->setBrush(QBrush(Qt::white));
    painter->setOpacity(0.8);

    // Draw the background rectangle
    painter->drawRoundedRect(0, 0, LEGEND_WIDTH, LEGEND_HEIGHT, 5, 5);

    // Rectangle for the gradient
    const QRectF grad_rect(5, 5, LEGEND_WIDTH-10, LEGEND_HEIGHT-20);

    // Get the color points for the gradient
    const QColor color_start = SimulationData::ratioToColor(1.00);
    const QColor color_mid1  = SimulationData::ratioToColor(0.75);
    const QColor color_mid2  = SimulationData::ratioToColor(0.50);
    const QColor color_mid3  = SimulationData::ratioToColor(0.25);
    const QColor color_end   = SimulationData::ratioToColor(0.00);

    // Create the gradient
    QLinearGradient gradient(
                (grad_rect.topLeft() + grad_rect.bottomLeft()) / 2,
                (grad_rect.topRight() + grad_rect.bottomRight()) / 2);

    gradient.setColorAt(0.00, color_start);
    gradient.setColorAt(0.25, color_mid1);
    gradient.setColorAt(0.50, color_mid2);
    gradient.setColorAt(0.75, color_mid3);
    gradient.setColorAt(1.00, color_end);

    // Set pen and brush for the ruler's lines
    painter->setPen(QPen(QBrush(Qt::black), 1, Qt::SolidLine));
    painter->setBrush(QBrush(Qt::transparent));
    painter->setOpacity(1);

    // Draw the gradient rectangle
    painter->fillRect(grad_rect, gradient);
    painter->drawRect(grad_rect);

    // Draw vertical lines at start, middle, end
    painter->drawLine(grad_rect.topLeft(), grad_rect.bottomLeft() + QPointF(0, 10));
    painter->drawLine(
                grad_rect.center().x(),
                grad_rect.top(),
                grad_rect.center().x(),
                grad_rect.bottom() + 10);
    painter->drawLine(grad_rect.topRight(), grad_rect.bottomRight() + QPointF(0, 10));

    const QRectF data_start_rect(grad_rect.adjusted(4,0,0,0).bottomLeft(), TEXT_RECT_SIZE);
    const QRectF data_mid_rect(QPointF(grad_rect.center().x() + 4, grad_rect.bottom()), TEXT_RECT_SIZE);
    const QRectF data_end_rect(grad_rect.bottomRight() - QPointF(TEXT_RECT_SIZE.width() + 4, 0), TEXT_RECT_SIZE);

    // Draw the scale text
    painter->drawText(data_start_rect, Qt::AlignLeft | Qt::AlignTop, m_data_start_str);
    painter->drawText(data_mid_rect, Qt::AlignLeft   | Qt::AlignTop, m_data_mid_str);
    painter->drawText(data_end_rect, Qt::AlignRight  | Qt::AlignTop, m_data_end_str);
}
