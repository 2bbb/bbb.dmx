#pragma once

#include <array>
#include <cstdint>
#include <string>

#include "bbb/dmx/common.hpp"

namespace bbb::dmx {

struct movertrack_settings {
public:
    vec3 fixture_position{0.0, 0.0, 0.0};
    vec3 rotation_degrees{0.0, 0.0, 0.0};
    double pan_range_degrees{540.0};
    double tilt_range_degrees{270.0};
    double pan_offset_degrees{0.0};
    double tilt_offset_degrees{0.0};
    bool pan_invert{false};
    bool tilt_invert{false};
    byte_order order{byte_order::coarse_fine};
    bool shortest_pan{true};
};

struct movertrack_output {
public:
    std::uint16_t pan{neutral_u16()};
    std::uint16_t tilt{neutral_u16()};
    std::array<int, 4> bytes{127, 255, 127, 255};
    double pan_degrees{0.0};
    double tilt_degrees{0.0};
};

class movertrack_engine {
public:
    movertrack_engine() = default;
    explicit movertrack_engine(const movertrack_settings &settings)
    : settings_{settings}
    {
        settings_.pan_range_degrees = sanitize_positive_range(settings_.pan_range_degrees);
        settings_.tilt_range_degrees = sanitize_positive_range(settings_.tilt_range_degrees);
    }

    const movertrack_settings &settings() const {
        return settings_;
    }

    bool set_fixture_position(const vec3 &position) {
        if(!position.finite()) {
            return false;
        }
        settings_.fixture_position = position;
        return true;
    }

    bool set_rotation_degrees(const vec3 &rotation_degrees) {
        if(!rotation_degrees.finite()) {
            return false;
        }
        settings_.rotation_degrees = rotation_degrees;
        return true;
    }

    bool set_ranges(double pan_range_degrees, double tilt_range_degrees) {
        if(!is_finite(pan_range_degrees) || !is_finite(tilt_range_degrees)) {
            return false;
        }
        settings_.pan_range_degrees = sanitize_positive_range(pan_range_degrees);
        settings_.tilt_range_degrees = sanitize_positive_range(tilt_range_degrees);
        return true;
    }

    bool set_pan_range(double pan_range_degrees) {
        if(!is_finite(pan_range_degrees)) {
            return false;
        }
        settings_.pan_range_degrees = sanitize_positive_range(pan_range_degrees);
        return true;
    }

    bool set_tilt_range(double tilt_range_degrees) {
        if(!is_finite(tilt_range_degrees)) {
            return false;
        }
        settings_.tilt_range_degrees = sanitize_positive_range(tilt_range_degrees);
        return true;
    }

    bool set_pan_offset(double pan_offset_degrees) {
        if(!is_finite(pan_offset_degrees)) {
            return false;
        }
        settings_.pan_offset_degrees = pan_offset_degrees;
        return true;
    }

    bool set_tilt_offset(double tilt_offset_degrees) {
        if(!is_finite(tilt_offset_degrees)) {
            return false;
        }
        settings_.tilt_offset_degrees = tilt_offset_degrees;
        return true;
    }

    void set_pan_invert(bool pan_invert) {
        settings_.pan_invert = pan_invert;
    }

    void set_tilt_invert(bool tilt_invert) {
        settings_.tilt_invert = tilt_invert;
    }

    void set_byte_order(byte_order order) {
        settings_.order = order;
        if(has_last_output_) {
            last_output_.bytes = pan_tilt_to_bytes(last_output_.pan, last_output_.tilt, settings_.order);
        }
    }

    void set_shortest_pan(bool shortest_pan) {
        settings_.shortest_pan = shortest_pan;
    }

    void reset_tracking() {
        has_previous_pan_ = false;
    }

    bool has_last_target() const {
        return has_last_target_;
    }

    bool has_last_output() const {
        return has_last_output_;
    }

    movertrack_output bang() {
        if(has_last_target_) {
            return compute(last_target_);
        }
        return neutral_output();
    }

    movertrack_output compute(const vec3 &target_position) {
        if(!target_position.finite()) {
            return has_last_output_ ? last_output_with_current_order() : neutral_output();
        }

        last_target_ = target_position;
        has_last_target_ = true;

        const vec3 world_vector{target_position - settings_.fixture_position};
        if(world_vector.nearly_zero()) {
            return has_last_output_ ? last_output_with_current_order() : neutral_output();
        }

        const vec3 local_vector{world_to_fixture_local(
            world_vector,
            settings_.rotation_degrees.x,
            settings_.rotation_degrees.y,
            settings_.rotation_degrees.z
        )};

        pan_tilt_degrees angles{vector_to_pan_tilt(local_vector)};
        angles.pan += settings_.pan_offset_degrees;
        angles.tilt += settings_.tilt_offset_degrees;

        if(settings_.pan_invert) {
            angles.pan = -angles.pan;
        }
        if(settings_.tilt_invert) {
            angles.tilt = -angles.tilt;
        }

        if(settings_.shortest_pan && has_previous_pan_) {
            angles.pan = choose_shortest_pan(angles.pan, previous_pan_degrees_);
        }

        angles.pan = clamp_angle_to_range(angles.pan, settings_.pan_range_degrees);
        angles.tilt = clamp_angle_to_range(angles.tilt, settings_.tilt_range_degrees);

        previous_pan_degrees_ = angles.pan;
        has_previous_pan_ = true;

        movertrack_output output{};
        output.pan = angle_to_u16(angles.pan, settings_.pan_range_degrees);
        output.tilt = angle_to_u16(angles.tilt, settings_.tilt_range_degrees);
        output.bytes = pan_tilt_to_bytes(output.pan, output.tilt, settings_.order);
        output.pan_degrees = angles.pan;
        output.tilt_degrees = angles.tilt;

        last_output_ = output;
        has_last_output_ = true;
        return output;
    }

private:
    movertrack_output neutral_output() const {
        movertrack_output output{};
        output.pan = neutral_u16();
        output.tilt = neutral_u16();
        output.bytes = pan_tilt_to_bytes(output.pan, output.tilt, settings_.order);
        output.pan_degrees = 0.0;
        output.tilt_degrees = 0.0;
        return output;
    }

    movertrack_output last_output_with_current_order() const {
        movertrack_output output{last_output_};
        output.bytes = pan_tilt_to_bytes(output.pan, output.tilt, settings_.order);
        return output;
    }

    movertrack_settings settings_{};
    vec3 last_target_{};
    bool has_last_target_{false};
    double previous_pan_degrees_{0.0};
    bool has_previous_pan_{false};
    movertrack_output last_output_{};
    bool has_last_output_{false};
};

} // namespace bbb::dmx
