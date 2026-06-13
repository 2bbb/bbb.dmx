#include "c74_min.h"

#include <bbb/dmx/fixture_json.hpp>
#include <bbb/dmx/max_external_utils.hpp>

#include <set>
#include <string>

class bbb_dmx_fixtureinfo : public c74::min::object<bbb_dmx_fixtureinfo> {
private:
    bbb::dmx::fixture_mapper mapper_{};
    std::string patch_path_value_{};
    bool loaded_{false};

public:
    MIN_DESCRIPTION{"Inspect bbb.dmx fixture patch/profile metadata without outputting DMX."};
    MIN_TAGS{"dmx, lighting, fixture, patch, info"};
    MIN_AUTHOR{"2bit"};
    MIN_RELATED{"bbb.dmx.fixturemap, bbb.dmx.patchcheck"};

    c74::min::inlet<> input{this, "(read/bang/listfixtures/fixture/listparams/modeparams/param) fixture info input"};
    c74::min::outlet<> output{this, "(anything) fixture metadata and errors"};

    c74::min::attribute<c74::min::symbol> patch{this, "patch", "",
        c74::min::description{"Patch JSON path to inspect."},
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

    c74::min::timer<c74::min::timer_options::defer_delivery> init_timer{this,
        MIN_FUNCTION {
            if(!patch_path_value_.empty()) {
                load_patch(patch_path_value_);
            }
            return {};
        }
    };

    bbb_dmx_fixtureinfo() {
        init_timer.delay(0);
    }

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

    c74::min::message<> listfixtures_message{this, "listfixtures", "Output one fixture message per patched fixture.",
        MIN_FUNCTION {
            if(!ensure_loaded()) {
                return {};
            }
            output_fixtures();
            return {};
        }
    };

    c74::min::message<> dump_message{this, "dump", "Output summary and every fixture.",
        MIN_FUNCTION {
            if(!ensure_loaded()) {
                return {};
            }
            output_summary();
            output_fixtures();
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
    void load_patch(const std::string &path) {
        const std::string resolved_path{bbb::dmx::maxutil::resolve_file_path(path)};
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

    void output_fixtures() {
        for(const auto &fixture : mapper_.patch().fixtures) {
            output_fixture(fixture.id);
        }
    }

    void output_fixture(const std::string &fixture_id) {
        const bbb::dmx::fixture_instance *fixture{find_fixture(fixture_id)};
        if(!fixture) {
            send_error(("unknown fixture: " + fixture_id).c_str());
            return;
        }
        const bbb::dmx::fixture_profile *profile{mapper_.find_profile(fixture->profile)};
        const bbb::dmx::fixture_mode *mode{profile ? profile->find_mode(fixture->mode) : nullptr};
        c74::min::atoms atoms;
        atoms.push_back(c74::min::symbol("fixture"));
        atoms.push_back(c74::min::symbol(fixture->id.c_str()));
        atoms.push_back(c74::min::symbol("universe"));
        atoms.push_back(fixture->universe);
        atoms.push_back(c74::min::symbol("address"));
        atoms.push_back(fixture->address);
        atoms.push_back(c74::min::symbol("profile"));
        atoms.push_back(c74::min::symbol(fixture->profile.c_str()));
        atoms.push_back(c74::min::symbol("mode"));
        atoms.push_back(c74::min::symbol(fixture->mode.c_str()));
        atoms.push_back(c74::min::symbol("footprint"));
        atoms.push_back(mode ? mode->footprint : 0);
        atoms.push_back(c74::min::symbol("position"));
        atoms.push_back(fixture->position.x);
        atoms.push_back(fixture->position.y);
        atoms.push_back(fixture->position.z);
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


    void send_error(const char *message) {
        cerr << "bbb.dmx.fixtureinfo: " << message << c74::min::endl;
        bbb::dmx::maxutil::send_status(output, "error", message);
    }
};

MIN_EXTERNAL(bbb_dmx_fixtureinfo);
