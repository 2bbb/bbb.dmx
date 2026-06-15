#include "c74_min.h"

#include <bbb/dmx/fixture_groups.hpp>
#include <bbb/dmx/fixture_json.hpp>
#include <bbb/dmx/max_external_utils.hpp>
#include <bbb/dmx/movertrack.hpp>

#include <algorithm>

#include <cstdint>
#include <map>
#include <string>
#include <vector>

class bbb_dmx_fixturemap : public c74::min::object<bbb_dmx_fixturemap> {
private:
    bbb::dmx::fixture_mapper mapper_{};
    std::string patch_path_value_{};
    std::string groups_path_value_{};
    bbb::dmx::fixture_group_set groups_{};
    int universe_value_{1};
    bool autobang_value_{true};
    bool output_all_universes_{false};
    bbb::dmx::tracking_mode tracking_mode_value_{bbb::dmx::tracking_mode::smart};
    double default_pan_range_value_{540.0};
    double default_tilt_range_value_{270.0};
    bool track_strict_value_{false};
    bool color_use_white_value_{true};
    bool color_wheel_fallback_value_{false};
    std::map<std::string, bbb::dmx::movertrack_engine> tracking_engines_{};
    bool warn_invalid_numeric_{false};
    bool warn_invalid_universe_mode_{false};
    bool warn_invalid_tracking_mode_{false};
    bool warn_invalid_range_{false};
    bool warn_runtime_error_{false};
    bool patch_load_pending_{false};
    bool groups_load_pending_{false};
    bool groups_loaded_{false};
    bool groups_validated_{false};
    bool suppress_patch_attribute_load_{false};
    bool suppress_groups_attribute_load_{false};

public:
    MIN_DESCRIPTION{"Map semantic fixture parameters into one or more 512-channel DMX universe lists."};
    MIN_TAGS{"dmx, lighting, fixture, patch, universe, mapping"};
    MIN_AUTHOR{"2bit"};
    MIN_RELATED{"bbb.dmx.movertrack"};

    c74::min::inlet<> input{this, "(read/readgroups/set/setall/setgroup/nset/nsetall/nsetgroup/color/colorall/colorgroup/shutter/shutterall/shuttergroup/track/trackall/trackgroup/trackrel/trackallrel/trackgrouprel/ptbytes/channel/bang/bangall) fixture mapping control"};
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

    c74::min::timer<c74::min::timer_options::defer_delivery> groups_load_timer{this,
        MIN_FUNCTION {
            if(groups_load_pending_ && !groups_path_value_.empty()) {
                groups_load_pending_ = false;
                load_groups_file(groups_path_value_);
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

    c74::min::attribute<c74::min::symbol> groups{this, "groups", "",
        c74::min::description{"Optional bbb.dmx groups JSON path. Groups are validated against the loaded patch and used by *group messages."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(args.empty()) {
                groups_path_value_.clear();
                groups_load_pending_ = false;
                groups_ = bbb::dmx::fixture_group_set{};
                groups_loaded_ = false;
                groups_validated_ = false;
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

    c74::min::attribute<c74::min::symbol> tracking_mode{this, "tracking_mode", "smart",
        c74::min::description{"Pan continuity mode for track/trackall messages: smart, pan, or off."},
        c74::min::enum_map{"smart", "pan", "off"},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(args.empty()) {
                return {c74::min::symbol(bbb::dmx::tracking_mode_to_string(tracking_mode_value_))};
            }
            bbb::dmx::tracking_mode mode{tracking_mode_value_};
            if(!bbb::dmx::tracking_mode_from_string(symbol_arg(args[0]), mode)) {
                warn_once(warn_invalid_tracking_mode_, "invalid tracking_mode ignored");
                return {c74::min::symbol(bbb::dmx::tracking_mode_to_string(tracking_mode_value_))};
            }
            tracking_mode_value_ = mode;
            return {c74::min::symbol(bbb::dmx::tracking_mode_to_string(tracking_mode_value_))};
        }}
    };

    c74::min::attribute<double> default_pan_range{this, "default_pan_range", 540.0,
        c74::min::description{"Fallback pan range in degrees when the fixture profile omits pan.range_degrees."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(args.empty() || !finite_atom(args[0]) || (double)args[0] <= 0.0) {
                warn_once(warn_invalid_range_, "invalid default_pan_range ignored");
                return {default_pan_range_value_};
            }
            default_pan_range_value_ = (double)args[0];
            return {default_pan_range_value_};
        }}
    };

    c74::min::attribute<double> default_tilt_range{this, "default_tilt_range", 270.0,
        c74::min::description{"Fallback tilt range in degrees when the fixture profile omits tilt.range_degrees."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(args.empty() || !finite_atom(args[0]) || (double)args[0] <= 0.0) {
                warn_once(warn_invalid_range_, "invalid default_tilt_range ignored");
                return {default_tilt_range_value_};
            }
            default_tilt_range_value_ = (double)args[0];
            return {default_tilt_range_value_};
        }}
    };

    c74::min::attribute<bool> track_strict{this, "track_strict", false,
        c74::min::description{"When non-zero, trackall reports fixtures without pan/tilt as errors instead of skipping them."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            track_strict_value_ = !args.empty() && ((int)args[0] != 0);
            return {track_strict_value_};
        }}
    };

    c74::min::attribute<bool> color_use_white{this, "color_use_white", true,
        c74::min::description{"When non-zero, semantic color extracts RGBW white from min(r,g,b). When zero, semantic color leaves white untouched and RGB channels carry the full color."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            color_use_white_value_ = args.empty() || ((int)args[0] != 0);
            return {color_use_white_value_};
        }}
    };

    c74::min::attribute<bool> color_wheel_fallback{this, "color_wheel_fallback", false,
        c74::min::description{"When non-zero, fixtures without RGB/RGBW/CMY use color wheel hue plus dimmer brightness when available."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            color_wheel_fallback_value_ = !args.empty() && ((int)args[0] != 0);
            return {color_wheel_fallback_value_};
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

    c74::min::message<> readgroups_message{this, "readgroups", "readgroups groups_json_path",
        MIN_FUNCTION {
            if(args.empty()) {
                report_error("readgroups requires a groups JSON path");
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

    c74::min::message<> reload_message{this, "reload", "Reload the current patch JSON file.",
        MIN_FUNCTION {
            if(patch_path_value_.empty()) {
                report_error("reload requires a previously loaded patch path");
                return {};
            }
            load_patch_file(patch_path_value_);
            if(!groups_path_value_.empty()) {
                load_groups_file(groups_path_value_);
            }
            return {};
        }
    };

    c74::min::message<> clear_message{this, "clear", "Clear all loaded profiles, patch data, and universe buffers.",
        MIN_FUNCTION {
            mapper_.clear();
            groups_ = bbb::dmx::fixture_group_set{};
            groups_loaded_ = false;
            groups_validated_ = false;
            tracking_engines_.clear();
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
            const bbb::dmx::mapper_result result{set_parameter_args(fixture_id, args, 1, false)};
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

    c74::min::message<> setgroup_message{this, "setgroup", "setgroup group_id parameter value [parameter value ...]",
        MIN_FUNCTION {
            if(args.size() < 3) {
                report_error("setgroup requires group_id parameter value");
                return {};
            }
            const bbb::dmx::fixture_mapper previous_mapper{mapper_};
            const bbb::dmx::mapper_result result{set_group_parameter_args(symbol_arg(args[0]), args)};
            if(!handle_result(result)) {
                mapper_ = previous_mapper;
                return {};
            }
            output_if_autobang();
            return {};
        }
    };

    c74::min::message<> nset_message{this, "nset", "nset fixture_id parameter normalized_0_to_1 [parameter normalized_0_to_1 ...]",
        MIN_FUNCTION {
            if(args.size() < 3) {
                report_error("nset requires fixture_id parameter numeric_value");
                return {};
            }
            const bbb::dmx::fixture_mapper previous_mapper{mapper_};
            const bbb::dmx::mapper_result result{set_normalized_parameter_args(symbol_arg(args[0]), args, 1, false)};
            if(!handle_result(result)) {
                mapper_ = previous_mapper;
                return {};
            }
            output_if_autobang();
            return {};
        }
    };

    c74::min::message<> nsetall_message{this, "nsetall", "nsetall parameter normalized_0_to_1 [parameter normalized_0_to_1 ...]",
        MIN_FUNCTION {
            if(args.size() < 2) {
                report_error("nsetall requires parameter numeric_value");
                return {};
            }
            const bbb::dmx::fixture_mapper previous_mapper{mapper_};
            const bbb::dmx::mapper_result result{set_all_normalized_parameter_args(args)};
            if(!handle_result(result)) {
                mapper_ = previous_mapper;
                return {};
            }
            output_if_autobang();
            return {};
        }
    };

    c74::min::message<> nsetgroup_message{this, "nsetgroup", "nsetgroup group_id parameter normalized_0_to_1 [parameter normalized_0_to_1 ...]",
        MIN_FUNCTION {
            if(args.size() < 3) {
                report_error("nsetgroup requires group_id parameter numeric_value");
                return {};
            }
            const bbb::dmx::fixture_mapper previous_mapper{mapper_};
            const bbb::dmx::mapper_result result{set_group_normalized_parameter_args(symbol_arg(args[0]), args)};
            if(!handle_result(result)) {
                mapper_ = previous_mapper;
                return {};
            }
            output_if_autobang();
            return {};
        }
    };

    c74::min::message<> color_message{this, "color", "color fixture_id rgb r g b OR color fixture_id rgb8 r g b",
        MIN_FUNCTION {
            if(args.size() < 4) {
                report_error("color requires fixture_id rgb r g b");
                return {};
            }
            bbb::dmx::semantic_color_request color{};
            bbb::dmx::mapper_result result{parse_color_args(args, 1, "color", color)};
            if(!result.ok) {
                handle_result(result);
                return {};
            }
            const bbb::dmx::fixture_mapper previous_mapper{mapper_};
            result = apply_semantic_color(symbol_arg(args[0]), color, false);
            if(!handle_result(result)) {
                mapper_ = previous_mapper;
                return {};
            }
            output_if_autobang();
            return {};
        }
    };

    c74::min::message<> colorall_message{this, "colorall", "colorall rgb r g b OR colorall rgb8 r g b",
        MIN_FUNCTION {
            if(args.size() < 3) {
                report_error("colorall requires rgb r g b");
                return {};
            }
            bbb::dmx::semantic_color_request color{};
            bbb::dmx::mapper_result result{parse_color_args(args, 0, "colorall", color)};
            if(!result.ok) {
                handle_result(result);
                return {};
            }
            const bbb::dmx::fixture_mapper previous_mapper{mapper_};
            result = apply_semantic_color_all(color);
            if(!handle_result(result)) {
                mapper_ = previous_mapper;
                return {};
            }
            output_if_autobang();
            return {};
        }
    };

    c74::min::message<> colorgroup_message{this, "colorgroup", "colorgroup group_id rgb r g b OR colorgroup group_id rgb8 r g b",
        MIN_FUNCTION {
            if(args.size() < 4) {
                report_error("colorgroup requires group_id rgb r g b");
                return {};
            }
            bbb::dmx::semantic_color_request color{};
            bbb::dmx::mapper_result result{parse_color_args(args, 1, "colorgroup", color)};
            if(!result.ok) {
                handle_result(result);
                return {};
            }
            const bbb::dmx::fixture_mapper previous_mapper{mapper_};
            result = apply_semantic_color_group(symbol_arg(args[0]), color);
            if(!handle_result(result)) {
                mapper_ = previous_mapper;
                return {};
            }
            output_if_autobang();
            return {};
        }
    };

    c74::min::message<> shutter_message{this, "shutter", "shutter fixture_id 1|0",
        MIN_FUNCTION {
            if(args.size() < 2) {
                report_error("shutter requires fixture_id state");
                return {};
            }
            bool open{false};
            bbb::dmx::mapper_result result{parse_shutter_state(args[1], open)};
            if(!result.ok) {
                handle_result(result);
                return {};
            }
            const bbb::dmx::fixture_mapper previous_mapper{mapper_};
            result = apply_semantic_shutter(symbol_arg(args[0]), open, false);
            if(!handle_result(result)) {
                mapper_ = previous_mapper;
                return {};
            }
            output_if_autobang();
            return {};
        }
    };

    c74::min::message<> shutterall_message{this, "shutterall", "shutterall 1|0",
        MIN_FUNCTION {
            if(args.empty()) {
                report_error("shutterall requires state");
                return {};
            }
            bool open{false};
            bbb::dmx::mapper_result result{parse_shutter_state(args[0], open)};
            if(!result.ok) {
                handle_result(result);
                return {};
            }
            const bbb::dmx::fixture_mapper previous_mapper{mapper_};
            result = apply_semantic_shutter_all(open);
            if(!handle_result(result)) {
                mapper_ = previous_mapper;
                return {};
            }
            output_if_autobang();
            return {};
        }
    };

    c74::min::message<> shuttergroup_message{this, "shuttergroup", "shuttergroup group_id 1|0",
        MIN_FUNCTION {
            if(args.size() < 2) {
                report_error("shuttergroup requires group_id state");
                return {};
            }
            bool open{false};
            bbb::dmx::mapper_result result{parse_shutter_state(args[1], open)};
            if(!result.ok) {
                handle_result(result);
                return {};
            }
            const bbb::dmx::fixture_mapper previous_mapper{mapper_};
            result = apply_semantic_shutter_group(symbol_arg(args[0]), open);
            if(!handle_result(result)) {
                mapper_ = previous_mapper;
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

    c74::min::message<> track_message{this, "track", "track fixture_id target_x target_y target_z",
        MIN_FUNCTION {
            if(args.size() < 4 || !finite_atoms(args, 1, 3)) {
                report_error("track requires fixture_id target_x target_y target_z");
                return {};
            }
            const bbb::dmx::fixture_mapper previous_mapper{mapper_};
            const auto previous_tracking_engines = tracking_engines_;
            const bbb::dmx::mapper_result result{track_fixture(
                symbol_arg(args[0]),
                bbb::dmx::vec3{(double)args[1], (double)args[2], (double)args[3]},
                false,
                false
            )};
            if(!handle_result(result)) {
                mapper_ = previous_mapper;
                tracking_engines_ = previous_tracking_engines;
                return {};
            }
            output_if_autobang();
            return {};
        }
    };

    c74::min::message<> trackrel_message{this, "trackrel", "trackrel fixture_id rel_x rel_y rel_z",
        MIN_FUNCTION {
            if(args.size() < 4 || !finite_atoms(args, 1, 3)) {
                report_error("trackrel requires fixture_id rel_x rel_y rel_z");
                return {};
            }
            const bbb::dmx::fixture_mapper previous_mapper{mapper_};
            const auto previous_tracking_engines = tracking_engines_;
            const bbb::dmx::mapper_result result{track_fixture(
                symbol_arg(args[0]),
                bbb::dmx::vec3{(double)args[1], (double)args[2], (double)args[3]},
                true,
                false
            )};
            if(!handle_result(result)) {
                mapper_ = previous_mapper;
                tracking_engines_ = previous_tracking_engines;
                return {};
            }
            output_if_autobang();
            return {};
        }
    };

    c74::min::message<> trackall_message{this, "trackall", "trackall target_x target_y target_z",
        MIN_FUNCTION {
            if(args.size() < 3 || !finite_atoms(args, 0, 3)) {
                report_error("trackall requires target_x target_y target_z");
                return {};
            }
            const bbb::dmx::fixture_mapper previous_mapper{mapper_};
            const auto previous_tracking_engines = tracking_engines_;
            const bbb::dmx::mapper_result result{track_all(
                bbb::dmx::vec3{(double)args[0], (double)args[1], (double)args[2]},
                false
            )};
            if(!handle_result(result)) {
                mapper_ = previous_mapper;
                tracking_engines_ = previous_tracking_engines;
                return {};
            }
            output_if_autobang();
            return {};
        }
    };

    c74::min::message<> trackgroup_message{this, "trackgroup", "trackgroup group_id target_x target_y target_z",
        MIN_FUNCTION {
            if(args.size() < 4 || !finite_atoms(args, 1, 3)) {
                report_error("trackgroup requires group_id target_x target_y target_z");
                return {};
            }
            const bbb::dmx::fixture_mapper previous_mapper{mapper_};
            const auto previous_tracking_engines = tracking_engines_;
            const bbb::dmx::mapper_result result{track_group(
                symbol_arg(args[0]),
                bbb::dmx::vec3{(double)args[1], (double)args[2], (double)args[3]},
                false
            )};
            if(!handle_result(result)) {
                mapper_ = previous_mapper;
                tracking_engines_ = previous_tracking_engines;
                return {};
            }
            output_if_autobang();
            return {};
        }
    };

    c74::min::message<> trackallrel_message{this, "trackallrel", "trackallrel rel_x rel_y rel_z",
        MIN_FUNCTION {
            if(args.size() < 3 || !finite_atoms(args, 0, 3)) {
                report_error("trackallrel requires rel_x rel_y rel_z");
                return {};
            }
            const bbb::dmx::fixture_mapper previous_mapper{mapper_};
            const auto previous_tracking_engines = tracking_engines_;
            const bbb::dmx::mapper_result result{track_all(
                bbb::dmx::vec3{(double)args[0], (double)args[1], (double)args[2]},
                true
            )};
            if(!handle_result(result)) {
                mapper_ = previous_mapper;
                tracking_engines_ = previous_tracking_engines;
                return {};
            }
            output_if_autobang();
            return {};
        }
    };

    c74::min::message<> trackgrouprel_message{this, "trackgrouprel", "trackgrouprel group_id rel_x rel_y rel_z",
        MIN_FUNCTION {
            if(args.size() < 4 || !finite_atoms(args, 1, 3)) {
                report_error("trackgrouprel requires group_id rel_x rel_y rel_z");
                return {};
            }
            const bbb::dmx::fixture_mapper previous_mapper{mapper_};
            const auto previous_tracking_engines = tracking_engines_;
            const bbb::dmx::mapper_result result{track_group(
                symbol_arg(args[0]),
                bbb::dmx::vec3{(double)args[1], (double)args[2], (double)args[3]},
                true
            )};
            if(!handle_result(result)) {
                mapper_ = previous_mapper;
                tracking_engines_ = previous_tracking_engines;
                return {};
            }
            output_if_autobang();
            return {};
        }
    };

    c74::min::message<> trackreset_message{this, "trackreset", "trackreset [fixture_id]",
        MIN_FUNCTION {
            if(args.empty()) {
                tracking_engines_.clear();
                report_status("trackreset");
                return {};
            }
            tracking_engines_.erase(symbol_arg(args[0]));
            report_status("trackreset");
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
            status_atoms.push_back(c74::min::symbol("tracking_mode"));
            status_atoms.push_back(c74::min::symbol(bbb::dmx::tracking_mode_to_string(tracking_mode_value_)));
            status_atoms.push_back(c74::min::symbol("track_strict"));
            status_atoms.push_back(track_strict_value_ ? 1 : 0);
            status_atoms.push_back(c74::min::symbol("color_use_white"));
            status_atoms.push_back(color_use_white_value_ ? 1 : 0);
            status_atoms.push_back(c74::min::symbol("color_wheel_fallback"));
            status_atoms.push_back(color_wheel_fallback_value_ ? 1 : 0);
            status_atoms.push_back(c74::min::symbol("groups_loaded"));
            status_atoms.push_back(groups_loaded_ ? 1 : 0);
            status_atoms.push_back(c74::min::symbol("groups_validated"));
            status_atoms.push_back(groups_validated_ ? 1 : 0);
            status_atoms.push_back(c74::min::symbol("groups"));
            for(const auto &group : groups_.groups) {
                status_atoms.push_back(c74::min::symbol(group.id.c_str()));
            }
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

    void schedule_groups_load() {
        if(groups_path_value_.empty()) {
            groups_load_pending_ = false;
            return;
        }
        groups_load_pending_ = true;
        groups_load_timer.delay(0);
    }

    void load_patch_file(const std::string &path) {
        const std::string resolved_path{bbb::dmx::maxutil::resolve_file_path(path)};
        bbb::dmx::fixture_mapper loaded_mapper{};
        const bbb::dmx::mapper_result result{bbb::dmx::load_fixture_mapper_from_patch_file(resolved_path, loaded_mapper)};
        if(!handle_result(result)) {
            return;
        }
        mapper_ = loaded_mapper;
        tracking_engines_.clear();
        groups_validated_ = false;
        if(groups_loaded_) {
            const bbb::dmx::mapper_result groups_result{validate_loaded_groups()};
            if(!groups_result.ok) {
                handle_result(groups_result);
            }
        }
        report_status("loaded");
        output_if_autobang();
    }

    void load_groups_file(const std::string &path) {
        const std::string resolved_path{bbb::dmx::maxutil::resolve_file_path(path)};
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
        report_status("groups_loaded");
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
        if(result.ok) {
            groups_validated_ = true;
        } else {
            groups_validated_ = false;
        }
        return result;
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
        if(atom.a_type != c74::max::A_LONG && atom.a_type != c74::max::A_FLOAT) {
            return false;
        }
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

    bbb::dmx::mapper_result parse_color_args(const c74::min::atoms &args, std::size_t start_index, const char *message_name, bbb::dmx::semantic_color_request &color) const {
        if(args.size() <= start_index) {
            return bbb::dmx::mapper_result::failure(std::string(message_name) + " requires rgb r g b");
        }

        bool rgb8{false};
        std::size_t value_index{start_index};
        if(!finite_atom(args[start_index])) {
            const std::string color_space{symbol_arg(args[start_index])};
            value_index = start_index + 1;
            if(color_space == "rgb") {
                rgb8 = false;
            } else if(color_space == "rgb8") {
                rgb8 = true;
            } else {
                return bbb::dmx::mapper_result::failure(std::string(message_name) + " color space must be rgb or rgb8");
            }
        }

        if(args.size() <= value_index + 2 || !finite_atoms(args, value_index, 3)) {
            return bbb::dmx::mapper_result::failure(std::string(message_name) + " requires three numeric color values");
        }

        const double scale{rgb8 ? 255.0 : 1.0};
        color = bbb::dmx::make_semantic_color_request(
            (double)args[value_index] / scale,
            (double)args[value_index + 1] / scale,
            (double)args[value_index + 2] / scale
        );
        return bbb::dmx::mapper_result::success();
    }

    bbb::dmx::mapper_result apply_normalized_color_parameters(
        bbb::dmx::fixture_mapper &trial_mapper,
        const std::string &fixture_id,
        const std::vector<std::pair<std::string, double>> &parameters
    ) const
    {
        for(const auto &parameter : parameters) {
            const bbb::dmx::mapper_result result{trial_mapper.set_normalized(fixture_id, parameter.first, parameter.second)};
            if(!result.ok) {
                return result;
            }
        }
        return bbb::dmx::mapper_result::success();
    }

    std::vector<std::pair<std::string, int>> current_color_wheel_parameter_values(const std::string &fixture_id, const bbb::dmx::fixture_mode &mode) const {
        std::vector<std::pair<std::string, int>> values;
        for(const auto &parameter : mode.parameters) {
            if(!bbb::dmx::parameter_is_likely_color_wheel(parameter)) {
                continue;
            }
            int value{0};
            const bbb::dmx::mapper_result result{mapper_.current_raw_value(fixture_id, parameter.key, value)};
            if(result.ok) {
                values.push_back({parameter.key, value});
            }
        }
        return values;
    }

    bbb::dmx::mapper_result apply_semantic_color(const std::string &fixture_id, const bbb::dmx::semantic_color_request &color, bool ignore_non_color_fixtures) {
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

        std::vector<std::pair<std::string, int>> current_color_wheel_values;
        if(color_wheel_fallback_value_) {
            current_color_wheel_values = current_color_wheel_parameter_values(fixture_id, *mode);
        }
        const bbb::dmx::semantic_color_mapping mapping{bbb::dmx::semantic_color_parameters_for_mode(
            profile,
            *mode,
            color,
            bbb::dmx::semantic_color_options{color_use_white_value_, color_wheel_fallback_value_},
            current_color_wheel_values
        )};
        if(!mapping.ok) {
            if(ignore_non_color_fixtures) {
                return bbb::dmx::mapper_result::success();
            }
            return bbb::dmx::mapper_result::failure("fixture has no semantic color model: " + fixture_id);
        }

        bbb::dmx::fixture_mapper trial_mapper{mapper_};
        const bbb::dmx::mapper_result result{apply_normalized_color_parameters(trial_mapper, fixture_id, mapping.parameters)};
        if(!result.ok) {
            return result;
        }
        mapper_ = trial_mapper;
        return bbb::dmx::mapper_result::success();
    }

    bbb::dmx::mapper_result apply_semantic_color_all(const bbb::dmx::semantic_color_request &color) {
        if(mapper_.patch().fixtures.empty()) {
            return bbb::dmx::mapper_result::failure("colorall requires a loaded patch with fixtures");
        }
        for(const auto &fixture : mapper_.patch().fixtures) {
            const bbb::dmx::mapper_result result{apply_semantic_color(fixture.id, color, true)};
            if(!result.ok) {
                return bbb::dmx::mapper_result::failure("colorall fixture " + fixture.id + ": " + result.message);
            }
        }
        return bbb::dmx::mapper_result::success();
    }

    bbb::dmx::mapper_result resolve_group_fixture_ids(const std::string &group_id, std::vector<std::string> &fixture_ids) {
        if(mapper_.patch().fixtures.empty()) {
            return bbb::dmx::mapper_result::failure("group operation requires a loaded patch with fixtures");
        }
        if(!groups_loaded_) {
            return bbb::dmx::mapper_result::failure("group operation requires loaded groups");
        }
        if(!groups_validated_) {
            const bbb::dmx::mapper_result validate_result{validate_loaded_groups()};
            if(!validate_result.ok) {
                return validate_result;
            }
        }
        return bbb::dmx::resolve_fixture_group_fixture_ids(groups_, mapper_.patch(), group_id, fixture_ids);
    }

    bbb::dmx::mapper_result apply_semantic_color_group(const std::string &group_id, const bbb::dmx::semantic_color_request &color) {
        std::vector<std::string> fixture_ids{};
        bbb::dmx::mapper_result result{resolve_group_fixture_ids(group_id, fixture_ids)};
        if(!result.ok) {
            return result;
        }
        for(const auto &fixture_id : fixture_ids) {
            result = apply_semantic_color(fixture_id, color, true);
            if(!result.ok) {
                return bbb::dmx::mapper_result::failure("colorgroup fixture " + fixture_id + ": " + result.message);
            }
        }
        return bbb::dmx::mapper_result::success();
    }

    bbb::dmx::mapper_result parse_shutter_state(const c74::min::atom &atom, bool &open) const {
        if(finite_atom(atom)) {
            open = (int)atom != 0;
            return bbb::dmx::mapper_result::success();
        }

        const std::string text{bbb::dmx::normalized_semantic_key(symbol_arg(atom))};
        if(text == "open" || text == "on" || text == "true") {
            open = true;
            return bbb::dmx::mapper_result::success();
        }
        if(text == "close" || text == "closed" || text == "off" || text == "false" || text == "blackout") {
            open = false;
            return bbb::dmx::mapper_result::success();
        }
        return bbb::dmx::mapper_result::failure("shutter state must be 1/open or 0/closed");
    }

    bbb::dmx::mapper_result apply_semantic_shutter(const std::string &fixture_id, bool open, bool ignore_unsupported_fixtures) {
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

        const bbb::dmx::semantic_shutter_mapping mapping{bbb::dmx::semantic_shutter_parameter_for_mode(*mode, open)};
        if(!mapping.ok) {
            if(ignore_unsupported_fixtures) {
                return bbb::dmx::mapper_result::success();
            }
            return bbb::dmx::mapper_result::failure("fixture has no semantic shutter parameter: " + fixture_id);
        }

        bbb::dmx::fixture_mapper trial_mapper{mapper_};
        const bbb::dmx::mapper_result result{set_parameter_value_on_mapper(trial_mapper, fixture_id, mapping.parameter, mapping.value)};
        if(!result.ok) {
            return result;
        }
        mapper_ = trial_mapper;
        return bbb::dmx::mapper_result::success();
    }

    bbb::dmx::mapper_result apply_semantic_shutter_all(bool open) {
        if(mapper_.patch().fixtures.empty()) {
            return bbb::dmx::mapper_result::failure("shutterall requires a loaded patch with fixtures");
        }
        for(const auto &fixture : mapper_.patch().fixtures) {
            const bbb::dmx::mapper_result result{apply_semantic_shutter(fixture.id, open, true)};
            if(!result.ok) {
                return bbb::dmx::mapper_result::failure("shutterall fixture " + fixture.id + ": " + result.message);
            }
        }
        return bbb::dmx::mapper_result::success();
    }

    bbb::dmx::mapper_result apply_semantic_shutter_group(const std::string &group_id, bool open) {
        std::vector<std::string> fixture_ids{};
        bbb::dmx::mapper_result result{resolve_group_fixture_ids(group_id, fixture_ids)};
        if(!result.ok) {
            return result;
        }
        for(const auto &fixture_id : fixture_ids) {
            result = apply_semantic_shutter(fixture_id, open, true);
            if(!result.ok) {
                return bbb::dmx::mapper_result::failure("shuttergroup fixture " + fixture_id + ": " + result.message);
            }
        }
        return bbb::dmx::mapper_result::success();
    }

    static double normalized_from_u16(std::uint16_t value) {
        return (double)value / 65535.0;
    }

    const bbb::dmx::fixture_instance *find_fixture_instance(const std::string &fixture_id) const {
        for(const auto &fixture : mapper_.patch().fixtures) {
            if(fixture.id == fixture_id) {
                return &fixture;
            }
        }
        return nullptr;
    }

    bbb::dmx::mapper_result configure_tracking_engine(const bbb::dmx::fixture_instance &fixture, bbb::dmx::movertrack_engine &engine) const {
        const bbb::dmx::fixture_profile *profile{mapper_.find_profile(fixture.profile)};
        if(!profile) {
            return bbb::dmx::mapper_result::failure("missing profile: " + fixture.profile);
        }
        const bbb::dmx::fixture_mode *mode{profile->find_mode(fixture.mode)};
        if(!mode) {
            return bbb::dmx::mapper_result::failure("missing mode: " + fixture.mode);
        }
        const bbb::dmx::fixture_parameter *pan_parameter{mode->find_parameter("pan")};
        const bbb::dmx::fixture_parameter *tilt_parameter{mode->find_parameter("tilt")};
        if(!pan_parameter || !tilt_parameter) {
            return bbb::dmx::mapper_result::failure("fixture is not a mover: " + fixture.id);
        }

        const double pan_range{0.0 < pan_parameter->range_degrees ? pan_parameter->range_degrees : default_pan_range_value_};
        const double tilt_range{0.0 < tilt_parameter->range_degrees ? tilt_parameter->range_degrees : default_tilt_range_value_};
        if(!bbb::dmx::is_finite(pan_range) || !bbb::dmx::is_finite(tilt_range) || pan_range <= 0.0 || tilt_range <= 0.0) {
            return bbb::dmx::mapper_result::failure("invalid pan/tilt range for fixture: " + fixture.id);
        }

        if(!engine.set_fixture_position(fixture.position)) {
            return bbb::dmx::mapper_result::failure("invalid fixture position: " + fixture.id);
        }
        if(!engine.set_rotation_degrees(fixture.rotation)) {
            return bbb::dmx::mapper_result::failure("invalid fixture rotation: " + fixture.id);
        }
        if(!engine.set_ranges(pan_range, tilt_range)) {
            return bbb::dmx::mapper_result::failure("invalid pan/tilt range for fixture: " + fixture.id);
        }
        if(!engine.set_pan_offset(fixture.calibration.pan_offset)) {
            return bbb::dmx::mapper_result::failure("invalid pan offset for fixture: " + fixture.id);
        }
        if(!engine.set_tilt_offset(fixture.calibration.tilt_offset)) {
            return bbb::dmx::mapper_result::failure("invalid tilt offset for fixture: " + fixture.id);
        }
        engine.set_pan_invert(fixture.calibration.pan_invert);
        engine.set_tilt_invert(fixture.calibration.tilt_invert);
        engine.set_tracking_mode(tracking_mode_value_);
        return bbb::dmx::mapper_result::success();
    }

    bbb::dmx::movertrack_engine tracking_engine_copy_for(const std::string &fixture_id) const {
        const auto found = tracking_engines_.find(fixture_id);
        if(found == tracking_engines_.end()) {
            return bbb::dmx::movertrack_engine{};
        }
        return found->second;
    }

    bbb::dmx::mapper_result apply_tracking_output(const std::string &fixture_id, const bbb::dmx::movertrack_output &output, bool ignore_unknown_parameters) {
        bbb::dmx::fixture_mapper trial_mapper{mapper_};
        bbb::dmx::mapper_result result{trial_mapper.set_normalized(fixture_id, "pan", normalized_from_u16(output.pan))};
        if(result.ok) {
            result = trial_mapper.set_normalized(fixture_id, "tilt", normalized_from_u16(output.tilt));
        }
        if(!result.ok) {
            if(ignore_unknown_parameters && is_unknown_parameter_result(result)) {
                return bbb::dmx::mapper_result::success();
            }
            return result;
        }
        mapper_ = trial_mapper;
        return bbb::dmx::mapper_result::success();
    }

    bbb::dmx::mapper_result track_fixture(const std::string &fixture_id, const bbb::dmx::vec3 &target, bool relative, bool ignore_non_movers) {
        const bbb::dmx::fixture_instance *fixture{find_fixture_instance(fixture_id)};
        if(!fixture) {
            return bbb::dmx::mapper_result::failure("unknown fixture: " + fixture_id);
        }

        bbb::dmx::movertrack_engine trial_engine{tracking_engine_copy_for(fixture_id)};
        bbb::dmx::mapper_result result{configure_tracking_engine(*fixture, trial_engine)};
        if(!result.ok) {
            if(ignore_non_movers && is_non_mover_result(result)) {
                return bbb::dmx::mapper_result::success();
            }
            return result;
        }

        const bbb::dmx::movertrack_output output{relative ? trial_engine.compute_relative(target) : trial_engine.compute(target)};
        result = apply_tracking_output(fixture_id, output, ignore_non_movers);
        if(!result.ok) {
            return result;
        }
        tracking_engines_[fixture_id] = trial_engine;
        return bbb::dmx::mapper_result::success();
    }

    bbb::dmx::mapper_result track_all(const bbb::dmx::vec3 &target, bool relative) {
        if(mapper_.patch().fixtures.empty()) {
            return bbb::dmx::mapper_result::failure("trackall requires a loaded patch with fixtures");
        }
        const bool ignore_non_movers{!track_strict_value_};
        for(const auto &fixture : mapper_.patch().fixtures) {
            const bbb::dmx::mapper_result result{track_fixture(fixture.id, target, relative, ignore_non_movers)};
            if(!result.ok) {
                return bbb::dmx::mapper_result::failure("trackall fixture " + fixture.id + ": " + result.message);
            }
        }
        return bbb::dmx::mapper_result::success();
    }

    bbb::dmx::mapper_result track_group(const std::string &group_id, const bbb::dmx::vec3 &target, bool relative) {
        std::vector<std::string> fixture_ids{};
        bbb::dmx::mapper_result result{resolve_group_fixture_ids(group_id, fixture_ids)};
        if(!result.ok) {
            return result;
        }
        const bool ignore_non_movers{!track_strict_value_};
        for(const auto &fixture_id : fixture_ids) {
            result = track_fixture(fixture_id, target, relative, ignore_non_movers);
            if(!result.ok) {
                return bbb::dmx::mapper_result::failure("trackgroup fixture " + fixture_id + ": " + result.message);
            }
        }
        return bbb::dmx::mapper_result::success();
    }

    bbb::dmx::mapper_result set_all_parameter_args(const c74::min::atoms &args) {
        if(mapper_.patch().fixtures.empty()) {
            return bbb::dmx::mapper_result::failure("setall requires a loaded patch with fixtures");
        }
        for(const auto &fixture : mapper_.patch().fixtures) {
            const bbb::dmx::mapper_result result{set_parameter_args(fixture.id, args, 0, true)};
            if(!result.ok) {
                return bbb::dmx::mapper_result::failure("setall fixture " + fixture.id + ": " + result.message);
            }
        }
        return bbb::dmx::mapper_result::success();
    }

    bbb::dmx::mapper_result set_group_parameter_args(const std::string &group_id, const c74::min::atoms &args) {
        std::vector<std::string> fixture_ids{};
        bbb::dmx::mapper_result result{resolve_group_fixture_ids(group_id, fixture_ids)};
        if(!result.ok) {
            return result;
        }
        for(const auto &fixture_id : fixture_ids) {
            result = set_parameter_args(fixture_id, args, 1, true);
            if(!result.ok) {
                return bbb::dmx::mapper_result::failure("setgroup fixture " + fixture_id + ": " + result.message);
            }
        }
        return bbb::dmx::mapper_result::success();
    }

    bbb::dmx::mapper_result set_all_normalized_parameter_args(const c74::min::atoms &args) {
        if(mapper_.patch().fixtures.empty()) {
            return bbb::dmx::mapper_result::failure("nsetall requires a loaded patch with fixtures");
        }
        for(const auto &fixture : mapper_.patch().fixtures) {
            const bbb::dmx::mapper_result result{set_normalized_parameter_args(fixture.id, args, 0, true)};
            if(!result.ok) {
                return bbb::dmx::mapper_result::failure("nsetall fixture " + fixture.id + ": " + result.message);
            }
        }
        return bbb::dmx::mapper_result::success();
    }

    bbb::dmx::mapper_result set_group_normalized_parameter_args(const std::string &group_id, const c74::min::atoms &args) {
        std::vector<std::string> fixture_ids{};
        bbb::dmx::mapper_result result{resolve_group_fixture_ids(group_id, fixture_ids)};
        if(!result.ok) {
            return result;
        }
        for(const auto &fixture_id : fixture_ids) {
            result = set_normalized_parameter_args(fixture_id, args, 1, true);
            if(!result.ok) {
                return bbb::dmx::mapper_result::failure("nsetgroup fixture " + fixture_id + ": " + result.message);
            }
        }
        return bbb::dmx::mapper_result::success();
    }

    bbb::dmx::mapper_result set_parameter_args(const std::string &fixture_id, const c74::min::atoms &args, std::size_t start_index, bool ignore_unknown_parameters) {
        std::size_t index{start_index};
        while(index < args.size()) {
            const std::string parameter{symbol_arg(args[index])};
            if(parameter == "pan_tilt") {
                if(args.size() <= index + 2 || !finite_atom(args[index + 1]) || !finite_atom(args[index + 2])) {
                    return bbb::dmx::mapper_result::failure("set pan_tilt requires two numeric u16 values");
                }
                const bbb::dmx::mapper_result result{set_pan_tilt_values(
                    fixture_id,
                    clamp_int((int)args[index + 1], 0, 65535),
                    clamp_int((int)args[index + 2], 0, 65535),
                    ignore_unknown_parameters
                )};
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
                if(ignore_unknown_parameters && is_unknown_parameter_result(result)) {
                    index += 2;
                    continue;
                }
                return result;
            }
            index += 2;
        }
        return bbb::dmx::mapper_result::success();
    }

    bbb::dmx::mapper_result set_normalized_parameter_args(const std::string &fixture_id, const c74::min::atoms &args, std::size_t start_index, bool ignore_unknown_parameters) {
        std::size_t index{start_index};
        while(index < args.size()) {
            const std::string parameter{symbol_arg(args[index])};
            if(args.size() <= index + 1) {
                return bbb::dmx::mapper_result::failure("nset requires parameter/value pairs");
            }
            if(!finite_atom(args[index + 1])) {
                return bbb::dmx::mapper_result::failure("nset value must be numeric: " + parameter);
            }
            const bbb::dmx::mapper_result result{mapper_.set_normalized(fixture_id, parameter, (double)args[index + 1])};
            if(!result.ok) {
                if(ignore_unknown_parameters && is_unknown_parameter_result(result)) {
                    index += 2;
                    continue;
                }
                return result;
            }
            index += 2;
        }
        return bbb::dmx::mapper_result::success();
    }

    bbb::dmx::mapper_result set_pan_tilt_values(const std::string &fixture_id, int pan_value, int tilt_value, bool ignore_unknown_parameters) {
        bbb::dmx::fixture_mapper trial_mapper{mapper_};
        bbb::dmx::mapper_result result{trial_mapper.set_u16(fixture_id, "pan", (std::uint16_t)pan_value)};
        if(result.ok) {
            result = trial_mapper.set_u16(fixture_id, "tilt", (std::uint16_t)tilt_value);
        }
        if(!result.ok) {
            if(ignore_unknown_parameters && is_unknown_parameter_result(result)) {
                return bbb::dmx::mapper_result::success();
            }
            return result;
        }
        mapper_ = trial_mapper;
        return bbb::dmx::mapper_result::success();
    }

    bbb::dmx::mapper_result set_parameter_value(const std::string &fixture_id, const std::string &parameter, int value) {
        return set_parameter_value_on_mapper(mapper_, fixture_id, parameter, value);
    }

    bbb::dmx::mapper_result set_parameter_value_on_mapper(bbb::dmx::fixture_mapper &target_mapper, const std::string &fixture_id, const std::string &parameter, int value) const {
        bbb::dmx::mapper_result result{target_mapper.set_u24(fixture_id, parameter, (std::uint32_t)clamp_int(value, 0, 16777215))};
        if(!result.ok) {
            result = target_mapper.set_u16(fixture_id, parameter, (std::uint16_t)clamp_int(value, 0, 65535));
        }
        if(!result.ok) {
            result = target_mapper.set_u8(fixture_id, parameter, value);
        }
        return result;
    }

    static bool is_unknown_parameter_result(const bbb::dmx::mapper_result &result) {
        const std::string prefix{"unknown parameter: "};
        return !result.ok && result.message.rfind(prefix, 0) == 0;
    }

    static bool is_non_mover_result(const bbb::dmx::mapper_result &result) {
        const std::string prefix{"fixture is not a mover: "};
        return !result.ok && result.message.rfind(prefix, 0) == 0;
    }

    void warn_once(bool &flag, const char *message) {
        if(!flag) {
            cerr << "bbb.dmx.fixturemap: " << message << c74::min::endl;
            flag = true;
        }
    }
};

MIN_EXTERNAL(bbb_dmx_fixturemap);
