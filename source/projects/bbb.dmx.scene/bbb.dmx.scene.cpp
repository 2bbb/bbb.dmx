#include "c74_min.h"

#include <bbb/dmx/fixture_json.hpp>
#include <bbb/dmx/max_external_utils.hpp>
#include <bbb/dmx/pattern.hpp>

#include <map>
#include <set>
#include <string>
#include <vector>

namespace {

using parameter_map = std::map<std::string, double>;

struct scene_rule {
public:
    std::string fixture_pattern{};
    parameter_map parameters{};
};

bool parse_parameter_map(const bbb::dmx::json_value &value, parameter_map &parameters, std::string &error) {
    if(value.type != bbb::dmx::json_type::object) {
        error = "parameter map must be object";
        return false;
    }
    parameters.clear();
    for(const auto &entry : value.object_value) {
        if(entry.second.type != bbb::dmx::json_type::number) {
            error = "scene parameter must be numeric: " + entry.first;
            return false;
        }
        parameters[entry.first] = entry.second.number_value;
    }
    return true;
}

} // namespace

class bbb_dmx_scene : public c74::min::object<bbb_dmx_scene> {
private:
    bbb::dmx::fixture_mapper mapper_{};
    std::map<std::string, std::vector<scene_rule>> scenes_{};
    std::string patch_path_value_{};
    std::string scene_path_value_{};
    bool patch_loaded_{false};
    bool scene_loaded_{false};
    bool autobang_value_{true};

public:
    MIN_DESCRIPTION{"Recall named fixture-parameter scenes into multi-universe DMX frames."};
    MIN_TAGS{"dmx, lighting, fixture, scene, preset"};
    MIN_AUTHOR{"2bit"};
    MIN_RELATED{"bbb.dmx.fixturemap, bbb.dmx.palette, bbb.dmx.record"};

    c74::min::inlet<> input{this, "(readpatch/readscene/apply/bang) scene control"};
    c74::min::outlet<> output{this, "(anything) universe frames"};
    c74::min::outlet<> status_output{this, "(anything) status and error messages"};

    c74::min::attribute<c74::min::symbol> patch{this, "patch", "",
        c74::min::description{"Fixture patch JSON path."},
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

    c74::min::attribute<c74::min::symbol> scene{this, "scene", "",
        c74::min::description{"Scene JSON path."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(args.empty()) {
                scene_path_value_.clear();
                return {c74::min::symbol("")};
            }
            const c74::min::symbol symbol_value{(c74::min::symbol)args[0]};
            scene_path_value_ = symbol_value.c_str();
            return {symbol_value};
        }}
    };

    c74::min::attribute<bool> autobang{this, "autobang", true,
        c74::min::description{"Output frames immediately after scene recall."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            autobang_value_ = args.empty() || ((int)args[0] != 0);
            return {autobang_value_};
        }}
    };

    c74::min::timer<c74::min::timer_options::defer_delivery> init_timer{this,
        MIN_FUNCTION {
            if(!patch_path_value_.empty()) {
                load_patch(patch_path_value_);
            }
            if(!scene_path_value_.empty()) {
                load_scene(scene_path_value_);
            }
            return {};
        }
    };

    bbb_dmx_scene() {
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

    c74::min::message<> readscene_message{this, "readscene", "readscene scene_json_path",
        MIN_FUNCTION {
            if(args.empty()) {
                report_error("readscene requires scene JSON path");
                return {};
            }
            const c74::min::symbol path_symbol{(c74::min::symbol)args[0]};
            scene_path_value_ = path_symbol.c_str();
            scene = path_symbol;
            load_scene(scene_path_value_);
            return {};
        }
    };

    c74::min::message<> reload_message{this, "reload", "Reload patch and scene files.",
        MIN_FUNCTION {
            if(!patch_path_value_.empty()) {
                load_patch(patch_path_value_);
            }
            if(!scene_path_value_.empty()) {
                load_scene(scene_path_value_);
            }
            return {};
        }
    };

    c74::min::message<> apply_message{this, "apply", "apply scene_name",
        MIN_FUNCTION {
            recall_from_args(args);
            return {};
        }
    };

    c74::min::message<> bang_message{this, "bang", "Output all patched universes.",
        MIN_FUNCTION {
            output_all_universes();
            return {};
        }
    };

    c74::min::message<> dump_message{this, "dump", "Output scene count.",
        MIN_FUNCTION {
            c74::min::atoms atoms;
            atoms.push_back(c74::min::symbol("status"));
            atoms.push_back(c74::min::symbol("scenes"));
            atoms.push_back((int)scenes_.size());
            status_output.send(atoms);
            return {};
        }
    };

private:
    void recall_from_args(const c74::min::atoms &args) {
        if(args.empty()) {
            report_error("apply requires scene name");
            return;
        }
        if(!ensure_ready()) {
            return;
        }
        const std::string scene_name{bbb::dmx::maxutil::symbol_arg(args[0])};
        const auto found = scenes_.find(scene_name);
        if(found == scenes_.end()) {
            report_error("unknown scene: " + scene_name);
            return;
        }
        const bbb::dmx::mapper_result reset_result{mapper_.reset_universes()};
        if(!reset_result.ok) {
            report_error(reset_result.message);
            return;
        }
        int applied_count{0};
        for(const auto &rule : found->second) {
            for(const auto &fixture : mapper_.patch().fixtures) {
                if(!bbb::dmx::wildcard_match(rule.fixture_pattern, fixture.id)) {
                    continue;
                }
                for(const auto &parameter : rule.parameters) {
                    const bbb::dmx::mapper_result result{mapper_.set_normalized(fixture.id, parameter.first, parameter.second)};
                    if(result.ok) {
                        applied_count++;
                    }
                }
            }
        }
        c74::min::atoms atoms;
        atoms.push_back(c74::min::symbol("status"));
        atoms.push_back(c74::min::symbol("recalled"));
        atoms.push_back(c74::min::symbol(scene_name.c_str()));
        atoms.push_back(applied_count);
        status_output.send(atoms);
        if(autobang_value_) {
            output_all_universes();
        }
    }

    bool ensure_ready() {
        if(!patch_loaded_ && !patch_path_value_.empty()) {
            load_patch(patch_path_value_);
        }
        if(!scene_loaded_ && !scene_path_value_.empty()) {
            load_scene(scene_path_value_);
        }
        if(!patch_loaded_) {
            report_error("no patch loaded");
            return false;
        }
        if(!scene_loaded_) {
            report_error("no scene loaded");
            return false;
        }
        return true;
    }

    void load_patch(const std::string &path) {
        bbb::dmx::fixture_mapper loaded_mapper{};
        const bbb::dmx::mapper_result result{bbb::dmx::load_fixture_mapper_from_patch_file(resolve_file_path(path), loaded_mapper)};
        if(!result.ok) {
            patch_loaded_ = false;
            report_error(result.message);
            return;
        }
        mapper_ = loaded_mapper;
        patch_loaded_ = true;
        report_status("patch_loaded");
    }

    void load_scene(const std::string &path) {
        std::string text{};
        bbb::dmx::mapper_result result{bbb::dmx::read_text_file(resolve_file_path(path), text)};
        if(!result.ok) {
            scene_loaded_ = false;
            report_error(result.message);
            return;
        }
        const bbb::dmx::json_parse_result parsed{bbb::dmx::parse_json_text(text)};
        if(!parsed.ok) {
            scene_loaded_ = false;
            report_error(parsed.message);
            return;
        }
        result = parse_scene_json(parsed.value);
        if(!result.ok) {
            scene_loaded_ = false;
            report_error(result.message);
            return;
        }
        scene_loaded_ = true;
        report_status("scene_loaded");
    }

    bbb::dmx::mapper_result parse_scene_json(const bbb::dmx::json_value &root) {
        if(root.type != bbb::dmx::json_type::object) {
            return bbb::dmx::mapper_result::failure("scene root must be object");
        }
        const bbb::dmx::json_value *scenes_value{root.find("scenes")};
        if(!scenes_value || scenes_value->type != bbb::dmx::json_type::object) {
            return bbb::dmx::mapper_result::failure("scenes must be object");
        }
        std::map<std::string, std::vector<scene_rule>> loaded{};
        std::string error{};
        for(const auto &scene_entry : scenes_value->object_value) {
            if(scene_entry.second.type != bbb::dmx::json_type::object) {
                return bbb::dmx::mapper_result::failure("scene entry must be object: " + scene_entry.first);
            }
            std::vector<scene_rule> rules{};
            for(const auto &rule_entry : scene_entry.second.object_value) {
                scene_rule rule{};
                rule.fixture_pattern = rule_entry.first;
                if(!parse_parameter_map(rule_entry.second, rule.parameters, error)) {
                    return bbb::dmx::mapper_result::failure(error);
                }
                rules.push_back(rule);
            }
            loaded[scene_entry.first] = rules;
        }
        scenes_ = loaded;
        return bbb::dmx::mapper_result::success();
    }

    void output_all_universes() {
        std::set<int> universes{};
        for(const auto &fixture : mapper_.patch().fixtures) {
            universes.insert(fixture.universe);
        }
        for(const int universe_id : universes) {
            output.send(bbb::dmx::maxutil::universe_atoms(universe_id, mapper_.universe(universe_id)));
        }
    }

    std::string resolve_file_path(const std::string &path) {
        if(path.empty() || bbb::dmx::path_is_absolute(path)) {
            return path;
        }
        c74::max::t_symbol *resolved_symbol{nullptr};
        const c74::max::t_max_err error{c74::max::path_absolutepath(&resolved_symbol, c74::max::gensym(path.c_str()), nullptr, 0)};
        if(error == 0 && resolved_symbol && resolved_symbol->s_name) {
            return resolved_symbol->s_name;
        }
        return path;
    }

    void report_status(const char *message) {
        status_output.send(bbb::dmx::maxutil::status_atoms("status", message));
    }

    void report_error(const std::string &message) {
        cerr << "bbb.dmx.scene: " << message << c74::min::endl;
        status_output.send(bbb::dmx::maxutil::status_atoms("error", message));
    }
};

MIN_EXTERNAL(bbb_dmx_scene);
