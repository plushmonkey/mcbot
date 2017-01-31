#ifndef MCBOT_MATH_H_
#define MCBOT_MATH_H_

#include <mclib/Vector.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_TAU
#define M_TAU M_PI * 2
#endif

inline Vector3d Hadamard(Vector3d first, Vector3d second) {
    return Vector3d(first.x * second.x, first.y * second.y, first.z * second.z);
}

inline double wrapMax(double x, double max) {
    return fmod(max + fmod(x, max), max);
}

inline double wrapMinMax(double x, double min, double max) {
    return min + wrapMax(x - min, max - min);
}

template <typename Engine>
double randomBinomial(Engine engine) {
    std::uniform_real_distribution<double> dist(0, 1);

    return dist(engine) - dist(engine);
}

inline double wrapToPi(double rads) {
    return wrapMinMax(rads, -M_PI, M_PI);
}

#endif
