#include "scaleruleritem.h"
#include "simulationscene.h"

#include <QPainter>

#define LEGEND_WIDTH 140
#define LEGEND_HEIGHT 30

const QRectF TEXT_RECT(15, LEGEND_HEIGHT/4.0, LEGEND_WIDTH-30, LEGEND_HEIGHT/2.0);

ScaleRulerItem::ScaleRulerItem() : QGraphicsItem()
{
    // This item must not scale with the view scale
    setFlag(QGraphicsItem::ItemIgnoresTransformations);

    // This item is above all others
    setZValue(9999999);
}

void ScaleRulerItem::viewScaleChanged(qreal scale) {
    m_view_scale = scale;
}

QRectF ScaleRulerItem::boundingRect() const {
    // Return the rectangle containing the line
    return shape().controlPointRect();
}

QPainterPath ScaleRulerItem::shape() const {
    QPainterPath path;
    path.addRect(-LEGEND_WIDTH-10, -LEGEND_HEIGHT-10, 10, 10);
    return path;
}

void ScaleRulerItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
    // Scale the painter if we are printing in an image (exportation)
    if (dynamic_cast<QImage*>(painter->device())) {
        // Scale by 2 (resolution scale)
        painter->scale(2, 2);
    }

    // Translate the painter (origin on the bottom right)
    painter->translate(-LEGEND_WIDTH-10, -LEGEND_HEIGHT-10);

    // Set pen and brush for the background
    painter->setPen(QPen(QBrush(Qt::transparent), 1));
    painter->setBrush(QBrush(Qt::white));
    painter->setOpacity(0.8);

    // Draw the background rectangle
    painter->drawRoundedRect(0, 0, LEGEND_WIDTH, LEGEND_HEIGHT, 10, 10);

    // Set pen and brush for the ruler's lines
    painter->setPen(QPen(QBrush(Qt::black), 2, Qt::DashDotLine));
    painter->setBrush(QBrush(Qt::transparent));
    painter->setOpacity(1);

    // Get the simulation scale (pixels per meter)
    const qreal sim_scale = getSimulationScale();

    // Get the best bar width and measure
    qreal bar_width = 0;
    qreal measure = 0;
    while (bar_width < LEGEND_WIDTH*0.4)
    {
        if (measure < 1) {
            measure += 0.20;
        }
        else {
            measure += 1;
        }

        bar_width = measure * sim_scale * m_view_scale;
    }

    // Draw the scale ruler
    painter->drawLine((LEGEND_WIDTH-10)-bar_width, LEGEND_HEIGHT/4.0, LEGEND_WIDTH-10, LEGEND_HEIGHT/4.0);
    painter->drawLine((LEGEND_WIDTH-10)-bar_width, LEGEND_HEIGHT/4.0, (LEGEND_WIDTH-10)-bar_width, LEGEND_HEIGHT*3/4.0);
    painter->drawLine(LEGEND_WIDTH-10, LEGEND_HEIGHT/4.0, LEGEND_WIDTH-10, LEGEND_HEIGHT*3/4.0);

    // Draw the scale text
    const QString measure_str = QString("%1 m").arg(measure);
    painter->drawText(TEXT_RECT, Qt::AlignRight | Qt::AlignTop, measure_str);
}

qreal ScaleRulerItem::getSimulationScale() {
    SimulationScene * sim_scene = dynamic_cast<SimulationScene*>(scene());

    if (sim_scene) {
        return sim_scene->simulationScale();
    } else {
        return 1.0;
    }
}
