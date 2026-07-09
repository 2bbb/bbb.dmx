#include "c74_min.h"

#include <bbb/dmx/build_info.hpp>

#include <bbb/dmx/fixture_json.hpp>
#include <bbb/dmx/frame_set.hpp>
#include <bbb/dmx/max_external_utils.hpp>

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace {


bool optional_int(const bbb::dmx::json_value &object, const std::string &key, int &value, std::string &error) {
    const bbb::dmx::json_value *child{object.find(key)};
    if(!child) {
        return false;
    }
    if(child->type != bbb::dmx::json_type::number) {
        error = "expected number: " + key;
        return false;
    }
    value = (int)std::round(child->number_value);
    return true;
}

struct assert_rule {
public:
    std::string name{};
    int universe{1};
    int start{1};
    int count{1};
    bool has_minimum{false};
    bool has_maximum{false};
    bool has_equals{false};
    int minimum{0};
    int maximum{255};
    int equals{0};
};

} // namespace

class bbb_dmx_assert : public c74::min::object<bbb_dmx_assert> {
private:
    bbb::dmx::dmx_frame_set frames_{};
    std::vector<assert_rule> rules_{};
    std::string config_path_value_{};
    int universe_value_{1};
    bool autobang_value_{true};

public:
    MIN_DESCRIPTION{"Validate multi-universe DMX frames against range/equality assertions."};
    MIN_TAGS{"dmx, lighting, assert, validate, test"};
    MIN_AUTHOR{"2bit"};
    MIN_RELATED{"bbb.dmx.diff, bbb.dmx.monitor, bbb.dmx.patchcheck"};

    c74::min::inlet<> input{this, "(list/universe/channel/channels/read/bang) DMX assertion input"};
    c74::min::outlet<> output{this, "(anything) assertion results"};
    c74::min::outlet<> status_output{this, "(anything) status and error messages"};

    c74::min::timer<c74::min::timer_options::defer_delivery> init_timer{this,
        MIN_FUNCTION {
            if(!config_path_value_.empty()) {
                load_config(config_path_value_);
            }
            return {};
        }
    };

    bbb_dmx_assert() {
        bbb::dmx::report_external_build_info(cout, "bbb.dmx.assert");
        init_timer.delay(0);
    }

    c74::min::attribute<c74::min::symbol> config{this, "config", "",
        c74::min::description{"Assertion JSON path."},
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
        c74::min::description{"Default universe for bare list input."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(args.empty() || !bbb::dmx::maxutil::finite_atom(args[0])) {
                return {universe_value_};
            }
            universe_value_ = bbb::dmx::sanitize_universe_id((int)args[0]);
            return {universe_value_};
        }}
    };

    c74::min::attribute<bool> autobang{this, "autobang", true,
        c74::min::description{"Run assertions immediately after frame updates."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            autobang_value_ = args.empty() || ((int)args[0] != 0);
            return {autobang_value_};
        }}
    };

    c74::min::message<> read_message{this, "read", "read assert_json_path",
        MIN_FUNCTION {
            if(args.empty()) {
                report_error("read requires assertion JSON path");
                return {};
            }
            const c74::min::symbol path_symbol{(c74::min::symbol)args[0]};
            config_path_value_ = path_symbol.c_str();
            config = path_symbol;
            load_config(config_path_value_);
            return {};
        }
    };

    c74::min::message<> reload_message{this, "reload", "Reload current assertion config.",
        MIN_FUNCTION {
            if(config_path_value_.empty()) {
                report_error("reload requires a previously loaded config path");
                return {};
            }
            load_config(config_path_value_);
            return {};
        }
    };

    c74::min::message<> list_message{this, "list", "512 values for default universe.",
        MIN_FUNCTION {
            handle_frame_write(bbb::dmx::maxutil::write_universe_from_atoms(frames_, universe_value_, args, 0, "list requires 512 numeric values"));
            return {};
        }
    };

    c74::min::message<> universe_message{this, "universe", "universe id value1 ... value512",
        MIN_FUNCTION {
            handle_frame_write(bbb::dmx::maxutil::write_universe_message(frames_, args, "universe requires id and 512 values"));
            return {};
        }
    };

    c74::min::message<> channel_message{this, "channel", "channel universe address value",
        MIN_FUNCTION {
            handle_frame_write(bbb::dmx::maxutil::write_channel_message(frames_, args, "channel requires universe address value"));
            return {};
        }
    };

    c74::min::message<> channels_message{this, "channels", "channels universe address value ...",
        MIN_FUNCTION {
            handle_frame_write(bbb::dmx::maxutil::write_channels_message(frames_, args, "channels requires universe and address/value pairs", "channels pair must be numeric"));
            return {};
        }
    };

    c74::min::message<> bang_message{this, "bang", "Run all assertions.",
        MIN_FUNCTION {
            run_assertions();
            return {};
        }
    };

    c74::min::message<> clear_message{this, "clear", "Clear frames.",
        MIN_FUNCTION {
            frames_.clear();
            report_status("clear");
            return {};
        }
    };

private:
    void handle_frame_write(const bbb::dmx::maxutil::frame_write_result &result) {
        if(!result.ok) {
            report_error(result.message);
            return;
        }
        if(autobang_value_) {
            run_assertions();
        }
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
        std::vector<assert_rule> loaded{};
        result = parse_rules(parsed.value, loaded);
        if(!result.ok) {
            report_error(result.message);
            return;
        }
        rules_ = loaded;
        report_status("config_loaded");
    }

    bbb::dmx::mapper_result parse_rules(const bbb::dmx::json_value &root, std::vector<assert_rule> &rules) const {
        if(root.type != bbb::dmx::json_type::object) {
            return bbb::dmx::mapper_result::failure("assert root must be object");
        }
        const bbb::dmx::json_value *rule_array{root.find("rules")};
        if(!rule_array || rule_array->type != bbb::dmx::json_type::array) {
            return bbb::dmx::mapper_result::failure("assert rules must be array");
        }
        std::string error{};
        for(const auto &rule_value : rule_array->array_value) {
            if(rule_value.type != bbb::dmx::json_type::object) {
                return bbb::dmx::mapper_result::failure("assert rule must be object");
            }
            assert_rule rule{};
            bbb::dmx::json_string(rule_value, "name", rule.name, false, error);
            if(!bbb::dmx::json_int(rule_value, "universe", rule.universe, true, error)) {
                return bbb::dmx::mapper_result::failure(error);
            }
            int channel{0};
            if(optional_int(rule_value, "channel", channel, error) && 0 < channel) {
                rule.start = channel;
                rule.count = 1;
            } else {
                if(!bbb::dmx::json_int(rule_value, "start", rule.start, true, error)) {
                    return bbb::dmx::mapper_result::failure(error);
                }
                int end{0};
                optional_int(rule_value, "count", rule.count, error);
                if(optional_int(rule_value, "end", end, error) && 0 < end) {
                    rule.count = end - rule.start + 1;
                }
                if(rule.count <= 0) {
                    rule.count = 1;
                }
            }
            if(optional_int(rule_value, "min", rule.minimum, error)) {
                rule.has_minimum = true;
            }
            if(optional_int(rule_value, "max", rule.maximum, error)) {
                rule.has_maximum = true;
            }
            if(optional_int(rule_value, "equals", rule.equals, error)) {
                rule.has_equals = true;
            }
            if(!error.empty()) {
                return bbb::dmx::mapper_result::failure(error);
            }
            rule.universe = bbb::dmx::sanitize_universe_id(rule.universe);
            rule.start = bbb::dmx::maxutil::clamp_int(rule.start, 1, bbb::dmx::universe_channel_count);
            rule.count = std::max(1, std::min(rule.count, bbb::dmx::universe_channel_count - rule.start + 1));
            rules.push_back(rule);
        }
        return bbb::dmx::mapper_result::success();
    }

    void run_assertions() {
        int pass_count{0};
        int fail_count{0};
        for(const auto &rule : rules_) {
            for(int address = rule.start; address < rule.start + rule.count; address++) {
                const int value{frames_.universe(rule.universe).channel(address)};
                std::string reason{};
                if(rule.has_equals && value != rule.equals) {
                    reason = "equals";
                } else if(rule.has_minimum && value < rule.minimum) {
                    reason = "min";
                } else if(rule.has_maximum && rule.maximum < value) {
                    reason = "max";
                }
                if(reason.empty()) {
                    pass_count++;
                } else {
                    fail_count++;
                    output_failure(rule, address, value, reason);
                }
            }
        }
        c74::min::atoms atoms;
        atoms.push_back(fail_count == 0 ? c74::min::symbol("ok") : c74::min::symbol("summary"));
        atoms.push_back(c74::min::symbol("pass"));
        atoms.push_back(pass_count);
        atoms.push_back(c74::min::symbol("fail"));
        atoms.push_back(fail_count);
        output.send(atoms);
    }

    void output_failure(const assert_rule &rule, int address, int value, const std::string &reason) {
        c74::min::atoms atoms;
        atoms.push_back(c74::min::symbol("fail"));
        atoms.push_back(rule.universe);
        atoms.push_back(address);
        atoms.push_back(value);
        atoms.push_back(c74::min::symbol(reason.c_str()));
        if(!rule.name.empty()) {
            atoms.push_back(c74::min::symbol("name"));
            atoms.push_back(c74::min::symbol(rule.name.c_str()));
        }
        output.send(atoms);
    }

    void report_status(const char *message) {
        bbb::dmx::maxutil::send_status(status_output, "status", message);
    }

    void report_error(const std::string &message) {
        cerr << "bbb.dmx.assert: " << message << c74::min::endl;
        bbb::dmx::maxutil::send_status(status_output, "error", message);
    }
};

MIN_EXTERNAL(bbb_dmx_assert);
