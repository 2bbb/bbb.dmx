#include "c74_min.h"
#include "c74_jitter.h"

#include <bbb/dmx/fixture_json.hpp>
#include <bbb/dmx/frame_set.hpp>
#include <bbb/dmx/max_external_utils.hpp>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace {

enum class sample_mode {
    point,
    average,
};

enum class color_source_kind {
    red,
    green,
    blue,
    alpha,
    luma,
    max_rgb,
    constant,
};

enum class plane_order_kind {
    rgba,
    argb,
    bgra,
    gray,
};

enum class universe_output_mode {
    all,
    selected,
    changed,
};

enum class matrix_value_kind {
    uint8,
    float32,
};

struct color_value {
public:
    double red{0.0};
    double green{0.0};
    double blue{0.0};
    double alpha{1.0};
};

struct color_source {
public:
    color_source_kind kind{color_source_kind::red};
    double constant_value{0.0};
};

struct parameter_binding {
public:
    std::string parameter{};
    color_source source{};
};

struct sample_region {
public:
    sample_mode mode{sample_mode::point};
    double x{0.5};
    double y{0.5};
    double width{0.0};
    double height{0.0};
};

struct fixture_mapping {
public:
    std::string fixture_id{};
    sample_region sample{};
    std::vector<parameter_binding> parameters{};
};

struct matrix_map_config {
public:
    std::vector<fixture_mapping> fixtures{};
};

struct matrix_read_view {
public:
    const char *data{nullptr};
    long width{0};
    long height{0};
    long plane_count{0};
    long stride_x{0};
    long stride_y{0};
    plane_order_kind plane_order{plane_order_kind::rgba};
    matrix_value_kind value_kind{matrix_value_kind::uint8};
};

double clamp_normalized(double value) {
    if(value < 0.0) {
        return 0.0;
    }
    if(1.0 < value) {
        return 1.0;
    }
    return value;
}

double constant_source_to_normalized(double value) {
    if(1.0 < value) {
        return clamp_normalized(value / 255.0);
    }
    return clamp_normalized(value);
}

bool parse_sample_mode(const std::string &text, sample_mode &mode) {
    if(text == "point") {
        mode = sample_mode::point;
        return true;
    }
    if(text == "average") {
        mode = sample_mode::average;
        return true;
    }
    return false;
}

bool parse_plane_order(const std::string &text, plane_order_kind &order) {
    if(text == "rgba") {
        order = plane_order_kind::rgba;
        return true;
    }
    if(text == "argb") {
        order = plane_order_kind::argb;
        return true;
    }
    if(text == "bgra") {
        order = plane_order_kind::bgra;
        return true;
    }
    if(text == "gray") {
        order = plane_order_kind::gray;
        return true;
    }
    return false;
}

const char *plane_order_to_string(plane_order_kind order) {
    switch(order) {
        case plane_order_kind::argb: return "argb";
        case plane_order_kind::bgra: return "bgra";
        case plane_order_kind::gray: return "gray";
        case plane_order_kind::rgba:
        default: return "rgba";
    }
}

bool parse_universe_output_mode(const std::string &text, universe_output_mode &mode) {
    if(text == "all") {
        mode = universe_output_mode::all;
        return true;
    }
    if(text == "selected") {
        mode = universe_output_mode::selected;
        return true;
    }
    if(text == "changed") {
        mode = universe_output_mode::changed;
        return true;
    }
    return false;
}

const char *universe_output_mode_to_string(universe_output_mode mode) {
    switch(mode) {
        case universe_output_mode::selected: return "selected";
        case universe_output_mode::changed: return "changed";
        case universe_output_mode::all:
        default: return "all";
    }
}

bool parse_color_source(const std::string &text, color_source &source) {
    if(text == "r" || text == "red") {
        source.kind = color_source_kind::red;
        return true;
    }
    if(text == "g" || text == "green") {
        source.kind = color_source_kind::green;
        return true;
    }
    if(text == "b" || text == "blue") {
        source.kind = color_source_kind::blue;
        return true;
    }
    if(text == "a" || text == "alpha") {
        source.kind = color_source_kind::alpha;
        return true;
    }
    if(text == "luma") {
        source.kind = color_source_kind::luma;
        return true;
    }
    if(text == "maxrgb") {
        source.kind = color_source_kind::max_rgb;
        return true;
    }
    const std::string prefix{"constant:"};
    if(text.rfind(prefix, 0) == 0) {
        source.kind = color_source_kind::constant;
        source.constant_value = constant_source_to_normalized(std::atof(text.substr(prefix.size()).c_str()));
        return true;
    }
    return false;
}

double color_source_value(const color_source &source, const color_value &color) {
    switch(source.kind) {
        case color_source_kind::green: return color.green;
        case color_source_kind::blue: return color.blue;
        case color_source_kind::alpha: return color.alpha;
        case color_source_kind::luma: return clamp_normalized(0.2126 * color.red + 0.7152 * color.green + 0.0722 * color.blue);
        case color_source_kind::max_rgb: return std::max(color.red, std::max(color.green, color.blue));
        case color_source_kind::constant: return source.constant_value;
        case color_source_kind::red:
        default: return color.red;
    }
}

long clamp_long(long value, long minimum, long maximum) {
    return std::max(minimum, std::min(maximum, value));
}

int plane_index_for(plane_order_kind order, char component, long plane_count) {
    if(order == plane_order_kind::gray) {
        return component == 'a' ? -1 : 0;
    }
    int index{0};
    switch(order) {
        case plane_order_kind::argb:
            if(component == 'a') { index = 0; }
            if(component == 'r') { index = 1; }
            if(component == 'g') { index = 2; }
            if(component == 'b') { index = 3; }
            break;
        case plane_order_kind::bgra:
            if(component == 'b') { index = 0; }
            if(component == 'g') { index = 1; }
            if(component == 'r') { index = 2; }
            if(component == 'a') { index = 3; }
            break;
        case plane_order_kind::rgba:
        default:
            if(component == 'r') { index = 0; }
            if(component == 'g') { index = 1; }
            if(component == 'b') { index = 2; }
            if(component == 'a') { index = 3; }
            break;
    }
    if(index < 0 || plane_count <= index) {
        return component == 'a' ? -1 : 0;
    }
    return index;
}

double pixel_component(const matrix_read_view &view, long x, long y, char component) {
    if(!view.data || view.width <= 0 || view.height <= 0) {
        return component == 'a' ? 1.0 : 0.0;
    }
    const int plane_index{plane_index_for(view.plane_order, component, view.plane_count)};
    if(plane_index < 0) {
        return 1.0;
    }
    x = clamp_long(x, 0, view.width - 1);
    y = clamp_long(y, 0, view.height - 1);
    const char *pixel = view.data + y * view.stride_y + x * view.stride_x;
    if(view.value_kind == matrix_value_kind::float32) {
        const float *values = (const float *)pixel;
        return clamp_normalized((double)values[plane_index]);
    }
    const unsigned char *values = (const unsigned char *)pixel;
    return (double)values[plane_index] / 255.0;
}

color_value sample_point(const matrix_read_view &view, double normalized_x, double normalized_y) {
    const long x{clamp_long((long)std::llround(clamp_normalized(normalized_x) * (double)(std::max<long>(1, view.width) - 1)), 0, std::max<long>(0, view.width - 1))};
    const long y{clamp_long((long)std::llround(clamp_normalized(normalized_y) * (double)(std::max<long>(1, view.height) - 1)), 0, std::max<long>(0, view.height - 1))};
    return color_value{
        pixel_component(view, x, y, 'r'),
        pixel_component(view, x, y, 'g'),
        pixel_component(view, x, y, 'b'),
        pixel_component(view, x, y, 'a'),
    };
}

color_value sample_average(const matrix_read_view &view, const sample_region &region) {
    const double half_width{std::max(0.0, region.width) * 0.5};
    const double half_height{std::max(0.0, region.height) * 0.5};
    const double left{clamp_normalized(region.x - half_width)};
    const double right{clamp_normalized(region.x + half_width)};
    const double top{clamp_normalized(region.y - half_height)};
    const double bottom{clamp_normalized(region.y + half_height)};
    const long x0{clamp_long((long)std::floor(left * (double)(view.width - 1)), 0, view.width - 1)};
    const long x1{clamp_long((long)std::ceil(right * (double)(view.width - 1)), 0, view.width - 1)};
    const long y0{clamp_long((long)std::floor(top * (double)(view.height - 1)), 0, view.height - 1)};
    const long y1{clamp_long((long)std::ceil(bottom * (double)(view.height - 1)), 0, view.height - 1)};
    double red{0.0};
    double green{0.0};
    double blue{0.0};
    double alpha{0.0};
    long long count{0};
    for(long y = y0; y <= y1; y++) {
        for(long x = x0; x <= x1; x++) {
            red += pixel_component(view, x, y, 'r');
            green += pixel_component(view, x, y, 'g');
            blue += pixel_component(view, x, y, 'b');
            alpha += pixel_component(view, x, y, 'a');
            count++;
        }
    }
    if(count <= 0) {
        return sample_point(view, region.x, region.y);
    }
    return color_value{
        red / (double)count,
        green / (double)count,
        blue / (double)count,
        alpha / (double)count,
    };
}

color_value sample_region_color(const matrix_read_view &view, const sample_region &region) {
    if(region.mode == sample_mode::average && 0.0 < region.width && 0.0 < region.height) {
        return sample_average(view, region);
    }
    return sample_point(view, region.x, region.y);
}

bbb::dmx::mapper_result parse_sample_region(const bbb::dmx::json_value &object, sample_region &sample) {
    if(object.type != bbb::dmx::json_type::object) {
        return bbb::dmx::mapper_result::failure("sample must be object");
    }
    std::string error{};
    std::string mode_text{"point"};
    bbb::dmx::json_string(object, "mode", mode_text, false, error);
    if(!parse_sample_mode(mode_text, sample.mode)) {
        return bbb::dmx::mapper_result::failure("unknown sample mode: " + mode_text);
    }
    bbb::dmx::json_double(object, "x", sample.x, true, error);
    if(!error.empty()) {
        return bbb::dmx::mapper_result::failure(error);
    }
    bbb::dmx::json_double(object, "y", sample.y, true, error);
    if(!error.empty()) {
        return bbb::dmx::mapper_result::failure(error);
    }
    bbb::dmx::json_double(object, "w", sample.width, false, error);
    bbb::dmx::json_double(object, "h", sample.height, false, error);
    sample.x = clamp_normalized(sample.x);
    sample.y = clamp_normalized(sample.y);
    sample.width = std::max(0.0, std::min(1.0, sample.width));
    sample.height = std::max(0.0, std::min(1.0, sample.height));
    return bbb::dmx::mapper_result::success();
}

bbb::dmx::mapper_result parse_parameter_bindings(const bbb::dmx::json_value &object, std::vector<parameter_binding> &parameters) {
    if(object.type != bbb::dmx::json_type::object) {
        return bbb::dmx::mapper_result::failure("params must be object");
    }
    parameters.clear();
    for(const auto &entry : object.object_value) {
        parameter_binding binding{};
        binding.parameter = entry.first;
        if(entry.second.type == bbb::dmx::json_type::string) {
            if(!parse_color_source(entry.second.string_value, binding.source)) {
                return bbb::dmx::mapper_result::failure("unknown color source: " + entry.second.string_value);
            }
        } else if(entry.second.type == bbb::dmx::json_type::number) {
            binding.source.kind = color_source_kind::constant;
            binding.source.constant_value = constant_source_to_normalized(entry.second.number_value);
        } else {
            return bbb::dmx::mapper_result::failure("param source must be string or number: " + entry.first);
        }
        parameters.push_back(binding);
    }
    return bbb::dmx::mapper_result::success();
}

bbb::dmx::mapper_result parse_fixture_mapping(const bbb::dmx::json_value &object, fixture_mapping &mapping) {
    if(object.type != bbb::dmx::json_type::object) {
        return bbb::dmx::mapper_result::failure("fixture mapping must be object");
    }
    std::string error{};
    if(!bbb::dmx::json_string(object, "id", mapping.fixture_id, true, error)) {
        return bbb::dmx::mapper_result::failure(error);
    }
    const bbb::dmx::json_value *sample{object.find("sample")};
    if(!sample) {
        return bbb::dmx::mapper_result::failure("fixture mapping requires sample: " + mapping.fixture_id);
    }
    bbb::dmx::mapper_result result{parse_sample_region(*sample, mapping.sample)};
    if(!result.ok) {
        return result;
    }
    const bbb::dmx::json_value *params{object.find("params")};
    if(!params) {
        return bbb::dmx::mapper_result::failure("fixture mapping requires params: " + mapping.fixture_id);
    }
    return parse_parameter_bindings(*params, mapping.parameters);
}

bbb::dmx::mapper_result expand_grid(const bbb::dmx::json_value &object, matrix_map_config &config) {
    if(object.type != bbb::dmx::json_type::object) {
        return bbb::dmx::mapper_result::failure("grid must be object");
    }
    std::string error{};
    std::string fixture_pattern{};
    if(!bbb::dmx::json_string(object, "fixture_pattern", fixture_pattern, true, error)) {
        return bbb::dmx::mapper_result::failure(error);
    }
    int cols{0};
    int rows{0};
    int start_index{1};
    if(!bbb::dmx::json_int(object, "cols", cols, true, error)) {
        return bbb::dmx::mapper_result::failure(error);
    }
    if(!bbb::dmx::json_int(object, "rows", rows, true, error)) {
        return bbb::dmx::mapper_result::failure(error);
    }
    bbb::dmx::json_int(object, "start_index", start_index, false, error);
    std::string mode_text{"average"};
    bbb::dmx::json_string(object, "mode", mode_text, false, error);
    sample_mode mode{sample_mode::average};
    if(!parse_sample_mode(mode_text, mode)) {
        return bbb::dmx::mapper_result::failure("unknown grid sample mode: " + mode_text);
    }
    const bbb::dmx::json_value *params{object.find("params")};
    if(!params) {
        return bbb::dmx::mapper_result::failure("grid requires params");
    }
    std::vector<parameter_binding> bindings{};
    bbb::dmx::mapper_result result{parse_parameter_bindings(*params, bindings)};
    if(!result.ok) {
        return result;
    }
    if(cols <= 0 || rows <= 0) {
        return bbb::dmx::mapper_result::failure("grid cols and rows must be positive");
    }
    const double cell_width{1.0 / (double)cols};
    const double cell_height{1.0 / (double)rows};
    for(int row = 0; row < rows; row++) {
        for(int col = 0; col < cols; col++) {
            const int fixture_index{start_index + row * cols + col};
            char fixture_id_buffer[512];
            std::snprintf(fixture_id_buffer, sizeof(fixture_id_buffer), fixture_pattern.c_str(), fixture_index);
            fixture_mapping mapping{};
            mapping.fixture_id = fixture_id_buffer;
            mapping.sample.mode = mode;
            mapping.sample.x = ((double)col + 0.5) * cell_width;
            mapping.sample.y = ((double)row + 0.5) * cell_height;
            mapping.sample.width = cell_width;
            mapping.sample.height = cell_height;
            mapping.parameters = bindings;
            config.fixtures.push_back(mapping);
        }
    }
    return bbb::dmx::mapper_result::success();
}

bbb::dmx::mapper_result parse_matrix_map_text(const std::string &text, matrix_map_config &config) {
    const bbb::dmx::json_parse_result parsed{bbb::dmx::parse_json_text(text)};
    if(!parsed.ok) {
        return bbb::dmx::mapper_result::failure(parsed.message);
    }
    if(parsed.value.type != bbb::dmx::json_type::object) {
        return bbb::dmx::mapper_result::failure("matrixmap root must be object");
    }
    matrix_map_config loaded{};
    const bbb::dmx::json_value *fixtures{parsed.value.find("fixtures")};
    if(fixtures) {
        if(fixtures->type != bbb::dmx::json_type::array) {
            return bbb::dmx::mapper_result::failure("fixtures must be array");
        }
        for(const auto &fixture_value : fixtures->array_value) {
            fixture_mapping mapping{};
            bbb::dmx::mapper_result result{parse_fixture_mapping(fixture_value, mapping)};
            if(!result.ok) {
                return result;
            }
            loaded.fixtures.push_back(mapping);
        }
    }
    const bbb::dmx::json_value *grid{parsed.value.find("grid")};
    if(grid) {
        bbb::dmx::mapper_result result{expand_grid(*grid, loaded)};
        if(!result.ok) {
            return result;
        }
    }
    const bbb::dmx::json_value *grids{parsed.value.find("grids")};
    if(grids) {
        if(grids->type != bbb::dmx::json_type::array) {
            return bbb::dmx::mapper_result::failure("grids must be array");
        }
        for(const auto &grid_value : grids->array_value) {
            bbb::dmx::mapper_result result{expand_grid(grid_value, loaded)};
            if(!result.ok) {
                return result;
            }
        }
    }
    if(loaded.fixtures.empty()) {
        return bbb::dmx::mapper_result::failure("matrixmap has no fixtures or grids");
    }
    config = loaded;
    return bbb::dmx::mapper_result::success();
}

} // namespace

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
        const std::string resolved_path{resolve_file_path(path)};
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
        const std::string resolved_path{resolve_file_path(path)};
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

    void report_error(const char *message) {
        cerr << "bbb.dmx.matrixmap: " << message << c74::min::endl;
        status_output.send(bbb::dmx::maxutil::status_atoms("error", message));
    }
};

MIN_EXTERNAL(bbb_dmx_matrixmap);
