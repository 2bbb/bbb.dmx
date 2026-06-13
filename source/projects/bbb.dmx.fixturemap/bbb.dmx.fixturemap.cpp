#include "c74_min.h"

#include <bbb/dmx/fixture_json.hpp>
#include <bbb/dmx/max_external_utils.hpp>

#include <algorithm>

#include <cstdint>
#include <string>
#include <vector>

class bbb_dmx_fixturemap : public c74::min::object<bbb_dmx_fixturemap> {
private:
    bbb::dmx::fixture_mapper mapper_{};
    std::string patch_path_value_{};
    int universe_value_{1};
    bool autobang_value_{true};
    bool output_all_universes_{false};
    bool warn_invalid_numeric_{false};
    bool warn_invalid_universe_mode_{false};
    bool warn_runtime_error_{false};
    bool patch_load_pending_{false};
    bool suppress_patch_attribute_load_{false};

public:
    MIN_DESCRIPTION{"Map semantic fixture parameters into one or more 512-channel DMX universe lists."};
    MIN_TAGS{"dmx, lighting, fixture, patch, universe, mapping"};
    MIN_AUTHOR{"2bit"};
    MIN_RELATED{"bbb.dmx.movertrack"};

    c74::min::inlet<> input{this, "(read/set/setall/nset/ptbytes/channel/bang/bangall) fixture mapping control"};
    c74::min::outlet<> universe_output{this, "(list/anything) selected 512-byte list, or universe id followed by 512 bytes"};
    c74::min::outlet<> status_output{this, "(anything) status and error messages"};

    c74::min::timer<c74::min::timer_options::defer_delivery> patch_load_timer{this,
        MIN_FUNCTION {
            if(patch_load_pending_ && !patch_path_value_.empty()) {
                patch_load_pending_ = false;
                load_patch_file(patch_path_value_);
            }
            return {};
        }
    };

    bbb_dmx_fixturemap() = default;

    c74::min::attribute<c74::min::symbol> patch{this, "patch", "",
        c74::min::description{"Patch JSON file path to load on object initialization."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(args.empty()) {
                patch_path_value_.clear();
                patch_load_pending_ = false;
                return {c74::min::symbol("")};
            }
            const c74::min::symbol symbol_value{(c74::min::symbol)args[0]};
            patch_path_value_ = symbol_value.c_str();
            if(!suppress_patch_attribute_load_) {
                schedule_patch_load();
            }
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

    c74::min::attribute<c74::min::symbol> universe_mode{this, "universe_mode", "selected",
        c74::min::description{"Autobang and bang output mode: selected or all. selected outputs a bare 512-byte list; all outputs universe id 512-byte messages."},
        c74::min::enum_map{"selected", "all"},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(args.empty()) {
                return {c74::min::symbol(output_all_universes_ ? "all" : "selected")};
            }
            const std::string text{symbol_arg(args[0])};
            if(text == "all") {
                output_all_universes_ = true;
                return {c74::min::symbol("all")};
            }
            if(text == "selected") {
                output_all_universes_ = false;
                return {c74::min::symbol("selected")};
            }
            warn_once(warn_invalid_universe_mode_, "invalid universe_mode ignored");
            return {c74::min::symbol(output_all_universes_ ? "all" : "selected")};
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
            suppress_patch_attribute_load_ = true;
            patch = path_symbol;
            suppress_patch_attribute_load_ = false;
            patch_load_pending_ = false;
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

    c74::min::message<> bang_message{this, "bang", "Output according to @universe_mode.",
        MIN_FUNCTION {
            output_current_mode();
            return {};
        }
    };

    c74::min::message<> bangall_message{this, "bangall", "Output all known universes as universe id 512-byte messages.",
        MIN_FUNCTION {
            output_all_universes();
            return {};
        }
    };

    c74::min::message<> set_message{this, "set", "set fixture_id parameter value [parameter value ...] OR set fixture_id pan_tilt pan_u16 tilt_u16",
        MIN_FUNCTION {
            if(args.size() < 3) {
                report_error("set requires fixture_id parameter value");
                return {};
            }
            const std::string fixture_id{symbol_arg(args[0])};
            const bbb::dmx::fixture_mapper previous_mapper{mapper_};
            const bbb::dmx::mapper_result result{set_parameter_args(fixture_id, args, 1)};
            if(!handle_result(result)) {
                mapper_ = previous_mapper;
                return {};
            }
            output_if_autobang();
            return {};
        }
    };

    c74::min::message<> setall_message{this, "setall", "setall parameter value [parameter value ...]",
        MIN_FUNCTION {
            if(args.size() < 2) {
                report_error("setall requires parameter value");
                return {};
            }
            const bbb::dmx::fixture_mapper previous_mapper{mapper_};
            const bbb::dmx::mapper_result result{set_all_parameter_args(args)};
            if(!handle_result(result)) {
                mapper_ = previous_mapper;
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
            status_atoms.push_back(c74::min::symbol("universe_mode"));
            status_atoms.push_back(c74::min::symbol(output_all_universes_ ? "all" : "selected"));
            status_atoms.push_back(c74::min::symbol("universes"));
            for(const int universe_id : mapper_.universe_ids()) {
                status_atoms.push_back(universe_id);
            }
            status_output.send(status_atoms);
            return {};
        }
    };

private:
    void schedule_patch_load() {
        if(patch_path_value_.empty()) {
            patch_load_pending_ = false;
            return;
        }
        patch_load_pending_ = true;
        patch_load_timer.delay(0);
    }

    void load_patch_file(const std::string &path) {
        const std::string resolved_path{bbb::dmx::maxutil::resolve_file_path(path)};
        bbb::dmx::fixture_mapper loaded_mapper{};
        const bbb::dmx::mapper_result result{bbb::dmx::load_fixture_mapper_from_patch_file(resolved_path, loaded_mapper)};
        if(!handle_result(result)) {
            return;
        }
        mapper_ = loaded_mapper;
        report_status("loaded");
        output_if_autobang();
    }


    void output_if_autobang() {
        if(autobang_value_) {
            output_current_mode();
        }
    }

    void output_current_mode() {
        if(output_all_universes_) {
            output_all_universes();
            return;
        }
        output_selected_universe();
    }

    void output_selected_universe() {
        const auto values = mapper_.universe(universe_value_).to_int_vector();
        c74::min::atoms output_atoms;
        output_atoms.reserve(values.size());
        for(const int value : values) {
            output_atoms.push_back(value);
        }
        universe_output.send(output_atoms);
    }

    void output_all_universes() {
        std::vector<int> universe_ids{mapper_.universe_ids()};
        if(universe_ids.empty()) {
            output_universe_message(universe_value_);
            return;
        }
        for(const int universe_id : universe_ids) {
            output_universe_message(universe_id);
        }
    }

    void output_universe_message(int universe_id) {
        const int sanitized_universe{std::max(1, universe_id)};
        universe_output.send(bbb::dmx::maxutil::universe_atoms(sanitized_universe, mapper_.universe(sanitized_universe)));
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
        return bbb::dmx::maxutil::symbol_arg(atom);
    }

    bbb::dmx::mapper_result set_all_parameter_args(const c74::min::atoms &args) {
        if(mapper_.patch().fixtures.empty()) {
            return bbb::dmx::mapper_result::failure("setall requires a loaded patch with fixtures");
        }
        for(const auto &fixture : mapper_.patch().fixtures) {
            const bbb::dmx::mapper_result result{set_parameter_args(fixture.id, args, 0)};
            if(!result.ok) {
                return bbb::dmx::mapper_result::failure("setall fixture " + fixture.id + ": " + result.message);
            }
        }
        return bbb::dmx::mapper_result::success();
    }

    bbb::dmx::mapper_result set_parameter_args(const std::string &fixture_id, const c74::min::atoms &args, std::size_t start_index) {
        std::size_t index{start_index};
        while(index < args.size()) {
            const std::string parameter{symbol_arg(args[index])};
            if(parameter == "pan_tilt") {
                if(args.size() <= index + 2 || !finite_atom(args[index + 1]) || !finite_atom(args[index + 2])) {
                    return bbb::dmx::mapper_result::failure("set pan_tilt requires two numeric u16 values");
                }
                bbb::dmx::mapper_result result{mapper_.set_u16(fixture_id, "pan", (std::uint16_t)clamp_int((int)args[index + 1], 0, 65535))};
                if(result.ok) {
                    result = mapper_.set_u16(fixture_id, "tilt", (std::uint16_t)clamp_int((int)args[index + 2], 0, 65535));
                }
                if(!result.ok) {
                    return result;
                }
                index += 3;
                continue;
            }
            if(args.size() <= index + 1) {
                return bbb::dmx::mapper_result::failure("set requires parameter/value pairs");
            }
            if(!finite_atom(args[index + 1])) {
                return bbb::dmx::mapper_result::failure("set value must be numeric: " + parameter);
            }
            const int value{(int)args[index + 1]};
            const bbb::dmx::mapper_result result{set_parameter_value(fixture_id, parameter, value)};
            if(!result.ok) {
                return result;
            }
            index += 2;
        }
        return bbb::dmx::mapper_result::success();
    }

    bbb::dmx::mapper_result set_parameter_value(const std::string &fixture_id, const std::string &parameter, int value) {
        bbb::dmx::mapper_result result{mapper_.set_u24(fixture_id, parameter, (std::uint32_t)clamp_int(value, 0, 16777215))};
        if(!result.ok) {
            result = mapper_.set_u16(fixture_id, parameter, (std::uint16_t)clamp_int(value, 0, 65535));
        }
        if(!result.ok) {
            result = mapper_.set_u8(fixture_id, parameter, value);
        }
        return result;
    }

    void warn_once(bool &flag, const char *message) {
        if(!flag) {
            cerr << "bbb.dmx.fixturemap: " << message << c74::min::endl;
            flag = true;
        }
    }
};

MIN_EXTERNAL(bbb_dmx_fixturemap);
