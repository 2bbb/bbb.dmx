#pragma once

#include "c74_min.h"

#include <algorithm>
#include <cmath>
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

inline std::string resolve_file_path(const std::string &path) {
    if(path.empty() || path_is_absolute(path)) {
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
        return resolved_symbol->s_name;
    }
    return path;
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
