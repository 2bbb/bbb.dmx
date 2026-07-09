#include "c74_min.h"

#include <bbb/dmx/build_info.hpp>

#include <bbb/dmx/curve.hpp>
#include <bbb/dmx/fixture_json.hpp>
#include <bbb/dmx/frame_set.hpp>
#include <bbb/dmx/max_external_utils.hpp>

#include <algorithm>
#include <string>
#include <vector>

class bbb_dmx_curve : public c74::min::object<bbb_dmx_curve> {
private:
    bbb::dmx::dmx_frame_set input_frames_{};
    bbb::dmx::dmx_frame_set output_frames_{};
    std::vector<bbb::dmx::dmx_curve_rule> rules_{};
    std::string config_path_value_{};
    int universe_value_{1};
    bool autobang_value_{true};

public:
    MIN_DESCRIPTION{"Apply channel/range curves to multi-universe DMX frames."};
    MIN_TAGS{"dmx, lighting, curve, gamma, universe"};
    MIN_AUTHOR{"2bit"};
    MIN_RELATED{"bbb.dmx.matrixmap, bbb.dmx.mask, bbb.dmx.safety"};

    c74::min::inlet<> input{this, "(list/universe/channel/channels/read/bang) DMX curve input"};
    c74::min::outlet<> output{this, "(anything) curved universe data"};
    c74::min::outlet<> status_output{this, "(anything) status and error messages"};

    c74::min::timer<c74::min::timer_options::defer_delivery> init_timer{this,
        MIN_FUNCTION {
            if(!config_path_value_.empty()) {
                load_config(config_path_value_);
            }
            return {};
        }
    };

    bbb_dmx_curve() {
        bbb::dmx::report_external_build_info(cout, "bbb.dmx.curve");
        init_timer.delay(0);
    }

    c74::min::attribute<c74::min::symbol> config{this, "config", "",
        c74::min::description{"Curve JSON path."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(args.empty()) {
                config_path_value_.clear();
                return {c74::min::symbol("")};
            }
            const c74::min::symbol symbol_value{(c74::min::symbol)args[0]};
            config_path_value_ = symbol_value.c_str();
            return {symbol_value};
        }}
    };

    c74::min::attribute<int> universe{this, "universe", 1,
        c74::min::description{"Default universe for bare list input and bang output."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(args.empty() || !bbb::dmx::maxutil::finite_atom(args[0])) {
                return {universe_value_};
            }
            universe_value_ = bbb::dmx::sanitize_universe_id((int)args[0]);
            return {universe_value_};
        }}
    };

    c74::min::attribute<bool> autobang{this, "autobang", true,
        c74::min::description{"Output immediately after input updates."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            autobang_value_ = args.empty() || ((int)args[0] != 0);
            return {autobang_value_};
        }}
    };

    c74::min::message<> read_message{this, "read", "read curve_json_path",
        MIN_FUNCTION {
            if(args.empty()) {
                report_error("read requires curve JSON path");
                return {};
            }
            const c74::min::symbol path_symbol{(c74::min::symbol)args[0]};
            config_path_value_ = path_symbol.c_str();
            config = path_symbol;
            load_config(config_path_value_);
            return {};
        }
    };

    c74::min::message<> reload_message{this, "reload", "Reload current config file.",
        MIN_FUNCTION {
            if(config_path_value_.empty()) {
                report_error("reload requires a previously loaded config path");
                return {};
            }
            load_config(config_path_value_);
            return {};
        }
    };

    c74::min::message<> clear_message{this, "clear", "Clear frames and rules.",
        MIN_FUNCTION {
            input_frames_.clear();
            output_frames_.clear();
            rules_.clear();
            report_status("clear");
            return {};
        }
    };

    c74::min::message<> list_message{this, "list", "512 values for default universe.",
        MIN_FUNCTION {
            handle_frame_write(bbb::dmx::maxutil::write_universe_from_atoms(
                input_frames_,
                universe_value_,
                args,
                0,
                "universe input requires 512 numeric values"
            ));
            return {};
        }
    };

    c74::min::message<> universe_message{this, "universe", "universe id value1 ... value512",
        MIN_FUNCTION {
            handle_frame_write(bbb::dmx::maxutil::write_universe_message(
                input_frames_,
                args,
                "universe requires id and 512 values"
            ));
            return {};
        }
    };

    c74::min::message<> channel_message{this, "channel", "channel universe address value",
        MIN_FUNCTION {
            handle_frame_write(bbb::dmx::maxutil::write_channel_message(
                input_frames_,
                args,
                "channel requires universe address value"
            ));
            return {};
        }
    };

    c74::min::message<> channels_message{this, "channels", "channels universe address value ...",
        MIN_FUNCTION {
            handle_frame_write(bbb::dmx::maxutil::write_channels_message(
                input_frames_,
                args,
                "channels requires universe and address/value pairs",
                "channels pair must be numeric"
            ));
            return {};
        }
    };

    c74::min::message<> gamma_message{this, "gamma", "gamma universe start count gamma",
        MIN_FUNCTION {
            if(args.size() < 4 || !bbb::dmx::maxutil::finite_atoms(args, 0, 4)) {
                report_error("gamma requires universe start count gamma");
                return {};
            }
            bbb::dmx::dmx_curve_rule rule{};
            rule.universe = std::max(0, (int)args[0]);
            rule.start = (int)args[1];
            rule.count = (int)args[2];
            rule.type = bbb::dmx::dmx_curve_type::gamma;
            rule.gamma = (double)args[3];
            rules_.push_back(rule);
            report_status("rule_added");
            return {};
        }
    };

    c74::min::message<> bang_message{this, "bang", "Output default universe.",
        MIN_FUNCTION {
            output_universe(universe_value_);
            return {};
        }
    };

    c74::min::message<> bangall_message{this, "bangall", "Output all known universes.",
        MIN_FUNCTION {
            for(const int universe_id : output_frames_.universe_ids()) {
                output_universe(universe_id);
            }
            return {};
        }
    };

    c74::min::message<> dump_message{this, "dump", "Output current rule count.",
        MIN_FUNCTION {
            c74::min::atoms atoms;
            atoms.push_back(c74::min::symbol("status"));
            atoms.push_back(c74::min::symbol("rules"));
            atoms.push_back((int)rules_.size());
            status_output.send(atoms);
            return {};
        }
    };

private:
    void handle_frame_write(const bbb::dmx::maxutil::frame_write_result &result) {
        if(!result.ok) {
            report_error(result.message);
            return;
        }
        apply_and_output(result.universe);
    }

    void apply_and_output(int universe_id) {
        const int sanitized_universe{bbb::dmx::sanitize_universe_id(universe_id)};
        output_frames_.universes[sanitized_universe] = bbb::dmx::apply_curve_rules(input_frames_.universe(sanitized_universe), sanitized_universe, rules_);
        if(autobang_value_) {
            output_universe(sanitized_universe);
        }
    }

    void output_universe(int universe_id) {
        const int sanitized_universe{bbb::dmx::sanitize_universe_id(universe_id)};
        output.send(bbb::dmx::maxutil::universe_atoms(sanitized_universe, output_frames_.universe(sanitized_universe)));
    }

    void load_config(const std::string &path) {
        const std::string resolved_path{bbb::dmx::maxutil::resolve_file_path(this->maxobj(), path)};
        std::string text{};
        bbb::dmx::mapper_result result{bbb::dmx::read_text_file(resolved_path, text)};
        if(!result.ok) {
            report_error(result.message);
            return;
        }
        const bbb::dmx::json_parse_result parsed{bbb::dmx::parse_json_text(text)};
        if(!parsed.ok) {
            report_error(parsed.message);
            return;
        }
        std::vector<bbb::dmx::dmx_curve_rule> loaded{};
        result = parse_rules(parsed.value, loaded);
        if(!result.ok) {
            report_error(result.message);
            return;
        }
        rules_ = loaded;
        report_status("config_loaded");
    }

    bbb::dmx::mapper_result parse_rules(const bbb::dmx::json_value &root, std::vector<bbb::dmx::dmx_curve_rule> &rules) const {
        if(root.type != bbb::dmx::json_type::object) {
            return bbb::dmx::mapper_result::failure("curve root must be object");
        }
        const bbb::dmx::json_value *rule_array{root.find("rules")};
        if(!rule_array || rule_array->type != bbb::dmx::json_type::array) {
            return bbb::dmx::mapper_result::failure("curve rules must be array");
        }
        std::string error{};
        for(const auto &rule_value : rule_array->array_value) {
            if(rule_value.type != bbb::dmx::json_type::object) {
                return bbb::dmx::mapper_result::failure("curve rule must be object");
            }
            bbb::dmx::dmx_curve_rule rule{};
            bbb::dmx::json_int(rule_value, "universe", rule.universe, false, error);
            bbb::dmx::json_int(rule_value, "start", rule.start, false, error);
            int end{0};
            bbb::dmx::json_int(rule_value, "count", rule.count, false, error);
            if(bbb::dmx::json_int(rule_value, "end", end, false, error) && 0 < end) {
                rule.count = end - rule.start + 1;
            }
            std::string curve_text{"linear"};
            bbb::dmx::json_string(rule_value, "curve", curve_text, false, error);
            if(!bbb::dmx::curve_type_from_string(curve_text, rule.type)) {
                return bbb::dmx::mapper_result::failure("unknown curve: " + curve_text);
            }
            bbb::dmx::json_double(rule_value, "gamma", rule.gamma, false, error);
            bbb::dmx::json_double(rule_value, "threshold", rule.threshold, false, error);
            const bbb::dmx::json_value *points{rule_value.find("points")};
            if(points) {
                if(points->type != bbb::dmx::json_type::array) {
                    return bbb::dmx::mapper_result::failure("curve points must be array");
                }
                rule.type = bbb::dmx::dmx_curve_type::points;
                for(const auto &point_value : points->array_value) {
                    if(point_value.type != bbb::dmx::json_type::array || point_value.array_value.size() < 2 || point_value.array_value[0].type != bbb::dmx::json_type::number || point_value.array_value[1].type != bbb::dmx::json_type::number) {
                        return bbb::dmx::mapper_result::failure("curve point must be [x,y]");
                    }
                    rule.points.push_back(bbb::dmx::dmx_curve_point{point_value.array_value[0].number_value, point_value.array_value[1].number_value});
                }
            }
            rules.push_back(rule);
        }
        return bbb::dmx::mapper_result::success();
    }


    void report_status(const char *message) {
        bbb::dmx::maxutil::send_status(status_output, "status", message);
    }

    void report_error(const std::string &message) {
        cerr << "bbb.dmx.curve: " << message << c74::min::endl;
        bbb::dmx::maxutil::send_status(status_output, "error", message);
    }
};

MIN_EXTERNAL(bbb_dmx_curve);
