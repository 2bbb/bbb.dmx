#include "c74_min.h"

#include <bbb/dmx/build_info.hpp>

#include <bbb/dmx/fixture_groups.hpp>
#include <bbb/dmx/fixture_json.hpp>
#include <bbb/dmx/max_external_utils.hpp>

#include <set>
#include <string>

class bbb_dmx_patchcheck : public c74::min::object<bbb_dmx_patchcheck> {
private:
    std::string setup_path_value_{};
    std::string patch_path_value_{};
    std::string groups_path_value_{};
    bool setup_load_pending_{false};
    bool suppress_setup_attribute_load_{false};
    bool suppress_patch_attribute_load_{false};
    bool suppress_groups_attribute_load_{false};
    bool patch_attribute_overridden_{false};
    bool groups_attribute_overridden_{false};

public:
    MIN_DESCRIPTION{"Validate bbb.dmx fixture patch/profile JSON without outputting DMX."};
    MIN_TAGS{"dmx, lighting, fixture, patch, validation"};
    MIN_AUTHOR{"2bit"};
    MIN_RELATED{"bbb.dmx.fixturemap, bbb.dmx.fixtureinfo"};

    c74::min::inlet<> input{this, "(read/bang) patch validation input"};
    c74::min::outlet<> output{this, "(anything) validation result"};

    c74::min::attribute<c74::min::symbol> setup{this, "setup", "",
        c74::min::description{"Optional bbb.dmx.setup.v1 JSON path. Uses top-level patch and groups values unless explicit object attributes override them."},
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
        c74::min::description{"Patch JSON path to validate."},
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

    c74::min::attribute<c74::min::symbol> groups{this, "groups", "",
        c74::min::description{"Optional bbb.dmx groups JSON path to validate against the patch."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(args.empty()) {
                groups_path_value_.clear();
                return {c74::min::symbol("")};
            }
            const c74::min::symbol symbol_value{(c74::min::symbol)args[0]};
            groups_path_value_ = symbol_value.c_str();
            if(bbb::dmx::maxutil::should_mark_explicit_symbol_override(args, false, suppress_groups_attribute_load_)) {
                groups_attribute_overridden_ = true;
            }
            return {symbol_value};
        }}
    };

    c74::min::timer<c74::min::timer_options::defer_delivery> init_timer{this,
        MIN_FUNCTION {
            if(!setup_path_value_.empty()) {
                setup_load_pending_ = false;
                load_setup(setup_path_value_);
                return {};
            }
            if(!patch_path_value_.empty()) {
                validate_patch(patch_path_value_);
            }
            return {};
        }
    };

    bbb_dmx_patchcheck() {
        bbb::dmx::report_external_build_info(cout, "bbb.dmx.patchcheck");
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

    c74::min::message<> readgroups_message{this, "readgroups", "readgroups groups_json_path",
        MIN_FUNCTION {
            if(args.empty()) {
                send_error("readgroups requires groups JSON path");
                return {};
            }
            const c74::min::symbol path_symbol{(c74::min::symbol)args[0]};
            groups_path_value_ = path_symbol.c_str();
            groups = path_symbol;
            if(patch_path_value_.empty()) {
                send_error("readgroups requires a patch path before validation");
                return {};
            }
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
        }
        if(setup_result.values.groups.has_value() && !groups_attribute_overridden_) {
            const std::string resolved_path{bbb::dmx::maxutil::setup_relative_path(setup_result.base_directory, setup_result.values.groups.value())};
            bbb::dmx::maxutil::apply_setup_symbol_path(groups, suppress_groups_attribute_load_, groups_path_value_, resolved_path);
        }
        bbb::dmx::maxutil::send_status(output, "status", "setup_loaded");
        if(!patch_path_value_.empty()) {
            validate_patch(patch_path_value_);
        }
    }

    void validate_patch(const std::string &path) {
        const std::string resolved_path{bbb::dmx::maxutil::resolve_file_path(this->maxobj(), path)};
        bbb::dmx::fixture_mapper mapper{};
        const bbb::dmx::mapper_result result{bbb::dmx::load_fixture_mapper_from_patch_file(resolved_path, mapper)};
        if(!result.ok) {
            send_error(result.message.c_str());
            return;
        }
        bbb::dmx::fixture_group_set group_set{};
        bool groups_loaded{false};
        if(!groups_path_value_.empty()) {
            const std::string resolved_groups_path{bbb::dmx::maxutil::resolve_file_path(this->maxobj(), groups_path_value_)};
            const bbb::dmx::mapper_result groups_read_result{bbb::dmx::read_fixture_groups_file(resolved_groups_path, group_set)};
            if(!groups_read_result.ok) {
                send_error(groups_read_result.message.c_str());
                return;
            }
            const bbb::dmx::mapper_result groups_validate_result{bbb::dmx::validate_fixture_groups_for_patch(group_set, mapper.patch())};
            if(!groups_validate_result.ok) {
                send_error(groups_validate_result.message.c_str());
                return;
            }
            groups_loaded = true;
        }
        std::set<int> universes;
        for(const auto &fixture : mapper.patch().fixtures) {
            universes.insert(fixture.universe);
        }
        c74::min::atoms atoms;
        atoms.push_back(c74::min::symbol("ok"));
        atoms.push_back(c74::min::symbol("fixtures"));
        atoms.push_back((int)mapper.patch().fixtures.size());
        atoms.push_back(c74::min::symbol("groups"));
        atoms.push_back(groups_loaded ? (int)group_set.groups.size() : 0);
        atoms.push_back(c74::min::symbol("universes"));
        for(const int universe_id : universes) {
            atoms.push_back(universe_id);
        }
        output.send(atoms);
    }


    void send_error(const char *message) {
        cerr << "bbb.dmx.patchcheck: " << message << c74::min::endl;
        bbb::dmx::maxutil::send_status(output, "error", message);
    }
};

MIN_EXTERNAL(bbb_dmx_patchcheck);
