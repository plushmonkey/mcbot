#include "Utility.h"

#include <chrono>

using mc::Vector3d;

namespace util {

s64 GetTime() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}

Vector3d OrientationToVector(double orientation) {
    double pitch = 0.0;

    return Vector3d(
        -std::cos(pitch) * std::sin(orientation),
        -std::sin(pitch),
        std::cos(pitch) * std::cos(orientation)
    );
}

} // ns util
