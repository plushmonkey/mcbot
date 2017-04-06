#ifndef MCBOT_MATH_H_
#define MCBOT_MATH_H_

#include <mclib/common/Vector.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_TAU
#define M_TAU M_PI * 2
#endif

inline mc::Vector3d Hadamard(mc::Vector3d first, mc::Vector3d second) {
    return mc::Vector3d(first.x * second.x, first.y * second.y, first.z * second.z);
}

inline double WrapMax(double x, double max) {
    return fmod(max + fmod(x, max), max);
}

inline double WrapMinMax(double x, double min, double max) {
    return min + WrapMax(x - min, max - min);
}

template <typename Engine>
double RandomBinomial(Engine engine) {
    std::uniform_real_distribution<double> dist(0, 1);

    return dist(engine) - dist(engine);
}

inline double WrapToPi(double rads) {
    return WrapMinMax(rads, -M_PI, M_PI);
}

#endif
