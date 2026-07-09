#include "c74_min.h"

#include <bbb/dmx/build_info.hpp>

#include <bbb/dmx/fixture_json.hpp>
#include <bbb/dmx/frame_set.hpp>
#include <bbb/dmx/max_external_utils.hpp>

#include <cstdint>
#include <set>
#include <string>
#include <vector>

class bbb_dmx_fixtureview : public c74::min::object<bbb_dmx_fixtureview> {
private:
    bbb::dmx::fixture_mapper mapper_{};
    bbb::dmx::dmx_frame_set frames_{};
    std::string setup_path_value_{};
    std::string patch_path_value_{};
    int universe_value_{1};
    bool loaded_{false};
    bool setup_load_pending_{false};
    bool suppress_setup_attribute_load_{false};
    bool suppress_patch_attribute_load_{false};
    bool patch_attribute_overridden_{false};

public:
    MIN_DESCRIPTION{"Inspect current DMX frame values through fixture patch/profile metadata."};
    MIN_TAGS{"dmx, lighting, fixture, monitor, inspect"};
    MIN_AUTHOR{"2bit"};
    MIN_RELATED{"bbb.dmx.fixtureinfo, bbb.dmx.monitor, bbb.dmx.fixturemap"};

    c74::min::inlet<> input{this, "(read/universe/list/channel/fixture/param) fixture view input"};
    c74::min::outlet<> output{this, "(anything) decoded fixture values and errors"};

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
        c74::min::description{"Patch JSON path to inspect."},
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

    c74::min::attribute<int> universe{this, "universe", 1,
        c74::min::description{"Default universe for bare list and channel messages."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(args.empty() || !bbb::dmx::maxutil::finite_atom(args[0])) {
                return {universe_value_};
            }
            universe_value_ = bbb::dmx::sanitize_universe_id((int)args[0]);
            return {universe_value_};
        }}
    };

    c74::min::timer<c74::min::timer_options::defer_delivery> init_timer{this,
        MIN_FUNCTION {
            if(!setup_path_value_.empty()) {
                setup_load_pending_ = false;
                load_setup(setup_path_value_);
            }
            if(!loaded_ && !patch_path_value_.empty()) {
                load_patch(patch_path_value_);
            }
            return {};
        }
    };

    bbb_dmx_fixtureview() {
        bbb::dmx::report_external_build_info(cout, "bbb.dmx.fixtureview");
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

    c74::min::message<> readsetup_message{this, "readsetup", "readsetup setup_json_path",
        MIN_FUNCTION {
            if(args.empty()) {
                send_error("readsetup requires setup JSON path");
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

    c74::min::message<> reload_message{this, "reload", "Reload patch JSON file.",
        MIN_FUNCTION {
            if(patch_path_value_.empty()) {
                send_error("reload requires a previously loaded patch path");
                return {};
            }
            load_patch(patch_path_value_);
            return {};
        }
    };

    c74::min::message<> list_message{this, "list", "512 values for default universe.",
        MIN_FUNCTION {
            set_universe_from_atoms(universe_value_, args, 0);
            return {};
        }
    };

    c74::min::message<> universe_message{this, "universe", "universe id value1 ... value512",
        MIN_FUNCTION {
            if(args.size() < 513 || !bbb::dmx::maxutil::finite_atom(args[0])) {
                send_error("universe requires id and 512 values");
                return {};
            }
            set_universe_from_atoms((int)args[0], args, 1);
            return {};
        }
    };

    c74::min::message<> channel_message{this, "channel", "channel address value OR channel universe address value",
        MIN_FUNCTION {
            if(args.size() == 2 && bbb::dmx::maxutil::finite_atoms(args, 0, 2)) {
                set_channel(universe_value_, (int)args[0], (int)args[1]);
                return {};
            }
            if(args.size() >= 3 && bbb::dmx::maxutil::finite_atoms(args, 0, 3)) {
                set_channel((int)args[0], (int)args[1], (int)args[2]);
                return {};
            }
            send_error("channel requires address value or universe address value");
            return {};
        }
    };

    c74::min::message<> channels_message{this, "channels", "channels universe address value ...",
        MIN_FUNCTION {
            if(args.size() < 3 || ((args.size() - 1) % 2) != 0 || !bbb::dmx::maxutil::finite_atom(args[0])) {
                send_error("channels requires universe and address/value pairs");
                return {};
            }
            const int universe_id{bbb::dmx::sanitize_universe_id((int)args[0])};
            for(std::size_t index = 1; index < args.size(); index += 2) {
                if(!bbb::dmx::maxutil::finite_atoms(args, index, 2)) {
                    send_error("channels pair must be numeric");
                    return {};
                }
                set_channel(universe_id, (int)args[index], (int)args[index + 1]);
            }
            return {};
        }
    };

    c74::min::message<> bang_message{this, "bang", "Output patch/frame summary.",
        MIN_FUNCTION {
            if(!ensure_loaded()) {
                return {};
            }
            output_summary();
            return {};
        }
    };

    c74::min::message<> listfixtures_message{this, "listfixtures", "Output one fixture message per fixture.",
        MIN_FUNCTION {
            if(!ensure_loaded()) {
                return {};
            }
            for(const auto &fixture : mapper_.patch().fixtures) {
                output_fixture(fixture.id);
            }
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

    c74::min::message<> listparams_message{this, "listparams", "listparams fixture_id",
        MIN_FUNCTION {
            if(args.empty()) {
                send_error("listparams requires fixture_id");
                return {};
            }
            if(!ensure_loaded()) {
                return {};
            }
            const std::string fixture_id{bbb::dmx::maxutil::symbol_arg(args[0])};
            const bbb::dmx::fixture_mode *mode{mode_for_fixture(fixture_id)};
            if(!mode) {
                return {};
            }
            for(const auto &parameter : mode->parameters) {
                output_parameter(fixture_id, parameter.key);
            }
            return {};
        }
    };

    c74::min::message<> param_message{this, "param", "param fixture_id parameter_key",
        MIN_FUNCTION {
            if(args.size() < 2) {
                send_error("param requires fixture_id parameter_key");
                return {};
            }
            if(!ensure_loaded()) {
                return {};
            }
            output_parameter(bbb::dmx::maxutil::symbol_arg(args[0]), bbb::dmx::maxutil::symbol_arg(args[1]));
            return {};
        }
    };

private:
    void load_setup(const std::string &path) {
        bbb::dmx::maxutil::setup_load_result setup_result{};
        const bbb::dmx::mapper_result result{bbb::dmx::maxutil::read_common_setup_values(this->maxobj(), path, setup_result)};
        if(!result.ok) {
            send_error(result.message.c_str());
            return;
        }
        if(setup_result.values.patch.has_value() && !patch_attribute_overridden_) {
            const std::string resolved_path{bbb::dmx::maxutil::setup_relative_path(setup_result.base_directory, setup_result.values.patch.value())};
            bbb::dmx::maxutil::apply_setup_symbol_path(patch, suppress_patch_attribute_load_, patch_path_value_, resolved_path);
            load_patch(patch_path_value_);
        }
        bbb::dmx::maxutil::send_status(output, "status", "setup_loaded");
    }

    void set_universe_from_atoms(int universe_id, const c74::min::atoms &args, std::size_t start) {
        if(args.size() < start + (std::size_t)bbb::dmx::universe_channel_count || !bbb::dmx::maxutil::finite_atoms(args, start, bbb::dmx::universe_channel_count)) {
            send_error("universe input requires 512 numeric values");
            return;
        }
        const bbb::dmx::write_result result{frames_.set_universe(universe_id, bbb::dmx::maxutil::values_from_atoms(args, start))};
        if(!result.ok) {
            send_error(result.message);
        }
    }

    void set_channel(int universe_id, int address, int value) {
        const bbb::dmx::write_result result{frames_.set_channel(universe_id, address, value)};
        if(!result.ok) {
            send_error(result.message);
        }
    }

    void load_patch(const std::string &path) {
        bbb::dmx::fixture_mapper loaded_mapper{};
        const bbb::dmx::mapper_result result{bbb::dmx::load_fixture_mapper_from_patch_file(bbb::dmx::maxutil::resolve_file_path(this->maxobj(), path), loaded_mapper)};
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
        std::set<int> patch_universes{};
        for(const auto &fixture : mapper_.patch().fixtures) {
            patch_universes.insert(fixture.universe);
        }
        c74::min::atoms atoms;
        atoms.push_back(c74::min::symbol("summary"));
        atoms.push_back(c74::min::symbol("fixtures"));
        atoms.push_back((int)mapper_.patch().fixtures.size());
        atoms.push_back(c74::min::symbol("patch_universes"));
        for(const int universe_id : patch_universes) {
            atoms.push_back(universe_id);
        }
        atoms.push_back(c74::min::symbol("frame_universes"));
        for(const int universe_id : frames_.universe_ids()) {
            atoms.push_back(universe_id);
        }
        output.send(atoms);
    }

    void output_fixture(const std::string &fixture_id) {
        const bbb::dmx::fixture_instance *fixture{find_fixture(fixture_id)};
        if(!fixture) {
            send_error(("unknown fixture: " + fixture_id).c_str());
            return;
        }
        const bbb::dmx::fixture_mode *mode{mode_for_fixture(fixture_id)};
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
        output.send(atoms);
    }

    void output_parameter(const std::string &fixture_id, const std::string &parameter_key) {
        const bbb::dmx::fixture_instance *fixture{find_fixture(fixture_id)};
        if(!fixture) {
            send_error(("unknown fixture: " + fixture_id).c_str());
            return;
        }
        const bbb::dmx::fixture_mode *mode{mode_for_fixture(fixture_id)};
        if(!mode) {
            return;
        }
        const bbb::dmx::fixture_parameter *parameter{mode->find_parameter(parameter_key)};
        if(!parameter) {
            send_error(("unknown parameter: " + fixture_id + ":" + parameter_key).c_str());
            return;
        }
        std::vector<int> addresses{};
        std::vector<int> bytes{};
        for(const auto &channel_key : parameter->channels) {
            const bbb::dmx::fixture_channel *channel{mode->find_channel(channel_key)};
            if(!channel) {
                continue;
            }
            const int address{fixture->address + channel->offset - 1};
            addresses.push_back(address);
            bytes.push_back(frames_.universe(fixture->universe).channel(address));
        }
        const std::uint32_t raw_value{raw_parameter_value(*parameter, bytes)};
        const double normalized{normalized_parameter_value(*parameter, raw_value)};
        c74::min::atoms atoms;
        atoms.push_back(c74::min::symbol("param"));
        atoms.push_back(c74::min::symbol(fixture_id.c_str()));
        atoms.push_back(c74::min::symbol(parameter->key.c_str()));
        atoms.push_back(c74::min::symbol("type"));
        atoms.push_back(c74::min::symbol(parameter_type_to_string(parameter->type)));
        atoms.push_back(c74::min::symbol("universe"));
        atoms.push_back(fixture->universe);
        atoms.push_back(c74::min::symbol("addresses"));
        for(const int address : addresses) {
            atoms.push_back(address);
        }
        atoms.push_back(c74::min::symbol("bytes"));
        for(const int value : bytes) {
            atoms.push_back(value);
        }
        atoms.push_back(c74::min::symbol("raw"));
        atoms.push_back((int)raw_value);
        atoms.push_back(c74::min::symbol("normalized"));
        atoms.push_back(normalized);
        output.send(atoms);
    }

    std::uint32_t raw_parameter_value(const bbb::dmx::fixture_parameter &parameter, const std::vector<int> &bytes) const {
        if(parameter.type == bbb::dmx::fixture_parameter_type::u24 && 3 <= bytes.size()) {
            return bbb::dmx::combine_24((std::uint8_t)bytes[0], (std::uint8_t)bytes[1], (std::uint8_t)bytes[2], parameter.order);
        }
        if(parameter.type == bbb::dmx::fixture_parameter_type::u16 && 2 <= bytes.size()) {
            return bbb::dmx::combine_16((std::uint8_t)bytes[0], (std::uint8_t)bytes[1], parameter.order);
        }
        if(!bytes.empty()) {
            return (std::uint32_t)bytes[0];
        }
        return 0;
    }

    double normalized_parameter_value(const bbb::dmx::fixture_parameter &parameter, std::uint32_t raw_value) const {
        if(parameter.type == bbb::dmx::fixture_parameter_type::u24) {
            return (double)raw_value / 16777215.0;
        }
        if(parameter.type == bbb::dmx::fixture_parameter_type::u16) {
            return (double)raw_value / 65535.0;
        }
        return (double)raw_value / 255.0;
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
        cerr << "bbb.dmx.fixtureview: " << message << c74::min::endl;
        bbb::dmx::maxutil::send_status(output, "error", message);
    }
};

MIN_EXTERNAL(bbb_dmx_fixtureview);
