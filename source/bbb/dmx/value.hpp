#pragma once

#include <array>
#include <cstdint>
#include <cmath>
#include <string>

#include "bbb/dmx/math.hpp"

namespace bbb::dmx {

enum class byte_order {
    coarse_fine,
    fine_coarse,
};

struct split_u16 {
public:
    std::uint8_t coarse{0};
    std::uint8_t fine{0};
};

inline std::uint16_t angle_to_u16(double degrees, double range_degrees) {
    const double sanitized_range{sanitize_positive_range(range_degrees)};
    const double clamped_degrees{clamp_angle_to_range(degrees, sanitized_range)};
    const double half_range{sanitized_range * 0.5};
    const double normalized{(clamped_degrees + half_range) / sanitized_range};
    int value{(int)std::round(normalized * 65535.0)};
    value = std::max(0, std::min(65535, value));
    return (std::uint16_t)value;
}

inline double u16_to_angle(std::uint16_t value, double range_degrees) {
    const double sanitized_range{sanitize_positive_range(range_degrees)};
    const double half_range{sanitized_range * 0.5};
    const double normalized{(double)value / 65535.0};
    return normalized * sanitized_range - half_range;
}

inline split_u16 split_16(std::uint16_t value) {
    return split_u16{
        (std::uint8_t)(value >> 8),
        (std::uint8_t)(value & 255),
    };
}

inline std::uint16_t combine_16(std::uint8_t first, std::uint8_t second, byte_order order) {
    if(order == byte_order::fine_coarse) {
        return (std::uint16_t)(((std::uint16_t)second << 8) | first);
    }
    return (std::uint16_t)(((std::uint16_t)first << 8) | second);
}

inline std::array<int, 4> pan_tilt_to_bytes(std::uint16_t pan_value, std::uint16_t tilt_value, byte_order order) {
    const split_u16 pan{split_16(pan_value)};
    const split_u16 tilt{split_16(tilt_value)};

    if(order == byte_order::fine_coarse) {
        return {pan.fine, pan.coarse, tilt.fine, tilt.coarse};
    }
    return {pan.coarse, pan.fine, tilt.coarse, tilt.fine};
}

inline std::uint16_t neutral_u16() {
    return 32768;
}

inline bool byte_order_from_string(const std::string &text, byte_order &order) {
    if(text == "coarsefine") {
        order = byte_order::coarse_fine;
        return true;
    }
    if(text == "finecoarse") {
        order = byte_order::fine_coarse;
        return true;
    }
    return false;
}

inline const char *byte_order_to_string(byte_order order) {
    if(order == byte_order::fine_coarse) {
        return "finecoarse";
    }
    return "coarsefine";
}

} // namespace bbb::dmx
