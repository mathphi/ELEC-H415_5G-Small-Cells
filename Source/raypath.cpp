#include "raypath.h"
#include "simulationscene.h"
#include "simulationdata.h"

#include <QPainter>

#define PEN_WIDTH 1

RayPath::RayPath(Emitter *em, Receiver *rv, QList<QLineF> rays, vector<complex> En) : SimulationItem()
{
    m_emitter = em;
    m_receiver = rv;
    m_rays = rays;
    m_electric_field = En;

    // Under the buildings
    setZValue(500);
}

Emitter *RayPath::getEmitter() const {
    return m_emitter;
}

Receiver *RayPath::getReceiver() const {
    return m_receiver;
}

QList<QLineF> RayPath::getRays() const {
    return m_rays;
}

vector<complex> RayPath::getElectricField() const {
    return m_electric_field;
}

/**
 * @brief RayPath::computePower
 * @return
 *
 * This function computes the power of this ray path
 * (equation 3.51, applyed to one ray)
 *
 */
double RayPath::computePower() const {
    // Incidence angle of the ray to the receiver (first ray in the list)
    double phi = m_receiver->getIncidentRayAngle(m_rays.first());

    // Get the frequency from the emitter
    double frequency = m_emitter->getFrequency();

    // Get the antenna's resistance and effective height
    double Ra = m_receiver->getResistance();
    vector<complex> he = m_receiver->getEffectiveHeight(phi, frequency);

    // norm() = square of modulus
    return norm(dotProduct(he, m_electric_field)) / (8.0 * Ra);
}


QLineF RayPath::getScaledLine(QLineF r) const {
    const qreal sim_scale = simulationScene()->simulationScale();
    return QLineF(r.p1() * sim_scale, r.p2() * sim_scale);
}

QRectF RayPath::boundingRect() const {
    // Return the rectangle containing all the ray lines
    return shape().controlPointRect();
}

QPainterPath RayPath::shape() const {
    QPainterPath path;

    // Each ray is a line
    foreach (QLineF ray, m_rays) {
        QLineF l = getScaledLine(ray);
        path.moveTo(l.p1());
        path.lineTo(l.p2());
    }

    // Take care of the width of the pen
    QPainterPathStroker ps;
    ps.setWidth(PEN_WIDTH);

    QPainterPath p = ps.createStroke(path);
    p.addPath(path);
    return p;
}

void RayPath::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
    // Get the pen color (function of the power)
    const double dbm_power = SimulationData::convertPowerTodBm(computePower());
    //TODO: clean this colour computation...
    const QColor pen_color(SimulationData::ratioToColor(1.0 - (dbm_power+60)/-100.0));

    // Set the pen for this raypath
    painter->setPen(QPen(pen_color, PEN_WIDTH));

    // Draw each ray as a line
    for (int i = 0 ; i < m_rays.size() ; i++) {
        QLineF ray = m_rays[i];

        painter->drawLine(getScaledLine(ray));
    }
}
