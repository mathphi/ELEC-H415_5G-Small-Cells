#include "emitter.h"
#include "constants.h"
#include "simulationdata.h"

#include <QPainter>

#define EMITTER_WIDTH 8
#define EMITTER_HEIGHT 20
#define EMITTER_TEXT_WIDTH 24
#define EMITTER_TEXT_HEIGHT 20

#define EMITTER_POLYGAIN_SIZE 9.0


// Defines the rectangle where to place the emitter's label
const QRectF TEXT_RECT(
        -EMITTER_TEXT_WIDTH/2,
        -EMITTER_HEIGHT - EMITTER_TEXT_HEIGHT,
        EMITTER_TEXT_WIDTH,
        EMITTER_TEXT_HEIGHT);

Emitter::Emitter(double frequency,
        double eirp,
        Antenna *antenna) : SimulationItem()
{
    // Over receivers and buildings
    setZValue(5000);

    m_frequency  = frequency;
    m_antenna    = antenna;
    m_eirp       = eirp;

    // Setup the tooltip
    updateTooltip();
}

Emitter::Emitter(
        double frequency,
        double eirp,
        double efficiency,
        AntennaType::AntennaType antenna_type)
    : Emitter(
          frequency,
          eirp,
          Antenna::createAntenna(antenna_type, efficiency))
{
    // Create the associated antenna of right type
}

Emitter::~Emitter()
{
    delete m_antenna;
}

/**
 * @brief Emitter::clone
 * @return
 *
 * This function returns a new Emitter with the same properties
 */
Emitter* Emitter::clone() {
    return new Emitter(getFrequency(), getPower(), getEfficiency(), m_antenna->getAntennaType());
}

/**
 * @brief Emitter::setRotation
 * @param angle
 *
 * Sets the rotation angle of the emitter (in radians)
 */
void Emitter::setRotation(double angle) {
    m_antenna->setRotation(angle);
}

/**
 * @brief Emitter::getRotation
 * @return
 *
 * Get the rotation angle of the antenna (in radians)
 */
double Emitter::getRotation() const {
    return m_antenna->getRotation();
}

/**
 * @brief Emitter::getIncidentRayAngle
 * @param ray
 * @return
 *
 * Returns the incidence angle of the ray to the emitter (in radians).
 * This function assumes the ray comes out the emitter.
 */
double Emitter::getIncidentRayAngle(QLineF ray) {
    double ray_angle = ray.angle() / 180.0 * M_PI;
    return ray_angle - getRotation();
}


void Emitter::setAntenna(AntennaType::AntennaType type, double efficiency) {
    setAntenna(Antenna::createAntenna(type, efficiency));
}

void Emitter::setAntenna(Antenna *a) {
    if (m_antenna != nullptr) {
        delete m_antenna;
    }

    m_antenna = a;

    // Update the tooltip
    updateTooltip();

    // Update the graphics
    prepareGeometryChange();
    update();
}

void Emitter::setFrequency(double freq) {
    m_frequency = freq;

    // Update the tooltip
    updateTooltip();
}

void Emitter::setPower(double power) {
    m_eirp = power * m_antenna->getGainMax();
}

void Emitter::setEIRP(double eirp) {
    m_eirp = eirp;

    // Update the tooltip
    updateTooltip();
}

Antenna *Emitter::getAntenna() {
    return m_antenna;
}

double Emitter::getFrequency() const {
    return m_frequency;
}

double Emitter::getPower() const {
    return m_eirp / m_antenna->getGainMax();     //Aassuming L_TX = 1 (no loss in antenna's wires)
}

double Emitter::getEIRP() const {
    return m_eirp;
}

double Emitter::getEfficiency() const {
    return m_antenna->getEfficiency();
}

double Emitter::getResistance() const {
    return m_antenna->getResistance();
}

/**
 * @brief Emitter::getEffectiveHeight
 * @param phi
 * @return
 *
 * Returns the same as getEffectiveHeight(theta, phi), but with the default angle
 * theta to π/2, since the 2D simulation is in the plane θ = π/2
 */
vector<complex> Emitter::getEffectiveHeight(double phi) const {
    return m_antenna->getEffectiveHeight(M_PI_2, phi, m_frequency);
}

/**
 * @brief Emitter::getGain
 * @param phi
 * @return
 *
 * Returns the same as getGain(theta, phi), but with the default angle
 * theta to π/2, since the 2D simulation is in the plane θ = π/2
 */
double Emitter::getGain(double phi) const {
    return m_antenna->getGain(M_PI_2, phi);
}

/**
 * @brief Emitter::getPolarization
 * @return
 *
 * Returns the antenna's polarization vector
 */
vector<complex> Emitter::getPolarization() const{
    return m_antenna->getPolarization();
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// -------------------------------------- GRAPHICS FUNCTIONS ------------------------------------ //
////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Emitter::updateTooltip
 *
 * This function updates the tooltip of the emitter
 */
void Emitter::updateTooltip() {
    // Setup the tooltip with all emitter's info
    QString tip("<b><u>Emitter</u></b><br/>"
                "<b><i>%1</i></b><br/>"
                "<b>Frequency:</b> %2 GHz<br/>"
                "<b>EIRP:</b> %3 W<br/>"
                "<b>Power:</b> %4 dBm<br/>"
                "<b>Efficiency:</b> %5%");

    tip = tip.arg(getAntenna()->getAntennaName())
            .arg(getFrequency() * 1e-9, 0, 'f', 2)
            .arg(getEIRP(), 0, 'f', 2)
            .arg(SimulationData::convertPowerTodBm(getPower()), 0, 'f', 2)
            .arg(getEfficiency() * 100.0, 0, 'f', 1);

    setToolTip(tip);
}

/**
 * @brief Emitter::getPolyGain
 * @return
 *
 * This function returns a polygon that represent the gain of the emitter around the phi angle
 */
QPolygonF Emitter::getPolyGain() const {
    QPolygonF poly_gain;
    QPointF pt;

    for (double phi = -M_PI ; phi < M_PI + 0.1 ; phi += 0.1) {
        pt = QPointF(cos(phi), sin(phi));
        poly_gain.append(pt * getGain(phi + getRotation()) * EMITTER_POLYGAIN_SIZE);
    }

    return poly_gain;
}

QRectF Emitter::boundingRect() const {
    QRectF emitter_rect(-EMITTER_WIDTH/2 - 2, -EMITTER_HEIGHT - 2,
                        EMITTER_WIDTH + 4, EMITTER_HEIGHT + 4);

    QRectF gain_rect = getPolyGain().boundingRect();

    // Bounding rect contains the emitter and his text
    return emitter_rect.united(gain_rect).united(TEXT_RECT);
}

QPainterPath Emitter::shape() const {
    QPainterPath path;
    path.addRect(-EMITTER_WIDTH/2 - 4, -EMITTER_HEIGHT - 4,
                 EMITTER_WIDTH + 8, EMITTER_HEIGHT + 8);
    path.addRect(TEXT_RECT);
    path.addPolygon(getPolyGain());
    return path;
}

void Emitter::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) {
    // Draw the shape of the gain in blue
    painter->setPen(QPen(QBrush(Qt::blue), 1));
    painter->setBrush(Qt::transparent);
    painter->drawPolygon(getPolyGain());

    // Draw a circle over a line, with the origin at the end of the line
    painter->setPen(QPen(QBrush(Qt::black), 1));
    painter->setBrush(Qt::red);
    painter->drawEllipse(-EMITTER_WIDTH/2, -EMITTER_HEIGHT, EMITTER_WIDTH, EMITTER_WIDTH);
    painter->drawLine(0, 0, 0, -(EMITTER_HEIGHT - EMITTER_WIDTH));

    // Draw the label of the emitter
    painter->drawText(TEXT_RECT, Qt::AlignHCenter | Qt::AlignTop, m_antenna->getAntennaLabel());
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// -------------------------------- DATA SERIALIZATION FUNCTIONS -------------------------------- //
////////////////////////////////////////////////////////////////////////////////////////////////////

QDataStream &operator>>(QDataStream &in, Emitter *&e) {
    Antenna *ant;
    double power;
    double frequency;
    double rotation;
    QPoint pos;

    in >> ant;
    in >> power;
    in >> frequency;
    in >> rotation;
    in >> pos;

    e = new Emitter(frequency, power, ant);
    e->setRotation(rotation);
    e->setPos(pos);

    return in;
}

QDataStream &operator<<(QDataStream &out, Emitter *e) {
    out << e->getAntenna();
    out << e->getPower();
    out << e->getFrequency();
    out << e->getRotation();
    out << e->pos().toPoint();

    return out;
}
