#pragma once

#include <cmath>

#include "bbb/dmx/math.hpp"

namespace bbb::dmx {

struct pan_tilt_degrees {
public:
    double pan{0.0};
    double tilt{0.0};
};

inline pan_tilt_degrees vector_to_pan_tilt(const vec3 &local_vector) {
    const double pan_radians{std::atan2(local_vector.x, local_vector.y)};
    const double horizontal_distance{std::sqrt(local_vector.x * local_vector.x + local_vector.y * local_vector.y)};
    const double tilt_radians{std::atan2(local_vector.z, horizontal_distance)};
    return pan_tilt_degrees{
        radians_to_degrees(pan_radians),
        radians_to_degrees(tilt_radians),
    };
}

} // namespace bbb::dmx
