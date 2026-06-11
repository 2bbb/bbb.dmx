#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "bbb/dmx/value.hpp"

namespace bbb::dmx {

enum class fixture_parameter_type {
    u8,
    u16,
    enum_u8,
};

struct fixture_channel {
public:
    int offset{0};
    std::string key{};
    int default_value{0};
    std::string label{};
    bool hold{false};
};

struct fixture_parameter {
public:
    std::string key{};
    fixture_parameter_type type{fixture_parameter_type::u8};
    std::vector<std::string> channels{};
    byte_order order{byte_order::coarse_fine};
    int default_value{0};
    double range_degrees{0.0};
};

struct fixture_mode {
public:
    std::string key{};
    std::string label{};
    int footprint{0};
    std::vector<fixture_channel> channels{};
    std::vector<fixture_parameter> parameters{};

    const fixture_channel *find_channel(const std::string &channel_key) const {
        for(const auto &channel : channels) {
            if(channel.key == channel_key) {
                return &channel;
            }
        }
        return nullptr;
    }

    const fixture_parameter *find_parameter(const std::string &parameter_key) const {
        for(const auto &parameter : parameters) {
            if(parameter.key == parameter_key) {
                return &parameter;
            }
        }
        return nullptr;
    }
};

struct fixture_profile {
public:
    std::string key{};
    std::string manufacturer{};
    std::string model{};
    std::vector<fixture_mode> modes{};

    const fixture_mode *find_mode(const std::string &mode_key) const {
        for(const auto &mode : modes) {
            if(mode.key == mode_key) {
                return &mode;
            }
        }
        return nullptr;
    }
};

} // namespace bbb::dmx
