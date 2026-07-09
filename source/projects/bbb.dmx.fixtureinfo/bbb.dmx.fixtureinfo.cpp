#include "c74_min.h"

#include <bbb/dmx/build_info.hpp>

#include <bbb/dmx/fixture_json.hpp>
#include <bbb/dmx/max_external_utils.hpp>
#include <bbb/dmx/setup.hpp>

#include <algorithm>
#include <cctype>
#include <set>
#include <string>
#include <vector>

class bbb_dmx_fixtureinfo : public c74::min::object<bbb_dmx_fixtureinfo> {
private:
    enum class fixture_sort_key {
        patch,
        id,
        universe,
        fixturetype
    };

    bbb::dmx::fixture_mapper mapper_{};
    std::string setup_path_value_{};
    std::string patch_path_value_{};
    bool loaded_{false};
    bool setup_load_pending_{false};
    bool patch_load_pending_{false};
    bool applying_setup_{false};
    bool suppress_setup_attribute_load_{false};
    bool suppress_patch_attribute_load_{false};
    bool patch_attribute_overridden_{false};

public:
    MIN_DESCRIPTION{"Inspect bbb.dmx fixture patch/profile metadata without outputting DMX."};
    MIN_TAGS{"dmx, lighting, fixture, patch, info"};
    MIN_AUTHOR{"2bit"};
    MIN_RELATED{"bbb.dmx.fixturemap, bbb.dmx.patchcheck"};

    c74::min::inlet<> input{this, "(readsetup/read/bang/listfixtures/fixture/listparams/modeparams/param) fixture info input"};
    c74::min::outlet<> output{this, "(anything) fixture metadata and errors"};

    c74::min::attribute<c74::min::symbol> setup{this, "setup", "",
        c74::min::description{"Optional bbb.dmx.setup.v1 JSON path. The fixtureinfo section overrides top-level setup values."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(args.empty()) {
                setup_path_value_.clear();
                return {c74::min::symbol("")};
            }
            const c74::min::symbol symbol_value{(c74::min::symbol)args[0]};
            setup_path_value_ = symbol_value.c_str();
            if(!suppress_setup_attribute_load_) {
                setup_load_pending_ = true;
            }
            return {symbol_value};
        }}
    };

    c74::min::attribute<c74::min::symbol> patch{this, "patch", "",
        c74::min::description{"Patch JSON path to inspect."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(args.empty()) {
                patch_path_value_.clear();
                return {c74::min::symbol("")};
            }
            const c74::min::symbol symbol_value{(c74::min::symbol)args[0]};
            patch_path_value_ = symbol_value.c_str();
            if(!suppress_patch_attribute_load_) {
                if(!applying_setup_) {
                    patch_attribute_overridden_ = true;
                }
                patch_load_pending_ = true;
            }
            return {symbol_value};
        }}
    };

    c74::min::timer<c74::min::timer_options::defer_delivery> init_timer{this,
        MIN_FUNCTION {
            if(setup_load_pending_ && !setup_path_value_.empty()) {
                setup_load_pending_ = false;
                load_setup(setup_path_value_);
                return {};
            }
            if(patch_load_pending_ && !patch_path_value_.empty()) {
                patch_load_pending_ = false;
                load_patch(patch_path_value_);
                return {};
            }
            if(!setup_path_value_.empty()) {
                load_setup(setup_path_value_);
                return {};
            }
            if(!patch_path_value_.empty()) {
                load_patch(patch_path_value_);
            }
            return {};
        }
    };

    bbb_dmx_fixtureinfo() {
        bbb::dmx::report_external_build_info(cout, "bbb.dmx.fixtureinfo");
        init_timer.delay(0);
    }


    c74::min::message<> readsetup_message{this, "readsetup", "readsetup setup_json_path",
        MIN_FUNCTION {
            if(args.empty()) {
                send_error("readsetup requires a setup JSON path");
                return {};
            }
            const c74::min::symbol path_symbol{(c74::min::symbol)args[0]};
            setup_path_value_ = path_symbol.c_str();
            setup = path_symbol;
            load_setup(setup_path_value_);
            return {};
        }
    };

    c74::min::message<> read_message{this, "read", "read patch_json_path",
        MIN_FUNCTION {
            if(args.empty()) {
                send_error("read requires patch JSON path");
                return {};
            }
            const c74::min::symbol path_symbol{(c74::min::symbol)args[0]};
            patch_path_value_ = path_symbol.c_str();
            patch = path_symbol;
            load_patch(patch_path_value_);
            return {};
        }
    };

    c74::min::message<> bang_message{this, "bang", "Output patch summary.",
        MIN_FUNCTION {
            if(!ensure_loaded()) {
                return {};
            }
            output_summary();
            return {};
        }
    };

    c74::min::message<> listfixtures_message{this, "listfixtures", "listfixtures [id|universe|fixturetype]",
        MIN_FUNCTION {
            if(!ensure_loaded()) {
                return {};
            }
            fixture_sort_key sort_key{fixture_sort_key::patch};
            if(!parse_fixture_sort_key(args, sort_key)) {
                return {};
            }
            output_fixtures(sort_key);
            return {};
        }
    };

    c74::min::message<> listfixture_message{this, "listfixture", "Alias for listfixtures [id|universe|fixturetype].",
        MIN_FUNCTION {
            if(!ensure_loaded()) {
                return {};
            }
            fixture_sort_key sort_key{fixture_sort_key::patch};
            if(!parse_fixture_sort_key(args, sort_key)) {
                return {};
            }
            output_fixtures(sort_key);
            return {};
        }
    };

    c74::min::message<> dump_message{this, "dump", "Output summary and every fixture.",
        MIN_FUNCTION {
            if(!ensure_loaded()) {
                return {};
            }
            output_summary();
            output_fixtures(fixture_sort_key::patch);
            return {};
        }
    };

    c74::min::message<> fixture_message{this, "fixture", "fixture fixture_id",
        MIN_FUNCTION {
            if(args.empty()) {
                send_error("fixture requires fixture_id");
                return {};
            }
            if(!ensure_loaded()) {
                return {};
            }
            output_fixture(bbb::dmx::maxutil::symbol_arg(args[0]));
            return {};
        }
    };

    c74::min::message<> listparams_message{this, "listparams", "listparams fixture_id OR listparams profile_key mode_key",
        MIN_FUNCTION {
            if(args.empty()) {
                send_error("listparams requires fixture_id or profile_key mode_key");
                return {};
            }
            if(!ensure_loaded()) {
                return {};
            }
            if(args.size() == 1) {
                output_parameters(bbb::dmx::maxutil::symbol_arg(args[0]));
                return {};
            }
            output_mode_parameters(bbb::dmx::maxutil::symbol_arg(args[0]), bbb::dmx::maxutil::symbol_arg(args[1]));
            return {};
        }
    };

    c74::min::message<> modeparams_message{this, "modeparams", "modeparams profile_key mode_key",
        MIN_FUNCTION {
            if(args.size() < 2) {
                send_error("modeparams requires profile_key mode_key");
                return {};
            }
            if(!ensure_loaded()) {
                return {};
            }
            output_mode_parameters(bbb::dmx::maxutil::symbol_arg(args[0]), bbb::dmx::maxutil::symbol_arg(args[1]));
            return {};
        }
    };

    c74::min::message<> param_message{this, "param", "param fixture_id parameter_key OR param profile_key mode_key parameter_key",
        MIN_FUNCTION {
            if(args.size() < 2) {
                send_error("param requires fixture_id parameter_key or profile_key mode_key parameter_key");
                return {};
            }
            if(!ensure_loaded()) {
                return {};
            }
            if(args.size() == 2) {
                output_parameter(bbb::dmx::maxutil::symbol_arg(args[0]), bbb::dmx::maxutil::symbol_arg(args[1]));
                return {};
            }
            output_mode_parameter(
                bbb::dmx::maxutil::symbol_arg(args[0]),
                bbb::dmx::maxutil::symbol_arg(args[1]),
                bbb::dmx::maxutil::symbol_arg(args[2])
            );
            return {};
        }
    };

private:
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

    void apply_setup_file_path(
        c74::min::attribute<c74::min::symbol> &attribute,
        bool &suppress_attribute_load,
        std::string &storage,
        const std::string &path
    ) {
        suppress_attribute_load = true;
        storage = path;
        attribute = c74::min::symbol(path.c_str());
        suppress_attribute_load = false;
    }

    void apply_setup_values(const bbb::dmx::dmx_setup_values &values, const std::string &base_directory) {
        applying_setup_ = true;
        if(values.patch.has_value() && !patch_attribute_overridden_) {
            const std::string resolved_path{setup_relative_path(base_directory, values.patch.value())};
            apply_setup_file_path(patch, suppress_patch_attribute_load_, patch_path_value_, resolved_path);
        }
        applying_setup_ = false;
    }

    void load_setup(const std::string &path) {
        const std::string resolved_path{bbb::dmx::maxutil::resolve_file_path(this->maxobj(), path)};
        bbb::dmx::dmx_setup_document setup_document{};
        const bbb::dmx::mapper_result result{bbb::dmx::read_dmx_setup_file(resolved_path, setup_document)};
        if(!result.ok) {
            send_error(result.message.c_str());
            return;
        }
        const std::string base_directory{bbb::dmx::parent_directory(resolved_path)};
        const bbb::dmx::dmx_setup_values values{bbb::dmx::merge_setup_values(setup_document.common, setup_document.fixtureinfo)};
        apply_setup_values(values, base_directory);
        if(!patch_path_value_.empty()) {
            load_patch(patch_path_value_);
        }
    }

    void load_patch(const std::string &path) {
        const std::string resolved_path{bbb::dmx::maxutil::resolve_file_path(this->maxobj(), path)};
        bbb::dmx::fixture_mapper loaded_mapper{};
        const bbb::dmx::mapper_result result{bbb::dmx::load_fixture_mapper_from_patch_file(resolved_path, loaded_mapper)};
        if(!result.ok) {
            loaded_ = false;
            send_error(result.message.c_str());
            return;
        }
        mapper_ = loaded_mapper;
        loaded_ = true;
        output_summary();
    }

    bool ensure_loaded() {
        if(loaded_) {
            return true;
        }
        if(!patch_path_value_.empty()) {
            load_patch(patch_path_value_);
            return loaded_;
        }
        send_error("no patch loaded");
        return false;
    }

    void output_summary() {
        std::set<int> universes;
        for(const auto &fixture : mapper_.patch().fixtures) {
            universes.insert(fixture.universe);
        }
        c74::min::atoms atoms;
        atoms.push_back(c74::min::symbol("summary"));
        atoms.push_back(c74::min::symbol("fixtures"));
        atoms.push_back((int)mapper_.patch().fixtures.size());
        atoms.push_back(c74::min::symbol("universes"));
        for(const int universe_id : universes) {
            atoms.push_back(universe_id);
        }
        output.send(atoms);
    }

    void output_fixtures(fixture_sort_key sort_key) {
        std::vector<const bbb::dmx::fixture_instance *> fixtures;
        fixtures.reserve(mapper_.patch().fixtures.size());
        for(const auto &fixture : mapper_.patch().fixtures) {
            fixtures.push_back(&fixture);
        }
        sort_fixtures(fixtures, sort_key);
        for(const auto *fixture : fixtures) {
            if(fixture) {
                output_fixture_atoms(*fixture);
            }
        }
    }

    void output_fixture(const std::string &fixture_id) {
        const bbb::dmx::fixture_instance *fixture{find_fixture(fixture_id)};
        if(!fixture) {
            send_error(("unknown fixture: " + fixture_id).c_str());
            return;
        }
        output_fixture_atoms(*fixture);
    }

    void output_fixture_atoms(const bbb::dmx::fixture_instance &fixture) {
        const bbb::dmx::fixture_profile *profile{mapper_.find_profile(fixture.profile)};
        const bbb::dmx::fixture_mode *mode{profile ? profile->find_mode(fixture.mode) : nullptr};
        c74::min::atoms atoms;
        atoms.push_back(c74::min::symbol("fixture"));
        atoms.push_back(c74::min::symbol(fixture.id.c_str()));
        atoms.push_back(c74::min::symbol("universe"));
        atoms.push_back(fixture.universe);
        atoms.push_back(c74::min::symbol("address"));
        atoms.push_back(fixture.address);
        atoms.push_back(c74::min::symbol("profile"));
        atoms.push_back(c74::min::symbol(fixture.profile.c_str()));
        atoms.push_back(c74::min::symbol("mode"));
        atoms.push_back(c74::min::symbol(fixture.mode.c_str()));
        atoms.push_back(c74::min::symbol("footprint"));
        atoms.push_back(mode ? mode->footprint : 0);
        atoms.push_back(c74::min::symbol("position"));
        atoms.push_back(fixture.position.x);
        atoms.push_back(fixture.position.y);
        atoms.push_back(fixture.position.z);
        output.send(atoms);
    }

    void output_parameters(const std::string &fixture_id) {
        const bbb::dmx::fixture_mode *mode{mode_for_fixture(fixture_id)};
        if(!mode) {
            return;
        }
        for(const auto &parameter : mode->parameters) {
            output_parameter_atoms(fixture_id, parameter);
        }
    }

    void output_parameter(const std::string &fixture_id, const std::string &parameter_key) {
        const bbb::dmx::fixture_mode *mode{mode_for_fixture(fixture_id)};
        if(!mode) {
            return;
        }
        const bbb::dmx::fixture_parameter *parameter{mode->find_parameter(parameter_key)};
        if(!parameter) {
            send_error(("unknown parameter: " + fixture_id + ":" + parameter_key).c_str());
            return;
        }
        output_parameter_atoms(fixture_id, *parameter);
    }

    void output_mode_parameters(const std::string &profile_key, const std::string &mode_key) {
        const bbb::dmx::fixture_mode *mode{mode_for_profile(profile_key, mode_key)};
        if(!mode) {
            return;
        }
        for(const auto &parameter : mode->parameters) {
            output_mode_parameter_atoms(profile_key, mode_key, parameter);
        }
    }

    void output_mode_parameter(const std::string &profile_key, const std::string &mode_key, const std::string &parameter_key) {
        const bbb::dmx::fixture_mode *mode{mode_for_profile(profile_key, mode_key)};
        if(!mode) {
            return;
        }
        const bbb::dmx::fixture_parameter *parameter{mode->find_parameter(parameter_key)};
        if(!parameter) {
            send_error(("unknown parameter: " + profile_key + ":" + mode_key + ":" + parameter_key).c_str());
            return;
        }
        output_mode_parameter_atoms(profile_key, mode_key, *parameter);
    }

    void output_parameter_atoms(const std::string &fixture_id, const bbb::dmx::fixture_parameter &parameter) {
        c74::min::atoms atoms;
        atoms.push_back(c74::min::symbol("param"));
        atoms.push_back(c74::min::symbol(fixture_id.c_str()));
        atoms.push_back(c74::min::symbol(parameter.key.c_str()));
        append_parameter_detail_atoms(atoms, parameter);
        output.send(atoms);
    }

    void output_mode_parameter_atoms(const std::string &profile_key, const std::string &mode_key, const bbb::dmx::fixture_parameter &parameter) {
        c74::min::atoms atoms;
        atoms.push_back(c74::min::symbol("param"));
        atoms.push_back(c74::min::symbol(profile_key.c_str()));
        atoms.push_back(c74::min::symbol(mode_key.c_str()));
        atoms.push_back(c74::min::symbol(parameter.key.c_str()));
        append_parameter_detail_atoms(atoms, parameter);
        output.send(atoms);
    }

    void append_parameter_detail_atoms(c74::min::atoms &atoms, const bbb::dmx::fixture_parameter &parameter) {
        atoms.push_back(c74::min::symbol("type"));
        atoms.push_back(c74::min::symbol(parameter_type_to_string(parameter.type)));
        atoms.push_back(c74::min::symbol("channels"));
        for(const auto &channel : parameter.channels) {
            atoms.push_back(c74::min::symbol(channel.c_str()));
        }
        atoms.push_back(c74::min::symbol("byte_order"));
        atoms.push_back(c74::min::symbol(bbb::dmx::byte_order_to_string(parameter.order)));
        atoms.push_back(c74::min::symbol("default"));
        atoms.push_back(parameter.default_value);
        atoms.push_back(c74::min::symbol("range_degrees"));
        atoms.push_back(parameter.range_degrees);
        atoms.push_back(c74::min::symbol("ranges"));
        atoms.push_back((int)parameter.ranges.size());
        for(const auto &range : parameter.ranges) {
            atoms.push_back(c74::min::symbol("range"));
            atoms.push_back(range.from);
            atoms.push_back(range.to);
            atoms.push_back(c74::min::symbol("function"));
            atoms.push_back(c74::min::symbol(range.function.c_str()));
            atoms.push_back(c74::min::symbol("label"));
            atoms.push_back(c74::min::symbol(range.label.c_str()));
        }
    }

    const bbb::dmx::fixture_instance *find_fixture(const std::string &fixture_id) const {
        for(const auto &fixture : mapper_.patch().fixtures) {
            if(fixture.id == fixture_id) {
                return &fixture;
            }
        }
        return nullptr;
    }

    const bbb::dmx::fixture_mode *mode_for_fixture(const std::string &fixture_id) {
        const bbb::dmx::fixture_instance *fixture{find_fixture(fixture_id)};
        if(!fixture) {
            send_error(("unknown fixture: " + fixture_id).c_str());
            return nullptr;
        }
        const bbb::dmx::fixture_profile *profile{mapper_.find_profile(fixture->profile)};
        if(!profile) {
            send_error(("missing profile: " + fixture->profile).c_str());
            return nullptr;
        }
        const bbb::dmx::fixture_mode *mode{profile->find_mode(fixture->mode)};
        if(!mode) {
            send_error(("missing mode: " + fixture->mode).c_str());
            return nullptr;
        }
        return mode;
    }

    const bbb::dmx::fixture_mode *mode_for_profile(const std::string &profile_key, const std::string &mode_key) {
        const bbb::dmx::fixture_profile *profile{mapper_.find_profile(profile_key)};
        if(!profile) {
            send_error(("unknown profile: " + profile_key).c_str());
            return nullptr;
        }
        const bbb::dmx::fixture_mode *mode{profile->find_mode(mode_key)};
        if(!mode) {
            send_error(("missing mode: " + profile_key + ":" + mode_key).c_str());
            return nullptr;
        }
        return mode;
    }

    static const char *parameter_type_to_string(bbb::dmx::fixture_parameter_type type) {
        switch(type) {
            case bbb::dmx::fixture_parameter_type::u16: return "u16";
            case bbb::dmx::fixture_parameter_type::u24: return "u24";
            case bbb::dmx::fixture_parameter_type::enum_u8: return "enum_u8";
            case bbb::dmx::fixture_parameter_type::u8:
            default: return "u8";
        }
    }

    bool parse_fixture_sort_key(const c74::min::atoms &args, fixture_sort_key &sort_key) {
        if(args.empty()) {
            sort_key = fixture_sort_key::patch;
            return true;
        }
        std::size_t option_index{0};
        const std::string first_option{lowercase(bbb::dmx::maxutil::symbol_arg(args[0]))};
        if((first_option == "sort" || first_option == "by" || first_option == "@sort") && 1 < args.size()) {
            option_index = 1;
        }
        const std::string option{lowercase(bbb::dmx::maxutil::symbol_arg(args[option_index]))};
        if(option == "patch" || option == "none" || option == "original") {
            sort_key = fixture_sort_key::patch;
            return true;
        }
        if(option == "id" || option == "fixture" || option == "fixtureid") {
            sort_key = fixture_sort_key::id;
            return true;
        }
        if(option == "universe" || option == "address" || option == "dmx") {
            sort_key = fixture_sort_key::universe;
            return true;
        }
        if(option == "fixturetype" || option == "type" || option == "profile") {
            sort_key = fixture_sort_key::fixturetype;
            return true;
        }
        send_error(("unknown listfixtures sort option: " + option).c_str());
        return false;
    }

    static std::string lowercase(const std::string &value) {
        std::string result{value};
        std::transform(result.begin(), result.end(), result.begin(), [](unsigned char character) {
            return (char)std::tolower(character);
        });
        return result;
    }

    static bool decimal_string(const std::string &value) {
        if(value.empty()) {
            return false;
        }
        return std::all_of(value.begin(), value.end(), [](unsigned char character) {
            return std::isdigit(character) != 0;
        });
    }

    static bool identifier_less(const std::string &left, const std::string &right) {
        const bool left_decimal{decimal_string(left)};
        const bool right_decimal{decimal_string(right)};
        if(left_decimal && right_decimal && left.size() != right.size()) {
            return left.size() < right.size();
        }
        return left < right;
    }

    static bool fixture_id_less(const bbb::dmx::fixture_instance *left, const bbb::dmx::fixture_instance *right) {
        if(!left || !right) {
            return left < right;
        }
        return identifier_less(left->id, right->id);
    }

    static void sort_fixtures(std::vector<const bbb::dmx::fixture_instance *> &fixtures, fixture_sort_key sort_key) {
        switch(sort_key) {
            case fixture_sort_key::id:
                std::stable_sort(fixtures.begin(), fixtures.end(), [](const auto *left, const auto *right) {
                    return fixture_id_less(left, right);
                });
                break;
            case fixture_sort_key::universe:
                std::stable_sort(fixtures.begin(), fixtures.end(), [](const auto *left, const auto *right) {
                    if(!left || !right) {
                        return left < right;
                    }
                    if(left->universe != right->universe) {
                        return left->universe < right->universe;
                    }
                    if(left->address != right->address) {
                        return left->address < right->address;
                    }
                    return fixture_id_less(left, right);
                });
                break;
            case fixture_sort_key::fixturetype:
                std::stable_sort(fixtures.begin(), fixtures.end(), [](const auto *left, const auto *right) {
                    if(!left || !right) {
                        return left < right;
                    }
                    if(left->profile != right->profile) {
                        return left->profile < right->profile;
                    }
                    if(left->mode != right->mode) {
                        return left->mode < right->mode;
                    }
                    return fixture_id_less(left, right);
                });
                break;
            case fixture_sort_key::patch:
            default:
                break;
        }
    }

    void send_error(const char *message) {
        cerr << "bbb.dmx.fixtureinfo: " << message << c74::min::endl;
        bbb::dmx::maxutil::send_status(output, "error", message);
    }
};

MIN_EXTERNAL(bbb_dmx_fixtureinfo);
