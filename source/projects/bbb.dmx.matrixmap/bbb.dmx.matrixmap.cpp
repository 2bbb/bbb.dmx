#include "c74_min.h"

#include <bbb/dmx/build_info.hpp>
#include "c74_jitter.h"

#include <bbb/dmx/color_mapping.hpp>
#include <bbb/dmx/fixture_groups.hpp>
#include <bbb/dmx/fixture_json.hpp>
#include <bbb/dmx/frame_set.hpp>
#include <bbb/dmx/max_external_utils.hpp>
#include <bbb/dmx/matrix_map.hpp>
#include <bbb/dmx/semantic_overrides.hpp>

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>

using bbb::dmx::matrixmap::clamp_normalized;
using bbb::dmx::matrixmap::color_source_value;
using bbb::dmx::matrixmap::color_value;
using bbb::dmx::matrixmap::fixture_mapping;
using bbb::dmx::matrixmap::matrix_map_config;
using bbb::dmx::matrixmap::matrix_read_view;
using bbb::dmx::matrixmap::matrix_value_kind;
using bbb::dmx::matrixmap::parameter_binding;
using bbb::dmx::matrixmap::parse_matrix_map_text;
using bbb::dmx::matrixmap::parse_plane_order;
using bbb::dmx::matrixmap::parse_universe_output_mode;
using bbb::dmx::matrixmap::plane_order_kind;
using bbb::dmx::matrixmap::plane_order_to_string;
using bbb::dmx::matrixmap::sample_region;
using bbb::dmx::matrixmap::sample_region_color;
using bbb::dmx::matrixmap::universe_output_mode;
using bbb::dmx::matrixmap::universe_output_mode_to_string;

class bbb_dmx_matrixmap : public c74::min::object<bbb_dmx_matrixmap> {
private:
    bbb::dmx::fixture_mapper mapper_{};
    matrix_map_config matrix_map_{};
    bbb::dmx::fixture_group_set groups_{};
    bbb::dmx::fixture_semantic_overrides semantic_overrides_{};
    std::string patch_path_value_{};
    std::string map_path_value_{};
    std::string groups_path_value_{};
    std::string semantic_overrides_path_value_{};
    int universe_value_{1};
    plane_order_kind plane_order_value_{plane_order_kind::rgba};
    universe_output_mode output_mode_value_{universe_output_mode::all};
    double gamma_value_{1.0};
    double brightness_value_{1.0};
    bool autobang_value_{true};
    bool invert_x_value_{false};
    bool invert_y_value_{false};
    bool color_use_white_value_{true};
    bool color_wheel_fallback_value_{false};
    bool patch_loaded_{false};
    bool map_loaded_{false};
    bool groups_loaded_{false};
    bool groups_validated_{false};
    bool semantic_overrides_loaded_{false};
    bool semantic_overrides_validated_{false};
    bool patch_load_pending_{false};
    bool map_load_pending_{false};
    bool groups_load_pending_{false};
    bool semantic_overrides_load_pending_{false};
    bool suppress_patch_attribute_load_{false};
    bool suppress_map_attribute_load_{false};
    bool suppress_groups_attribute_load_{false};
    bool suppress_group_attribute_load_{false};
    bool suppress_semantic_overrides_attribute_load_{false};
    std::map<int, bbb::dmx::dmx_universe> previous_output_{};
    std::map<int, std::set<int>> owned_output_channels_{};
    std::map<int, std::set<int>> pending_output_channels_{};
    bool collecting_output_channels_{false};

public:
    MIN_DESCRIPTION{"Sample jit.matrix color data and patch it into multi-universe DMX fixture parameters."};
    MIN_TAGS{"dmx, jitter, matrix, color, fixture, universe"};
    MIN_AUTHOR{"2bit"};
    MIN_RELATED{"bbb.dmx.fixturemap, bbb.dmx.merge, bbb.dmx.safety"};

    c74::min::inlet<> input{this, "(jit_matrix/readpatch/readmap/readgroups/readoverrides/bang) matrix-to-DMX mapper input"};
    c74::min::outlet<> output{this, "(anything) universe id and 512 DMX bytes"};
    c74::min::outlet<> status_output{this, "(anything) status and error messages"};

    c74::min::timer<c74::min::timer_options::defer_delivery> patch_load_timer{this,
        MIN_FUNCTION {
            if(patch_load_pending_ && !patch_path_value_.empty()) {
                patch_load_pending_ = false;
                load_patch(patch_path_value_);
            }
            return {};
        }
    };

    c74::min::timer<c74::min::timer_options::defer_delivery> map_load_timer{this,
        MIN_FUNCTION {
            if(map_load_pending_ && !map_path_value_.empty()) {
                map_load_pending_ = false;
                load_map(map_path_value_);
            }
            return {};
        }
    };

    c74::min::timer<c74::min::timer_options::defer_delivery> groups_load_timer{this,
        MIN_FUNCTION {
            if(groups_load_pending_ && !groups_path_value_.empty()) {
                groups_load_pending_ = false;
                load_groups(groups_path_value_);
            }
            return {};
        }
    };

    c74::min::timer<c74::min::timer_options::defer_delivery> semantic_overrides_load_timer{this,
        MIN_FUNCTION {
            if(semantic_overrides_load_pending_ && !semantic_overrides_path_value_.empty()) {
                semantic_overrides_load_pending_ = false;
                load_semantic_overrides(semantic_overrides_path_value_);
            }
            return {};
        }
    };

    c74::min::attribute<c74::min::symbol> patch{this, "patch", "",
        c74::min::description{"Fixture patch JSON path."},
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

    c74::min::attribute<c74::min::symbol> map{this, "map", "",
        c74::min::description{"Matrix map JSON path."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(args.empty()) {
                map_path_value_.clear();
                map_load_pending_ = false;
                return {c74::min::symbol("")};
            }
            const c74::min::symbol symbol_value{(c74::min::symbol)args[0]};
            map_path_value_ = symbol_value.c_str();
            if(!suppress_map_attribute_load_) {
                schedule_map_load();
            }
            return {symbol_value};
        }}
    };

    c74::min::attribute<c74::min::symbol> groups{this, "groups", "",
        c74::min::description{"Optional bbb.dmx groups JSON path. Explicit matrix map entries may target group names with \"group\"."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
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
        c74::min::description{"Optional bbb.dmx semantic overrides JSON path. Enables aliases and semantic RGB/CMY/color-wheel matrix mapping."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(args.empty()) {
                semantic_overrides_path_value_.clear();
                semantic_overrides_load_pending_ = false;
                semantic_overrides_ = bbb::dmx::fixture_semantic_overrides{};
                semantic_overrides_loaded_ = false;
                semantic_overrides_validated_ = false;
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
        c74::min::description{"Selected universe for universe_mode selected."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(args.empty() || !bbb::dmx::maxutil::finite_atom(args[0])) {
                return {universe_value_};
            }
            universe_value_ = bbb::dmx::sanitize_universe_id((int)args[0]);
            return {universe_value_};
        }}
    };

    c74::min::attribute<bool> color_use_white{this, "color_use_white", true,
        c74::min::description{"RGBW semantic matrix color behavior. When false, RGBW white parameters are left untouched."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            color_use_white_value_ = args.empty() || ((int)args[0] != 0);
            return {color_use_white_value_};
        }}
    };

    c74::min::attribute<bool> color_wheel_fallback{this, "color_wheel_fallback", false,
        c74::min::description{"Allow semantic matrix RGB mappings to drive nearest color-wheel slots when no RGB/CMY model exists."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            color_wheel_fallback_value_ = !args.empty() && ((int)args[0] != 0);
            return {color_wheel_fallback_value_};
        }}
    };

    c74::min::attribute<c74::min::symbol> plane_order{this, "plane_order", "rgba",
        c74::min::description{"Matrix plane order: rgba, argb, bgra, or gray."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(args.empty()) {
                return {c74::min::symbol(plane_order_to_string(plane_order_value_))};
            }
            const std::string text{bbb::dmx::maxutil::symbol_arg(args[0])};
            plane_order_kind parsed{};
            if(!parse_plane_order(text, parsed)) {
                report_error(("unknown plane_order: " + text).c_str());
                return {c74::min::symbol(plane_order_to_string(plane_order_value_))};
            }
            plane_order_value_ = parsed;
            return {c74::min::symbol(text.c_str())};
        }}
    };

    c74::min::attribute<c74::min::symbol> universe_mode{this, "universe_mode", "all",
        c74::min::description{"Output mode: all, selected, or changed."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(args.empty()) {
                return {c74::min::symbol(universe_output_mode_to_string(output_mode_value_))};
            }
            const std::string text{bbb::dmx::maxutil::symbol_arg(args[0])};
            universe_output_mode parsed{};
            if(!parse_universe_output_mode(text, parsed)) {
                report_error(("unknown universe_mode: " + text).c_str());
                return {c74::min::symbol(universe_output_mode_to_string(output_mode_value_))};
            }
            output_mode_value_ = parsed;
            return {c74::min::symbol(text.c_str())};
        }}
    };

    c74::min::attribute<double> gamma{this, "gamma", 1.0,
        c74::min::description{"Gamma applied to sampled values. 1.0 leaves values unchanged."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(args.empty() || !bbb::dmx::maxutil::finite_atom(args[0])) {
                return {gamma_value_};
            }
            gamma_value_ = std::max(0.01, std::min(8.0, (double)args[0]));
            return {gamma_value_};
        }}
    };

    c74::min::attribute<double> brightness{this, "brightness", 1.0,
        c74::min::description{"Brightness multiplier applied after gamma."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(args.empty() || !bbb::dmx::maxutil::finite_atom(args[0])) {
                return {brightness_value_};
            }
            brightness_value_ = std::max(0.0, std::min(8.0, (double)args[0]));
            return {brightness_value_};
        }}
    };

    c74::min::attribute<bool> autobang{this, "autobang", true,
        c74::min::description{"Output DMX immediately when a jit_matrix is processed."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            autobang_value_ = args.empty() || ((int)args[0] != 0);
            return {autobang_value_};
        }}
    };

    c74::min::attribute<bool> invert_x{this, "invert_x", false,
        c74::min::description{"Mirror normalized sample coordinates horizontally."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            invert_x_value_ = !args.empty() && ((int)args[0] != 0);
            return {invert_x_value_};
        }}
    };

    c74::min::attribute<bool> invert_y{this, "invert_y", false,
        c74::min::description{"Mirror normalized sample coordinates vertically."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            invert_y_value_ = !args.empty() && ((int)args[0] != 0);
            return {invert_y_value_};
        }}
    };

    bbb_dmx_matrixmap() {
        bbb::dmx::report_external_build_info(cout, "bbb.dmx.matrixmap");
    }

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
            load_patch(patch_path_value_);
            return {};
        }
    };

    c74::min::message<> readmap_message{this, "readmap", "readmap matrix_map_json_path",
        MIN_FUNCTION {
            if(args.empty()) {
                report_error("readmap requires matrix map JSON path");
                return {};
            }
            const c74::min::symbol path_symbol{(c74::min::symbol)args[0]};
            map_path_value_ = path_symbol.c_str();
            suppress_map_attribute_load_ = true;
            map = path_symbol;
            suppress_map_attribute_load_ = false;
            map_load_pending_ = false;
            load_map(map_path_value_);
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
            suppress_group_attribute_load_ = true;
            group = path_symbol;
            suppress_group_attribute_load_ = false;
            groups_load_pending_ = false;
            load_groups(groups_path_value_);
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
            load_semantic_overrides(semantic_overrides_path_value_);
            return {};
        }
    };

    c74::min::message<> jit_matrix_message{this, "jit_matrix", "Process a Jitter matrix by name.",
        MIN_FUNCTION {
            if(args.empty()) {
                report_error("jit_matrix requires matrix name");
                return {};
            }
            const c74::min::symbol matrix_name{(c74::min::symbol)args[0]};
            process_jit_matrix(matrix_name);
            return {};
        }
    };

    c74::min::message<> bang_message{this, "bang", "Output the current mapped universe buffers.",
        MIN_FUNCTION {
            output_universes();
            return {};
        }
    };

    c74::min::message<> reload_message{this, "reload", "Reload current patch and map files.",
        MIN_FUNCTION {
            if(!patch_path_value_.empty()) {
                load_patch(patch_path_value_);
            }
            if(!map_path_value_.empty()) {
                load_map(map_path_value_);
            }
            if(!groups_path_value_.empty()) {
                load_groups(groups_path_value_);
            }
            if(!semantic_overrides_path_value_.empty()) {
                load_semantic_overrides(semantic_overrides_path_value_);
            }
            return {};
        }
    };

    c74::min::message<> dump_message{this, "dump", "Output load status.",
        MIN_FUNCTION {
            c74::min::atoms atoms;
            atoms.push_back(c74::min::symbol("status"));
            atoms.push_back(c74::min::symbol("patch_loaded"));
            atoms.push_back(patch_loaded_ ? 1 : 0);
            atoms.push_back(c74::min::symbol("map_loaded"));
            atoms.push_back(map_loaded_ ? 1 : 0);
            atoms.push_back(c74::min::symbol("groups_loaded"));
            atoms.push_back(groups_loaded_ ? 1 : 0);
            atoms.push_back(c74::min::symbol("groups_validated"));
            atoms.push_back(groups_validated_ ? 1 : 0);
            atoms.push_back(c74::min::symbol("semantic_overrides_loaded"));
            atoms.push_back(semantic_overrides_loaded_ ? 1 : 0);
            atoms.push_back(c74::min::symbol("semantic_overrides_validated"));
            atoms.push_back(semantic_overrides_validated_ ? 1 : 0);
            atoms.push_back(c74::min::symbol("fixtures"));
            atoms.push_back((int)matrix_map_.fixtures.size());
            atoms.push_back(c74::min::symbol("universe_mode"));
            atoms.push_back(c74::min::symbol(universe_output_mode_to_string(output_mode_value_)));
            status_output.send(atoms);
            return {};
        }
    };

private:
    void load_patch(const std::string &path) {
        const std::string resolved_path{bbb::dmx::maxutil::resolve_file_path(path)};
        bbb::dmx::fixture_mapper loaded_mapper{};
        const bbb::dmx::mapper_result result{bbb::dmx::load_fixture_mapper_from_patch_file(resolved_path, loaded_mapper)};
        if(!result.ok) {
            patch_loaded_ = false;
            report_error(result.message.c_str());
            return;
        }
        mapper_ = loaded_mapper;
        patch_loaded_ = true;
        previous_output_.clear();
        owned_output_channels_.clear();
        pending_output_channels_.clear();
        collecting_output_channels_ = false;
        groups_validated_ = false;
        semantic_overrides_validated_ = false;
        if(groups_loaded_) {
            const bbb::dmx::mapper_result groups_result{validate_loaded_groups()};
            if(!groups_result.ok) {
                report_error(groups_result.message.c_str());
                return;
            }
        }
        if(semantic_overrides_loaded_) {
            const bbb::dmx::mapper_result overrides_result{validate_loaded_semantic_overrides()};
            if(!overrides_result.ok) {
                report_error(overrides_result.message.c_str());
                return;
            }
        }
        report_status("patch_loaded");
    }

    void load_map(const std::string &path) {
        const std::string resolved_path{bbb::dmx::maxutil::resolve_file_path(path)};
        std::string text{};
        bbb::dmx::mapper_result result{bbb::dmx::read_text_file(resolved_path, text)};
        if(!result.ok) {
            map_loaded_ = false;
            report_error(result.message.c_str());
            return;
        }
        matrix_map_config loaded{};
        result = parse_matrix_map_text(text, loaded);
        if(!result.ok) {
            map_loaded_ = false;
            report_error(result.message.c_str());
            return;
        }
        matrix_map_ = loaded;
        map_loaded_ = true;
        report_status("map_loaded");
    }

    void schedule_patch_load() {
        if(patch_path_value_.empty()) {
            patch_load_pending_ = false;
            return;
        }
        patch_load_pending_ = true;
        patch_load_timer.delay(0);
    }

    void schedule_map_load() {
        if(map_path_value_.empty()) {
            map_load_pending_ = false;
            return;
        }
        map_load_pending_ = true;
        map_load_timer.delay(0);
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
    }

    void load_groups(const std::string &path) {
        const std::string resolved_path{bbb::dmx::maxutil::resolve_file_path(path)};
        bbb::dmx::fixture_group_set loaded_groups{};
        const bbb::dmx::mapper_result result{bbb::dmx::read_fixture_groups_file(resolved_path, loaded_groups)};
        if(!result.ok) {
            groups_loaded_ = false;
            groups_validated_ = false;
            report_error(result.message.c_str());
            return;
        }
        groups_ = loaded_groups;
        groups_loaded_ = true;
        groups_validated_ = false;
        if(patch_loaded_) {
            const bbb::dmx::mapper_result validate_result{validate_loaded_groups()};
            if(!validate_result.ok) {
                report_error(validate_result.message.c_str());
                return;
            }
        }
        report_status("groups_loaded");
    }

    void load_semantic_overrides(const std::string &path) {
        const std::string resolved_path{bbb::dmx::maxutil::resolve_file_path(path)};
        bbb::dmx::fixture_semantic_overrides loaded_overrides{};
        const bbb::dmx::mapper_result result{bbb::dmx::read_fixture_semantic_overrides_file(resolved_path, loaded_overrides)};
        if(!result.ok) {
            semantic_overrides_loaded_ = false;
            semantic_overrides_validated_ = false;
            report_error(result.message.c_str());
            return;
        }
        semantic_overrides_ = loaded_overrides;
        semantic_overrides_loaded_ = true;
        semantic_overrides_validated_ = false;
        if(patch_loaded_) {
            const bbb::dmx::mapper_result validate_result{validate_loaded_semantic_overrides()};
            if(!validate_result.ok) {
                report_error(validate_result.message.c_str());
                return;
            }
        }
        report_status("semantic_overrides_loaded");
    }

    bbb::dmx::mapper_result validate_loaded_groups() {
        if(!groups_loaded_) {
            return bbb::dmx::mapper_result::failure("no groups loaded");
        }
        if(!patch_loaded_) {
            groups_validated_ = false;
            return bbb::dmx::mapper_result::success();
        }
        const bbb::dmx::mapper_result result{bbb::dmx::validate_fixture_groups_for_patch(groups_, mapper_.patch())};
        groups_validated_ = result.ok;
        return result;
    }

    bbb::dmx::mapper_result validate_loaded_semantic_overrides() {
        if(!semantic_overrides_loaded_) {
            return bbb::dmx::mapper_result::failure("no semantic overrides loaded");
        }
        if(!patch_loaded_) {
            semantic_overrides_validated_ = false;
            return bbb::dmx::mapper_result::success();
        }
        for(const auto &fixture : mapper_.patch().fixtures) {
            const bbb::dmx::fixture_semantic_mode_override *mode_override{semantic_override_for_fixture(fixture)};
            if(!mode_override) {
                continue;
            }
            const bbb::dmx::fixture_profile *profile{mapper_.find_profile(fixture.profile)};
            if(!profile) {
                semantic_overrides_validated_ = false;
                return bbb::dmx::mapper_result::failure("semantic_overrides missing profile: " + fixture.profile);
            }
            const bbb::dmx::fixture_mode *mode{profile->find_mode(fixture.mode)};
            if(!mode) {
                semantic_overrides_validated_ = false;
                return bbb::dmx::mapper_result::failure("semantic_overrides missing mode: " + fixture.profile + ":" + fixture.mode);
            }
            bbb::dmx::mapper_result result{validate_semantic_override_parameters(fixture, *mode, *mode_override)};
            if(!result.ok) {
                semantic_overrides_validated_ = false;
                return result;
            }
        }
        semantic_overrides_validated_ = true;
        return bbb::dmx::mapper_result::success();
    }

    void process_jit_matrix(c74::min::symbol matrix_name) {
        if(!patch_loaded_) {
            report_error("no fixture patch loaded");
            return;
        }
        if(!map_loaded_) {
            report_error("no matrix map loaded");
            return;
        }
        void *matrix{c74::max::jit_object_findregistered(c74::max::gensym(matrix_name.c_str()))};
        if(!matrix) {
            report_error("jit_matrix not found");
            return;
        }
        if(c74::max::jit_object_classname(matrix) != c74::max::gensym("jit_matrix")) {
            report_error("registered object is not a jit_matrix");
            return;
        }

        c74::max::t_jit_matrix_info info{};
        c74::max::jit_object_method(matrix, c74::max::gensym("getinfo"), &info);
        matrix_value_kind value_kind{matrix_value_kind::uint8};
        if(info.type == c74::max::gensym("char")) {
            value_kind = matrix_value_kind::uint8;
        } else if(info.type == c74::max::gensym("float32")) {
            value_kind = matrix_value_kind::float32;
        } else {
            report_error("only char and float32 jit.matrix input is supported");
            return;
        }
        if(info.dim[0] <= 0 || info.planecount <= 0) {
            report_error("jit.matrix has invalid dimensions or planes");
            return;
        }
        c74::max::t_atom_long savelock{(c74::max::t_atom_long)c74::max::jit_object_method(matrix, c74::max::gensym("lock"), (void *)1)};
        char *data{nullptr};
        c74::max::jit_object_method(matrix, c74::max::gensym("getdata"), &data);
        if(!data) {
            c74::max::jit_object_method(matrix, c74::max::gensym("lock"), (void *)savelock);
            report_error("jit.matrix has no readable data");
            return;
        }

        matrix_read_view view{};
        view.data = data;
        view.width = info.dim[0];
        view.height = info.dimcount < 2 ? 1 : std::max<long>(1, info.dim[1]);
        view.plane_count = info.planecount;
        view.stride_x = info.dimstride[0];
        view.stride_y = info.dimcount < 2 ? 0 : info.dimstride[1];
        if(view.stride_x <= 0 || (1 < view.height && view.stride_y <= 0)) {
            c74::max::jit_object_method(matrix, c74::max::gensym("lock"), (void *)savelock);
            report_error("jit.matrix has invalid data strides");
            return;
        }
        view.plane_order = plane_order_value_;
        view.value_kind = value_kind;

        apply_matrix(view);
        c74::max::jit_object_method(matrix, c74::max::gensym("lock"), (void *)savelock);
        if(autobang_value_) {
            output_universes();
        }
    }

    void apply_matrix(const matrix_read_view &view) {
        pending_output_channels_.clear();
        collecting_output_channels_ = true;
        for(const auto &mapping : matrix_map_.fixtures) {
            sample_region region{mapping.sample};
            if(invert_x_value_) {
                region.x = 1.0 - region.x;
            }
            if(invert_y_value_) {
                region.y = 1.0 - region.y;
            }
            const color_value sampled{adjust_color(sample_region_color(view, region))};
            std::vector<std::string> fixture_ids{};
            bbb::dmx::mapper_result result{resolve_mapping_fixture_ids(mapping, fixture_ids)};
            if(!result.ok) {
                report_error(result.message.c_str());
                continue;
            }
            for(const auto &fixture_id : fixture_ids) {
                result = apply_matrix_mapping_to_fixture(fixture_id, mapping, sampled);
                if(!result.ok) {
                    report_error(result.message.c_str());
                }
            }
        }
        collecting_output_channels_ = false;
        owned_output_channels_ = pending_output_channels_;
        pending_output_channels_.clear();
    }

    bbb::dmx::mapper_result resolve_mapping_fixture_ids(const fixture_mapping &mapping, std::vector<std::string> &fixture_ids) {
        fixture_ids.clear();
        if(!mapping.fixture_id.empty()) {
            fixture_ids.push_back(mapping.fixture_id);
            return bbb::dmx::mapper_result::success();
        }
        if(mapping.group_id.empty()) {
            return bbb::dmx::mapper_result::failure("matrix mapping has no fixture id or group");
        }
        if(!groups_loaded_) {
            return bbb::dmx::mapper_result::failure("matrix mapping group requires loaded groups: " + mapping.group_id);
        }
        if(!groups_validated_) {
            const bbb::dmx::mapper_result validate_result{validate_loaded_groups()};
            if(!validate_result.ok) {
                return validate_result;
            }
        }
        return bbb::dmx::resolve_fixture_group_fixture_ids(groups_, mapper_.patch(), mapping.group_id, fixture_ids);
    }

    bbb::dmx::mapper_result apply_matrix_mapping_to_fixture(const std::string &fixture_id, const fixture_mapping &mapping, const color_value &sampled) {
        bool semantic_color_applied{false};
        const bbb::dmx::mapper_result semantic_result{apply_semantic_matrix_color_if_present(fixture_id, mapping, sampled, semantic_color_applied)};
        if(!semantic_result.ok) {
            return semantic_result;
        }
        for(const auto &binding : mapping.parameters) {
            if(semantic_color_applied && semantic_color_binding_parameter(binding.parameter)) {
                continue;
            }
            const std::string parameter{resolve_parameter_alias(fixture_id, binding.parameter)};
            const double value{color_source_value(binding.source, sampled)};
            const bbb::dmx::mapper_result result{mapper_.set_normalized(fixture_id, parameter, value)};
            if(!result.ok) {
                return result;
            }
            const bbb::dmx::mapper_result owned_result{mark_parameter_owned(fixture_id, parameter)};
            if(!owned_result.ok) {
                return owned_result;
            }
        }
        return bbb::dmx::mapper_result::success();
    }

    static bool semantic_color_binding_parameter(const std::string &parameter) {
        return parameter == "red" || parameter == "green" || parameter == "blue";
    }

    const parameter_binding *find_parameter_binding(const fixture_mapping &mapping, const std::string &parameter) const {
        for(const auto &binding : mapping.parameters) {
            if(binding.parameter == parameter) {
                return &binding;
            }
        }
        return nullptr;
    }

    bbb::dmx::mapper_result apply_semantic_matrix_color_if_present(
        const std::string &fixture_id,
        const fixture_mapping &mapping,
        const color_value &sampled,
        bool &applied
    ) {
        applied = false;
        const parameter_binding *red_binding{find_parameter_binding(mapping, "red")};
        const parameter_binding *green_binding{find_parameter_binding(mapping, "green")};
        const parameter_binding *blue_binding{find_parameter_binding(mapping, "blue")};
        if(!red_binding || !green_binding || !blue_binding) {
            return bbb::dmx::mapper_result::success();
        }

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
        const bbb::dmx::semantic_color_request color{bbb::dmx::make_semantic_color_request(
            color_source_value(red_binding->source, sampled),
            color_source_value(green_binding->source, sampled),
            color_source_value(blue_binding->source, sampled)
        )};
        const bbb::dmx::fixture_semantic_mode_override *mode_override{semantic_override_for_fixture(*fixture)};
        const bbb::dmx::semantic_color_mapping color_mapping{bbb::dmx::semantic_color_parameters_for_mode(
            profile,
            *mode,
            color,
            bbb::dmx::semantic_color_options{color_use_white_value_, color_wheel_fallback_value_},
            current_color_wheel_values,
            mode_override
        )};
        if(!color_mapping.ok) {
            return bbb::dmx::mapper_result::failure(
                "fixture " + fixture_id + " " + fixture->profile + ":" + fixture->mode
                + " cannot map semantic RGB from matrix: " + color_mapping.message
                + ". Enable @color_wheel_fallback 1 for color-wheel fixtures or provide semantic_overrides."
            );
        }
        for(const auto &parameter : color_mapping.parameters) {
            const bbb::dmx::mapper_result result{mapper_.set_normalized(fixture_id, parameter.first, parameter.second)};
            if(!result.ok) {
                return result;
            }
            const bbb::dmx::mapper_result owned_result{mark_parameter_owned(fixture_id, parameter.first)};
            if(!owned_result.ok) {
                return owned_result;
            }
        }
        applied = true;
        return bbb::dmx::mapper_result::success();
    }

    color_value adjust_color(const color_value &color) const {
        return color_value{
            adjust_normalized(color.red),
            adjust_normalized(color.green),
            adjust_normalized(color.blue),
            adjust_normalized(color.alpha),
        };
    }

    double adjust_normalized(double value) const {
        const double normalized{clamp_normalized(value)};
        const double corrected{std::pow(normalized, gamma_value_) * brightness_value_};
        return clamp_normalized(corrected);
    }

    const bbb::dmx::fixture_instance *find_fixture_instance(const std::string &fixture_id) const {
        for(const auto &fixture : mapper_.patch().fixtures) {
            if(fixture.id == fixture_id) {
                return &fixture;
            }
        }
        return nullptr;
    }

    const bbb::dmx::fixture_semantic_mode_override *semantic_override_for_fixture(const bbb::dmx::fixture_instance &fixture) const {
        if(!semantic_overrides_loaded_) {
            return nullptr;
        }
        return semantic_overrides_.find_mode_override(fixture.profile, fixture.mode);
    }

    std::string resolve_parameter_alias(const std::string &fixture_id, const std::string &parameter_key) const {
        const bbb::dmx::fixture_instance *fixture{find_fixture_instance(fixture_id)};
        if(!fixture) {
            return parameter_key;
        }
        const bbb::dmx::fixture_semantic_mode_override *mode_override{semantic_override_for_fixture(*fixture)};
        if(!mode_override) {
            return parameter_key;
        }
        return mode_override->resolve_alias(parameter_key);
    }

    std::vector<std::pair<std::string, int>> current_color_wheel_parameter_values(const std::string &fixture_id, const bbb::dmx::fixture_mode &mode) const {
        std::vector<std::pair<std::string, int>> values;
        for(const auto &parameter : mode.parameters) {
            if(!bbb::dmx::parameter_is_likely_color_wheel(parameter)) {
                continue;
            }
            if(!parameter_owned(fixture_id, parameter.key)) {
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

    bbb::dmx::mapper_result mark_parameter_owned(const std::string &fixture_id, const std::string &parameter_key) {
        std::vector<std::pair<int, int>> addresses;
        const bbb::dmx::mapper_result result{mapper_.parameter_channel_addresses(fixture_id, parameter_key, addresses)};
        if(!result.ok) {
            return result;
        }
        std::map<int, std::set<int>> &target_channels = collecting_output_channels_ ? pending_output_channels_ : owned_output_channels_;
        for(const auto &address : addresses) {
            target_channels[address.first].insert(address.second);
        }
        return bbb::dmx::mapper_result::success();
    }

    bool parameter_owned(const std::string &fixture_id, const std::string &parameter_key) const {
        std::vector<std::pair<int, int>> addresses;
        const bbb::dmx::mapper_result result{mapper_.parameter_channel_addresses(fixture_id, parameter_key, addresses)};
        if(!result.ok) {
            return false;
        }
        for(const auto &address : addresses) {
            const auto universe_found = owned_output_channels_.find(address.first);
            if(universe_found == owned_output_channels_.end()) {
                return false;
            }
            if(universe_found->second.find(address.second) == universe_found->second.end()) {
                return false;
            }
        }
        return true;
    }

    bbb::dmx::mapper_result validate_semantic_override_parameter(
        const bbb::dmx::fixture_instance &fixture,
        const bbb::dmx::fixture_mode &mode,
        const std::string &parameter_key,
        const std::string &usage
    ) const
    {
        if(parameter_key.empty()) {
            return bbb::dmx::mapper_result::success();
        }
        if(!mode.find_parameter(parameter_key)) {
            return bbb::dmx::mapper_result::failure(
                "semantic_overrides " + fixture.profile + ":" + fixture.mode + " " + usage + " references unknown parameter: " + parameter_key
            );
        }
        return bbb::dmx::mapper_result::success();
    }

    bbb::dmx::mapper_result validate_semantic_override_parameters(
        const bbb::dmx::fixture_instance &fixture,
        const bbb::dmx::fixture_mode &mode,
        const bbb::dmx::fixture_semantic_mode_override &mode_override
    ) const
    {
        for(const auto &alias : mode_override.aliases) {
            bbb::dmx::mapper_result result{validate_semantic_override_parameter(fixture, mode, alias.second, "alias " + alias.first)};
            if(!result.ok) {
                return result;
            }
        }
        for(const auto &parameter_key : mode_override.intensity_parameters) {
            bbb::dmx::mapper_result result{validate_semantic_override_parameter(fixture, mode, parameter_key, "intensity")};
            if(!result.ok) {
                return result;
            }
        }
        bbb::dmx::mapper_result result{validate_semantic_override_parameter(fixture, mode, mode_override.primary_intensity_parameter, "primary_intensity")};
        if(!result.ok) {
            return result;
        }
        for(const auto &block : mode_override.rgb_blocks) {
            for(const auto &parameter_key : {block.red, block.green, block.blue, block.white, block.dimmer}) {
                result = validate_semantic_override_parameter(fixture, mode, parameter_key, "rgb");
                if(!result.ok) {
                    return result;
                }
            }
        }
        for(const auto &block : mode_override.cmy_blocks) {
            for(const auto &parameter_key : {block.cyan, block.magenta, block.yellow, block.dimmer}) {
                result = validate_semantic_override_parameter(fixture, mode, parameter_key, "cmy");
                if(!result.ok) {
                    return result;
                }
            }
        }
        return bbb::dmx::mapper_result::success();
    }

    void output_universes() {
        if(!patch_loaded_) {
            report_error("no fixture patch loaded");
            return;
        }
        if(output_mode_value_ == universe_output_mode::selected) {
            if(owned_output_channels_.find(universe_value_) != owned_output_channels_.end()) {
                output_universe(universe_value_);
            }
            return;
        }
        std::set<int> universe_ids;
        for(const auto &entry : owned_output_channels_) {
            universe_ids.insert(bbb::dmx::sanitize_universe_id(entry.first));
        }
        for(const int universe_id : universe_ids) {
            if(output_mode_value_ == universe_output_mode::changed) {
                const auto found = previous_output_.find(universe_id);
                if(found != previous_output_.end()) {
                    const bbb::dmx::dmx_universe universe_ref{owned_universe(universe_id)};
                    const std::vector<int> changes{bbb::dmx::changed_channels(found->second, universe_ref)};
                    if(changes.empty()) {
                        continue;
                    }
                }
            }
            output_universe(universe_id);
        }
    }

    void output_universe(int universe_id) {
        const int sanitized_universe{bbb::dmx::sanitize_universe_id(universe_id)};
        const bbb::dmx::dmx_universe universe_ref{owned_universe(sanitized_universe)};
        output.send(bbb::dmx::maxutil::universe_atoms(sanitized_universe, universe_ref));
        previous_output_[sanitized_universe] = universe_ref;
    }

    bbb::dmx::dmx_universe owned_universe(int universe_id) const {
        bbb::dmx::dmx_universe output_universe{};
        const auto owned_found = owned_output_channels_.find(universe_id);
        if(owned_found == owned_output_channels_.end()) {
            return output_universe;
        }
        const bbb::dmx::dmx_universe &mapped_universe = mapper_.universe(universe_id);
        for(const int address : owned_found->second) {
            output_universe.set_channel(address, mapped_universe.channel(address));
        }
        return output_universe;
    }


    void report_status(const char *message) {
        bbb::dmx::maxutil::send_status(status_output, "status", message);
    }

    void report_error(const char *message) {
        cerr << "bbb.dmx.matrixmap: " << message << c74::min::endl;
        bbb::dmx::maxutil::send_status(status_output, "error", message);
    }
};

MIN_EXTERNAL(bbb_dmx_matrixmap);
