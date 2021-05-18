#include "analysisline.h"
#include "receiver.h"
#include "simulationscene.h"
#include "simulationhandler.h"

#include <QPainter>


#define ATTACH_POINT_RADIUS 5.0
#define LINE_THICKNESS      2.5

AnalysisLine::AnalysisLine(QLineF line)
    : SimulationItem()
{
    m_analysis_line = line;
}

AnalysisLine::AnalysisLine(QPointF start_point)
    : AnalysisLine(QLineF(start_point, start_point))
{

}

AnalysisLine::~AnalysisLine() {
    deleteReceivers();
}


/**
 * @brief AnalysisLine::setEndPoint
 * @param end_point
 *
 * This function defines the end-point for the analysis line
 */
void AnalysisLine::setEndPoint(QPointF end_point) {
    prepareGeometryChange();
    m_analysis_line.setP2(end_point);
    update();
}

/**
 * @brief AnalysisLine::deleteReceivers
 *
 * This function deletes all the created receivers
 */
void AnalysisLine::deleteReceivers() {
    // Delete all the created receivers
    foreach(Receiver *r, m_receivers_list) {
        delete r;
    }

    m_receivers_list.clear();
}

/**
 * @brief AnalysisLine::createReceivers
 *
 * This function creates a list of receivers distributed along the line
 */
void AnalysisLine::createReceivers(AntennaType::AntennaType ant_type) {
    // Delete previous receivers (if one)
    deleteReceivers();

    // Get the real total length of the line (in meters)
    double real_length = m_analysis_line.length() / simulationScene()->simulationScale();

    // Compute the fraction of the total length representing one meter
    double dx = 1/real_length;

    // Variable pp = fraction of the total line length
    double pp = 0;

    // Loop until we reach the total line length
    while (pp <= 1.0) {
        // Get the point at this fraction of line length
        QPointF r_pos = m_analysis_line.pointAt(pp);

        // Check if this receiver overlaps a building
        bool overlaps_building = false;
        foreach(Building *b, SimulationHandler::simulationData()->getBuildingsList()) {
            // If the position is overlapped by a building -> don't place a receiver
            if (b->getRect().contains(r_pos)) {
                overlaps_building = true;
                break;
            }
        }

        // Don't add a receiver if this point overlaps a building
        if (!overlaps_building) {
            // Place a receiver at each meter on the scene
            Receiver *r = new Receiver(ant_type, 1.0);
            simulationScene()->addItem(r);
            r->setPos(r_pos);
            r->setFlat(true);

            m_receivers_list.append(r);
        }

        // Increase the line length fraction
        pp += dx;
    }
}

/**
 * @brief AnalysisLine::getReceiversList
 * @return
 *
 * This function returns the list of receivers created along the list
 */
QList<Receiver*> AnalysisLine::getReceiversList() {
    return m_receivers_list;
}


QRectF AnalysisLine::boundingRect() const {
    return shape().controlPointRect();
}

QPainterPath AnalysisLine::shape() const {

    QPainterPath path;
    path.moveTo(m_analysis_line.p1());
    path.lineTo(m_analysis_line.p2());
    path.addEllipse(m_analysis_line.p1(), ATTACH_POINT_RADIUS, ATTACH_POINT_RADIUS);
    path.addEllipse(m_analysis_line.p2(), ATTACH_POINT_RADIUS, ATTACH_POINT_RADIUS);

    // Take care of the width of the pen
    QPainterPathStroker ps;
    ps.setWidth(LINE_THICKNESS);

    QPainterPath p = ps.createStroke(path);
    p.addPath(path);
    return p;
}

void AnalysisLine::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
    // Get the painter scale
    qreal scale = painter->transform().m11();

    // Draw the line (blue dashed)
    QPen pen(QColor(0, 0, 255), LINE_THICKNESS / scale, Qt::DashLine);
    pen.setDashPattern({4.0, 4.0});
    painter->setPen(pen);
    painter->drawLine(m_analysis_line);

    // Draw the start-point circle
    painter->setPen(QPen(Qt::black, 1/scale));
    painter->setBrush(QColor(255, 0, 0));
    painter->drawEllipse(m_analysis_line.p1(), ATTACH_POINT_RADIUS/scale, ATTACH_POINT_RADIUS/scale);

    // If the line is completely drawn
    if (!placingMode()) {
        // Draw the end-point circle
        painter->setPen(QPen(Qt::black, 1/scale));
        painter->setBrush(QColor(0, 255, 0));
        painter->drawEllipse(m_analysis_line.p2(), ATTACH_POINT_RADIUS/scale, ATTACH_POINT_RADIUS/scale);
    }
}
