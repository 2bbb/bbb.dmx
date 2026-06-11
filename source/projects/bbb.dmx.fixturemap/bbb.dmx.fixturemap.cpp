#include "c74_min.h"

#include <bbb/dmx/fixture_json.hpp>

#include <cstdint>
#include <string>

class bbb_dmx_fixturemap : public c74::min::object<bbb_dmx_fixturemap> {
private:
    bbb::dmx::fixture_mapper mapper_{};
    std::string patch_path_value_{};
    int universe_value_{1};
    bool autobang_value_{true};
    bool warn_invalid_numeric_{false};
    bool warn_runtime_error_{false};

public:
    MIN_DESCRIPTION{"Map semantic fixture parameters into a 512-channel DMX universe list."};
    MIN_TAGS{"dmx, lighting, fixture, patch, universe, mapping"};
    MIN_AUTHOR{"2bit"};
    MIN_RELATED{"bbb.dmx.movertrack"};

    c74::min::inlet<> input{this, "(read/set/nset/ptbytes/channel/bang) fixture mapping control"};
    c74::min::outlet<> universe_output{this, "(list) 512 DMX byte values for the selected universe"};
    c74::min::outlet<> status_output{this, "(anything) status and error messages"};

    c74::min::timer<c74::min::timer_options::defer_delivery> init_timer{this,
        MIN_FUNCTION {
            if(!patch_path_value_.empty()) {
                load_patch_file(patch_path_value_);
            }
            return {};
        }
    };

    bbb_dmx_fixturemap() {
        init_timer.delay(0);
    }

    c74::min::attribute<c74::min::symbol> patch{this, "patch", "",
        c74::min::description{"Patch JSON file path to load on object initialization."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(args.empty()) {
                patch_path_value_.clear();
                return {c74::min::symbol("")};
            }
            const c74::min::symbol symbol_value{(c74::min::symbol)args[0]};
            patch_path_value_ = symbol_value.c_str();
            return {symbol_value};
        }}
    };

    c74::min::attribute<int> universe{this, "universe", 1,
        c74::min::description{"Universe id to output."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(args.empty() || !finite_atom(args[0])) {
                warn_once(warn_invalid_numeric_, "invalid universe ignored");
                return {universe_value_};
            }
            universe_value_ = std::max(1, (int)args[0]);
            return {universe_value_};
        }}
    };

    c74::min::attribute<bool> autobang{this, "autobang", true,
        c74::min::description{"Output the full universe immediately after successful updates."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            autobang_value_ = args.empty() || ((int)args[0] != 0);
            return {autobang_value_};
        }}
    };

    c74::min::message<> read_message{this, "read", "read patch_json_path",
        MIN_FUNCTION {
            if(args.empty()) {
                report_error("read requires a patch JSON path");
                return {};
            }
            const c74::min::symbol path_symbol{(c74::min::symbol)args[0]};
            patch_path_value_ = path_symbol.c_str();
            patch = path_symbol;
            load_patch_file(patch_path_value_);
            return {};
        }
    };

    c74::min::message<> reload_message{this, "reload", "Reload the current patch JSON file.",
        MIN_FUNCTION {
            if(patch_path_value_.empty()) {
                report_error("reload requires a previously loaded patch path");
                return {};
            }
            load_patch_file(patch_path_value_);
            return {};
        }
    };

    c74::min::message<> clear_message{this, "clear", "Clear all loaded profiles, patch data, and universe buffers.",
        MIN_FUNCTION {
            mapper_.clear();
            report_status("clear");
            output_if_autobang();
            return {};
        }
    };

    c74::min::message<> reset_message{this, "reset", "Reset loaded fixture channels to profile defaults.",
        MIN_FUNCTION {
            const bbb::dmx::mapper_result result{mapper_.reset_universes()};
            if(!handle_result(result)) {
                return {};
            }
            output_if_autobang();
            return {};
        }
    };

    c74::min::message<> bang_message{this, "bang", "Output the selected 512-channel universe list.",
        MIN_FUNCTION {
            output_universe();
            return {};
        }
    };

    c74::min::message<> set_message{this, "set", "set fixture_id parameter value OR set fixture_id pan_tilt pan_u16 tilt_u16",
        MIN_FUNCTION {
            if(args.size() < 3) {
                report_error("set requires fixture_id parameter value");
                return {};
            }
            const std::string fixture_id{symbol_arg(args[0])};
            const std::string parameter{symbol_arg(args[1])};
            bbb::dmx::mapper_result result{};
            if(parameter == "pan_tilt") {
                if(args.size() < 4 || !finite_atom(args[2]) || !finite_atom(args[3])) {
                    report_error("set fixture pan_tilt requires two numeric u16 values");
                    return {};
                }
                result = mapper_.set_u16(fixture_id, "pan", (std::uint16_t)clamp_int((int)args[2], 0, 65535));
                if(result.ok) {
                    result = mapper_.set_u16(fixture_id, "tilt", (std::uint16_t)clamp_int((int)args[3], 0, 65535));
                }
            } else {
                if(!finite_atom(args[2])) {
                    report_error("set value must be numeric");
                    return {};
                }
                const int value{(int)args[2]};
                result = mapper_.set_u16(fixture_id, parameter, (std::uint16_t)clamp_int(value, 0, 65535));
                if(!result.ok) {
                    result = mapper_.set_u8(fixture_id, parameter, value);
                }
            }
            if(!handle_result(result)) {
                return {};
            }
            output_if_autobang();
            return {};
        }
    };

    c74::min::message<> nset_message{this, "nset", "nset fixture_id parameter normalized_0_to_1",
        MIN_FUNCTION {
            if(args.size() < 3 || !finite_atom(args[2])) {
                report_error("nset requires fixture_id parameter numeric_value");
                return {};
            }
            const bbb::dmx::mapper_result result{mapper_.set_normalized(symbol_arg(args[0]), symbol_arg(args[1]), (double)args[2])};
            if(!handle_result(result)) {
                return {};
            }
            output_if_autobang();
            return {};
        }
    };

    c74::min::message<> ptbytes_message{this, "ptbytes", "ptbytes fixture_id pan1 pan2 tilt1 tilt2",
        MIN_FUNCTION {
            if(args.size() < 5 || !finite_atoms(args, 1, 4)) {
                report_error("ptbytes requires fixture_id pan1 pan2 tilt1 tilt2");
                return {};
            }
            const bbb::dmx::mapper_result result{mapper_.set_pan_tilt_bytes(
                symbol_arg(args[0]),
                (int)args[1],
                (int)args[2],
                (int)args[3],
                (int)args[4]
            )};
            if(!handle_result(result)) {
                return {};
            }
            output_if_autobang();
            return {};
        }
    };

    c74::min::message<> channel_message{this, "channel", "channel address value in selected universe",
        MIN_FUNCTION {
            if(args.size() < 2 || !finite_atoms(args, 0, 2)) {
                report_error("channel requires address value");
                return {};
            }
            const bbb::dmx::mapper_result result{mapper_.set_channel(universe_value_, (int)args[0], (int)args[1])};
            if(!handle_result(result)) {
                return {};
            }
            output_if_autobang();
            return {};
        }
    };

    c74::min::message<> channels_message{this, "channels", "channels address value ... in selected universe",
        MIN_FUNCTION {
            if(args.size() < 2 || (args.size() % 2) != 0) {
                report_error("channels requires address/value pairs");
                return {};
            }
            for(std::size_t index = 0; index < args.size(); index += 2) {
                if(!finite_atom(args[index]) || !finite_atom(args[index + 1])) {
                    report_error("channels pair must be numeric");
                    return {};
                }
                const bbb::dmx::mapper_result result{mapper_.set_channel(universe_value_, (int)args[index], (int)args[index + 1])};
                if(!handle_result(result)) {
                    return {};
                }
            }
            output_if_autobang();
            return {};
        }
    };

    c74::min::message<> dump_message{this, "dump", "Output status information from the status outlet.",
        MIN_FUNCTION {
            c74::min::atoms status_atoms;
            status_atoms.push_back(c74::min::symbol("status"));
            status_atoms.push_back(c74::min::symbol(mapper_.validated() ? "loaded" : "empty"));
            status_atoms.push_back(c74::min::symbol("universe"));
            status_atoms.push_back(universe_value_);
            status_output.send(status_atoms);
            return {};
        }
    };

private:
    void load_patch_file(const std::string &path) {
        bbb::dmx::fixture_mapper loaded_mapper{};
        const bbb::dmx::mapper_result result{bbb::dmx::load_fixture_mapper_from_patch_file(path, loaded_mapper)};
        if(!handle_result(result)) {
            return;
        }
        mapper_ = loaded_mapper;
        report_status("loaded");
        output_if_autobang();
    }

    void output_if_autobang() {
        if(autobang_value_) {
            output_universe();
        }
    }

    void output_universe() {
        const auto values = mapper_.universe(universe_value_).to_int_vector();
        c74::min::atoms output_atoms;
        output_atoms.reserve(values.size());
        for(const int value : values) {
            output_atoms.push_back(value);
        }
        universe_output.send(output_atoms);
    }

    bool handle_result(const bbb::dmx::mapper_result &result) {
        if(result.ok) {
            warn_runtime_error_ = false;
            return true;
        }
        report_error(result.message.c_str());
        return false;
    }

    void report_status(const char *message) {
        c74::min::atoms status_atoms;
        status_atoms.push_back(c74::min::symbol("status"));
        status_atoms.push_back(c74::min::symbol(message));
        status_output.send(status_atoms);
    }

    void report_error(const char *message) {
        cerr << "bbb.dmx.fixturemap: " << message << c74::min::endl;
        c74::min::atoms error_atoms;
        error_atoms.push_back(c74::min::symbol("error"));
        error_atoms.push_back(c74::min::symbol(message));
        status_output.send(error_atoms);
        warn_runtime_error_ = true;
    }

    static bool finite_atom(const c74::min::atom &atom) {
        return bbb::dmx::is_finite((double)atom);
    }

    static bool finite_atoms(const c74::min::atoms &atoms, std::size_t start, std::size_t count) {
        for(std::size_t index = start; index < start + count; index++) {
            if(atoms.size() <= index || !finite_atom(atoms[index])) {
                return false;
            }
        }
        return true;
    }

    static int clamp_int(int value, int minimum, int maximum) {
        return std::max(minimum, std::min(maximum, value));
    }

    static std::string symbol_arg(const c74::min::atom &atom) {
        const c74::min::symbol symbol_value{(c74::min::symbol)atom};
        return symbol_value.c_str();
    }

    void warn_once(bool &flag, const char *message) {
        if(!flag) {
            cerr << "bbb.dmx.fixturemap: " << message << c74::min::endl;
            flag = true;
        }
    }
};

MIN_EXTERNAL(bbb_dmx_fixturemap);
