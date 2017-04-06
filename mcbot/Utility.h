#ifndef AI_UTILITY_H_
#define AI_UTILITY_H_

#include <mclib/common/Common.h>

#undef min
#undef max

namespace util {

s64 GetTime();
mc::Vector3d OrientationToVector(double orientation);

template <typename T, int Amount>
class Smoother {
private:
    T m_Values[Amount];
    std::size_t m_Index;

public:
    Smoother() : m_Index(0) {
        for (std::size_t i = 0; i < Amount; ++i)
            m_Values[i] = T();
    }

    void AddValue(T value) {
        m_Values[m_Index] = value;
        m_Index = (m_Index + 1) % Amount;
    }

    T GetSmoothedValue() {
        T total = T();

        for (std::size_t i = 0; i < Amount; ++i)
            total = total + m_Values[i];

        return total / Amount;
    }
};

} // ns util

#endif
