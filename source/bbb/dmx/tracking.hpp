#pragma once

#include <cmath>

namespace bbb::dmx {

inline double choose_shortest_pan(double base_pan_degrees, double previous_pan_degrees) {
    const double turn_count{std::round((previous_pan_degrees - base_pan_degrees) / 360.0)};
    return base_pan_degrees + 360.0 * turn_count;
}

} // namespace bbb::dmx
