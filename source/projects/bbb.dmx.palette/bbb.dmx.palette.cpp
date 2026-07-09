#include "c74_min.h"

#include <bbb/dmx/build_info.hpp>

#include <bbb/dmx/fixture_json.hpp>
#include <bbb/dmx/fixture_runtime.hpp>
#include <bbb/dmx/max_external_utils.hpp>
#include <bbb/dmx/pattern.hpp>

#include <map>
#include <string>

class bbb_dmx_palette : public c74::min::object<bbb_dmx_palette> {
private:
    bbb::dmx::fixture_mapper mapper_{};
    std::map<std::string, bbb::dmx::fixture_parameter_values> palettes_{};
    std::string setup_path_value_{};
    std::string patch_path_value_{};
    std::string palette_path_value_{};
    bool patch_loaded_{false};
    bool palette_loaded_{false};
    bool autobang_value_{true};
    bool setup_load_pending_{false};
    bool suppress_setup_attribute_load_{false};
    bool suppress_patch_attribute_load_{false};
    bool patch_attribute_overridden_{false};

public:
    MIN_DESCRIPTION{"Apply named fixture-parameter palettes to patched fixtures."};
    MIN_TAGS{"dmx, lighting, fixture, palette, color"};
    MIN_AUTHOR{"2bit"};
    MIN_RELATED{"bbb.dmx.fixturemap, bbb.dmx.scene, bbb.dmx.matrixmap"};

    c74::min::inlet<> input{this, "(readpatch/readpalette/apply/bang) palette control"};
    c74::min::outlet<> output{this, "(anything) universe frames"};
    c74::min::outlet<> status_output{this, "(anything) status and error messages"};

    c74::min::attribute<c74::min::symbol> setup{this, "setup", "",
        c74::min::description{"Optional bbb.dmx.setup.v1 JSON path. Uses the top-level patch value unless @patch overrides it."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(args.empty()) {
                if(!suppress_setup_attribute_load_) {
                    setup_path_value_.clear();
                    setup_load_pending_ = false;
                }
                return {c74::min::symbol("")};
            }
            const c74::min::symbol symbol_value{(c74::min::symbol)args[0]};
            setup_path_value_ = symbol_value.c_str();
            if(!suppress_setup_attribute_load_) {
                setup_load_pending_ = true;
                init_timer.delay(0);
            }
            return {symbol_value};
        }}
    };

    c74::min::attribute<c74::min::symbol> patch{this, "patch", "",
        c74::min::description{"Fixture patch JSON path."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(args.empty()) {
                patch_path_value_.clear();
                return {c74::min::symbol("")};
            }
            const c74::min::symbol symbol_value{(c74::min::symbol)args[0]};
            patch_path_value_ = symbol_value.c_str();
            if(bbb::dmx::maxutil::should_mark_explicit_symbol_override(args, false, suppress_patch_attribute_load_)) {
                patch_attribute_overridden_ = true;
            }
            return {symbol_value};
        }}
    };

    c74::min::attribute<c74::min::symbol> palette{this, "palette", "",
        c74::min::description{"Palette JSON path."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(args.empty()) {
                palette_path_value_.clear();
                return {c74::min::symbol("")};
            }
            const c74::min::symbol symbol_value{(c74::min::symbol)args[0]};
            palette_path_value_ = symbol_value.c_str();
            return {symbol_value};
        }}
    };

    c74::min::attribute<bool> autobang{this, "autobang", true,
        c74::min::description{"Output frames immediately after apply."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            autobang_value_ = args.empty() || ((int)args[0] != 0);
            return {autobang_value_};
        }}
    };

    c74::min::timer<c74::min::timer_options::defer_delivery> init_timer{this,
        MIN_FUNCTION {
            if(!setup_path_value_.empty()) {
                setup_load_pending_ = false;
                load_setup(setup_path_value_);
            }
            if(!patch_loaded_ && !patch_path_value_.empty()) {
                load_patch(patch_path_value_);
            }
            if(!palette_path_value_.empty()) {
                load_palette(palette_path_value_);
            }
            return {};
        }
    };

    bbb_dmx_palette() {
        bbb::dmx::report_external_build_info(cout, "bbb.dmx.palette");
        init_timer.delay(0);
    }

    c74::min::message<> readpatch_message{this, "readpatch", "readpatch patch_json_path",
        MIN_FUNCTION {
            if(args.empty()) {
                report_error("readpatch requires patch JSON path");
                return {};
            }
            const c74::min::symbol path_symbol{(c74::min::symbol)args[0]};
            patch_path_value_ = path_symbol.c_str();
            patch = path_symbol;
            load_patch(patch_path_value_);
            return {};
        }
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
            load_setup(setup_path_value_);
            return {};
        }
    };

    c74::min::message<> readpalette_message{this, "readpalette", "readpalette palette_json_path",
        MIN_FUNCTION {
            if(args.empty()) {
                report_error("readpalette requires palette JSON path");
                return {};
            }
            const c74::min::symbol path_symbol{(c74::min::symbol)args[0]};
            palette_path_value_ = path_symbol.c_str();
            palette = path_symbol;
            load_palette(palette_path_value_);
            return {};
        }
    };

    c74::min::message<> reload_message{this, "reload", "Reload patch and palette files.",
        MIN_FUNCTION {
            if(!patch_path_value_.empty()) {
                load_patch(patch_path_value_);
            }
            if(!palette_path_value_.empty()) {
                load_palette(palette_path_value_);
            }
            return {};
        }
    };

    c74::min::message<> apply_message{this, "apply", "apply palette_name [fixture_pattern ...]",
        MIN_FUNCTION {
            apply_from_args(args);
            return {};
        }
    };

    c74::min::message<> bang_message{this, "bang", "Output all patched universes.",
        MIN_FUNCTION {
            output_all_universes();
            return {};
        }
    };

    c74::min::message<> clear_message{this, "clear", "Reset fixture channels to profile defaults.",
        MIN_FUNCTION {
            const bbb::dmx::mapper_result result{mapper_.reset_universes()};
            if(!result.ok) {
                report_error(result.message);
                return {};
            }
            report_status("clear");
            if(autobang_value_) {
                output_all_universes();
            }
            return {};
        }
    };

    c74::min::message<> dump_message{this, "dump", "Output palette count.",
        MIN_FUNCTION {
            c74::min::atoms atoms;
            atoms.push_back(c74::min::symbol("status"));
            atoms.push_back(c74::min::symbol("palettes"));
            atoms.push_back((int)palettes_.size());
            status_output.send(atoms);
            return {};
        }
    };

private:
    void load_setup(const std::string &path) {
        bbb::dmx::maxutil::setup_load_result setup_result{};
        const bbb::dmx::mapper_result result{bbb::dmx::maxutil::read_common_setup_values(this->maxobj(), path, setup_result)};
        if(!result.ok) {
            report_error(result.message);
            return;
        }
        if(setup_result.values.patch.has_value() && !patch_attribute_overridden_) {
            const std::string resolved_path{bbb::dmx::maxutil::setup_relative_path(setup_result.base_directory, setup_result.values.patch.value())};
            bbb::dmx::maxutil::apply_setup_symbol_path(patch, suppress_patch_attribute_load_, patch_path_value_, resolved_path);
            load_patch(patch_path_value_);
        }
        report_status("setup_loaded");
    }

    void apply_from_args(const c74::min::atoms &args) {
        if(args.empty()) {
            report_error("apply requires palette name");
            return;
        }
        if(!ensure_ready()) {
            return;
        }
        const std::string palette_name{bbb::dmx::maxutil::symbol_arg(args[0])};
        const auto found = palettes_.find(palette_name);
        if(found == palettes_.end()) {
            report_error("unknown palette: " + palette_name);
            return;
        }
        std::vector<std::string> patterns{};
        for(std::size_t index = 1; index < args.size(); index++) {
            patterns.push_back(bbb::dmx::maxutil::symbol_arg(args[index]));
        }
        int applied_count{0};
        for(const auto &fixture : mapper_.patch().fixtures) {
            if(!patterns.empty() && !matches_any_pattern(patterns, fixture.id)) {
                continue;
            }
            for(const auto &parameter : found->second) {
                const bbb::dmx::mapper_result result{mapper_.set_normalized(fixture.id, parameter.first, parameter.second)};
                if(result.ok) {
                    applied_count++;
                }
            }
        }
        c74::min::atoms atoms;
        atoms.push_back(c74::min::symbol("status"));
        atoms.push_back(c74::min::symbol("applied"));
        atoms.push_back(c74::min::symbol(palette_name.c_str()));
        atoms.push_back(applied_count);
        status_output.send(atoms);
        if(autobang_value_) {
            output_all_universes();
        }
    }

    bool matches_any_pattern(const std::vector<std::string> &patterns, const std::string &text) const {
        for(const auto &pattern : patterns) {
            if(bbb::dmx::wildcard_match(pattern, text)) {
                return true;
            }
        }
        return false;
    }

    bool ensure_ready() {
        if(!patch_loaded_ && !patch_path_value_.empty()) {
            load_patch(patch_path_value_);
        }
        if(!palette_loaded_ && !palette_path_value_.empty()) {
            load_palette(palette_path_value_);
        }
        if(!patch_loaded_) {
            report_error("no patch loaded");
            return false;
        }
        if(!palette_loaded_) {
            report_error("no palette loaded");
            return false;
        }
        return true;
    }

    void load_patch(const std::string &path) {
        bbb::dmx::fixture_mapper loaded_mapper{};
        const bbb::dmx::mapper_result result{bbb::dmx::load_fixture_mapper_from_patch_file(bbb::dmx::maxutil::resolve_file_path(this->maxobj(), path), loaded_mapper)};
        if(!result.ok) {
            patch_loaded_ = false;
            report_error(result.message);
            return;
        }
        mapper_ = loaded_mapper;
        patch_loaded_ = true;
        report_status("patch_loaded");
    }

    void load_palette(const std::string &path) {
        std::string text{};
        bbb::dmx::mapper_result result{bbb::dmx::read_text_file(bbb::dmx::maxutil::resolve_file_path(this->maxobj(), path), text)};
        if(!result.ok) {
            palette_loaded_ = false;
            report_error(result.message);
            return;
        }
        const bbb::dmx::json_parse_result parsed{bbb::dmx::parse_json_text(text)};
        if(!parsed.ok) {
            palette_loaded_ = false;
            report_error(parsed.message);
            return;
        }
        result = parse_palette_json(parsed.value);
        if(!result.ok) {
            palette_loaded_ = false;
            report_error(result.message);
            return;
        }
        palette_loaded_ = true;
        report_status("palette_loaded");
    }

    bbb::dmx::mapper_result parse_palette_json(const bbb::dmx::json_value &root) {
        if(root.type != bbb::dmx::json_type::object) {
            return bbb::dmx::mapper_result::failure("palette root must be object");
        }
        const bbb::dmx::json_value *palettes_value{root.find("palettes")};
        if(!palettes_value || palettes_value->type != bbb::dmx::json_type::object) {
            return bbb::dmx::mapper_result::failure("palettes must be object");
        }
        std::map<std::string, bbb::dmx::fixture_parameter_values> loaded{};
        for(const auto &entry : palettes_value->object_value) {
            bbb::dmx::fixture_parameter_values parameters{};
            const bbb::dmx::mapper_result result{bbb::dmx::parse_fixture_parameter_values(entry.second, parameters, "palette")};
            if(!result.ok) {
                return result;
            }
            loaded[entry.first] = parameters;
        }
        palettes_ = loaded;
        return bbb::dmx::mapper_result::success();
    }

    void output_all_universes() {
        for(const int universe_id : bbb::dmx::fixture_universe_ids(mapper_)) {
            output.send(bbb::dmx::maxutil::universe_atoms(universe_id, mapper_.universe(universe_id)));
        }
    }


    void report_status(const char *message) {
        bbb::dmx::maxutil::send_status(status_output, "status", message);
    }

    void report_error(const std::string &message) {
        cerr << "bbb.dmx.palette: " << message << c74::min::endl;
        bbb::dmx::maxutil::send_status(status_output, "error", message);
    }
};

MIN_EXTERNAL(bbb_dmx_palette);
