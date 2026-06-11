#include "c74_min.h"

#include <bbb/dmx/fixture_json.hpp>
#include <bbb/dmx/max_external_utils.hpp>

#include <set>
#include <string>

class bbb_dmx_patchcheck : public c74::min::object<bbb_dmx_patchcheck> {
private:
    std::string patch_path_value_{};

public:
    MIN_DESCRIPTION{"Validate bbb.dmx fixture patch/profile JSON without outputting DMX."};
    MIN_TAGS{"dmx, lighting, fixture, patch, validation"};
    MIN_AUTHOR{"2bit"};
    MIN_RELATED{"bbb.dmx.fixturemap, bbb.dmx.fixtureinfo"};

    c74::min::inlet<> input{this, "(read/bang) patch validation input"};
    c74::min::outlet<> output{this, "(anything) validation result"};

    c74::min::attribute<c74::min::symbol> patch{this, "patch", "",
        c74::min::description{"Patch JSON path to validate."},
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
                validate_patch(patch_path_value_);
            }
            return {};
        }
    };

    bbb_dmx_patchcheck() {
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
            validate_patch(patch_path_value_);
            return {};
        }
    };

    c74::min::message<> bang_message{this, "bang", "Validate the current patch path.",
        MIN_FUNCTION {
            if(patch_path_value_.empty()) {
                send_error("bang requires a patch path");
                return {};
            }
            validate_patch(patch_path_value_);
            return {};
        }
    };

private:
    void validate_patch(const std::string &path) {
        const std::string resolved_path{resolve_file_path(path)};
        bbb::dmx::fixture_mapper mapper{};
        const bbb::dmx::mapper_result result{bbb::dmx::load_fixture_mapper_from_patch_file(resolved_path, mapper)};
        if(!result.ok) {
            send_error(result.message.c_str());
            return;
        }
        std::set<int> universes;
        for(const auto &fixture : mapper.patch().fixtures) {
            universes.insert(fixture.universe);
        }
        c74::min::atoms atoms;
        atoms.push_back(c74::min::symbol("ok"));
        atoms.push_back(c74::min::symbol("fixtures"));
        atoms.push_back((int)mapper.patch().fixtures.size());
        atoms.push_back(c74::min::symbol("universes"));
        for(const int universe_id : universes) {
            atoms.push_back(universe_id);
        }
        output.send(atoms);
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

    void send_error(const char *message) {
        cerr << "bbb.dmx.patchcheck: " << message << c74::min::endl;
        output.send(bbb::dmx::maxutil::status_atoms("error", message));
    }
};

MIN_EXTERNAL(bbb_dmx_patchcheck);
