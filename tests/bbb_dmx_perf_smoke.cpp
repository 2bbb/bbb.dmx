#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "bbb/dmx/fixture_mapper.hpp"

namespace {

void require(bool condition, const char *message) {
    if(!condition) {
        std::cerr << "FAIL: " << message << std::endl;
        std::exit(1);
    }
}

bbb::dmx::fixture_channel make_channel(int offset, const std::string &key) {
    bbb::dmx::fixture_channel channel{};
    channel.offset = offset;
    channel.key = key;
    return channel;
}

bbb::dmx::fixture_parameter make_u16_parameter(const std::string &key, const std::string &coarse, const std::string &fine, double range_degrees) {
    bbb::dmx::fixture_parameter parameter{};
    parameter.key = key;
    parameter.type = bbb::dmx::fixture_parameter_type::u16;
    parameter.channels = {coarse, fine};
    parameter.order = bbb::dmx::byte_order::coarse_fine;
    parameter.range_degrees = range_degrees;
    return parameter;
}

bbb::dmx::fixture_parameter make_u8_parameter(const std::string &key, const std::string &channel_key) {
    bbb::dmx::fixture_parameter parameter{};
    parameter.key = key;
    parameter.type = bbb::dmx::fixture_parameter_type::u8;
    parameter.channels = {channel_key};
    return parameter;
}

bbb::dmx::fixture_profile make_mover_profile() {
    constexpr int dummy_parameter_count{48};

    bbb::dmx::fixture_mode mode{};
    mode.key = "mover";
    mode.footprint = dummy_parameter_count + 4;
    mode.channels.reserve((std::size_t)mode.footprint);
    mode.parameters.reserve((std::size_t)dummy_parameter_count + 2);
    for(int index{0}; index < dummy_parameter_count; index++) {
        const std::string key{"dummy_" + std::to_string(index)};
        mode.channels.push_back(make_channel(index + 1, key));
        mode.parameters.push_back(make_u8_parameter(key, key));
    }
    mode.channels.push_back(make_channel(dummy_parameter_count + 1, "pan.coarse"));
    mode.channels.push_back(make_channel(dummy_parameter_count + 2, "pan.fine"));
    mode.channels.push_back(make_channel(dummy_parameter_count + 3, "tilt.coarse"));
    mode.channels.push_back(make_channel(dummy_parameter_count + 4, "tilt.fine"));
    mode.parameters.push_back(make_u16_parameter("pan", "pan.coarse", "pan.fine", 540.0));
    mode.parameters.push_back(make_u16_parameter("tilt", "tilt.coarse", "tilt.fine", 270.0));

    bbb::dmx::fixture_profile profile{};
    profile.key = "perf.mover";
    profile.manufacturer = "bbb";
    profile.model = "perf mover";
    profile.modes = {mode};
    return profile;
}

bbb::dmx::fixture_patch make_patch(int fixture_count) {
    constexpr int footprint{52};
    constexpr int fixtures_per_universe{bbb::dmx::universe_channel_count / footprint};

    bbb::dmx::fixture_patch patch{};
    patch.coordinates = "gdtf";
    patch.fixtures.reserve((std::size_t)fixture_count);
    for(int index{0}; index < fixture_count; index++) {
        bbb::dmx::fixture_instance fixture{};
        fixture.id = "fixture_" + std::to_string(index);
        fixture.profile = "perf.mover";
        fixture.mode = "mover";
        fixture.universe = 1 + index / fixtures_per_universe;
        fixture.address = 1 + (index % fixtures_per_universe) * footprint;
        patch.fixtures.push_back(fixture);
    }
    return patch;
}

std::uint64_t checksum_mapper(const bbb::dmx::fixture_mapper &mapper) {
    std::uint64_t checksum{1469598103934665603ull};
    for(const int universe_id : mapper.universe_ids()) {
        const bbb::dmx::dmx_universe &universe{mapper.universe(universe_id)};
        for(int address{1}; address <= bbb::dmx::universe_channel_count; address++) {
            checksum ^= (std::uint64_t)(universe.channel(address) + universe_id * 257 + address * 17);
            checksum *= 1099511628211ull;
        }
    }
    return checksum;
}

} // namespace

int main() {
    constexpr int fixture_count{512};
    constexpr int iterations{120};

    bbb::dmx::fixture_mapper mapper{};
    require(mapper.add_profile(make_mover_profile()).ok, "add profile");
    require(mapper.set_patch(make_patch(fixture_count)).ok, "set patch");

    const auto start = std::chrono::steady_clock::now();
    for(int iteration{0}; iteration < iterations; iteration++) {
        for(int fixture_index{0}; fixture_index < fixture_count; fixture_index++) {
            const std::string fixture_id{"fixture_" + std::to_string(fixture_index)};
            const double pan{(double)((iteration + fixture_index) % 1000) / 999.0};
            const double tilt{(double)((iteration * 3 + fixture_index * 5) % 1000) / 999.0};
            bbb::dmx::mapper_result result{mapper.set_normalized(fixture_id, "pan", pan)};
            require(result.ok, "set pan");
            result = mapper.set_normalized(fixture_id, "tilt", tilt);
            require(result.ok, "set tilt");
        }
    }
    const auto end = std::chrono::steady_clock::now();
    const auto elapsed_microseconds = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
    const std::uint64_t checksum{checksum_mapper(mapper)};
    require(checksum != 0, "checksum non-zero");

    std::cout << "bbb_dmx_perf_smoke fixture_mapper_set_normalized fixtures=" << fixture_count
              << " iterations=" << iterations
              << " elapsed_us=" << elapsed_microseconds
              << " checksum=" << checksum
              << std::endl;
    return 0;
}
