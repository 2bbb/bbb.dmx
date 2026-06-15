#include "c74_min.h"

#include <bbb/dmx/build_info.hpp>
#include "c74_jitter.h"

#include <bbb/dmx/fixture_json.hpp>
#include <bbb/dmx/frame_set.hpp>
#include <bbb/dmx/max_external_utils.hpp>
#include <bbb/dmx/matrix_map.hpp>

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <string>

using bbb::dmx::matrixmap::clamp_normalized;
using bbb::dmx::matrixmap::color_source_value;
using bbb::dmx::matrixmap::color_value;
using bbb::dmx::matrixmap::matrix_map_config;
using bbb::dmx::matrixmap::matrix_read_view;
using bbb::dmx::matrixmap::matrix_value_kind;
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
    std::string patch_path_value_{};
    std::string map_path_value_{};
    int universe_value_{1};
    plane_order_kind plane_order_value_{plane_order_kind::rgba};
    universe_output_mode output_mode_value_{universe_output_mode::all};
    double gamma_value_{1.0};
    double brightness_value_{1.0};
    bool autobang_value_{true};
    bool invert_x_value_{false};
    bool invert_y_value_{false};
    bool patch_loaded_{false};
    bool map_loaded_{false};
    std::map<int, bbb::dmx::dmx_universe> previous_output_{};

public:
    MIN_DESCRIPTION{"Sample jit.matrix color data and patch it into multi-universe DMX fixture parameters."};
    MIN_TAGS{"dmx, jitter, matrix, color, fixture, universe"};
    MIN_AUTHOR{"2bit"};
    MIN_RELATED{"bbb.dmx.fixturemap, bbb.dmx.merge, bbb.dmx.safety"};

    c74::min::inlet<> input{this, "(jit_matrix/readpatch/readmap/bang) matrix-to-DMX mapper input"};
    c74::min::outlet<> output{this, "(anything) universe id and 512 DMX bytes"};
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

    c74::min::attribute<c74::min::symbol> map{this, "map", "",
        c74::min::description{"Matrix map JSON path."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(args.empty()) {
                map_path_value_.clear();
                return {c74::min::symbol("")};
            }
            const c74::min::symbol symbol_value{(c74::min::symbol)args[0]};
            map_path_value_ = symbol_value.c_str();
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

    c74::min::timer<c74::min::timer_options::defer_delivery> init_timer{this,
        MIN_FUNCTION {
            if(!patch_path_value_.empty()) {
                load_patch(patch_path_value_);
            }
            if(!map_path_value_.empty()) {
                load_map(map_path_value_);
            }
            return {};
        }
    };

    bbb_dmx_matrixmap() {
        bbb::dmx::report_external_build_info(cout, "bbb.dmx.matrixmap");
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

    c74::min::message<> readmap_message{this, "readmap", "readmap matrix_map_json_path",
        MIN_FUNCTION {
            if(args.empty()) {
                report_error("readmap requires matrix map JSON path");
                return {};
            }
            const c74::min::symbol path_symbol{(c74::min::symbol)args[0]};
            map_path_value_ = path_symbol.c_str();
            map = path_symbol;
            load_map(map_path_value_);
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
        char *data{(char *)c74::max::jit_object_method(matrix, c74::max::gensym("getdata"))};
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
        view.plane_order = plane_order_value_;
        view.value_kind = value_kind;

        apply_matrix(view);
        c74::max::jit_object_method(matrix, c74::max::gensym("lock"), (void *)savelock);
        if(autobang_value_) {
            output_universes();
        }
    }

    void apply_matrix(const matrix_read_view &view) {
        for(const auto &mapping : matrix_map_.fixtures) {
            sample_region region{mapping.sample};
            if(invert_x_value_) {
                region.x = 1.0 - region.x;
            }
            if(invert_y_value_) {
                region.y = 1.0 - region.y;
            }
            const color_value sampled{adjust_color(sample_region_color(view, region))};
            for(const auto &binding : mapping.parameters) {
                const double value{color_source_value(binding.source, sampled)};
                const bbb::dmx::mapper_result result{mapper_.set_normalized(mapping.fixture_id, binding.parameter, value)};
                if(!result.ok) {
                    report_error(result.message.c_str());
                }
            }
        }
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

    void output_universes() {
        if(!patch_loaded_) {
            report_error("no fixture patch loaded");
            return;
        }
        if(output_mode_value_ == universe_output_mode::selected) {
            output_universe(universe_value_);
            return;
        }
        std::set<int> universe_ids;
        for(const auto &fixture : mapper_.patch().fixtures) {
            universe_ids.insert(bbb::dmx::sanitize_universe_id(fixture.universe));
        }
        for(const int universe_id : universe_ids) {
            if(output_mode_value_ == universe_output_mode::changed) {
                const auto found = previous_output_.find(universe_id);
                if(found != previous_output_.end()) {
                    const std::vector<int> changes{bbb::dmx::changed_channels(found->second, mapper_.universe(universe_id))};
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
        const bbb::dmx::dmx_universe &universe_ref = mapper_.universe(sanitized_universe);
        output.send(bbb::dmx::maxutil::universe_atoms(sanitized_universe, universe_ref));
        previous_output_[sanitized_universe] = universe_ref;
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
