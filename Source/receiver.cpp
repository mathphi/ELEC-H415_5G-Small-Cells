#include "constants.h"
#include "receiver.h"
#include "simulationscene.h"
#include "simulationdata.h"
#include "simulationhandler.h"

#include <QPainter>
#include <QApplication>

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
    m_result_type = ResultType::Power;
    m_res_min = -100;
    m_res_max = 0;

    // Initialize the computation values
    m_received_power = NAN;
    m_user_end_SNR   = NAN;
    m_delay_spread   = NAN;
    m_rice_factor    = NAN;

    // Over buildings
    setZValue(2000);

    // Initially resetted
    reset();
}

Receiver::Receiver(AntennaType::AntennaType antenna_type, double efficiency)
    : Receiver(Antenna::createAntenna(antenna_type, efficiency))
{

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

vector<complex> Receiver::getEffectiveHeight(double theta, double phi, double frequency) const {
    return m_antenna->getEffectiveHeight(theta, phi, frequency);
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
    m_received_power = NAN;
    m_user_end_SNR   = NAN;
    m_delay_spread   = NAN;
    m_rice_factor    = NAN;

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
    m_received_power = NAN;
    m_user_end_SNR   = NAN;
    m_delay_spread   = NAN;
    m_rice_factor    = NAN;

    // Unlock the mutex to allow others threads to write
    m_mutex.unlock();
}

QList<RayPath*> Receiver::getRayPaths() {
    return m_received_rays;
}

/**
 * @brief Receiver::receivedPower
 * @return
 *
 * This function computes the received power using the equation (3.51)
 */
double Receiver::receivedPower() {
    // If the received power was already computed previously
    if (!isnan(m_received_power))
        return m_received_power;

    // Implementation of equation 3.51
    complex sum = 0;

    foreach (RayPath *rp, m_received_rays) {
        // Incidence angle of the ray to the receiver (first ray in the list)
        double phi = getIncidentRayAngle(rp->getRays().at(0));

        // Get the frequency from the emitter
        double frequency = rp->getEmitter()->getFrequency();

        // Get the antenna's resistance and effective height
        vector<complex> he = getEffectiveHeight(rp->getVerticalAngle(), phi, frequency);

        // Get the electric field of the incoming ray
        vector<complex> En = rp->getElectricField();

        // Sum inside the square modulus
        sum += dotProduct(he, En);
    }

    double Ra = getResistance();

    // norm() = square of modulus
    m_received_power =  norm(sum) / (8.0 * Ra);

    return m_received_power;
}

/**
 * @brief Receiver::userEndSNR
 * @return
 *
 * This function computes the SNR at user-end (as described in Table 3.3, p.60)
 */
double Receiver::userEndSNR() {
    // If the user-end SNR was already computed previously
    if (!isnan(m_user_end_SNR))
        return m_user_end_SNR;

    const double temperature = SimulationHandler::simulationData()->getSimulationTemperature();
    const double bandwidth = SimulationHandler::simulationData()->getSimulationBandwidth();

    const double therm_noise = 10.0 * log10(K_boltz * temperature * bandwidth / 1e-3);
    const double noise_fig = SimulationHandler::simulationData()->getSimulationNoiseFigure();

    const double noise_floor = therm_noise + noise_fig;
    const double rx_power = SimulationData::convertPowerTodBm(receivedPower());

    // SNR = RX_power [dBm] - Noise_power [dBm]
    m_user_end_SNR = rx_power - noise_floor;

    return m_user_end_SNR;
}

/**
 * @brief Receiver::delaySpread
 * @return
 *
 * This function computes the delay spread (Equation (1.24)).
 * The delay spread is defined if there is only one emitter in the simulation.
 */
double Receiver::delaySpread() {
    // No delay spread if more than one emitter or if less than two rays
    if (SimulationHandler::simulationData()->getEmittersList().size() != 1 ||
            m_received_rays.size() < 2) {
        return NAN;
    }

    // Re-use the previously computed value
    if (!isnan(m_delay_spread))
        return m_delay_spread;

    double delay_i, delay_j;
    double max_delay = 0;

    // Loop over each pair of rays
    for (int i = 0 ; i < m_received_rays.size() ; i++) {
        for (int j = i+1 ; j < m_received_rays.size() ; j++) {
            // Get the transmission delay of the two rays
            delay_i = m_received_rays.at(i)->getDelay();
            delay_j = m_received_rays.at(j)->getDelay();

            // Keep the maximal delay difference
            max_delay = max(max_delay, abs(delay_i - delay_j));
        }
    }

    // Store the computed delay spread for future usage
    m_delay_spread = max_delay;

    return m_delay_spread;
}

/**
 * @brief Receiver::riceFactor
 * @return
 *
 * This function computes the rice factor (Equation (4.18)).
 * The rice factor is defined if there is only one emitter in the simulation.
 */
double Receiver::riceFactor() {
    // No rice factor if more than one emitter or if less than two rays
    if (SimulationHandler::simulationData()->getEmittersList().size() != 1 ||
            m_received_rays.size() < 2) {
        return NAN;
    }

    // Re-use the previously computed value
    if (!isnan(m_rice_factor))
        return m_rice_factor;

    // Initialize to 0
    double los_val_sq = 0;
    double sum_ampl_sq = 0;

    foreach(RayPath *rp, m_received_rays) {
        // One of them may be a LOS
        if (rp->isLOS()) {
            los_val_sq = pow(rp->getAmplitude(), 2);
        }
        else {
            sum_ampl_sq += pow(rp->getAmplitude(), 2);
        }
    }

    // Rice factor in dB
    m_rice_factor = 10*log10(los_val_sq/sum_ampl_sq);

    return m_rice_factor;
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

    double data = 0;

    switch (m_result_type) {
    case ResultType::Power:
        data = SimulationData::convertPowerTodBm(receivedPower());
        break;
    case ResultType::SNR:
        data = userEndSNR();
        break;
    case ResultType::DelaySpread:
        data = delaySpread();
        break;
    case ResultType::RiceFactor:
        data = riceFactor();
        break;
    }

    QColor background_color;

    if (data != 0 && !isinf(data) && !isnan(data)) {
        double data_ratio = (data - m_res_min) / (m_res_max - m_res_min);

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

void Receiver::showResults(ResultType::ResultType type, double min, double max) {
    // Result type
    m_result_type = type;

    // If result is power -> convert watts to dBm
    switch (type) {
    case ResultType::Power: {
        min = floor(SimulationData::convertPowerTodBm(min));
        max = ceil(SimulationData::convertPowerTodBm(max));
        break;
    }
    case ResultType::SNR: {
        min = floor(min);
        max = ceil(max);
        break;
    }
    default:
        break;
    }

    // Store the data range
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
    //  - the UE SNR
    //  - the delay spread (if one)
    //  - the rice factor (if one)

    QString tip_str = QString(
                "<b><u>Receiver</u></b><br/>"
                "<b><i>%1</i></b><br/>"
                "<b>Incident rays:</b> %2<br>"
                "<b>Power:</b> %3&nbsp;dBm<br>"
                "<b>UE SNR:</b> %4&nbsp;dB")
            .arg(m_antenna->getAntennaName())
            .arg(getRayPaths().size())
            .arg(SimulationData::convertPowerTodBm(receivedPower()), 0, 'f', 2)
            .arg(userEndSNR(), 0, 'f', 2);

    double delay_spread = delaySpread();
    double rice_factor = riceFactor();

    if (!isnan(delay_spread)) {
        QString units;
        double hr_ds = SimulationData::delayToHumanReadable(delaySpread(), &units);
        tip_str.append(QString("<br><b>Delay spread: </b>%1&nbsp;%2").arg(hr_ds, 0, 'f', 2).arg(units));
    }
    if (!isnan(rice_factor) && !isinf(rice_factor)) {
        tip_str.append(QString("<br><b>Rice factor: </b>%1&nbsp;dB").arg(rice_factor, 0, 'f', 2));
    }

    setToolTip(tip_str);
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

void ReceiversArea::getReceivedDataBounds(ResultType::ResultType type, double *min, double *max) const {
    double val;

    // Initialize max and min to extreme values
    *min = qInf();
    *max = -qInf();

    foreach(Receiver *r, m_receivers_list) {
        switch (type) {
        case ResultType::Power:
            val = r->receivedPower();

            // Ignore zero-powers
            if (val == 0)
                continue;

            break;
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

QList<Receiver*> ReceiversArea::getReceiversList() const {
    return m_receivers_list;
}

void ReceiversArea::setArea(AntennaType::AntennaType type, QRectF area) {
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

QRectF ReceiversArea::getArea() {
    return m_area;
}

void ReceiversArea::createReceivers(AntennaType::AntennaType type, QRectF area) {
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

            m_receivers_list.append(rcv);
        }

        // Avoid freezing the UI
        qApp->processEvents();
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
