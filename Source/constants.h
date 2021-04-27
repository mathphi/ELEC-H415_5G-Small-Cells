#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <math.h>
#include <complex>
#include <vector>

// Avoid the std:: before each of these keywords
using std::vector;
using std::min;
using std::max;

// Define the type 'complex' as 'std::complex<double>'
typedef std::complex<double> complex;

// Needed to avoid a confusion with 1i implementations
using namespace std::literals::complex_literals;

// Pi definition (Windows compatibility)
#ifndef M_PI
#define M_PI    3.14159265358979323846  // pi
#define M_PI_2  1.57079632679489661923  // pi/2
#endif

// Universal constants
const double LIGHT_SPEED = 299792458.0;     // [m/s]
const double EPSILON_0   = 8.85418782e-12;  // [F/m]
const double MU_0        = 12.566370614e-7; // [T m/A]

// Vacuum impedance
const double Z_0 = sqrt(MU_0/EPSILON_0);  // [Ohm]

// Operator overload for vector component to component multiplication.
// This is only valid for 3-dimensionnal vectors.
vector<complex> operator*(const vector<complex> v1, const vector<complex> v2) ;

// Operator overload for vector component to component multiplication.
// This is only valid for 3-dimensionnal vectors.
vector<complex> operator*=(vector<complex> &v1, const vector<complex> v2) ;

// Dot product basic function. Applicable on two 3-dimensional vectors.
complex dotProduct(const vector<complex> v1, const vector<complex> v2);


#endif // CONSTANTS_H
