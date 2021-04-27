#include "antennas.h"

#include <QObject>
#include <QDataStream>


Antenna::Antenna(double efficiency) {
    // The default angle for the antenna is PI/2 (incidence to top)
    m_rotation_angle = M_PI_2;

    m_efficiency = efficiency;
}

Antenna::~Antenna() {

}

/**
 * @brief Antenna::setRotation
 * @param angle
 *
 * Sets the rotation angle of the antenna (in radians)
 */
void Antenna::setRotation(double angle) {
    m_rotation_angle = angle;
}

/**
 * @brief Antenna::getRotation
 * @return
 *
 * Get the rotation angle of the antenna (in radians)
 */
double Antenna::getRotation() const {
    return m_rotation_angle;
}

double Antenna::getEfficiency() const {
    return m_efficiency;
}

void Antenna::setEfficiency(double efficiency) {
    m_efficiency = efficiency;
}

Antenna *Antenna::createAntenna(AntennaType::AntennaType type, double efficiency) {
    Antenna *antenna;

    // Create the associated antenna of right type
    switch (type) {
    case AntennaType::HalfWaveDipoleVert: {
        antenna = new HalfWaveDipoleVert(efficiency);
        break;
    }
    case AntennaType::HalfWaveDipoleHoriz: {
        antenna = new HalfWaveDipoleHoriz(efficiency);
        break;
    }
    default:
        // Default antenna is HalfWaveDipoleVert
        antenna = new HalfWaveDipoleVert(efficiency);
        break;
    }

    return antenna;
}

QDataStream &operator>>(QDataStream &in, Antenna *&a) {
    int type;
    double efficiency;

    in >> type;
    in >> efficiency;

    a = Antenna::createAntenna((AntennaType::AntennaType) type, efficiency);

    return in;
}

QDataStream &operator<<(QDataStream &out, Antenna *a) {
    out << a->getAntennaType();
    out << a->getEfficiency();

    return out;
}



HalfWaveDipoleVert::HalfWaveDipoleVert(double efficiency) : Antenna(efficiency)
{

}

AntennaType::AntennaType HalfWaveDipoleVert::getAntennaType() const {
    return AntennaType::HalfWaveDipoleVert;
}

QString HalfWaveDipoleVert::getAntennaName() const {
    return "Dipôle λ/2 Vertical";
}

QString HalfWaveDipoleVert::getAntennaLabel() const {
    return "λ/2";
}

/**
 * @brief HalfWaveDipoleVert::getResistance
 * @return
 *
 * Returns the antenna's resistance
 */
double HalfWaveDipoleVert::getResistance() const {
    // Compute the radiation resistance of the HalfWave dipole (equations 5.48, 5.47, 5.10)
    //double Rar = 6.0 * Z_0 / 32.0;

    // The exact result for radiation resistance is 73Ω
    double Rar = 73;

    // The total resistance is the radiation resistance divided by the
    // efficiency (equations 5.13, 5.11)
    return Rar / getEfficiency();
}

/**
 * @brief HalfWaveDipoleVert::getGain
 * @param theta
 * @param phi
 * @return
 *
 * Returns the gain of the dipole at the given incidents angles
 */
double HalfWaveDipoleVert::getGain(double theta, double phi) const {
    Q_UNUSED(phi);

    // This function equals 0 for theta == 0, but avoid the 0/0 situation
    if (theta == 0) {
        return 0;
    }

    // Get the efficiency
    double eta = getEfficiency();

    // Compute the gain (equations 5.44, 5.24, 5.22)
    return eta * 16.0/(3*M_PI) * pow(cos(M_PI_2 * cos(theta)) / sin(theta), 2);
}

/**
 * @brief HalfWaveDipoleVert::getEffectiveHeight
 * @param theta
 * @param phi
 * @param frequency
 * @return
 *
 * Returns the effective height of the dipole at the given incidents angles.
 * The 'frequency' defines the design of the antenna (wave length)
 */
vector<complex> HalfWaveDipoleVert::getEffectiveHeight(
        double theta,
        double phi,
        double frequency) const
{
    Q_UNUSED(phi);

    // This function equals 0 for theta == 0, but avoid the 0/0 situation
    if (theta == 0) {
        return {0, 0, 0};
    }

    // Compute the wave length
    double lambda = LIGHT_SPEED / frequency;

    return {
        0,
        0,
        -lambda/M_PI * cos(M_PI_2 * cos(theta))/pow(sin(theta),2)
    };
}

/**
 * @brief HalfWaveDipoleVert::getPolarization
 * @return
 *
 * Returns the vector describing the polarization.
 * The first component is the parallel, the second is the orthogonal.
 */
vector<complex> HalfWaveDipoleVert::getPolarization() const {
    return {0, 1};
}


///////////////////////////////////////////////////////////////////////////////////

HalfWaveDipoleHoriz::HalfWaveDipoleHoriz(double efficiency) : Antenna(efficiency)
{

}

AntennaType::AntennaType HalfWaveDipoleHoriz::getAntennaType() const {
    return AntennaType::HalfWaveDipoleHoriz;
}

QString HalfWaveDipoleHoriz::getAntennaName() const {
    return "Dipôle λ/2 Horizontal";
}

QString HalfWaveDipoleHoriz::getAntennaLabel() const {
    return "λ/2";
}

/**
 * @brief HalfWaveDipoleVert::getResistance
 * @return
 *
 * Returns the antenna's resistance
 */
double HalfWaveDipoleHoriz::getResistance() const {
    // Compute the radiation resistance of the HalfWave dipole (equations 5.48, 5.47, 5.10)
    //double Rar = 6.0 * Z_0 / 32.0;

    // The exact result for radiation resistance is 73Ω
    double Rar = 73;

    // The total resistance is the radiation resistance divided by the
    // efficiency (equations 5.13, 5.11)
    return Rar / getEfficiency();
}

/**
 * @brief HalfWaveDipoleHoriz::getGain
 * @param theta
 * @param phi
 * @return
 *
 * Returns the gain of the dipole at the given incidents angles
 */
double HalfWaveDipoleHoriz::getGain(double theta, double phi) const {
    Q_UNUSED(theta);

    // This function equals 0 for theta == 0, but avoid the 0/0 situation
    if (phi == 0) {
        return 0;
    }

    // Get the efficiency
    double eta = getEfficiency();

    // Compute the gain (equations 5.44, 5.24, 5.22)
    return eta * 16.0/(3*M_PI) * pow(cos(M_PI_2 * cos(phi)) / sin(phi), 2);
}

/**
 * @brief HalfWaveDipoleHoriz::getEffectiveHeight
 * @param theta
 * @param phi
 * @param frequency
 * @return
 *
 * Returns the effective height of the dipole at the given incidents angles.
 * The 'frequency' defines the design of the antenna (wave length)
 *
 * WARNING: the y axis is upside down in the graphics scene !
 */
vector<complex> HalfWaveDipoleHoriz::getEffectiveHeight(
        double theta,
        double phi,
        double frequency) const
{
    Q_UNUSED(theta);

    // This function equals 0 for theta == 0, but avoid the 0/0 situation
    if (phi == 0) {
        return {0, 0, 0};
    }

    // Compute the wave length
    double lambda = LIGHT_SPEED / frequency;

    // Compute the effective height (equation 5.42)
    complex he = -lambda/M_PI * cos(M_PI_2 * cos(phi))/pow(sin(phi),2);

    return {
        cos(getRotation()) * he,
        -sin(getRotation()) * he,
        0
    };
}

/**
 * @brief HalfWaveDipoleHoriz::getPolarization
 * @return
 *
 * Returns the vector describing the polarization.
 * The first component is the parallel, the second is the orthogonal.
 */
vector<complex> HalfWaveDipoleHoriz::getPolarization() const {
    return {1, 0};
}

