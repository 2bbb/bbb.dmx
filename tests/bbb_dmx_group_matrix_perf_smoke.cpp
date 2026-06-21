#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "bbb/dmx/fixture_groups.hpp"
#include "bbb/dmx/fixture_mapper.hpp"

namespace {

struct run_result {
public:
    long long elapsed_microseconds{0};
    std::uint64_t checksum{0};
};

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

bbb::dmx::fixture_parameter make_u8_parameter(const std::string &key, const std::string &channel_key) {
    bbb::dmx::fixture_parameter parameter{};
    parameter.key = key;
    parameter.type = bbb::dmx::fixture_parameter_type::u8;
    parameter.channels = {channel_key};
    return parameter;
}

bbb::dmx::fixture_parameter make_u16_parameter(const std::string &key, const std::string &coarse, const std::string &fine) {
    bbb::dmx::fixture_parameter parameter{};
    parameter.key = key;
    parameter.type = bbb::dmx::fixture_parameter_type::u16;
    parameter.channels = {coarse, fine};
    parameter.order = bbb::dmx::byte_order::coarse_fine;
    return parameter;
}

bbb::dmx::fixture_profile make_profile() {
    bbb::dmx::fixture_mode mode{};
    mode.key = "perf";
    mode.footprint = 8;
    mode.channels = {
        make_channel(1, "pan.coarse"),
        make_channel(2, "pan.fine"),
        make_channel(3, "tilt.coarse"),
        make_channel(4, "tilt.fine"),
        make_channel(5, "dimmer"),
        make_channel(6, "red"),
        make_channel(7, "green"),
        make_channel(8, "blue"),
    };
    mode.parameters = {
        make_u16_parameter("pan", "pan.coarse", "pan.fine"),
        make_u16_parameter("tilt", "tilt.coarse", "tilt.fine"),
        make_u8_parameter("dimmer", "dimmer"),
        make_u8_parameter("red", "red"),
        make_u8_parameter("green", "green"),
        make_u8_parameter("blue", "blue"),
    };

    bbb::dmx::fixture_profile profile{};
    profile.key = "perf.fixture";
    profile.modes = {mode};
    return profile;
}

bbb::dmx::fixture_patch make_patch(int fixture_count) {
    constexpr int footprint{8};
    constexpr int fixtures_per_universe{bbb::dmx::universe_channel_count / footprint};
    bbb::dmx::fixture_patch patch{};
    patch.fixtures.reserve((std::size_t)fixture_count);
    for(int index{0}; index < fixture_count; index++) {
        bbb::dmx::fixture_instance fixture{};
        fixture.id = "fixture_" + std::to_string(index);
        fixture.profile = "perf.fixture";
        fixture.mode = "perf";
        fixture.universe = 1 + index / fixtures_per_universe;
        fixture.address = 1 + (index % fixtures_per_universe) * footprint;
        patch.fixtures.push_back(fixture);
    }
    return patch;
}

bbb::dmx::fixture_mapper make_mapper(int fixture_count) {
    bbb::dmx::fixture_mapper mapper{};
    require(mapper.add_profile(make_profile()).ok, "add profile");
    require(mapper.set_patch(make_patch(fixture_count)).ok, "set patch");
    return mapper;
}

bbb::dmx::fixture_group_set make_groups(int group_size) {
    bbb::dmx::fixture_group a{};
    a.id = "A";
    for(int index{0}; index < group_size / 2; index++) {
        a.entries.push_back(bbb::dmx::fixture_group_entry{bbb::dmx::fixture_group_entry_type::fixture, "fixture_" + std::to_string(index)});
    }

    bbb::dmx::fixture_group b{};
    b.id = "B";
    for(int index{group_size / 2}; index < group_size; index++) {
        b.entries.push_back(bbb::dmx::fixture_group_entry{bbb::dmx::fixture_group_entry_type::fixture, "fixture_" + std::to_string(index)});
    }

    bbb::dmx::fixture_group all{};
    all.id = "ALL";
    all.entries.push_back(bbb::dmx::fixture_group_entry{bbb::dmx::fixture_group_entry_type::group, "A"});
    all.entries.push_back(bbb::dmx::fixture_group_entry{bbb::dmx::fixture_group_entry_type::group, "B"});

    bbb::dmx::fixture_group_set groups{};
    groups.groups = {a, b, all};
    return groups;
}

std::uint64_t checksum_ids(const std::vector<std::string> &fixture_ids, int iteration) {
    std::uint64_t checksum{1469598103934665603ull};
    checksum ^= (std::uint64_t)fixture_ids.size();
    checksum *= 1099511628211ull;
    for(const std::string &fixture_id : fixture_ids) {
        for(const char character : fixture_id) {
            checksum ^= (std::uint64_t)(unsigned char)character;
            checksum *= 1099511628211ull;
        }
        checksum ^= (std::uint64_t)iteration * 17;
        checksum *= 1099511628211ull;
    }
    return checksum;
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

run_result run_group_resolve(bool cached, int fixture_count, int group_size, int iterations) {
    const bbb::dmx::fixture_patch patch{make_patch(fixture_count)};
    const bbb::dmx::fixture_group_set groups{make_groups(group_size)};
    require(bbb::dmx::validate_fixture_groups_for_patch(groups, patch).ok, "validate groups");
    std::map<std::string, std::vector<std::string>> cache{};
    std::uint64_t checksum{1099511628211ull};
    const auto start = std::chrono::steady_clock::now();
    for(int iteration{0}; iteration < iterations; iteration++) {
        std::vector<std::string> fixture_ids{};
        const auto cached_group = cache.find("ALL");
        if(cached && cached_group != cache.end()) {
            fixture_ids = cached_group->second;
        } else {
            const bbb::dmx::mapper_result result{bbb::dmx::resolve_fixture_group_fixture_ids(groups, patch, "ALL", fixture_ids)};
            require(result.ok, "resolve group");
            if(cached) {
                cache["ALL"] = fixture_ids;
            }
        }
        checksum ^= checksum_ids(fixture_ids, iteration);
        checksum *= 1099511628211ull;
    }
    const auto end = std::chrono::steady_clock::now();
    return run_result{std::chrono::duration_cast<std::chrono::microseconds>(end - start).count(), checksum};
}

run_result run_raw_mapper(int fixture_count, int group_size, int iterations) {
    bbb::dmx::fixture_mapper mapper{make_mapper(fixture_count)};
    std::uint64_t checksum{1099511628211ull};
    const auto start = std::chrono::steady_clock::now();
    for(int iteration{0}; iteration < iterations; iteration++) {
        for(int fixture_index{0}; fixture_index < group_size; fixture_index++) {
            const std::string fixture_id{"fixture_" + std::to_string(fixture_index)};
            const std::uint16_t pan{(std::uint16_t)((iteration * 13 + fixture_index * 17) & 65535)};
            const std::uint16_t tilt{(std::uint16_t)((iteration * 19 + fixture_index * 23) & 65535)};
            require(mapper.set_u16(fixture_id, "pan", pan).ok, "set pan raw");
            require(mapper.set_pan_tilt_bytes(fixture_id, (pan >> 8) & 255, pan & 255, (tilt >> 8) & 255, tilt & 255).ok, "set pan tilt bytes");
            std::vector<std::pair<int, int>> addresses{};
            require(mapper.parameter_channel_addresses(fixture_id, "pan", addresses).ok, "pan addresses");
            require(addresses.size() == 2, "pan address count");
            int value{0};
            require(mapper.current_raw_value(fixture_id, "pan", value).ok, "pan current raw");
            require(value == pan, "pan current raw matches written value");
            require(mapper.current_raw_value(fixture_id, "tilt", value).ok, "tilt current raw");
            require(value == tilt, "tilt current raw matches written value");
            checksum ^= (std::uint64_t)(value + addresses.size() * 257 + fixture_index * 17);
            checksum *= 1099511628211ull;
        }
    }
    const auto end = std::chrono::steady_clock::now();
    return run_result{std::chrono::duration_cast<std::chrono::microseconds>(end - start).count(), checksum ^ checksum_mapper(mapper)};
}

} // namespace

int main() {
    constexpr int fixture_count{512};
    constexpr int group_size{64};
    constexpr int iterations{4000};

    const run_result uncached_group{run_group_resolve(false, fixture_count, group_size, iterations)};
    const run_result cached_group{run_group_resolve(true, fixture_count, group_size, iterations)};
    require(uncached_group.checksum == cached_group.checksum, "group checksums match");

    const run_result raw_mapper{run_raw_mapper(fixture_count, group_size, iterations / 4)};
    constexpr std::uint64_t expected_checksum{13771189737396447034ull};
    const std::uint64_t checksum{cached_group.checksum ^ raw_mapper.checksum};
    require(checksum == expected_checksum, "group/matrix performance smoke checksum");

    std::cout << "bbb_dmx_group_matrix_perf_smoke fixtures=" << fixture_count
              << " group_size=" << group_size
              << " iterations=" << iterations
              << " uncached_group_elapsed_us=" << uncached_group.elapsed_microseconds
              << " cached_group_elapsed_us=" << cached_group.elapsed_microseconds
              << " raw_mapper_elapsed_us=" << raw_mapper.elapsed_microseconds
              << " checksum=" << checksum
              << std::endl;
    return 0;
}
