#include "constants.h"


// Operator overload for vector component to component multiplication.
// This is only valid for 3-dimensionnal vectors.
vector<complex> operator*(const vector<complex> v1, const vector<complex> v2) {
    return {
        v1[0] * v2[0],
        v1[1] * v2[1],
        v1[2] * v2[2]
    };
}

// Operator overload for vector component to component multiplication.
// This is only valid for 3-dimensionnal vectors.
vector<complex> operator*=(vector<complex> &v1, const vector<complex> v2) {
    v1 = {
        v1[0] * v2[0],
        v1[1] * v2[1],
        v1[2] * v2[2]
    };
    return v1;
}

// Dot product basic function. Applicable on two 3-dimensional vectors.
complex dotProduct(const vector<complex> v1, const vector<complex> v2) {
    return v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
}
