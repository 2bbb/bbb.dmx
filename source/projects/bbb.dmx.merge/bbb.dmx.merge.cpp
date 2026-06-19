#include "c74_min.h"

#include <bbb/dmx/build_info.hpp>

#include <bbb/dmx/frame_set.hpp>
#include <bbb/dmx/fixture_json.hpp>
#include <bbb/dmx/max_external_utils.hpp>
#include <bbb/dmx/semantic_merge.hpp>
#include <bbb/dmx/semantic_overrides.hpp>

#include <array>
#include <cstdint>
#include <limits>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace {

enum class merge_mode {
    priority,
    htp,
    ltp,
};

struct layer_universe {
public:
    std::array<std::uint8_t, bbb::dmx::universe_channel_count> values{};
    std::array<bool, bbb::dmx::universe_channel_count> valid{};
    std::array<std::uint64_t, bbb::dmx::universe_channel_count> sequence{};
};

struct merge_layer {
public:
    std::string name{};
    int priority{0};
    std::map<int, layer_universe> universes{};
};

} // namespace

class bbb_dmx_merge : public c74::min::object<bbb_dmx_merge> {
private:
    std::map<std::string, merge_layer> layers_{};
    std::uint64_t sequence_{1};
    int universe_value_{1};
    merge_mode mode_value_{merge_mode::priority};
    bbb::dmx::dmx_frame_set merged_{};
    bbb::dmx::fixture_mapper semantic_mapper_{};
    bbb::dmx::fixture_semantic_overrides semantic_overrides_{};
    bbb::dmx::semantic_merge_policy semantic_policy_{};
    std::string patch_path_value_{};
    std::string semantic_overrides_path_value_{};
    bool patch_loaded_{false};
    bool semantic_overrides_loaded_{false};
    std::map<int, std::map<int, std::size_t>> cmy_lowest_group_by_address_{};

public:
    MIN_DESCRIPTION{"Merge multiple multi-universe DMX layers using priority, HTP, or LTP rules."};
    MIN_TAGS{"dmx, lighting, merge, htp, ltp, universe"};
    MIN_AUTHOR{"2bit"};
    MIN_RELATED{"bbb.dmx.monitor, bbb.dmx.fixturemap"};

    bbb_dmx_merge() {
        bbb::dmx::report_external_build_info(cout, "bbb.dmx.merge");
    }

    c74::min::inlet<> input{this, "(layer/list/universe/channel/clear/bang) DMX layer input"};
    c74::min::outlet<> output{this, "(anything) merged universe data"};
    c74::min::outlet<> status_output{this, "(anything) status and error messages"};

    c74::min::attribute<int> universe{this, "universe", 1,
        c74::min::description{"Default universe for bare list input and bang output."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(args.empty() || !bbb::dmx::maxutil::finite_atom(args[0])) {
                return {universe_value_};
            }
            universe_value_ = bbb::dmx::sanitize_universe_id((int)args[0]);
            return {universe_value_};
        }}
    };

    c74::min::attribute<c74::min::symbol> mode{this, "mode", "priority",
        c74::min::description{"Merge mode: priority, htp, or ltp."},
        c74::min::enum_map{"priority", "htp", "ltp"},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(args.empty()) {
                return {c74::min::symbol(mode_to_string(mode_value_))};
            }
            const std::string text{bbb::dmx::maxutil::symbol_arg(args[0])};
            if(text == "htp") {
                mode_value_ = merge_mode::htp;
            } else if(text == "ltp") {
                mode_value_ = merge_mode::ltp;
            } else {
                mode_value_ = merge_mode::priority;
            }
            recompute_and_output_all();
            return {c74::min::symbol(mode_to_string(mode_value_))};
        }}
    };

    c74::min::attribute<c74::min::symbol> patch{this, "patch", "",
        c74::min::description{"Optional fixture patch JSON. Native CMY parameters, plus CMY semantic overrides when provided, are merged with parameter-aware lowest in HTP mode."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            const c74::min::symbol symbol_value = args.empty() ? c74::min::symbol("") : c74::min::symbol(bbb::dmx::maxutil::symbol_arg(args[0]).c_str());
            patch_path_value_ = symbol_value.c_str();
            if(patch_path_value_.empty()) {
                patch_loaded_ = false;
                semantic_mapper_.clear();
                rebuild_semantic_policy();
            } else {
                load_patch_file(patch_path_value_);
            }
            return {symbol_value};
        }}
    };

    c74::min::attribute<c74::min::symbol> semantic_overrides{this, "semantic_overrides", "",
        c74::min::description{"Optional semantic overrides JSON. Explicit CMY blocks are used as parameter-aware lowest merge groups in HTP mode."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            const c74::min::symbol symbol_value = args.empty() ? c74::min::symbol("") : c74::min::symbol(bbb::dmx::maxutil::symbol_arg(args[0]).c_str());
            semantic_overrides_path_value_ = symbol_value.c_str();
            if(semantic_overrides_path_value_.empty()) {
                semantic_overrides_loaded_ = false;
                semantic_overrides_ = bbb::dmx::fixture_semantic_overrides{};
                rebuild_semantic_policy();
            } else {
                load_semantic_overrides_file(semantic_overrides_path_value_);
            }
            return {symbol_value};
        }}
    };

    c74::min::message<> list_message{this, "list", "512 values for layer default and default universe.",
        MIN_FUNCTION {
            set_layer_universe("default", universe_value_, args, 0);
            return {};
        }
    };

    c74::min::message<> universe_message{this, "universe", "universe layer_name universe_id value1 ... value512",
        MIN_FUNCTION {
            if(args.size() < 514 || !bbb::dmx::maxutil::finite_atom(args[1])) {
                report_error("universe requires layer name, universe id, and 512 values");
                return {};
            }
            set_layer_universe(bbb::dmx::maxutil::symbol_arg(args[0]), bbb::dmx::sanitize_universe_id((int)args[1]), args, 2);
            return {};
        }
    };

    c74::min::message<> layer_message{this, "layer", "layer name universe_id value1 ... value512",
        MIN_FUNCTION {
            if(args.size() < 514 || !bbb::dmx::maxutil::finite_atom(args[1])) {
                report_error("layer requires name, universe id, and 512 values");
                return {};
            }
            set_layer_universe(bbb::dmx::maxutil::symbol_arg(args[0]), bbb::dmx::sanitize_universe_id((int)args[1]), args, 2);
            return {};
        }
    };

    c74::min::message<> channel_message{this, "channel", "channel layer_name universe address value",
        MIN_FUNCTION {
            if(args.size() < 4 || !bbb::dmx::maxutil::finite_atoms(args, 1, 3)) {
                report_error("channel requires layer name universe address value");
                return {};
            }
            set_layer_channel(bbb::dmx::maxutil::symbol_arg(args[0]), (int)args[1], (int)args[2], (int)args[3]);
            return {};
        }
    };

    c74::min::message<> channels_message{this, "channels", "channels layer_name universe address value ...",
        MIN_FUNCTION {
            if(args.size() < 4 || ((args.size() - 2) % 2) != 0 || !bbb::dmx::maxutil::finite_atom(args[1])) {
                report_error("channels requires layer name universe and address/value pairs");
                return {};
            }
            const std::string layer_name{bbb::dmx::maxutil::symbol_arg(args[0])};
            const int universe_id{bbb::dmx::sanitize_universe_id((int)args[1])};
            for(std::size_t index = 2; index < args.size(); index += 2) {
                if(!bbb::dmx::maxutil::finite_atoms(args, index, 2)) {
                    report_error("channels pair must be numeric");
                    return {};
                }
                if(!set_layer_channel_no_output(layer_name, universe_id, (int)args[index], (int)args[index + 1])) {
                    return {};
                }
            }
            recompute_and_output_all();
            return {};
        }
    };

    c74::min::message<> priority_message{this, "priority", "priority layer_name priority_int",
        MIN_FUNCTION {
            if(args.size() < 2 || !bbb::dmx::maxutil::finite_atom(args[1])) {
                report_error("priority requires layer name and integer priority");
                return {};
            }
            merge_layer &layer = ensure_layer(bbb::dmx::maxutil::symbol_arg(args[0]));
            layer.priority = (int)args[1];
            recompute_and_output_all();
            return {};
        }
    };

    c74::min::message<> readpatch_message{this, "readpatch", "readpatch patch_json_path",
        MIN_FUNCTION {
            if(args.empty()) {
                report_error("readpatch requires patch JSON path");
                return {};
            }
            patch_path_value_ = bbb::dmx::maxutil::symbol_arg(args[0]);
            load_patch_file(patch_path_value_);
            return {};
        }
    };

    c74::min::message<> readoverrides_message{this, "readoverrides", "readoverrides semantic_overrides_json_path",
        MIN_FUNCTION {
            if(args.empty()) {
                report_error("readoverrides requires semantic overrides JSON path");
                return {};
            }
            semantic_overrides_path_value_ = bbb::dmx::maxutil::symbol_arg(args[0]);
            load_semantic_overrides_file(semantic_overrides_path_value_);
            return {};
        }
    };

    c74::min::message<> reload_message{this, "reload", "Reload configured patch and semantic overrides files.",
        MIN_FUNCTION {
            if(!patch_path_value_.empty()) {
                load_patch_file(patch_path_value_);
            }
            if(!semantic_overrides_path_value_.empty()) {
                load_semantic_overrides_file(semantic_overrides_path_value_);
            }
            return {};
        }
    };

    c74::min::message<> clear_message{this, "clear", "clear [layer_name|all]",
        MIN_FUNCTION {
            if(args.empty() || bbb::dmx::maxutil::symbol_arg(args[0]) == "all") {
                layers_.clear();
            } else {
                layers_.erase(bbb::dmx::maxutil::symbol_arg(args[0]));
            }
            recompute_and_output_all();
            return {};
        }
    };

    c74::min::message<> bang_message{this, "bang", "Output the selected merged universe." ,
        MIN_FUNCTION {
            output.send(bbb::dmx::maxutil::universe_atoms(universe_value_, merged_.universe(universe_value_)));
            return {};
        }
    };

    c74::min::message<> bangall_message{this, "bangall", "Output all merged universes." ,
        MIN_FUNCTION {
            output_all();
            return {};
        }
    };

    c74::min::message<> dump_message{this, "dump", "Output layer names and known universes." ,
        MIN_FUNCTION {
            c74::min::atoms atoms;
            atoms.push_back(c74::min::symbol("layers"));
            for(const auto &entry : layers_) {
                atoms.push_back(c74::min::symbol(entry.first.c_str()));
            }
            atoms.push_back(c74::min::symbol("patch_loaded"));
            atoms.push_back(patch_loaded_ ? 1 : 0);
            atoms.push_back(c74::min::symbol("semantic_overrides_loaded"));
            atoms.push_back(semantic_overrides_loaded_ ? 1 : 0);
            atoms.push_back(c74::min::symbol("cmy_lowest_groups"));
            atoms.push_back((int)semantic_policy_.cmy_lowest_groups.size());
            status_output.send(atoms);
            return {};
        }
    };

private:
    static const char *mode_to_string(merge_mode mode_value) {
        if(mode_value == merge_mode::htp) {
            return "htp";
        }
        if(mode_value == merge_mode::ltp) {
            return "ltp";
        }
        return "priority";
    }

    merge_layer &ensure_layer(const std::string &name) {
        merge_layer &layer = layers_[name];
        if(layer.name.empty()) {
            layer.name = name;
            layer.priority = (int)layers_.size();
        }
        return layer;
    }

    void set_layer_universe(const std::string &layer_name, int universe_id, const c74::min::atoms &args, std::size_t start) {
        if(args.size() < start + (std::size_t)bbb::dmx::universe_channel_count || !bbb::dmx::maxutil::finite_atoms(args, start, bbb::dmx::universe_channel_count)) {
            report_error("layer universe requires 512 numeric values");
            return;
        }
        merge_layer &layer = ensure_layer(layer_name);
        layer_universe &universe = layer.universes[bbb::dmx::sanitize_universe_id(universe_id)];
        for(int channel = 0; channel < bbb::dmx::universe_channel_count; channel++) {
            universe.values[(std::size_t)channel] = (std::uint8_t)bbb::dmx::maxutil::clamp_int((int)args[start + (std::size_t)channel], 0, 255);
            universe.valid[(std::size_t)channel] = true;
            universe.sequence[(std::size_t)channel] = sequence_++;
        }
        recompute_and_output_all();
    }

    bool set_layer_channel_no_output(const std::string &layer_name, int universe_id, int address, int value) {
        if(address < 1 || bbb::dmx::universe_channel_count < address) {
            report_error("channel address outside 1..512");
            return false;
        }
        merge_layer &layer = ensure_layer(layer_name);
        layer_universe &universe = layer.universes[bbb::dmx::sanitize_universe_id(universe_id)];
        const std::size_t index{(std::size_t)(address - 1)};
        universe.values[index] = (std::uint8_t)bbb::dmx::maxutil::clamp_int(value, 0, 255);
        universe.valid[index] = true;
        universe.sequence[index] = sequence_++;
        return true;
    }

    void set_layer_channel(const std::string &layer_name, int universe_id, int address, int value) {
        if(set_layer_channel_no_output(layer_name, universe_id, address, value)) {
            recompute_and_output_all();
        }
    }

    std::set<int> known_universes() const {
        std::set<int> ids;
        for(const auto &layer_entry : layers_) {
            for(const auto &universe_entry : layer_entry.second.universes) {
                ids.insert(universe_entry.first);
            }
        }
        return ids;
    }

    void recompute_and_output_all() {
        merged_.clear();
        for(const int universe_id : known_universes()) {
            bbb::dmx::dmx_universe &merged_universe = merged_.ensure_universe(universe_id);
            for(int address = 1; address <= bbb::dmx::universe_channel_count; address++) {
                const bbb::dmx::semantic_lowest_parameter_group *lowest_group{semantic_lowest_group_at(universe_id, address)};
                if(lowest_group) {
                    if(group_first_channel_is(*lowest_group, universe_id, address)) {
                        write_semantic_lowest_group(merged_universe, *lowest_group);
                    }
                    continue;
                }
                merged_universe.set_channel(address, merged_value(universe_id, address));
            }
        }
        output_all();
    }

    const bbb::dmx::semantic_lowest_parameter_group *semantic_lowest_group_at(int universe_id, int address) const {
        if(mode_value_ != merge_mode::htp) {
            return nullptr;
        }
        const auto universe_found = cmy_lowest_group_by_address_.find(bbb::dmx::sanitize_universe_id(universe_id));
        if(universe_found == cmy_lowest_group_by_address_.end()) {
            return nullptr;
        }
        const auto address_found = universe_found->second.find(address);
        if(address_found == universe_found->second.end()) {
            return nullptr;
        }
        if(semantic_policy_.cmy_lowest_groups.size() <= address_found->second) {
            return nullptr;
        }
        return &semantic_policy_.cmy_lowest_groups[address_found->second];
    }

    static bool group_first_channel_is(const bbb::dmx::semantic_lowest_parameter_group &group, int universe_id, int address) {
        return !group.parameter.channels.empty() &&
            group.parameter.channels.front().universe == universe_id &&
            group.parameter.channels.front().address == address;
    }

    bool layer_parameter_values(
        const merge_layer &layer,
        const bbb::dmx::semantic_merge_parameter_reference &reference,
        std::vector<int> &values
    ) const
    {
        values.clear();
        values.reserve(reference.channels.size());
        for(const auto &channel : reference.channels) {
            const auto universe_found = layer.universes.find(channel.universe);
            if(universe_found == layer.universes.end()) {
                return false;
            }
            const int address{channel.address};
            if(address < 1 || bbb::dmx::universe_channel_count < address) {
                return false;
            }
            const std::size_t index{(std::size_t)(address - 1)};
            if(!universe_found->second.valid[index]) {
                return false;
            }
            values.push_back((int)universe_found->second.values[index]);
        }
        return true;
    }

    bool layer_parameter_raw_value(
        const merge_layer &layer,
        const bbb::dmx::semantic_merge_parameter_reference &reference,
        std::uint32_t &value
    ) const
    {
        std::vector<int> values{};
        if(!layer_parameter_values(layer, reference, values)) {
            return false;
        }
        value = bbb::dmx::semantic_merge_parameter_raw_value(reference, values);
        return true;
    }

    bool layer_semantic_gate_is_active(
        const merge_layer &layer,
        const bbb::dmx::semantic_lowest_parameter_group &group
    ) const
    {
        if(!group.has_gate) {
            return true;
        }
        std::uint32_t value{0};
        if(!layer_parameter_raw_value(layer, group.gate, value)) {
            return false;
        }
        return 0 < value;
    }

    void write_semantic_lowest_group(
        bbb::dmx::dmx_universe &merged_universe,
        const bbb::dmx::semantic_lowest_parameter_group &group
    ) const
    {
        const merge_layer *best_layer{nullptr};
        std::uint32_t best_value{std::numeric_limits<std::uint32_t>::max()};
        std::vector<int> best_values{};
        for(const auto &layer_entry : layers_) {
            const merge_layer &layer = layer_entry.second;
            if(!layer_semantic_gate_is_active(layer, group)) {
                continue;
            }
            std::uint32_t value{0};
            std::vector<int> values{};
            if(!layer_parameter_values(layer, group.parameter, values)) {
                continue;
            }
            value = bbb::dmx::semantic_merge_parameter_raw_value(group.parameter, values);
            if(!best_layer || value < best_value) {
                best_layer = &layer;
                best_value = value;
                best_values = values;
            }
        }

        if(!best_layer) {
            for(const auto &channel : group.parameter.channels) {
                merged_universe.set_channel(channel.address, merged_value(channel.universe, channel.address));
            }
            return;
        }

        for(std::size_t index{0}; index < group.parameter.channels.size() && index < best_values.size(); index++) {
            merged_universe.set_channel(group.parameter.channels[index].address, best_values[index]);
        }
    }

    int merged_value(int universe_id, int address) const {
        const std::size_t index{(std::size_t)(address - 1)};
        if(mode_value_ == merge_mode::htp) {
            int value{0};
            for(const auto &layer_entry : layers_) {
                const auto universe_found = layer_entry.second.universes.find(universe_id);
                if(universe_found != layer_entry.second.universes.end() && universe_found->second.valid[index]) {
                    value = std::max(value, (int)universe_found->second.values[index]);
                }
            }
            return value;
        }
        if(mode_value_ == merge_mode::ltp) {
            std::uint64_t best_sequence{0};
            int value{0};
            for(const auto &layer_entry : layers_) {
                const auto universe_found = layer_entry.second.universes.find(universe_id);
                if(universe_found != layer_entry.second.universes.end() && universe_found->second.valid[index] && best_sequence <= universe_found->second.sequence[index]) {
                    best_sequence = universe_found->second.sequence[index];
                    value = (int)universe_found->second.values[index];
                }
            }
            return value;
        }
        int best_priority{-2147483647};
        std::string best_name{};
        int value{0};
        for(const auto &layer_entry : layers_) {
            const auto universe_found = layer_entry.second.universes.find(universe_id);
            if(universe_found == layer_entry.second.universes.end() || !universe_found->second.valid[index]) {
                continue;
            }
            if(best_priority < layer_entry.second.priority || (best_priority == layer_entry.second.priority && best_name < layer_entry.first)) {
                best_priority = layer_entry.second.priority;
                best_name = layer_entry.first;
                value = (int)universe_found->second.values[index];
            }
        }
        return value;
    }

    void load_patch_file(const std::string &path) {
        const std::string resolved_path{bbb::dmx::maxutil::resolve_file_path(path)};
        bbb::dmx::fixture_mapper loaded_mapper{};
        const bbb::dmx::mapper_result result{bbb::dmx::load_fixture_mapper_from_patch_file(resolved_path, loaded_mapper)};
        if(!result.ok) {
            patch_loaded_ = false;
            semantic_mapper_.clear();
            rebuild_semantic_policy();
            report_error(result.message.c_str());
            return;
        }
        semantic_mapper_ = loaded_mapper;
        patch_loaded_ = true;
        if(rebuild_semantic_policy()) {
            recompute_and_output_all();
        }
        report_status("patch_loaded");
    }

    void load_semantic_overrides_file(const std::string &path) {
        const std::string resolved_path{bbb::dmx::maxutil::resolve_file_path(path)};
        bbb::dmx::fixture_semantic_overrides loaded_overrides{};
        const bbb::dmx::mapper_result result{bbb::dmx::read_fixture_semantic_overrides_file(resolved_path, loaded_overrides)};
        if(!result.ok) {
            semantic_overrides_loaded_ = false;
            semantic_overrides_ = bbb::dmx::fixture_semantic_overrides{};
            rebuild_semantic_policy();
            report_error(result.message.c_str());
            return;
        }
        semantic_overrides_ = loaded_overrides;
        semantic_overrides_loaded_ = true;
        if(rebuild_semantic_policy()) {
            recompute_and_output_all();
        }
        report_status("semantic_overrides_loaded");
    }

    bool rebuild_semantic_policy() {
        semantic_policy_ = bbb::dmx::semantic_merge_policy{};
        cmy_lowest_group_by_address_.clear();
        if(!patch_loaded_) {
            return true;
        }

        bbb::dmx::semantic_merge_policy built_policy{};
        const bbb::dmx::mapper_result result{bbb::dmx::build_semantic_cmy_lowest_merge_policy(semantic_mapper_, semantic_overrides_, built_policy)};
        if(!result.ok) {
            report_error(result.message.c_str());
            return false;
        }
        semantic_policy_ = built_policy;
        for(std::size_t group_index{0}; group_index < semantic_policy_.cmy_lowest_groups.size(); group_index++) {
            const auto &group = semantic_policy_.cmy_lowest_groups[group_index];
            for(const auto &channel : group.parameter.channels) {
                cmy_lowest_group_by_address_[channel.universe][channel.address] = group_index;
            }
        }
        return true;
    }

    void output_all() {
        const std::vector<int> ids{merged_.universe_ids()};
        if(ids.empty()) {
            output.send(bbb::dmx::maxutil::universe_atoms(universe_value_, merged_.universe(universe_value_)));
            return;
        }
        for(const int universe_id : ids) {
            output.send(bbb::dmx::maxutil::universe_atoms(universe_id, merged_.universe(universe_id)));
        }
    }

    void report_status(const char *message) {
        status_output.send(bbb::dmx::maxutil::status_atoms("status", message));
    }

    void report_error(const char *message) {
        cerr << "bbb.dmx.merge: " << message << c74::min::endl;
        status_output.send(bbb::dmx::maxutil::status_atoms("error", message));
    }
};

MIN_EXTERNAL(bbb_dmx_merge);
