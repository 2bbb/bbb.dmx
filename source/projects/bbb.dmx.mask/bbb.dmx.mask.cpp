#include "c74_min.h"

#include <bbb/dmx/build_info.hpp>

#include <bbb/dmx/color_mapping.hpp>
#include <bbb/dmx/fixture_groups.hpp>
#include <bbb/dmx/fixture_json.hpp>
#include <bbb/dmx/frame_set.hpp>
#include <bbb/dmx/mask.hpp>
#include <bbb/dmx/max_external_utils.hpp>
#include <bbb/dmx/semantic_overrides.hpp>
#include <bbb/dmx/shutter_mapping.hpp>
#include <bbb/dmx/setup.hpp>

#include <algorithm>
#include <map>
#include <string>
#include <vector>

class bbb_dmx_mask : public c74::min::object<bbb_dmx_mask> {
private:
    bbb::dmx::dmx_frame_set input_frames_{};
    bbb::dmx::dmx_frame_set output_frames_{};
    std::vector<bbb::dmx::dmx_mask_rule> rules_{};
    bbb::dmx::fixture_mapper mapper_{};
    bbb::dmx::fixture_group_set groups_{};
    bbb::dmx::fixture_semantic_overrides semantic_overrides_{};
    std::map<std::string, std::size_t> fixture_indices_{};
    std::map<std::string, std::vector<std::string>> group_fixture_ids_cache_{};
    mutable std::map<std::pair<std::string, std::string>, std::string> parameter_alias_cache_{};
    std::string setup_path_value_{};
    std::string config_path_value_{};
    std::string patch_path_value_{};
    std::string groups_path_value_{};
    std::string semantic_overrides_path_value_{};
    int universe_value_{1};
    bool autobang_value_{true};
    bool setup_load_pending_{false};
    bool setup_loaded_{false};
    bool patch_load_pending_{false};
    bool groups_load_pending_{false};
    bool semantic_overrides_load_pending_{false};
    bool groups_loaded_{false};
    bool groups_validated_{false};
    bool semantic_overrides_loaded_{false};
    bool applying_setup_{false};
    bool suppress_setup_attribute_load_{false};
    bool suppress_config_attribute_load_{false};
    bool suppress_patch_attribute_load_{false};
    bool suppress_groups_attribute_load_{false};
    bool suppress_group_attribute_load_{false};
    bool suppress_semantic_overrides_attribute_load_{false};
    bool config_attribute_overridden_{false};
    bool patch_attribute_overridden_{false};
    bool groups_attribute_overridden_{false};
    bool semantic_overrides_attribute_overridden_{false};
    bool universe_attribute_overridden_{false};
    bool autobang_attribute_overridden_{false};

public:
    MIN_DESCRIPTION{"Mute, hold, allow, or force channel ranges in multi-universe DMX frames."};
    MIN_TAGS{"dmx, lighting, mask, mute, hold, universe, fixture, group"};
    MIN_AUTHOR{"2bit"};
    MIN_RELATED{"bbb.dmx.curve, bbb.dmx.safety, bbb.dmx.merge"};

    c74::min::inlet<> input{this, "(list/universe/channel/channels/readsetup/read/readpatch/readgroups/readoverrides/mute/hold/allow/force/*group/bang) DMX mask input"};
    c74::min::outlet<> output{this, "(anything) masked universe data"};
    c74::min::outlet<> status_output{this, "(anything) status and error messages"};

    c74::min::timer<c74::min::timer_options::defer_delivery> init_timer{this,
        MIN_FUNCTION {
            if(!config_path_value_.empty()) {
                load_config(config_path_value_);
            }
            return {};
        }
    };

    c74::min::timer<c74::min::timer_options::defer_delivery> setup_load_timer{this,
        MIN_FUNCTION {
            if(setup_load_pending_ && !setup_path_value_.empty()) {
                setup_load_pending_ = false;
                load_setup_file(setup_path_value_);
            }
            return {};
        }
    };

    c74::min::timer<c74::min::timer_options::defer_delivery> patch_load_timer{this,
        MIN_FUNCTION {
            if(patch_load_pending_ && !patch_path_value_.empty()) {
                patch_load_pending_ = false;
                load_patch_file(patch_path_value_);
            }
            return {};
        }
    };

    c74::min::timer<c74::min::timer_options::defer_delivery> groups_load_timer{this,
        MIN_FUNCTION {
            if(groups_load_pending_ && !groups_path_value_.empty()) {
                groups_load_pending_ = false;
                load_groups_file(groups_path_value_);
            }
            return {};
        }
    };

    c74::min::timer<c74::min::timer_options::defer_delivery> semantic_overrides_load_timer{this,
        MIN_FUNCTION {
            if(semantic_overrides_load_pending_ && !semantic_overrides_path_value_.empty()) {
                semantic_overrides_load_pending_ = false;
                load_semantic_overrides_file(semantic_overrides_path_value_);
            }
            return {};
        }
    };

    bbb_dmx_mask() {
        bbb::dmx::report_external_build_info(cout, "bbb.dmx.mask");
        init_timer.delay(0);
    }

    c74::min::attribute<c74::min::symbol> setup{this, "setup", "",
        c74::min::description{"Optional bbb.dmx setup JSON path. Loads mask config, shared paths, and defaults unless explicit object attributes override them."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(args.empty()) {
                if(!applying_setup_ && !suppress_setup_attribute_load_) {
                    setup_path_value_.clear();
                    setup_load_pending_ = false;
                    setup_loaded_ = false;
                }
                return {c74::min::symbol("")};
            }
            const c74::min::symbol symbol_value{(c74::min::symbol)args[0]};
            setup_path_value_ = symbol_value.c_str();
            if(!suppress_setup_attribute_load_) {
                schedule_setup_load();
            }
            return {symbol_value};
        }}
    };

    c74::min::attribute<c74::min::symbol> config{this, "config", "",
        c74::min::description{"Mask JSON path."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(!applying_setup_ && !suppress_config_attribute_load_) {
                config_attribute_overridden_ = true;
            }
            if(args.empty()) {
                config_path_value_.clear();
                return {c74::min::symbol("")};
            }
            const c74::min::symbol symbol_value{(c74::min::symbol)args[0]};
            config_path_value_ = symbol_value.c_str();
            return {symbol_value};
        }}
    };

    c74::min::attribute<c74::min::symbol> patch{this, "patch", "",
        c74::min::description{"Patch JSON file path used for fixture/parameter mask rules."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(!applying_setup_ && !suppress_patch_attribute_load_) {
                patch_attribute_overridden_ = true;
            }
            if(args.empty()) {
                patch_path_value_.clear();
                patch_load_pending_ = false;
                mapper_.clear();
                fixture_indices_.clear();
                group_fixture_ids_cache_.clear();
                parameter_alias_cache_.clear();
                groups_validated_ = false;
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

    c74::min::attribute<c74::min::symbol> groups{this, "groups", "",
        c74::min::description{"Optional bbb.dmx groups JSON path for *group mask messages."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(!applying_setup_ && !suppress_groups_attribute_load_) {
                groups_attribute_overridden_ = true;
            }
            if(args.empty()) {
                clear_groups();
                return {c74::min::symbol("")};
            }
            const c74::min::symbol symbol_value{(c74::min::symbol)args[0]};
            groups_path_value_ = symbol_value.c_str();
            if(!suppress_groups_attribute_load_) {
                schedule_groups_load();
            }
            return {symbol_value};
        }}
    };

    c74::min::attribute<c74::min::symbol> group{this, "group", "",
        c74::min::description{"Alias for @groups."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(!applying_setup_ && !suppress_group_attribute_load_) {
                groups_attribute_overridden_ = true;
            }
            if(args.empty()) {
                clear_groups();
                return {c74::min::symbol("")};
            }
            const c74::min::symbol symbol_value{(c74::min::symbol)args[0]};
            groups_path_value_ = symbol_value.c_str();
            if(!suppress_group_attribute_load_) {
                schedule_groups_load();
            }
            return {symbol_value};
        }}
    };

    c74::min::attribute<c74::min::symbol> semantic_overrides{this, "semantic_overrides", "",
        c74::min::description{"Optional bbb.dmx semantic overrides JSON path for fixture parameter aliases."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(!applying_setup_ && !suppress_semantic_overrides_attribute_load_) {
                semantic_overrides_attribute_overridden_ = true;
            }
            if(args.empty()) {
                semantic_overrides_path_value_.clear();
                semantic_overrides_load_pending_ = false;
                semantic_overrides_ = bbb::dmx::fixture_semantic_overrides{};
                semantic_overrides_loaded_ = false;
                parameter_alias_cache_.clear();
                return {c74::min::symbol("")};
            }
            const c74::min::symbol symbol_value{(c74::min::symbol)args[0]};
            semantic_overrides_path_value_ = symbol_value.c_str();
            if(!suppress_semantic_overrides_attribute_load_) {
                schedule_semantic_overrides_load();
            }
            return {symbol_value};
        }}
    };

    c74::min::attribute<int> universe{this, "universe", 1,
        c74::min::description{"Default universe for bare list input and bang output."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(!applying_setup_) {
                universe_attribute_overridden_ = true;
            }
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
            if(!applying_setup_) {
                autobang_attribute_overridden_ = true;
            }
            autobang_value_ = args.empty() || ((int)args[0] != 0);
            return {autobang_value_};
        }}
    };

    c74::min::message<> readsetup_message{this, "readsetup", "readsetup setup_json_path",
        MIN_FUNCTION {
            if(args.empty()) {
                report_error("readsetup requires setup JSON path");
                return {};
            }
            const c74::min::symbol path_symbol{(c74::min::symbol)args[0]};
            setup_path_value_ = path_symbol.c_str();
            suppress_setup_attribute_load_ = true;
            setup = path_symbol;
            suppress_setup_attribute_load_ = false;
            setup_load_pending_ = false;
            load_setup_file(setup_path_value_);
            return {};
        }
    };

    c74::min::message<> read_message{this, "read", "read mask_json_path",
        MIN_FUNCTION {
            if(args.empty()) {
                report_error("read requires mask JSON path");
                return {};
            }
            const c74::min::symbol path_symbol{(c74::min::symbol)args[0]};
            config_path_value_ = path_symbol.c_str();
            suppress_config_attribute_load_ = true;
            config = path_symbol;
            suppress_config_attribute_load_ = false;
            load_config(config_path_value_);
            return {};
        }
    };

    c74::min::message<> readpatch_message{this, "readpatch", "readpatch patch_json_path",
        MIN_FUNCTION {
            if(args.empty()) {
                report_error("readpatch requires patch JSON path");
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

    c74::min::message<> readgroups_message{this, "readgroups", "readgroups groups_json_path",
        MIN_FUNCTION {
            if(args.empty()) {
                report_error("readgroups requires groups JSON path");
                return {};
            }
            const c74::min::symbol path_symbol{(c74::min::symbol)args[0]};
            groups_path_value_ = path_symbol.c_str();
            suppress_groups_attribute_load_ = true;
            groups = path_symbol;
            suppress_groups_attribute_load_ = false;
            groups_load_pending_ = false;
            load_groups_file(groups_path_value_);
            return {};
        }
    };

    c74::min::message<> readoverrides_message{this, "readoverrides", "readoverrides semantic_overrides_json_path",
        MIN_FUNCTION {
            if(args.empty()) {
                report_error("readoverrides requires semantic overrides JSON path");
                return {};
            }
            const c74::min::symbol path_symbol{(c74::min::symbol)args[0]};
            semantic_overrides_path_value_ = path_symbol.c_str();
            suppress_semantic_overrides_attribute_load_ = true;
            semantic_overrides = path_symbol;
            suppress_semantic_overrides_attribute_load_ = false;
            semantic_overrides_load_pending_ = false;
            load_semantic_overrides_file(semantic_overrides_path_value_);
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

    c74::min::message<> mute_message{this, "mute", "mute universe start count OR mute fixture_id parameter [parameter ...]",
        MIN_FUNCTION {
            if(args.size() >= 3 && finite_atoms(args, 0, 3)) {
                add_rule((int)args[0], (int)args[1], (int)args[2], bbb::dmx::dmx_mask_action::mute, 0);
                return {};
            }
            const std::vector<bbb::dmx::dmx_mask_rule> previous_rules{rules_};
            handle_rule_result(add_fixture_parameter_rules(args, 0, bbb::dmx::dmx_mask_action::mute, 0, false, "mute"), previous_rules);
            return {};
        }
    };

    c74::min::message<> hold_message{this, "hold", "hold universe start count OR hold fixture_id parameter [parameter ...]",
        MIN_FUNCTION {
            if(args.size() >= 3 && finite_atoms(args, 0, 3)) {
                add_rule((int)args[0], (int)args[1], (int)args[2], bbb::dmx::dmx_mask_action::hold, 0);
                return {};
            }
            const std::vector<bbb::dmx::dmx_mask_rule> previous_rules{rules_};
            handle_rule_result(add_fixture_parameter_rules(args, 0, bbb::dmx::dmx_mask_action::hold, 0, false, "hold"), previous_rules);
            return {};
        }
    };

    c74::min::message<> allow_message{this, "allow", "allow universe start count OR allow fixture_id parameter [parameter ...]",
        MIN_FUNCTION {
            if(args.size() >= 3 && finite_atoms(args, 0, 3)) {
                add_rule((int)args[0], (int)args[1], (int)args[2], bbb::dmx::dmx_mask_action::allow, 0);
                return {};
            }
            const std::vector<bbb::dmx::dmx_mask_rule> previous_rules{rules_};
            handle_rule_result(add_fixture_parameter_rules(args, 0, bbb::dmx::dmx_mask_action::allow, 0, false, "allow"), previous_rules);
            return {};
        }
    };

    c74::min::message<> force_message{this, "force", "force universe start count value OR force fixture_id parameter value [parameter value ...]",
        MIN_FUNCTION {
            if(args.size() >= 4 && finite_atoms(args, 0, 4)) {
                add_rule((int)args[0], (int)args[1], (int)args[2], bbb::dmx::dmx_mask_action::force, (int)args[3]);
                return {};
            }
            const std::vector<bbb::dmx::dmx_mask_rule> previous_rules{rules_};
            handle_rule_result(add_fixture_force_parameter_rules(args, 0, false, "force"), previous_rules);
            return {};
        }
    };

    c74::min::message<> mutefixture_message{this, "mutefixture", "mutefixture fixture_id parameter [parameter ...]",
        MIN_FUNCTION {
            const std::vector<bbb::dmx::dmx_mask_rule> previous_rules{rules_};
            handle_rule_result(add_fixture_parameter_rules(args, 0, bbb::dmx::dmx_mask_action::mute, 0, false, "mutefixture"), previous_rules);
            return {};
        }
    };

    c74::min::message<> holdfixture_message{this, "holdfixture", "holdfixture fixture_id parameter [parameter ...]",
        MIN_FUNCTION {
            const std::vector<bbb::dmx::dmx_mask_rule> previous_rules{rules_};
            handle_rule_result(add_fixture_parameter_rules(args, 0, bbb::dmx::dmx_mask_action::hold, 0, false, "holdfixture"), previous_rules);
            return {};
        }
    };

    c74::min::message<> allowfixture_message{this, "allowfixture", "allowfixture fixture_id parameter [parameter ...]",
        MIN_FUNCTION {
            const std::vector<bbb::dmx::dmx_mask_rule> previous_rules{rules_};
            handle_rule_result(add_fixture_parameter_rules(args, 0, bbb::dmx::dmx_mask_action::allow, 0, false, "allowfixture"), previous_rules);
            return {};
        }
    };

    c74::min::message<> forcefixture_message{this, "forcefixture", "forcefixture fixture_id parameter value [parameter value ...]",
        MIN_FUNCTION {
            const std::vector<bbb::dmx::dmx_mask_rule> previous_rules{rules_};
            handle_rule_result(add_fixture_force_parameter_rules(args, 0, false, "forcefixture"), previous_rules);
            return {};
        }
    };

    c74::min::message<> mutegroup_message{this, "mutegroup", "mutegroup group_id parameter [parameter ...]",
        MIN_FUNCTION {
            const std::vector<bbb::dmx::dmx_mask_rule> previous_rules{rules_};
            handle_rule_result(add_group_parameter_rules(args, bbb::dmx::dmx_mask_action::mute, 0, "mutegroup"), previous_rules);
            return {};
        }
    };

    c74::min::message<> holdgroup_message{this, "holdgroup", "holdgroup group_id parameter [parameter ...]",
        MIN_FUNCTION {
            const std::vector<bbb::dmx::dmx_mask_rule> previous_rules{rules_};
            handle_rule_result(add_group_parameter_rules(args, bbb::dmx::dmx_mask_action::hold, 0, "holdgroup"), previous_rules);
            return {};
        }
    };

    c74::min::message<> allowgroup_message{this, "allowgroup", "allowgroup group_id parameter [parameter ...]",
        MIN_FUNCTION {
            const std::vector<bbb::dmx::dmx_mask_rule> previous_rules{rules_};
            handle_rule_result(add_group_parameter_rules(args, bbb::dmx::dmx_mask_action::allow, 0, "allowgroup"), previous_rules);
            return {};
        }
    };

    c74::min::message<> forcegroup_message{this, "forcegroup", "forcegroup group_id parameter value [parameter value ...]",
        MIN_FUNCTION {
            const std::vector<bbb::dmx::dmx_mask_rule> previous_rules{rules_};
            handle_rule_result(add_group_force_parameter_rules(args, "forcegroup"), previous_rules);
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
            atoms.push_back(c74::min::symbol("setup_loaded"));
            atoms.push_back(setup_loaded_ ? 1 : 0);
            atoms.push_back(c74::min::symbol("patch_loaded"));
            atoms.push_back(mapper_.validated() ? 1 : 0);
            atoms.push_back(c74::min::symbol("groups_loaded"));
            atoms.push_back(groups_loaded_ ? 1 : 0);
            atoms.push_back(c74::min::symbol("semantic_overrides_loaded"));
            atoms.push_back(semantic_overrides_loaded_ ? 1 : 0);
            status_output.send(atoms);
            return {};
        }
    };

private:
    void add_rule(int universe_id, int start, int count, bbb::dmx::dmx_mask_action action, int value) {
        bbb::dmx::dmx_mask_rule rule{};
        rule.universe = std::max(0, universe_id);
        rule.start = start;
        rule.count = count;
        rule.action = action;
        rule.value = value;
        rules_.push_back(rule);
        report_status("rule_added");
    }

    void handle_frame_write(const bbb::dmx::maxutil::frame_write_result &result) {
        if(!result.ok) {
            report_error(result.message);
            return;
        }
        apply_and_output(result.universe);
    }

    void apply_and_output(int universe_id) {
        const int sanitized_universe{bbb::dmx::sanitize_universe_id(universe_id)};
        const bbb::dmx::dmx_universe previous_output{output_frames_.universe(sanitized_universe)};
        output_frames_.universes[sanitized_universe] = bbb::dmx::apply_mask_rules(input_frames_.universe(sanitized_universe), previous_output, sanitized_universe, rules_);
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
        std::vector<bbb::dmx::dmx_mask_rule> loaded{};
        result = parse_rules(parsed.value, loaded);
        if(!result.ok) {
            report_error(result.message);
            return;
        }
        rules_ = loaded;
        report_status("config_loaded");
    }

    bbb::dmx::mapper_result parse_rules(const bbb::dmx::json_value &root, std::vector<bbb::dmx::dmx_mask_rule> &rules) const {
        if(root.type != bbb::dmx::json_type::object) {
            return bbb::dmx::mapper_result::failure("mask root must be object");
        }
        const bbb::dmx::json_value *rule_array{root.find("rules")};
        if(!rule_array || rule_array->type != bbb::dmx::json_type::array) {
            return bbb::dmx::mapper_result::failure("mask rules must be array");
        }
        std::string error{};
        for(const auto &rule_value : rule_array->array_value) {
            if(rule_value.type != bbb::dmx::json_type::object) {
                return bbb::dmx::mapper_result::failure("mask rule must be object");
            }
            bbb::dmx::dmx_mask_rule rule{};
            bbb::dmx::json_int(rule_value, "universe", rule.universe, false, error);
            bbb::dmx::json_int(rule_value, "start", rule.start, false, error);
            int end{0};
            bbb::dmx::json_int(rule_value, "count", rule.count, false, error);
            if(bbb::dmx::json_int(rule_value, "end", end, false, error) && 0 < end) {
                rule.count = end - rule.start + 1;
            }
            std::string action_text{"mute"};
            bbb::dmx::json_string(rule_value, "action", action_text, false, error);
            if(!bbb::dmx::mask_action_from_string(action_text, rule.action)) {
                return bbb::dmx::mapper_result::failure("unknown mask action: " + action_text);
            }
            bbb::dmx::json_int(rule_value, "value", rule.value, false, error);
            rules.push_back(rule);
        }
        return bbb::dmx::mapper_result::success();
    }



    static bool finite_atom(const c74::min::atom &atom) {
        if(atom.a_type != c74::max::A_LONG && atom.a_type != c74::max::A_FLOAT) {
            return false;
        }
        return bbb::dmx::maxutil::finite_atom(atom);
    }

    static bool finite_atoms(const c74::min::atoms &atoms, std::size_t start, std::size_t count) {
        for(std::size_t index{start}; index < start + count; index++) {
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

    bool handle_result(const bbb::dmx::mapper_result &result) {
        if(result.ok) {
            return true;
        }
        report_error(result.message);
        return false;
    }

    bool handle_rule_result(const bbb::dmx::mapper_result &result, const std::vector<bbb::dmx::dmx_mask_rule> &previous_rules) {
        if(result.ok) {
            return true;
        }
        rules_ = previous_rules;
        report_error(result.message);
        return false;
    }

    void schedule_setup_load() {
        if(setup_path_value_.empty()) {
            setup_load_pending_ = false;
            return;
        }
        setup_load_pending_ = true;
        setup_load_timer.delay(0);
    }

    std::string setup_relative_path(const std::string &base_directory, const std::string &path) const {
        if(path.empty()) {
            return path;
        }
        if(bbb::dmx::path_is_absolute(path)) {
            const std::string system_path{bbb::dmx::maxutil::max_path_to_system_path(path)};
            if(!system_path.empty()) {
                return system_path;
            }
            return path;
        }
        return bbb::dmx::join_relative_path(base_directory, path);
    }

    void set_symbol_attribute_from_setup(c74::min::attribute<c74::min::symbol> &attribute, const std::string &value) {
        applying_setup_ = true;
        attribute = c74::min::symbol(value.c_str());
        applying_setup_ = false;
    }

    void set_int_attribute_from_setup(c74::min::attribute<int> &attribute, int value) {
        applying_setup_ = true;
        attribute = value;
        applying_setup_ = false;
    }

    void set_bool_attribute_from_setup(c74::min::attribute<bool> &attribute, bool value) {
        applying_setup_ = true;
        attribute = value;
        applying_setup_ = false;
    }

    void apply_setup_file_path(
        c74::min::attribute<c74::min::symbol> &attribute,
        bool &suppress_attribute_load,
        std::string &path_value,
        const std::string &resolved_path
    ) {
        suppress_attribute_load = true;
        set_symbol_attribute_from_setup(attribute, resolved_path);
        suppress_attribute_load = false;
        path_value = resolved_path;
    }

    void apply_setup_values(const bbb::dmx::dmx_setup_values &values, const std::string &base_directory) {
        if(values.universe.has_value() && !universe_attribute_overridden_) {
            set_int_attribute_from_setup(universe, values.universe.value());
        }
        if(values.autobang.has_value() && !autobang_attribute_overridden_) {
            set_bool_attribute_from_setup(autobang, values.autobang.value());
        }

        if(values.config.has_value() && !config_attribute_overridden_) {
            const std::string resolved_path{setup_relative_path(base_directory, values.config.value())};
            apply_setup_file_path(config, suppress_config_attribute_load_, config_path_value_, resolved_path);
            load_config(config_path_value_);
        }
        if(values.patch.has_value() && !patch_attribute_overridden_) {
            const std::string resolved_path{setup_relative_path(base_directory, values.patch.value())};
            apply_setup_file_path(patch, suppress_patch_attribute_load_, patch_path_value_, resolved_path);
            patch_load_pending_ = false;
            load_patch_file(patch_path_value_);
        }
        if(values.groups.has_value() && !groups_attribute_overridden_) {
            const std::string resolved_path{setup_relative_path(base_directory, values.groups.value())};
            apply_setup_file_path(groups, suppress_groups_attribute_load_, groups_path_value_, resolved_path);
            suppress_group_attribute_load_ = true;
            set_symbol_attribute_from_setup(group, resolved_path);
            suppress_group_attribute_load_ = false;
            groups_load_pending_ = false;
            load_groups_file(groups_path_value_);
        }
        if(values.semantic_overrides.has_value() && !semantic_overrides_attribute_overridden_) {
            const std::string resolved_path{setup_relative_path(base_directory, values.semantic_overrides.value())};
            apply_setup_file_path(semantic_overrides, suppress_semantic_overrides_attribute_load_, semantic_overrides_path_value_, resolved_path);
            semantic_overrides_load_pending_ = false;
            load_semantic_overrides_file(semantic_overrides_path_value_);
        }
    }

    void load_setup_file(const std::string &path) {
        const std::string resolved_path{bbb::dmx::maxutil::resolve_file_path(this->maxobj(), path)};
        bbb::dmx::dmx_setup_document setup_document{};
        const bbb::dmx::mapper_result result{bbb::dmx::read_dmx_setup_file(resolved_path, setup_document)};
        setup_loaded_ = false;
        if(!handle_result(result)) {
            return;
        }
        const std::string base_directory{bbb::dmx::parent_directory(resolved_path)};
        const bbb::dmx::dmx_setup_values values{bbb::dmx::merge_setup_values(setup_document.common, setup_document.mask)};
        apply_setup_values(values, base_directory);
        setup_loaded_ = true;
        report_status("setup_loaded");
    }

    void schedule_patch_load() {
        if(patch_path_value_.empty()) {
            patch_load_pending_ = false;
            return;
        }
        patch_load_pending_ = true;
        patch_load_timer.delay(0);
    }

    void schedule_groups_load() {
        if(groups_path_value_.empty()) {
            groups_load_pending_ = false;
            return;
        }
        groups_load_pending_ = true;
        groups_load_timer.delay(0);
    }

    void schedule_semantic_overrides_load() {
        if(semantic_overrides_path_value_.empty()) {
            semantic_overrides_load_pending_ = false;
            return;
        }
        semantic_overrides_load_pending_ = true;
        semantic_overrides_load_timer.delay(0);
    }

    void clear_groups() {
        groups_path_value_.clear();
        groups_load_pending_ = false;
        groups_ = bbb::dmx::fixture_group_set{};
        groups_loaded_ = false;
        groups_validated_ = false;
        group_fixture_ids_cache_.clear();
    }

    void load_patch_file(const std::string &path) {
        const std::string resolved_path{bbb::dmx::maxutil::resolve_file_path(this->maxobj(), path)};
        bbb::dmx::fixture_mapper loaded_mapper{};
        const bbb::dmx::mapper_result result{bbb::dmx::load_fixture_mapper_from_patch_file(resolved_path, loaded_mapper)};
        if(!handle_result(result)) {
            return;
        }
        mapper_ = loaded_mapper;
        rebuild_fixture_indices();
        group_fixture_ids_cache_.clear();
        parameter_alias_cache_.clear();
        groups_validated_ = false;
        if(groups_loaded_) {
            handle_result(validate_loaded_groups());
        }
        report_status("patch_loaded");
    }

    void load_groups_file(const std::string &path) {
        const std::string resolved_path{bbb::dmx::maxutil::resolve_file_path(this->maxobj(), path)};
        bbb::dmx::fixture_group_set loaded_groups{};
        const bbb::dmx::mapper_result result{bbb::dmx::read_fixture_groups_file(resolved_path, loaded_groups)};
        if(!handle_result(result)) {
            return;
        }
        if(!mapper_.patch().fixtures.empty()) {
            const bbb::dmx::mapper_result validate_result{bbb::dmx::validate_fixture_groups_for_patch(loaded_groups, mapper_.patch())};
            if(!handle_result(validate_result)) {
                return;
            }
        }
        groups_ = loaded_groups;
        groups_loaded_ = true;
        groups_validated_ = !mapper_.patch().fixtures.empty();
        group_fixture_ids_cache_.clear();
        report_status("groups_loaded");
    }

    void load_semantic_overrides_file(const std::string &path) {
        const std::string resolved_path{bbb::dmx::maxutil::resolve_file_path(this->maxobj(), path)};
        bbb::dmx::fixture_semantic_overrides loaded_overrides{};
        const bbb::dmx::mapper_result result{bbb::dmx::read_fixture_semantic_overrides_file(resolved_path, loaded_overrides)};
        if(!handle_result(result)) {
            return;
        }
        semantic_overrides_ = loaded_overrides;
        semantic_overrides_loaded_ = true;
        parameter_alias_cache_.clear();
        report_status("semantic_overrides_loaded");
    }

    bbb::dmx::mapper_result validate_loaded_groups() {
        if(!groups_loaded_) {
            return bbb::dmx::mapper_result::failure("no groups loaded");
        }
        if(mapper_.patch().fixtures.empty()) {
            groups_validated_ = false;
            return bbb::dmx::mapper_result::success();
        }
        const bbb::dmx::mapper_result result{bbb::dmx::validate_fixture_groups_for_patch(groups_, mapper_.patch())};
        groups_validated_ = result.ok;
        return result;
    }

    bbb::dmx::mapper_result resolve_group_fixture_ids(const std::string &group_id, std::vector<std::string> &fixture_ids) {
        fixture_ids.clear();
        if(mapper_.patch().fixtures.empty()) {
            return bbb::dmx::mapper_result::failure("group operation requires a loaded patch with fixtures");
        }
        if(!groups_loaded_) {
            return bbb::dmx::mapper_result::failure("group operation requires loaded groups");
        }
        if(!groups_validated_) {
            const bbb::dmx::mapper_result validate_result{validate_loaded_groups()};
            if(!validate_result.ok) {
                group_fixture_ids_cache_.clear();
                return validate_result;
            }
        }
        const auto cached_group = group_fixture_ids_cache_.find(group_id);
        if(cached_group != group_fixture_ids_cache_.end()) {
            fixture_ids = cached_group->second;
            return bbb::dmx::mapper_result::success();
        }
        bbb::dmx::mapper_result result{bbb::dmx::resolve_fixture_group_fixture_ids(groups_, mapper_.patch(), group_id, fixture_ids)};
        if(result.ok) {
            group_fixture_ids_cache_[group_id] = fixture_ids;
        }
        return result;
    }

    const bbb::dmx::fixture_instance *find_fixture_instance(const std::string &fixture_id) const {
        const auto indexed_fixture = fixture_indices_.find(fixture_id);
        if(indexed_fixture != fixture_indices_.end() && indexed_fixture->second < mapper_.patch().fixtures.size()) {
            const bbb::dmx::fixture_instance &fixture{mapper_.patch().fixtures[indexed_fixture->second]};
            if(fixture.id == fixture_id) {
                return &fixture;
            }
        }
        for(const auto &fixture : mapper_.patch().fixtures) {
            if(fixture.id == fixture_id) {
                return &fixture;
            }
        }
        return nullptr;
    }

    void rebuild_fixture_indices() {
        fixture_indices_.clear();
        for(std::size_t fixture_index{0}; fixture_index < mapper_.patch().fixtures.size(); fixture_index++) {
            fixture_indices_[mapper_.patch().fixtures[fixture_index].id] = fixture_index;
        }
    }

    const bbb::dmx::fixture_semantic_mode_override *semantic_override_for_fixture(const bbb::dmx::fixture_instance &fixture) const {
        if(!semantic_overrides_loaded_) {
            return nullptr;
        }
        return semantic_overrides_.find_mode_override(fixture.profile, fixture.mode);
    }

    std::string resolve_parameter_alias(const std::string &fixture_id, const std::string &parameter_key) const {
        const std::pair<std::string, std::string> cache_key{fixture_id, parameter_key};
        const auto cached_alias = parameter_alias_cache_.find(cache_key);
        if(cached_alias != parameter_alias_cache_.end()) {
            return cached_alias->second;
        }
        const bbb::dmx::fixture_instance *fixture{find_fixture_instance(fixture_id)};
        if(!fixture) {
            parameter_alias_cache_[cache_key] = parameter_key;
            return parameter_key;
        }
        const bbb::dmx::fixture_semantic_mode_override *mode_override{semantic_override_for_fixture(*fixture)};
        if(!mode_override) {
            parameter_alias_cache_[cache_key] = parameter_key;
            return parameter_key;
        }
        const std::string resolved_alias{mode_override->resolve_alias(parameter_key)};
        parameter_alias_cache_[cache_key] = resolved_alias;
        return resolved_alias;
    }

    static void append_unique_parameter(std::vector<std::string> &parameters, const std::string &parameter) {
        if(parameter.empty()) {
            return;
        }
        if(std::find(parameters.begin(), parameters.end(), parameter) == parameters.end()) {
            parameters.push_back(parameter);
        }
    }

    bbb::dmx::mapper_result semantic_parameter_keys_for_fixture(const std::string &fixture_id, const std::string &requested_parameter, std::vector<std::string> &parameters) const {
        parameters.clear();
        const bbb::dmx::fixture_instance *fixture{find_fixture_instance(fixture_id)};
        if(!fixture) {
            return bbb::dmx::mapper_result::failure("unknown fixture: " + fixture_id);
        }
        const bbb::dmx::fixture_profile *profile{mapper_.find_profile(fixture->profile)};
        if(!profile) {
            return bbb::dmx::mapper_result::failure("missing profile: " + fixture->profile);
        }
        const bbb::dmx::fixture_mode *mode{profile->find_mode(fixture->mode)};
        if(!mode) {
            return bbb::dmx::mapper_result::failure("missing mode: " + fixture->mode);
        }
        const bbb::dmx::fixture_semantic_mode_override *mode_override{semantic_override_for_fixture(*fixture)};
        const std::string normalized_key{bbb::dmx::normalized_semantic_key(requested_parameter)};
        if(normalized_key == "dimmer" || normalized_key == "intensity") {
            const bbb::dmx::semantic_color_mapping mapping{bbb::dmx::semantic_intensity_parameters_for_mode(*mode, 1.0, mode_override)};
            if(mapping.ok) {
                for(const auto &parameter : mapping.parameters) {
                    append_unique_parameter(parameters, parameter.first);
                }
            }
        } else if(normalized_key == "shutter") {
            const bbb::dmx::semantic_shutter_mappings mappings{bbb::dmx::semantic_shutter_parameters_for_mode(*mode, true, mode_override)};
            if(mappings.ok) {
                for(const auto &mapping : mappings.mappings) {
                    append_unique_parameter(parameters, mapping.parameter);
                }
            }
        } else if(normalized_key == "color" || normalized_key == "rgb" || normalized_key == "cmy") {
            const bbb::dmx::semantic_color_request white{bbb::dmx::make_semantic_color_request(1.0, 1.0, 1.0)};
            const bbb::dmx::semantic_color_mapping mapping{bbb::dmx::semantic_color_parameters_for_mode(
                profile,
                *mode,
                white,
                bbb::dmx::semantic_color_options{true, false},
                {},
                mode_override
            )};
            if(mapping.ok) {
                for(const auto &parameter : mapping.parameters) {
                    append_unique_parameter(parameters, parameter.first);
                }
            }
        }

        if(parameters.empty()) {
            const std::string resolved_parameter{resolve_parameter_alias(fixture_id, requested_parameter)};
            if(!mode->find_parameter(resolved_parameter)) {
                return bbb::dmx::mapper_result::failure("unknown parameter: " + requested_parameter);
            }
            parameters.push_back(resolved_parameter);
        }
        return bbb::dmx::mapper_result::success();
    }

    bbb::dmx::mapper_result add_fixture_footprint_rule(const std::string &fixture_id, bbb::dmx::dmx_mask_action action, int value) {
        const bbb::dmx::fixture_instance *fixture{find_fixture_instance(fixture_id)};
        if(!fixture) {
            return bbb::dmx::mapper_result::failure("unknown fixture: " + fixture_id);
        }
        const bbb::dmx::fixture_profile *profile{mapper_.find_profile(fixture->profile)};
        if(!profile) {
            return bbb::dmx::mapper_result::failure("missing profile: " + fixture->profile);
        }
        const bbb::dmx::fixture_mode *mode{profile->find_mode(fixture->mode)};
        if(!mode) {
            return bbb::dmx::mapper_result::failure("missing mode: " + fixture->mode);
        }
        add_rule(fixture->universe, fixture->address, mode->footprint, action, value);
        return bbb::dmx::mapper_result::success();
    }

    bbb::dmx::mapper_result add_parameter_address_rules(const std::string &fixture_id, const std::string &parameter, bbb::dmx::dmx_mask_action action, int value) {
        std::vector<std::pair<int, int>> addresses{};
        const bbb::dmx::mapper_result result{mapper_.parameter_channel_addresses(fixture_id, parameter, addresses)};
        if(!result.ok) {
            return result;
        }
        if(addresses.empty()) {
            return bbb::dmx::mapper_result::failure("parameter has no channels: " + parameter);
        }
        std::sort(addresses.begin(), addresses.end());
        int current_universe{addresses.front().first};
        int current_start{addresses.front().second};
        int previous_address{current_start};
        for(std::size_t index{1}; index < addresses.size(); index++) {
            const int universe_id{addresses[index].first};
            const int address{addresses[index].second};
            if(universe_id == current_universe && address == previous_address + 1) {
                previous_address = address;
                continue;
            }
            add_rule(current_universe, current_start, previous_address - current_start + 1, action, value);
            current_universe = universe_id;
            current_start = address;
            previous_address = address;
        }
        add_rule(current_universe, current_start, previous_address - current_start + 1, action, value);
        return bbb::dmx::mapper_result::success();
    }

    bool parameter_supported_for_fixture(const std::string &fixture_id, const std::string &requested_parameter) const {
        const std::string normalized_key{bbb::dmx::normalized_semantic_key(requested_parameter)};
        if(normalized_key == "fixture" || normalized_key == "footprint" || normalized_key == "all") {
            return find_fixture_instance(fixture_id) != nullptr;
        }
        std::vector<std::string> resolved_parameters{};
        return semantic_parameter_keys_for_fixture(fixture_id, requested_parameter, resolved_parameters).ok;
    }

    bbb::dmx::mapper_result add_fixture_parameter_rule(const std::string &fixture_id, const std::string &requested_parameter, bbb::dmx::dmx_mask_action action, int value, bool ignore_unknown_parameters) {
        const std::string normalized_key{bbb::dmx::normalized_semantic_key(requested_parameter)};
        if(normalized_key == "fixture" || normalized_key == "footprint" || normalized_key == "all") {
            return add_fixture_footprint_rule(fixture_id, action, value);
        }
        std::vector<std::string> parameters{};
        bbb::dmx::mapper_result result{semantic_parameter_keys_for_fixture(fixture_id, requested_parameter, parameters)};
        if(!result.ok) {
            if(ignore_unknown_parameters && is_unknown_parameter_result(result)) {
                return bbb::dmx::mapper_result::success();
            }
            return result;
        }
        int applied_count{0};
        for(const std::string &parameter : parameters) {
            result = add_parameter_address_rules(fixture_id, parameter, action, value);
            if(!result.ok) {
                if(ignore_unknown_parameters && is_unknown_parameter_result(result)) {
                    continue;
                }
                return result;
            }
            applied_count++;
        }
        if(applied_count == 0) {
            return bbb::dmx::mapper_result::failure("unknown parameter: " + requested_parameter);
        }
        return bbb::dmx::mapper_result::success();
    }

    bbb::dmx::mapper_result add_fixture_parameter_rules(const c74::min::atoms &args, std::size_t start_index, bbb::dmx::dmx_mask_action action, int value, bool ignore_unknown_parameters, const std::string &operation_name) {
        if(args.size() <= start_index + 1) {
            return bbb::dmx::mapper_result::failure(operation_name + " requires fixture_id parameter");
        }
        if(finite_atom(args[start_index])) {
            return bbb::dmx::mapper_result::failure(operation_name + " fixture_id must be a symbol");
        }
        const std::string fixture_id{symbol_arg(args[start_index])};
        for(std::size_t index{start_index + 1}; index < args.size(); index++) {
            if(finite_atom(args[index])) {
                return bbb::dmx::mapper_result::failure(operation_name + " parameter name expected before numeric value");
            }
            const bbb::dmx::mapper_result result{add_fixture_parameter_rule(fixture_id, symbol_arg(args[index]), action, value, ignore_unknown_parameters)};
            if(!result.ok) {
                return bbb::dmx::mapper_result::failure(operation_name + " fixture " + fixture_id + ": " + result.message);
            }
        }
        return bbb::dmx::mapper_result::success();
    }

    bbb::dmx::mapper_result add_fixture_force_parameter_rules(const c74::min::atoms &args, std::size_t start_index, bool ignore_unknown_parameters, const std::string &operation_name) {
        if(args.size() <= start_index + 2) {
            return bbb::dmx::mapper_result::failure(operation_name + " requires fixture_id parameter value");
        }
        if(finite_atom(args[start_index])) {
            return bbb::dmx::mapper_result::failure(operation_name + " fixture_id must be a symbol");
        }
        const std::string fixture_id{symbol_arg(args[start_index])};
        std::size_t index{start_index + 1};
        while(index < args.size()) {
            if(args.size() <= index + 1 || finite_atom(args[index]) || !finite_atom(args[index + 1])) {
                return bbb::dmx::mapper_result::failure(operation_name + " requires parameter/value pairs");
            }
            const std::string parameter{symbol_arg(args[index])};
            const int value{clamp_int((int)args[index + 1], 0, 255)};
            const bbb::dmx::mapper_result result{add_fixture_parameter_rule(fixture_id, parameter, bbb::dmx::dmx_mask_action::force, value, ignore_unknown_parameters)};
            if(!result.ok) {
                return bbb::dmx::mapper_result::failure(operation_name + " fixture " + fixture_id + ": " + result.message);
            }
            index += 2;
        }
        return bbb::dmx::mapper_result::success();
    }

    bbb::dmx::mapper_result add_group_parameter_rules(const c74::min::atoms &args, bbb::dmx::dmx_mask_action action, int value, const std::string &operation_name) {
        if(args.size() < 2) {
            return bbb::dmx::mapper_result::failure(operation_name + " requires group_id parameter");
        }
        if(finite_atom(args[0])) {
            return bbb::dmx::mapper_result::failure(operation_name + " group_id must be a symbol");
        }
        std::vector<std::string> fixture_ids{};
        bbb::dmx::mapper_result result{resolve_group_fixture_ids(symbol_arg(args[0]), fixture_ids)};
        if(!result.ok) {
            return result;
        }
        if(fixture_ids.empty()) {
            return bbb::dmx::mapper_result::failure(operation_name + " requires a non-empty group: " + symbol_arg(args[0]));
        }
        std::vector<std::string> parameters{};
        for(std::size_t index{1}; index < args.size(); index++) {
            if(finite_atom(args[index])) {
                return bbb::dmx::mapper_result::failure(operation_name + " parameter name expected before numeric value");
            }
            parameters.push_back(symbol_arg(args[index]));
        }
        std::vector<int> applied_counts(parameters.size(), 0);
        for(const std::string &fixture_id : fixture_ids) {
            for(std::size_t parameter_index{0}; parameter_index < parameters.size(); parameter_index++) {
                result = add_fixture_parameter_rule(fixture_id, parameters[parameter_index], action, value, true);
                if(!result.ok) {
                    return bbb::dmx::mapper_result::failure(operation_name + " fixture " + fixture_id + ": " + result.message);
                }
                if(parameter_supported_for_fixture(fixture_id, parameters[parameter_index])) {
                    applied_counts[parameter_index]++;
                }
            }
        }
        for(std::size_t parameter_index{0}; parameter_index < parameters.size(); parameter_index++) {
            if(applied_counts[parameter_index] == 0) {
                return bbb::dmx::mapper_result::failure(operation_name + " unknown parameter for all group fixtures: " + parameters[parameter_index]);
            }
        }
        return bbb::dmx::mapper_result::success();
    }

    bbb::dmx::mapper_result add_group_force_parameter_rules(const c74::min::atoms &args, const std::string &operation_name) {
        if(args.size() < 3) {
            return bbb::dmx::mapper_result::failure(operation_name + " requires group_id parameter value");
        }
        if(finite_atom(args[0])) {
            return bbb::dmx::mapper_result::failure(operation_name + " group_id must be a symbol");
        }
        std::vector<std::string> fixture_ids{};
        bbb::dmx::mapper_result result{resolve_group_fixture_ids(symbol_arg(args[0]), fixture_ids)};
        if(!result.ok) {
            return result;
        }
        if(fixture_ids.empty()) {
            return bbb::dmx::mapper_result::failure(operation_name + " requires a non-empty group: " + symbol_arg(args[0]));
        }
        struct force_assignment {
        public:
            std::string parameter{};
            int value{0};
        };
        std::vector<force_assignment> assignments{};
        std::size_t index{1};
        while(index < args.size()) {
            if(args.size() <= index + 1 || finite_atom(args[index]) || !finite_atom(args[index + 1])) {
                return bbb::dmx::mapper_result::failure(operation_name + " requires parameter/value pairs");
            }
            assignments.push_back(force_assignment{symbol_arg(args[index]), clamp_int((int)args[index + 1], 0, 255)});
            index += 2;
        }
        std::vector<int> applied_counts(assignments.size(), 0);
        for(const std::string &fixture_id : fixture_ids) {
            for(std::size_t assignment_index{0}; assignment_index < assignments.size(); assignment_index++) {
                result = add_fixture_parameter_rule(fixture_id, assignments[assignment_index].parameter, bbb::dmx::dmx_mask_action::force, assignments[assignment_index].value, true);
                if(!result.ok) {
                    return bbb::dmx::mapper_result::failure(operation_name + " fixture " + fixture_id + ": " + result.message);
                }
                if(parameter_supported_for_fixture(fixture_id, assignments[assignment_index].parameter)) {
                    applied_counts[assignment_index]++;
                }
            }
        }
        for(std::size_t assignment_index{0}; assignment_index < assignments.size(); assignment_index++) {
            if(applied_counts[assignment_index] == 0) {
                return bbb::dmx::mapper_result::failure(operation_name + " unknown parameter for all group fixtures: " + assignments[assignment_index].parameter);
            }
        }
        return bbb::dmx::mapper_result::success();
    }

    static bool is_unknown_parameter_result(const bbb::dmx::mapper_result &result) {
        const std::string prefix{"unknown parameter: "};
        return !result.ok && result.message.rfind(prefix, 0) == 0;
    }

    void report_status(const char *message) {
        bbb::dmx::maxutil::send_status(status_output, "status", message);
    }

    void report_error(const std::string &message) {
        cerr << "bbb.dmx.mask: " << message << c74::min::endl;
        bbb::dmx::maxutil::send_status(status_output, "error", message);
    }
};

MIN_EXTERNAL(bbb_dmx_mask);
