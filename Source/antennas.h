#ifndef ANTENNA_H
#define ANTENNA_H

#include "constants.h"
#include <QString>
#include <vector>

namespace AntennaType {
// Enumeration
enum AntennaType {
    HalfWaveDipoleVert,
    HalfWaveDipoleHoriz,
};

// Iterative list (must contains the same as the enum)
const AntennaType AntennaTypeList[] = {
    HalfWaveDipoleVert,
    HalfWaveDipoleHoriz,
};
}


class Antenna
{
public:
    Antenna(double efficiency = 1.0);
    virtual ~Antenna();

    double getRotation() const;
    void setRotation(double angle);

    double getEfficiency() const;
    void setEfficiency(double efficiency);

    static Antenna *createAntenna(AntennaType::AntennaType type, double efficiency);

    virtual AntennaType::AntennaType getAntennaType() const = 0;
    virtual QString getAntennaName() const = 0;
    virtual QString getAntennaLabel() const = 0;

    virtual double getResistance() const = 0;
    virtual vector<complex> getEffectiveHeight(double theta, double phi, double frequency) const = 0;
    virtual double getGain(double theta, double phi) const = 0;
    virtual vector<complex> getPolarization() const = 0;

private:
    double m_rotation_angle;
    double m_efficiency;
};

class HalfWaveDipoleVert : public Antenna
{
public:
    HalfWaveDipoleVert(double efficiency = 1.0);

    AntennaType::AntennaType getAntennaType() const override;
    QString getAntennaName() const override;
    QString getAntennaLabel() const override;

    double getResistance() const override;
    vector<complex> getEffectiveHeight(double theta, double phi, double frequency) const override;
    double getGain(double theta, double phi) const override;
    vector<complex> getPolarization() const override;

};


class HalfWaveDipoleHoriz : public Antenna
{
public:
    HalfWaveDipoleHoriz(double efficiency = 1.0);

    AntennaType::AntennaType getAntennaType() const override;
    QString getAntennaName() const override;
    QString getAntennaLabel() const override;

    double getResistance() const override;
    vector<complex> getEffectiveHeight(double theta, double phi, double frequency) const override;
    double getGain(double theta, double phi) const override;
    vector<complex> getPolarization() const override;

};


// Operator overload to write objects from the Antenna class into a files
QDataStream &operator>>(QDataStream &in, Antenna *&a);
QDataStream &operator<<(QDataStream &out, Antenna *a);

#endif // ANTENNA_H
