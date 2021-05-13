#include "constants.h"
#include "receiver.h"
#include "simulationscene.h"
#include "simulationdata.h"

#include <QPainter>

#define RECEIVER_AREA_SIZE      (1.0 * simulationScene()->simulationScale())
#define RECEIVER_CROSS_SIZE     (4.0 * simulationScene()->simulationScale())
#define RECEIVER_CIRCLE_SIZE    6 // Size of the circle at the center (in pixels)

Receiver::Receiver(Antenna *antenna) : SimulationItem()
{
    // The default angle for the emitter is PI/2 (incidence to top)
    m_rotation_angle = M_PI_2;

    // Create the associated antenna of right type
    m_antenna = antenna;

    // The receiver is shaped or flat
    m_flat = false;

    // If the results must be shown or not
    m_show_result = false;

    // Default type and range of the result
    m_res_min = 54;
    m_res_max = 433;

    // Over buildings
    setZValue(2000);

    // Initially resetted
    reset();
}

Receiver::Receiver(AntennaType::AntennaType antenna_type, double efficiency)
    : Receiver(Antenna::createAntenna(antenna_type, efficiency))
{
    //TODO: show a default tooltip + a label or polygain to recognize the antenna ?
}

Receiver::~Receiver()
{
    delete m_antenna;
}

/**
 * @brief Receiver::clone
 * @return
 *
 * This function returns a new Receiver with the same properties
 */
Receiver* Receiver::clone() {
    return new Receiver(m_antenna->getAntennaType(), getEfficiency());
}

Antenna *Receiver::getAntenna() {
    return m_antenna;
}

void Receiver::setAntenna(AntennaType::AntennaType type, double efficiency) {
    setAntenna(Antenna::createAntenna(type, efficiency));
}

void Receiver::setAntenna(Antenna *a) {
    if (m_antenna != nullptr) {
        delete m_antenna;
    }

    m_antenna = a;

    // Update the graphics
    prepareGeometryChange();
    update();
}

/**
 * @brief Receiver::setRotation
 * @param angle
 *
 * Sets the rotation angle of the emitter (in radians)
 */
void Receiver::setRotation(double angle) {
    m_rotation_angle = angle;
}

/**
 * @brief Receiver::getRotation
 * @return
 *
 * Get the rotation angle of the antenna (in radians)
 */
double Receiver::getRotation() const {
    return m_rotation_angle;
}

/**
 * @brief Receiver::getIncidentRayAngle
 * @param ray
 * @return
 *
 * Returns the incidence angle of the ray to the emitter (in radians)
 * This function assumes the ray comes into the emitter.
 */
double Receiver::getIncidentRayAngle(QLineF ray) const {
    double ray_angle = ray.angle() / 180.0 * M_PI - M_PI;
    return ray_angle - getRotation();
}

double Receiver::getEfficiency() const {
    return m_antenna->getEfficiency();
}

double Receiver::getResistance() const {
    return m_antenna->getResistance();
}

vector<complex> Receiver::getEffectiveHeight(double phi, double frequency) const {
    return m_antenna->getEffectiveHeight(M_PI_2, phi, frequency);
}

double Receiver::getGain(double phi) const {
    return m_antenna->getGain(M_PI_2, phi);
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// --------------------------------- RAYS RECEIVING FUNCTIONS ------------------------------------//
////////////////////////////////////////////////////////////////////////////////////////////////////

void Receiver::reset() {
    // Delete all RayPaths from this receiver
    foreach (RayPath *rp, m_received_rays) {
        delete rp;
    }

    m_received_rays.clear();
    m_received_power = -1;

    // Hide the results
    m_show_result = false;

    // Generate the idle tooltip
    generateIdleTooltip();

    // Update graphics
    prepareGeometryChange();
    update();
}

void Receiver::addRayPath(RayPath *rp) {
    // Don't add an invalid RayPath
    if (rp == nullptr)
        return;

    // Lock the mutex to ensure that only one thread write in the list at a time
    m_mutex.lock();

    // Append the new ray path to the list
    m_received_rays.append(rp);

    // Invalidate the previously computed power
    m_received_power = -1;

    // Unlock the mutex to allow others threads to write
    m_mutex.unlock();
}

QList<RayPath*> Receiver::getRayPaths() {
    return m_received_rays;
}

double Receiver::receivedPower() const {
    // If the received power was already computed previously
    if (m_received_power > 0)
        return m_received_power;

    // Implementation of equation 3.51
    complex sum = 0;

    foreach (RayPath *rp, m_received_rays) {
        // Incidence angle of the ray to the receiver (first ray in the list)
        double phi = getIncidentRayAngle(rp->getRays().first());

        // Get the frequency from the emitter
        double frequency = rp->getEmitter()->getFrequency();

        // Get the antenna's resistance and effective height
        vector<complex> he = getEffectiveHeight(phi, frequency);

        // Get the electric field of the incoming ray
        vector<complex> En = rp->getElectricField();

        // Sum inside the square modulus
        sum += dotProduct(he, En);
    }

    double Ra = getResistance();

    // norm() = square of modulus
    return norm(sum) / (8.0 * Ra);

    return m_received_power;
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// ------------------------------------ GRAPHICS FUNCTIONS ---------------------------------------//
////////////////////////////////////////////////////////////////////////////////////////////////////

QRectF Receiver::boundingRect() const {
    if (!m_flat) {
        return QRectF(-RECEIVER_CROSS_SIZE/2 - 1, -RECEIVER_CROSS_SIZE/2 - 1,
                       RECEIVER_CROSS_SIZE + 2,    RECEIVER_CROSS_SIZE + 2);
    }
    else {
        return QRectF(-RECEIVER_AREA_SIZE/2, -RECEIVER_AREA_SIZE/2,
                       RECEIVER_AREA_SIZE,    RECEIVER_AREA_SIZE);
    }
}

QPainterPath Receiver::shape() const {
    QPainterPath path;
    if (!m_flat) {
        path.addRect(-RECEIVER_CROSS_SIZE/2 - 1, -RECEIVER_CROSS_SIZE/2 - 1,
                      RECEIVER_CROSS_SIZE + 2,    RECEIVER_CROSS_SIZE + 2);
    }
    else {
        path.addRect(-RECEIVER_AREA_SIZE/2, -RECEIVER_AREA_SIZE/2,
                      RECEIVER_AREA_SIZE,    RECEIVER_AREA_SIZE);
    }

    return path;
}

void Receiver::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
    if (!m_flat) {
        paintShaped(painter);
    }
    else {
        paintFlat(painter);
    }
}

void Receiver::setFlat(bool flat) {
    m_flat = flat;

    if (flat) {
        setZValue(0);
    }
    else {
        setZValue(2000);
    }
}

void Receiver::paintShaped(QPainter *painter) {
    // Draw a cross on the center of the receiver
    painter->setBrush(Qt::transparent);
    painter->setPen(QPen(QBrush(Qt::black), 2.0, Qt::SolidLine));

    painter->drawLine(-RECEIVER_CROSS_SIZE/2, -RECEIVER_CROSS_SIZE/2,
                       RECEIVER_CROSS_SIZE/2,  RECEIVER_CROSS_SIZE/2);
    painter->drawLine(-RECEIVER_CROSS_SIZE/2,  RECEIVER_CROSS_SIZE/2,
                       RECEIVER_CROSS_SIZE/2, -RECEIVER_CROSS_SIZE/2);

    // Draw a circle on the center of the drawn square
    painter->setPen(QPen(QBrush(Qt::black), 1));
    painter->setBrush(Qt::green);
    painter->drawEllipse(
                -RECEIVER_CIRCLE_SIZE/2,
                -RECEIVER_CIRCLE_SIZE/2,
                RECEIVER_CIRCLE_SIZE,
                RECEIVER_CIRCLE_SIZE);
}

void Receiver::paintFlat(QPainter *painter) {
    // Nothing to paint
    if (!m_show_result) {
        return;
    }

    double data = SimulationData::convertPowerTodBm(receivedPower());

    QColor background_color;

    if (data != 0 && !isinf(data) && !isnan(data)) {
        double data_ratio = (data - m_res_min) / (double)(m_res_max - m_res_min);

        // Use the light color profile
        background_color = SimulationData::ratioToColor(data_ratio, true);
    }
    else {
        // Gray background
        background_color = qRgb(220,220,220);
    }

    painter->fillRect(
                -RECEIVER_AREA_SIZE/2, -RECEIVER_AREA_SIZE/2,
                 RECEIVER_AREA_SIZE,    RECEIVER_AREA_SIZE,
                background_color);
}

void Receiver::showResults(int min, int max) {
    // Result type and range
    m_res_min = min;
    m_res_max = max;

    // Paint the results
    m_show_result = true;

    // Update the tooltip
    generateResultsTooltip();

    // Update graphics
    prepareGeometryChange();
    update();
}

void Receiver::generateIdleTooltip() {
    // Set the tooltip of the receiver
    setToolTip(QString("<b><u>Receiver</u></b><br/>"
                       "<b><i>%1</i></b>")
               .arg(m_antenna->getAntennaName()));
}

void Receiver::generateResultsTooltip() {
    // Set the tooltip of the receiver with
    //  - the number of incident rays
    //  - the received power
    setToolTip(QString("<b><u>Receiver</u></b><br/>"
                       "<b><i>%1</i></b><br/>"
                       "<b>Incident rays&nbsp;:</b> %2<br>"
                       "<b>Power&nbsp;:</b> %3&nbsp;dBm")
               .arg(m_antenna->getAntennaName())
               .arg(getRayPaths().size())
               .arg(SimulationData::convertPowerTodBm(receivedPower()), 0, 'f', 2));
}


QDataStream &operator>>(QDataStream &in, Receiver *&r) {
    Antenna *ant;
    QPoint pos;

    in >> ant;
    in >> pos;

    r = new Receiver(ant);
    r->setPos(pos);

    return in;
}

QDataStream &operator<<(QDataStream &out, Receiver *r) {
    out << r->getAntenna();
    out << r->pos().toPoint();

    return out;
}



ReceiversArea::ReceiversArea() : QGraphicsRectItem(), SimulationItem()
{
    QGraphicsRectItem::setZValue(-10);
}

ReceiversArea::~ReceiversArea() {
    deleteReceivers();
}

QList<Receiver*> ReceiversArea::getReceiversList() {
    return m_receivers_list;
}

void ReceiversArea::setArea(AntennaType::AntennaType type, QRectF area) {
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

void ReceiversArea::createReceivers(AntennaType::AntennaType type, QRectF area) {
    if (!simulationScene())
        return;

    // Get the count of receivers in each dimension
    QSize num_rcv = (area.size() / simulationScene()->simulationScale()).toSize();

    // Get the initial position of the receivers
    QPointF init_pos = area.topLeft() + QPointF(RECEIVER_AREA_SIZE/2, RECEIVER_AREA_SIZE/2);

    // Add a receiver to each m² on the area
    for (int x = 0 ; x < num_rcv.width() ; x++) {
        for (int y = 0 ; y < num_rcv.height() ; y++) {
            QPointF delta_pos(x * RECEIVER_AREA_SIZE, y * RECEIVER_AREA_SIZE);
            QPointF rcv_pos = init_pos + delta_pos;

            bool overlap_building = false;

            // Check if this receiver overlaps a building
            foreach(QGraphicsItem *item, simulationScene()->items(rcv_pos)) {
                // If the overlapped item is a building -> don't place a receiver
                if (dynamic_cast<Building*>(item)) {
                    overlap_building = true;
                    break;;
                }
            }

            // Skip this position if overlapping with building
            if (overlap_building)
                continue;

            Receiver *rcv = new Receiver(type, 1.0);
            simulationScene()->addItem(rcv);

            rcv->setFlat(true);
            rcv->setPos(rcv_pos);

            m_receivers_list.append(rcv);
        }
    }
}

void ReceiversArea::deleteReceivers() {
    foreach(Receiver *r, m_receivers_list) {
        delete r;
    }

    m_receivers_list.clear();
}


QRectF ReceiversArea::boundingRect() const {
    return QGraphicsRectItem::boundingRect();
}

QPainterPath ReceiversArea::shape() const {
    return QGraphicsRectItem::shape();
}

void ReceiversArea::paint(QPainter *p, const QStyleOptionGraphicsItem *s, QWidget *w) {
    QGraphicsRectItem::paint(p, s, w);
}
