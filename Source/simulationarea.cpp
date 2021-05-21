#include "simulationarea.h"
#include "simulationscene.h"
#include "simulationhandler.h"

#include <QApplication>

#define RECEIVER_AREA_SIZE (1.0 * simulationScene()->simulationScale())

/*
 * This function allows "orderable positions", mendatory to use a position
 * as a QMap key.
 */
bool operator<(const QPoint& p1, const QPoint& p2) {
    if (p1.y() < p2.y()) {
        return true;
    }
    else if (p1.y() == p2.y() && p1.x() < p2.x()) {
        return true;
    }
    else {
        return false;
    }
}


SimulationArea::SimulationArea() : QGraphicsRectItem(), SimulationItem()
{
    QGraphicsRectItem::setZValue(-10);
}

SimulationArea::~SimulationArea() {
    deleteReceivers();
    deletePlacedEmitters();
}

void SimulationArea::getReceivedDataBounds(ResultType::ResultType type, double *min, double *max) const {
    double val;

    // Initialize max and min to extreme values
    *min = qInf();
    *max = -qInf();

    foreach(Receiver *r, m_receivers_map) {
        switch (type) {
        case ResultType::Power:
            val = r->receivedPower();

            // Ignore zero-powers
            if (val == 0)
                continue;

            break;
        case ResultType::CoverageMap:
        case ResultType::SNR:
            val = r->userEndSNR();
            break;
        case ResultType::DelaySpread:
            val = r->delaySpread();
            break;
        case ResultType::RiceFactor:
            val = r->riceFactor();
            break;
        }

        // Ignore non-numeric values
        if (isnan(val) || isinf(val))
            continue;

        if (val > *max) {
            *max = val;
        }
        else if (val < *min) {
            *min = val;
        }
    }
}

QList<Receiver*> SimulationArea::getReceiversList() const {
    return m_receivers_map.values();
}

QMap<QPoint,Receiver*> SimulationArea::getReceiversMap() const {
    return m_receivers_map;
}

void SimulationArea::setArea(AntennaType::AntennaType type, QRectF area) {
    // Store the given area
    m_area = area;

    // Compute the area as a rect of size multiple of 1m²
    qreal sim_scale = simulationScene()->simulationScale();

    // Compute the 1m² fitted rect
    QSizeF fit_size(round(area.width() / sim_scale) * sim_scale,
                    round(area.height() / sim_scale) * sim_scale);

    // Center the content in the area
    QSizeF diff_sz = fit_size - area.size();
    QRectF fit_area = area.adjusted(-diff_sz.width()/2, -diff_sz.height()/2,
                                     diff_sz.width()/2,  diff_sz.height()/2);

    // Draw the area rectangle
    setPen(QPen(Qt::darkGray, 1, Qt::DashDotDotLine));
    setBrush(QBrush(qRgba(225, 225, 255, 255), Qt::DiagCrossPattern));
    QGraphicsRectItem::setRect(fit_area);

    // Delete and recreate the receivers list
    deleteReceivers();
    createReceivers(type, fit_area);
}

QRectF SimulationArea::getArea() {
    return m_area;
}

QRectF SimulationArea::getRealArea() {
    return QRectF(
                m_area.topLeft()    / SimulationScene::simulationScale(),
                m_area.size()       / SimulationScene::simulationScale()
            );
}

void SimulationArea::addPlacedEmitter(Emitter *e) {
    m_placed_emitters.append(e);

    if (simulationScene()) {
        simulationScene()->addItem(e);
    }
}

void SimulationArea::createReceivers(AntennaType::AntennaType type, QRectF area) {
    if (!simulationScene())
        return;

    // Get the count of receivers in each dimension
    QSize num_rcv = (area.size() / simulationScene()->simulationScale()).toSize();

    // Get the initial position of the receivers
    QPointF init_pos = area.topLeft() + QPointF(RECEIVER_AREA_SIZE/2, RECEIVER_AREA_SIZE/2);

    // Add a receiver to each m² on the area
    for (int y = 0 ; y < num_rcv.height() ; y++) {
        for (int x = 0 ; x < num_rcv.width() ; x++) {
            QPointF delta_pos(x * RECEIVER_AREA_SIZE, y * RECEIVER_AREA_SIZE);
            QPointF rcv_pos = init_pos + delta_pos;

            bool overlap_building = false;

            // Check if this receiver overlaps a building
            foreach(Building *b, SimulationHandler::simulationData()->getBuildingsList()) {
                // If the position is overlapped by a building -> don't place a receiver
                if (b->getRect().contains(rcv_pos)) {
                    // Go to the end of this building
                    x = (b->getRect().right() - init_pos.x()) / simulationScene()->simulationScale();
                    overlap_building = true;
                    break;
                }
            }

            // Skip this position if overlapping with building
            if (overlap_building)
                continue;

            Receiver *rcv = new Receiver(type, 1.0);
            simulationScene()->addItem(rcv);

            rcv->setFlat(true);
            rcv->setPos(rcv_pos);

            m_receivers_map.insert(QPoint(x,y), rcv);
        }

        // Avoid freezing the UI
        qApp->processEvents();
    }
}

void SimulationArea::deleteReceivers() {
    foreach(Receiver *r, m_receivers_map) {
        delete r;
    }

    m_receivers_map.clear();
}

void SimulationArea::deletePlacedEmitters() {
    foreach(Emitter *e, m_placed_emitters) {
        delete e;
    }

    m_placed_emitters.clear();
}


QRectF SimulationArea::boundingRect() const {
    return QGraphicsRectItem::boundingRect();
}

QPainterPath SimulationArea::shape() const {
    return QGraphicsRectItem::shape();
}

void SimulationArea::paint(QPainter *p, const QStyleOptionGraphicsItem *s, QWidget *w) {
    QGraphicsRectItem::paint(p, s, w);
}

