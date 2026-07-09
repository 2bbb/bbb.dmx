#pragma once

#include "c74_min.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

#include "bbb/dmx/frame_set.hpp"
#include "bbb/dmx/math.hpp"

namespace bbb::dmx::maxutil {

inline bool finite_atom(const c74::min::atom &atom) {
    return bbb::dmx::is_finite((double)atom);
}

inline bool finite_atoms(const c74::min::atoms &atoms, std::size_t start, std::size_t count) {
    for(std::size_t index = start; index < start + count; index++) {
        if(atoms.size() <= index || !finite_atom(atoms[index])) {
            return false;
        }
    }
    return true;
}

inline int clamp_int(int value, int minimum, int maximum) {
    return std::max(minimum, std::min(maximum, value));
}

inline std::string symbol_arg(const c74::min::atom &atom) {
    if(atom.a_type == c74::max::A_LONG) {
        return std::to_string(atom.a_w.w_long);
    }
    if(atom.a_type == c74::max::A_FLOAT) {
        const double value{atom.a_w.w_float};
        if(std::isfinite(value) && std::floor(value) == value) {
            std::ostringstream stream{};
            stream << std::fixed << std::setprecision(0) << value;
            return stream.str();
        }
        std::ostringstream stream{};
        stream << std::setprecision(15) << value;
        return stream.str();
    }
    const c74::min::symbol symbol_value{(c74::min::symbol)atom};
    return symbol_value.c_str();
}

inline std::vector<int> values_from_atoms(const c74::min::atoms &atoms, std::size_t start) {
    std::vector<int> values;
    if(atoms.size() <= start) {
        return values;
    }
    values.reserve(atoms.size() - start);
    for(std::size_t index = start; index < atoms.size(); index++) {
        values.push_back(clamp_int((int)atoms[index], 0, 255));
    }
    return values;
}

struct frame_write_result {
public:
    bool ok{false};
    int universe{1};
    std::string message{};

    static frame_write_result success(int universe_id) {
        return frame_write_result{true, sanitize_universe_id(universe_id), ""};
    }

    static frame_write_result failure(const std::string &error_message) {
        return frame_write_result{false, 1, error_message};
    }
};

inline frame_write_result write_universe_from_atoms(
    bbb::dmx::dmx_frame_set &frames,
    int universe_id,
    const c74::min::atoms &atoms,
    std::size_t start,
    const std::string &error_message
) {
    if(atoms.size() < start + (std::size_t)universe_channel_count || !finite_atoms(atoms, start, universe_channel_count)) {
        return frame_write_result::failure(error_message);
    }
    const int sanitized_universe{sanitize_universe_id(universe_id)};
    const bbb::dmx::write_result result{frames.set_universe(sanitized_universe, values_from_atoms(atoms, start))};
    if(!result.ok) {
        return frame_write_result::failure(result.message);
    }
    return frame_write_result::success(sanitized_universe);
}

inline frame_write_result write_universe_message(
    bbb::dmx::dmx_frame_set &frames,
    const c74::min::atoms &atoms,
    const std::string &error_message
) {
    if(atoms.size() < 513 || !finite_atom(atoms[0])) {
        return frame_write_result::failure(error_message);
    }
    return write_universe_from_atoms(frames, (int)atoms[0], atoms, 1, error_message);
}

inline frame_write_result write_channel_message(
    bbb::dmx::dmx_frame_set &frames,
    const c74::min::atoms &atoms,
    const std::string &error_message
) {
    if(atoms.size() < 3 || !finite_atoms(atoms, 0, 3)) {
        return frame_write_result::failure(error_message);
    }
    const int sanitized_universe{sanitize_universe_id((int)atoms[0])};
    const bbb::dmx::write_result result{frames.set_channel(sanitized_universe, (int)atoms[1], (int)atoms[2])};
    if(!result.ok) {
        return frame_write_result::failure(result.message);
    }
    return frame_write_result::success(sanitized_universe);
}

inline frame_write_result write_channels_message(
    bbb::dmx::dmx_frame_set &frames,
    const c74::min::atoms &atoms,
    const std::string &error_message,
    const std::string &pair_error_message
) {
    if(atoms.size() < 3 || ((atoms.size() - 1) % 2) != 0 || !finite_atom(atoms[0])) {
        return frame_write_result::failure(error_message);
    }
    const int sanitized_universe{sanitize_universe_id((int)atoms[0])};
    for(std::size_t index = 1; index < atoms.size(); index += 2) {
        if(!finite_atoms(atoms, index, 2)) {
            return frame_write_result::failure(pair_error_message);
        }
        const bbb::dmx::write_result result{frames.set_channel(sanitized_universe, (int)atoms[index], (int)atoms[index + 1])};
        if(!result.ok) {
            return frame_write_result::failure(result.message);
        }
    }
    return frame_write_result::success(sanitized_universe);
}

inline c74::min::atoms universe_atoms(int universe_id, const bbb::dmx::dmx_universe &universe) {
    c74::min::atoms atoms;
    atoms.reserve((std::size_t)universe_channel_count + 2);
    atoms.push_back(c74::min::symbol("universe"));
    atoms.push_back(universe_id);
    for(const int value : universe.to_int_vector()) {
        atoms.push_back(value);
    }
    return atoms;
}

inline c74::min::atoms changed_atoms(int universe_id, const std::vector<int> &changes) {
    c74::min::atoms atoms;
    atoms.reserve(changes.size() + 2);
    atoms.push_back(c74::min::symbol("changed"));
    atoms.push_back(universe_id);
    for(const int value : changes) {
        atoms.push_back(value);
    }
    return atoms;
}

inline c74::min::atoms status_atoms(const char *selector, const std::string &message) {
    c74::min::atoms atoms;
    atoms.push_back(c74::min::symbol(selector));
    atoms.push_back(c74::min::symbol(message.c_str()));
    return atoms;
}

inline bool path_is_absolute(const std::string &path) {
    if(path.empty()) {
        return false;
    }
    if(path[0] == '/' || path[0] == '\\') {
        return true;
    }
    return 1 < path.size() && path[1] == ':';
}

inline std::string parent_directory(const std::string &path) {
    const std::size_t slash_position{path.find_last_of("/\\")};
    if(slash_position == std::string::npos) {
        return "";
    }
    return path.substr(0, slash_position + 1);
}

inline std::string join_relative_path(const std::string &base_directory, const std::string &path) {
    if(path_is_absolute(path) || base_directory.empty()) {
        return path;
    }
    return base_directory + path;
}

inline bool file_exists(const std::string &path) {
    if(path.empty()) {
        return false;
    }
    std::ifstream stream(path.c_str(), std::ios::in | std::ios::binary);
    return (bool)stream;
}

inline std::string max_path_to_system_path(const std::string &path) {
    if(path.empty()) {
        return {};
    }

    short path_id{0};
    char filename[c74::max::MAX_FILENAME_CHARS]{};
    const short pathname_error{c74::max::path_frompathname(path.c_str(), &path_id, filename)};
    if(pathname_error != 0) {
        return {};
    }

    char system_path[c74::max::MAX_PATH_CHARS]{};
    const c74::max::t_max_err system_path_error{c74::max::path_toabsolutesystempath(path_id, filename, system_path)};
    if(system_path_error != 0 || system_path[0] == '\0') {
        return {};
    }
    return system_path;
}

inline std::string normalize_system_path(const std::string &path) {
    const std::string system_path{max_path_to_system_path(path)};
    if(!system_path.empty()) {
        return system_path;
    }
    return path;
}

inline std::string patcher_file_path(c74::max::t_object *max_object) {
    if(!max_object) {
        return {};
    }

    c74::max::t_object *patcher{nullptr};
    const c74::max::t_max_err lookup_error{c74::max::object_obex_lookup(max_object, c74::max::gensym("#P"), &patcher)};
    if(lookup_error != 0 || !patcher) {
        return {};
    }

    c74::max::t_symbol *filepath_symbol{c74::max::jpatcher_get_filepath(patcher)};
    if(!filepath_symbol || !filepath_symbol->s_name || filepath_symbol->s_name[0] == '\0') {
        return {};
    }
    return normalize_system_path(filepath_symbol->s_name);
}

inline std::string patcher_directory(c74::max::t_object *max_object) {
    return parent_directory(patcher_file_path(max_object));
}

inline std::string resolve_file_path(const std::string &path) {
    if(path.empty()) {
        return path;
    }

    if(path_is_absolute(path)) {
        const std::string system_path{max_path_to_system_path(path)};
        if(!system_path.empty()) {
            return system_path;
        }
        return path;
    }

    c74::max::t_symbol *resolved_symbol{nullptr};
    const c74::max::t_max_err error{c74::max::path_absolutepath(
        &resolved_symbol,
        c74::max::gensym(path.c_str()),
        nullptr,
        0
    )};
    if(error == 0 && resolved_symbol && resolved_symbol->s_name) {
        const std::string resolved_path{resolved_symbol->s_name};
        const std::string system_path{max_path_to_system_path(resolved_path)};
        if(!system_path.empty()) {
            return system_path;
        }
        return resolved_path;
    }
    return path;
}

inline std::string resolve_file_path(c74::max::t_object *max_object, const std::string &path) {
    if(path.empty() || path_is_absolute(path)) {
        return resolve_file_path(path);
    }

    const std::string patcher_base_directory{patcher_directory(max_object)};
    if(!patcher_base_directory.empty()) {
        const std::string patcher_relative_path{normalize_system_path(join_relative_path(patcher_base_directory, path))};
        if(file_exists(patcher_relative_path)) {
            return patcher_relative_path;
        }
    }

    return resolve_file_path(path);
}

inline bool explicit_symbol_attribute_value(const c74::min::atoms &args) {
    if(args.empty()) {
        return false;
    }
    return !symbol_arg(args[0]).empty();
}

inline bool should_mark_explicit_symbol_override(
    const c74::min::atoms &args,
    bool applying_setup,
    bool suppress_attribute_load
) {
    return explicit_symbol_attribute_value(args) && !applying_setup && !suppress_attribute_load;
}

inline std::string setup_relative_path(const std::string &base_directory, const std::string &path) {
    if(path.empty()) {
        return path;
    }
    if(path_is_absolute(path)) {
        const std::string system_path{max_path_to_system_path(path)};
        if(!system_path.empty()) {
            return system_path;
        }
        return path;
    }
    return join_relative_path(base_directory, path);
}

inline void apply_setup_symbol_path(
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

template <typename outlet_type>
void send_status(outlet_type &outlet, const char *selector, const std::string &message) {
    outlet.send(status_atoms(selector, message));
}

template <typename outlet_type>
void send_status(outlet_type &outlet, const char *selector, const char *message) {
    outlet.send(status_atoms(selector, std::string(message)));
}

} // namespace bbb::dmx::maxutil
