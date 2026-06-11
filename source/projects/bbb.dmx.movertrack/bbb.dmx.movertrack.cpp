#include "c74_min.h"

#include <bbb/dmx/movertrack.hpp>

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>

class bbb_dmx_movertrack : public c74::min::object<bbb_dmx_movertrack> {
private:
    bbb::dmx::movertrack_engine engine_{};
    double fixture_x_value_{0.0};
    double fixture_y_value_{0.0};
    double fixture_z_value_{0.0};
    double rotation_x_value_{0.0};
    double rotation_y_value_{0.0};
    double rotation_z_value_{0.0};
    double pan_range_value_{540.0};
    double tilt_range_value_{270.0};
    double pan_offset_value_{0.0};
    double tilt_offset_value_{0.0};
    bbb::dmx::byte_order byte_order_value_{bbb::dmx::byte_order::coarse_fine};
    bbb::dmx::tracking_mode tracking_mode_value_{bbb::dmx::tracking_mode::smart};
    bool warn_invalid_numeric_{false};
    bool warn_invalid_range_{false};
    bool warn_invalid_byte_order_{false};
    bool warn_invalid_tracking_mode_{false};

public:
    MIN_DESCRIPTION{"Convert a 3D target position to 16-bit moving-light DMX pan/tilt bytes."};
    MIN_TAGS{"dmx, lighting, mover, pan, tilt, tracking"};
    MIN_AUTHOR{"2bit"};
    MIN_RELATED{"bbb.dmx"};

    c74::min::inlet<> input{this, "(list/target/pos/rot/range/bang) target and control input"};
    c74::min::outlet<> output{this, "(list) pan coarse/fine and tilt coarse/fine bytes"};

    c74::min::argument<double> fixture_x_arg{this, "fixture_x", "Fixture absolute X position.",
        MIN_ARGUMENT_FUNCTION {
            fixture_x = arg;
            apply_fixture_position();
        }
    };

    c74::min::argument<double> fixture_y_arg{this, "fixture_y", "Fixture absolute Y position.",
        MIN_ARGUMENT_FUNCTION {
            fixture_y = arg;
            apply_fixture_position();
        }
    };

    c74::min::argument<double> fixture_z_arg{this, "fixture_z", "Fixture absolute Z position.",
        MIN_ARGUMENT_FUNCTION {
            fixture_z = arg;
            apply_fixture_position();
        }
    };

    c74::min::attribute<double> fixture_x{this, "fixture_x", 0.0,
        c74::min::description{"Fixture absolute X position."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(args.empty() || !finite_atom(args[0])) {
                warn_once(warn_invalid_numeric_, "invalid fixture_x ignored");
                return {fixture_x_value_};
            }
            fixture_x_value_ = (double)args[0];
            apply_fixture_position();
            return args;
        }}
    };

    c74::min::attribute<double> fixture_y{this, "fixture_y", 0.0,
        c74::min::description{"Fixture absolute Y position."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(args.empty() || !finite_atom(args[0])) {
                warn_once(warn_invalid_numeric_, "invalid fixture_y ignored");
                return {fixture_y_value_};
            }
            fixture_y_value_ = (double)args[0];
            apply_fixture_position();
            return args;
        }}
    };

    c74::min::attribute<double> fixture_z{this, "fixture_z", 0.0,
        c74::min::description{"Fixture absolute Z position."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(args.empty() || !finite_atom(args[0])) {
                warn_once(warn_invalid_numeric_, "invalid fixture_z ignored");
                return {fixture_z_value_};
            }
            fixture_z_value_ = (double)args[0];
            apply_fixture_position();
            return args;
        }}
    };

    c74::min::attribute<double> pan_range{this, "pan_range", 540.0,
        c74::min::description{"Mechanical or DMX-addressable pan range in degrees."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(args.empty() || !finite_atom(args[0])) {
                warn_once(warn_invalid_numeric_, "invalid pan_range ignored");
                return {pan_range_value_};
            }
            pan_range_value_ = sanitize_range_atom(args[0], "pan_range");
            engine_.set_pan_range(pan_range_value_);
            return {pan_range_value_};
        }}
    };

    c74::min::attribute<double> tilt_range{this, "tilt_range", 270.0,
        c74::min::description{"Mechanical or DMX-addressable tilt range in degrees."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(args.empty() || !finite_atom(args[0])) {
                warn_once(warn_invalid_numeric_, "invalid tilt_range ignored");
                return {tilt_range_value_};
            }
            tilt_range_value_ = sanitize_range_atom(args[0], "tilt_range");
            engine_.set_tilt_range(tilt_range_value_);
            return {tilt_range_value_};
        }}
    };

    c74::min::attribute<c74::min::numbers> rot{this, "rot", c74::min::numbers{0.0, 0.0, 0.0},
        c74::min::description{"Fixture rotation rx ry rz in degrees. Rotation order is Rz * Ry * Rx."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(args.size() < 3 || !finite_atoms(args, 3)) {
                warn_once(warn_invalid_numeric_, "invalid rot ignored");
                return rotation_atoms();
            }
            rotation_x_value_ = (double)args[0];
            rotation_y_value_ = (double)args[1];
            rotation_z_value_ = (double)args[2];
            apply_rotation();
            return rotation_atoms();
        }}
    };

    c74::min::attribute<double> pan_offset{this, "pan_offset", 0.0,
        c74::min::description{"Pan offset in degrees, applied before inversion."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(args.empty() || !finite_atom(args[0])) {
                warn_once(warn_invalid_numeric_, "invalid pan_offset ignored");
                return {pan_offset_value_};
            }
            pan_offset_value_ = (double)args[0];
            engine_.set_pan_offset(pan_offset_value_);
            return args;
        }}
    };

    c74::min::attribute<double> tilt_offset{this, "tilt_offset", 0.0,
        c74::min::description{"Tilt offset in degrees, applied before inversion."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(args.empty() || !finite_atom(args[0])) {
                warn_once(warn_invalid_numeric_, "invalid tilt_offset ignored");
                return {tilt_offset_value_};
            }
            tilt_offset_value_ = (double)args[0];
            engine_.set_tilt_offset(tilt_offset_value_);
            return args;
        }}
    };

    c74::min::attribute<bool> pan_invert{this, "pan_invert", false,
        c74::min::description{"Invert pan direction."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            const bool value{!args.empty() && ((int)args[0] != 0)};
            engine_.set_pan_invert(value);
            return {value};
        }}
    };

    c74::min::attribute<bool> tilt_invert{this, "tilt_invert", false,
        c74::min::description{"Invert tilt direction."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            const bool value{!args.empty() && ((int)args[0] != 0)};
            engine_.set_tilt_invert(value);
            return {value};
        }}
    };

    c74::min::attribute<c74::min::symbol> byte_order{this, "byte_order", "coarsefine",
        c74::min::description{"Byte order within each 16-bit value."},
        c74::min::enum_map{"coarsefine", "finecoarse"},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(args.empty()) {
                warn_once(warn_invalid_byte_order_, "invalid byte_order ignored");
                return {c74::min::symbol(bbb::dmx::byte_order_to_string(byte_order_value_))};
            }
            const c74::min::symbol symbol_value{(c74::min::symbol)args[0]};
            bbb::dmx::byte_order parsed_order{byte_order_value_};
            if(!bbb::dmx::byte_order_from_string(symbol_value.c_str(), parsed_order)) {
                warn_once(warn_invalid_byte_order_, "invalid byte_order ignored");
                return {c74::min::symbol(bbb::dmx::byte_order_to_string(byte_order_value_))};
            }
            byte_order_value_ = parsed_order;
            engine_.set_byte_order(byte_order_value_);
            return {symbol_value};
        }}
    };

    c74::min::attribute<bool> shortest_pan{this, "shortest_pan", true,
        c74::min::description{"Compatibility switch. 1 enables smart pan/tilt tracking; 0 disables tracking."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            const bool value{args.empty() || ((int)args[0] != 0)};
            set_tracking_mode_value(value ? bbb::dmx::tracking_mode::smart : bbb::dmx::tracking_mode::off);
            return {value};
        }}
    };

    c74::min::attribute<c74::min::symbol> tracking_mode{this, "tracking_mode", "smart",
        c74::min::description{"Tracking mode: off, pan, or smart. smart chooses valid pan/tilt flip candidates before clipping."},
        c74::min::enum_map{"off", "pan", "smart"},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(args.empty()) {
                warn_once(warn_invalid_tracking_mode_, "invalid tracking_mode ignored");
                return {c74::min::symbol(bbb::dmx::tracking_mode_to_string(tracking_mode_value_))};
            }
            const c74::min::symbol symbol_value{(c74::min::symbol)args[0]};
            bbb::dmx::tracking_mode parsed_mode{tracking_mode_value_};
            if(!bbb::dmx::tracking_mode_from_string(symbol_value.c_str(), parsed_mode)) {
                warn_once(warn_invalid_tracking_mode_, "invalid tracking_mode ignored");
                return {c74::min::symbol(bbb::dmx::tracking_mode_to_string(tracking_mode_value_))};
            }
            set_tracking_mode_value(parsed_mode);
            return {c74::min::symbol(bbb::dmx::tracking_mode_to_string(tracking_mode_value_))};
        }}
    };

    c74::min::message<> list_message{this, "list", "target_x target_y target_z",
        MIN_FUNCTION {
            if(args.size() < 3 || !finite_atoms(args, 3)) {
                warn_once(warn_invalid_numeric_, "invalid target list ignored");
                return {};
            }
            compute_and_output((double)args[0], (double)args[1], (double)args[2]);
            return {};
        }
    };

    c74::min::message<> target_message{this, "target", "target target_x target_y target_z",
        MIN_FUNCTION {
            if(args.size() < 3 || !finite_atoms(args, 3)) {
                warn_once(warn_invalid_numeric_, "invalid target message ignored");
                return {};
            }
            compute_and_output((double)args[0], (double)args[1], (double)args[2]);
            return {};
        }
    };

    c74::min::message<> pos_message{this, "pos", "pos fixture_x fixture_y fixture_z",
        MIN_FUNCTION {
            if(args.size() < 3 || !finite_atoms(args, 3)) {
                warn_once(warn_invalid_numeric_, "invalid pos ignored");
                return {};
            }
            fixture_x_value_ = (double)args[0];
            fixture_y_value_ = (double)args[1];
            fixture_z_value_ = (double)args[2];
            fixture_x = fixture_x_value_;
            fixture_y = fixture_y_value_;
            fixture_z = fixture_z_value_;
            apply_fixture_position();
            return {};
        }
    };

    c74::min::message<> range_message{this, "range", "range pan_range tilt_range",
        MIN_FUNCTION {
            if(args.size() < 2 || !finite_atoms(args, 2)) {
                warn_once(warn_invalid_numeric_, "invalid range ignored");
                return {};
            }
            pan_range_value_ = sanitize_range_atom(args[0], "pan_range");
            tilt_range_value_ = sanitize_range_atom(args[1], "tilt_range");
            pan_range = pan_range_value_;
            tilt_range = tilt_range_value_;
            engine_.set_ranges(pan_range_value_, tilt_range_value_);
            return {};
        }
    };

    c74::min::message<> calibrate_pan_message{this, "calibrate_pan", "calibrate_pan target_x target_y target_z pan_u16 | pan_byte_1 pan_byte_2",
        MIN_FUNCTION {
            if(args.size() < 4 || !finite_atoms(args, args.size() < 5 ? 4 : 5)) {
                warn_once(warn_invalid_numeric_, "invalid calibrate_pan ignored");
                return {};
            }
            const bbb::dmx::vec3 target_position{(double)args[0], (double)args[1], (double)args[2]};
            const std::uint16_t desired_value{u16_from_calibration_args(args, 3)};
            if(!engine_.calibrate_pan_offset(target_position, desired_value)) {
                warn_once(warn_invalid_numeric_, "invalid calibrate_pan target ignored");
                return {};
            }
            pan_offset_value_ = engine_.settings().pan_offset_degrees;
            pan_offset = pan_offset_value_;
            compute_and_output(target_position.x, target_position.y, target_position.z);
            return {};
        }
    };

    c74::min::message<> calibrate_tilt_message{this, "calibrate_tilt", "calibrate_tilt target_x target_y target_z tilt_u16 | tilt_byte_1 tilt_byte_2",
        MIN_FUNCTION {
            if(args.size() < 4 || !finite_atoms(args, args.size() < 5 ? 4 : 5)) {
                warn_once(warn_invalid_numeric_, "invalid calibrate_tilt ignored");
                return {};
            }
            const bbb::dmx::vec3 target_position{(double)args[0], (double)args[1], (double)args[2]};
            const std::uint16_t desired_value{u16_from_calibration_args(args, 3)};
            if(!engine_.calibrate_tilt_offset(target_position, desired_value)) {
                warn_once(warn_invalid_numeric_, "invalid calibrate_tilt target ignored");
                return {};
            }
            tilt_offset_value_ = engine_.settings().tilt_offset_degrees;
            tilt_offset = tilt_offset_value_;
            compute_and_output(target_position.x, target_position.y, target_position.z);
            return {};
        }
    };

    c74::min::message<> reset_message{this, "reset", "Clear shortest-pan tracking state.",
        MIN_FUNCTION {
            engine_.reset_tracking();
            return {};
        }
    };

    c74::min::message<> bang_message{this, "bang", "Recompute using the last target or output neutral.",
        MIN_FUNCTION {
            output_dmx(engine_.bang());
            return {};
        }
    };

private:
    static bool finite_atom(const c74::min::atom &atom) {
        return bbb::dmx::is_finite((double)atom);
    }

    static bool finite_atoms(const c74::min::atoms &atoms, std::size_t count) {
        for(std::size_t index = 0; index < count; index++) {
            if(!finite_atom(atoms[index])) {
                return false;
            }
        }
        return true;
    }

    void warn_once(bool &flag, const char *message) {
        if(!flag) {
            cerr << "bbb.dmx.movertrack: " << message << c74::min::endl;
            flag = true;
        }
    }

    double sanitize_range_atom(const c74::min::atom &atom, const char *name) {
        const double value{(double)atom};
        if(value <= 0.0) {
            std::string message{name};
            message += " <= 0; clamped to 1.0";
            warn_once(warn_invalid_range_, message.c_str());
            return 1.0;
        }
        return value;
    }

    c74::min::atoms rotation_atoms() const {
        return {rotation_x_value_, rotation_y_value_, rotation_z_value_};
    }

    void apply_fixture_position() {
        engine_.set_fixture_position(bbb::dmx::vec3{fixture_x_value_, fixture_y_value_, fixture_z_value_});
    }

    void apply_rotation() {
        engine_.set_rotation_degrees(bbb::dmx::vec3{rotation_x_value_, rotation_y_value_, rotation_z_value_});
    }

    void set_tracking_mode_value(bbb::dmx::tracking_mode mode) {
        tracking_mode_value_ = mode;
        engine_.set_tracking_mode(mode);
    }

    static int clamp_int(int value, int minimum, int maximum) {
        return std::max(minimum, std::min(maximum, value));
    }

    std::uint16_t u16_from_calibration_args(const c74::min::atoms &args, std::size_t start_index) const {
        if(start_index + 1 < args.size()) {
            const std::uint8_t first{(std::uint8_t)clamp_int((int)args[start_index], 0, 255)};
            const std::uint8_t second{(std::uint8_t)clamp_int((int)args[start_index + 1], 0, 255)};
            return bbb::dmx::combine_16(first, second, byte_order_value_);
        }
        return (std::uint16_t)clamp_int((int)args[start_index], 0, 65535);
    }

    void compute_and_output(double target_x, double target_y, double target_z) {
        output_dmx(engine_.compute(bbb::dmx::vec3{target_x, target_y, target_z}));
    }

    void output_dmx(const bbb::dmx::movertrack_output &result) {
        c74::min::atoms output_atoms;
        output_atoms.reserve(4);
        for(const int byte : result.bytes) {
            output_atoms.push_back(byte);
        }
        output.send(output_atoms);
    }

};

MIN_EXTERNAL(bbb_dmx_movertrack);
