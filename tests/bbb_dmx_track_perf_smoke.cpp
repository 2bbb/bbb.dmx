#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "bbb/dmx/fixture_mapper.hpp"
#include "bbb/dmx/movertrack.hpp"

namespace {

struct channel_state {
public:
    int universe{0};
    int address{0};
    int value{0};
};

struct tracking_transaction {
public:
    std::vector<channel_state> channels{};
    std::set<std::pair<int, int>> captured_channels{};
    std::map<std::string, std::pair<bool, bbb::dmx::movertrack_engine>> engines{};
};

struct run_result {
public:
    long long elapsed_microseconds{0};
    std::uint64_t checksum{0};
};

struct lookup_result {
public:
    long long elapsed_microseconds{0};
    std::uint64_t checksum{0};
};

volatile std::uint64_t snapshot_observer{0};

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

bbb::dmx::fixture_parameter make_u16_parameter(const std::string &key, const std::string &coarse, const std::string &fine, double range_degrees) {
    bbb::dmx::fixture_parameter parameter{};
    parameter.key = key;
    parameter.type = bbb::dmx::fixture_parameter_type::u16;
    parameter.channels = {coarse, fine};
    parameter.order = bbb::dmx::byte_order::coarse_fine;
    parameter.range_degrees = range_degrees;
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
        fixture.position = bbb::dmx::vec3{
            (double)(index % 16) - 8.0,
            (double)(index / 16),
            6.0 + (double)(index % 7) * 0.25
        };
        fixture.rotation = bbb::dmx::vec3{0.0, 0.0, (double)(index % 5) * 7.5};
        patch.fixtures.push_back(fixture);
    }
    return patch;
}

bbb::dmx::fixture_mapper make_mapper(int fixture_count) {
    bbb::dmx::fixture_mapper mapper{};
    require(mapper.add_profile(make_mover_profile()).ok, "add profile");
    require(mapper.set_patch(make_patch(fixture_count)).ok, "set patch");
    return mapper;
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

std::uint64_t checksum_engines(const std::map<std::string, bbb::dmx::movertrack_engine> &engines) {
    std::uint64_t checksum{1099511628211ull};
    for(const auto &entry : engines) {
        const bbb::dmx::movertrack_settings &settings{entry.second.settings()};
        checksum ^= (std::uint64_t)entry.first.size();
        checksum *= 1099511628211ull;
        checksum ^= (std::uint64_t)(settings.pan_range_degrees * 100.0);
        checksum *= 1099511628211ull;
        checksum ^= (std::uint64_t)(settings.tilt_range_degrees * 100.0);
        checksum *= 1099511628211ull;
    }
    return checksum;
}

void configure_engine(const bbb::dmx::fixture_instance &fixture, bbb::dmx::movertrack_engine &engine) {
    require(engine.set_fixture_position(fixture.position), "set fixture position");
    require(engine.set_rotation_degrees(fixture.rotation), "set fixture rotation");
    require(engine.set_ranges(540.0, 270.0), "set ranges");
    engine.set_tracking_mode(bbb::dmx::tracking_mode::smart);
}

void capture_engine(
    tracking_transaction &transaction,
    const std::string &fixture_id,
    const std::map<std::string, bbb::dmx::movertrack_engine> &engines
) {
    if(transaction.engines.find(fixture_id) != transaction.engines.end()) {
        return;
    }
    const auto found = engines.find(fixture_id);
    if(found == engines.end()) {
        transaction.engines[fixture_id] = {false, bbb::dmx::movertrack_engine{}};
        return;
    }
    transaction.engines[fixture_id] = {true, found->second};
}

void capture_parameter(
    tracking_transaction &transaction,
    const bbb::dmx::fixture_mapper &mapper,
    const std::string &fixture_id,
    const std::string &parameter
) {
    std::vector<std::pair<int, int>> addresses{};
    const bbb::dmx::mapper_result result{mapper.parameter_channel_addresses(fixture_id, parameter, addresses)};
    require(result.ok, "parameter addresses");
    for(const auto &address : addresses) {
        if(!transaction.captured_channels.insert(address).second) {
            continue;
        }
        transaction.channels.push_back(channel_state{
            address.first,
            address.second,
            mapper.universe(address.first).channel(address.second)
        });
    }
}

void capture_pan_tilt(tracking_transaction &transaction, const bbb::dmx::fixture_mapper &mapper, const std::string &fixture_id) {
    capture_parameter(transaction, mapper, fixture_id, "pan");
    capture_parameter(transaction, mapper, fixture_id, "tilt");
}

void rollback(
    const tracking_transaction &transaction,
    bbb::dmx::fixture_mapper &mapper,
    std::map<std::string, bbb::dmx::movertrack_engine> &engines
) {
    for(const auto &state : transaction.channels) {
        require(mapper.set_channel(state.universe, state.address, state.value).ok, "restore channel");
    }
    for(const auto &entry : transaction.engines) {
        if(entry.second.first) {
            engines[entry.first] = entry.second.second;
        } else {
            engines.erase(entry.first);
        }
    }
}

void track_fixture(
    bbb::dmx::fixture_mapper &mapper,
    const bbb::dmx::fixture_instance &fixture,
    std::map<std::string, bbb::dmx::movertrack_engine> &engines,
    const bbb::dmx::vec3 &target,
    tracking_transaction *transaction
) {
    bbb::dmx::movertrack_engine engine{};
    const auto found = engines.find(fixture.id);
    if(found != engines.end()) {
        engine = found->second;
    }
    configure_engine(fixture, engine);

    if(transaction) {
        capture_engine(*transaction, fixture.id, engines);
        capture_pan_tilt(*transaction, mapper, fixture.id);
    }

    const bbb::dmx::movertrack_output output{engine.compute(target)};
    require(mapper.set_normalized(fixture.id, "pan", (double)output.pan / 65535.0).ok, "write pan");
    require(mapper.set_normalized(fixture.id, "tilt", (double)output.tilt / 65535.0).ok, "write tilt");
    engines[fixture.id] = engine;
}

std::vector<int> group_indices(int fixture_count, int group_size) {
    std::vector<int> indices{};
    indices.reserve((std::size_t)group_size);
    for(int index{0}; index < group_size; index++) {
        indices.push_back((index * 17) % fixture_count);
    }
    return indices;
}

const bbb::dmx::fixture_instance *find_fixture_linear(const bbb::dmx::fixture_patch &patch, const std::string &fixture_id) {
    for(const auto &fixture : patch.fixtures) {
        if(fixture.id == fixture_id) {
            return &fixture;
        }
    }
    return nullptr;
}

std::map<std::string, std::size_t> fixture_index_map(const bbb::dmx::fixture_patch &patch) {
    std::map<std::string, std::size_t> indices{};
    for(std::size_t fixture_index{0}; fixture_index < patch.fixtures.size(); fixture_index++) {
        indices[patch.fixtures[fixture_index].id] = fixture_index;
    }
    return indices;
}

const bbb::dmx::fixture_instance *find_fixture_indexed(
    const bbb::dmx::fixture_patch &patch,
    const std::map<std::string, std::size_t> &indices,
    const std::string &fixture_id
) {
    const auto found = indices.find(fixture_id);
    if(found == indices.end() || patch.fixtures.size() <= found->second) {
        return nullptr;
    }
    const bbb::dmx::fixture_instance &fixture{patch.fixtures[found->second]};
    if(fixture.id != fixture_id) {
        return nullptr;
    }
    return &fixture;
}

lookup_result run_lookup(bool indexed, int fixture_count, int group_size, int iterations) {
    const bbb::dmx::fixture_patch patch{make_patch(fixture_count)};
    const std::map<std::string, std::size_t> indices{fixture_index_map(patch)};
    const std::vector<int> group{group_indices(fixture_count, group_size)};
    std::vector<std::string> fixture_ids{};
    fixture_ids.reserve(group.size());
    for(const int fixture_index : group) {
        fixture_ids.push_back("fixture_" + std::to_string(fixture_index));
    }

    std::uint64_t checksum{1469598103934665603ull};
    const auto start = std::chrono::steady_clock::now();
    for(int iteration{0}; iteration < iterations; iteration++) {
        for(const std::string &fixture_id : fixture_ids) {
            const bbb::dmx::fixture_instance *fixture{indexed
                ? find_fixture_indexed(patch, indices, fixture_id)
                : find_fixture_linear(patch, fixture_id)};
            require(fixture != nullptr, "lookup fixture");
            checksum ^= (std::uint64_t)(fixture->universe * 257 + fixture->address * 17 + iteration);
            checksum *= 1099511628211ull;
        }
    }
    const auto end = std::chrono::steady_clock::now();
    const long long elapsed_microseconds{std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()};
    return lookup_result{elapsed_microseconds, checksum};
}

run_result run_tracking(bool full_snapshot, int fixture_count, int group_size, int iterations) {
    bbb::dmx::fixture_mapper mapper{make_mapper(fixture_count)};
    std::map<std::string, bbb::dmx::movertrack_engine> engines{};
    const std::vector<int> indices{group_indices(fixture_count, group_size)};

    const auto start = std::chrono::steady_clock::now();
    for(int iteration{0}; iteration < iterations; iteration++) {
        const bbb::dmx::vec3 target{
            (double)(iteration % 31) - 15.0,
            20.0 + (double)(iteration % 13) * 0.4,
            4.0 + (double)(iteration % 11) * 0.2
        };
        if(full_snapshot) {
            const auto previous_universes = mapper.universe_snapshot();
            const auto previous_engines = engines;
            snapshot_observer += previous_universes.size() + previous_engines.size();
            for(const int fixture_index : indices) {
                track_fixture(mapper, mapper.patch().fixtures[(std::size_t)fixture_index], engines, target, nullptr);
            }
        } else {
            tracking_transaction transaction{};
            for(const int fixture_index : indices) {
                track_fixture(mapper, mapper.patch().fixtures[(std::size_t)fixture_index], engines, target, &transaction);
            }
        }
    }
    const auto end = std::chrono::steady_clock::now();
    const long long elapsed_microseconds{std::chrono::duration_cast<std::chrono::microseconds>(end - start).count()};
    return run_result{elapsed_microseconds, checksum_mapper(mapper) ^ checksum_engines(engines)};
}

void verify_rollback(int fixture_count, int group_size) {
    bbb::dmx::fixture_mapper mapper{make_mapper(fixture_count)};
    std::map<std::string, bbb::dmx::movertrack_engine> engines{};
    const std::uint64_t original_checksum{checksum_mapper(mapper) ^ checksum_engines(engines)};
    const std::vector<int> indices{group_indices(fixture_count, group_size)};
    tracking_transaction transaction{};
    for(const int fixture_index : indices) {
        track_fixture(
            mapper,
            mapper.patch().fixtures[(std::size_t)fixture_index],
            engines,
            bbb::dmx::vec3{1.0, 25.0, 4.0},
            &transaction
        );
    }
    rollback(transaction, mapper, engines);
    const std::uint64_t rollback_checksum{checksum_mapper(mapper) ^ checksum_engines(engines)};
    require(original_checksum == rollback_checksum, "touched rollback restores original state");
}

} // namespace

int main() {
    constexpr int fixture_count{512};
    constexpr int group_size{16};
    constexpr int iterations{2000};

    verify_rollback(fixture_count, group_size);
    const lookup_result linear_lookup{run_lookup(false, fixture_count, group_size, iterations * 16)};
    const lookup_result indexed_lookup{run_lookup(true, fixture_count, group_size, iterations * 16)};
    require(linear_lookup.checksum == indexed_lookup.checksum, "linear and indexed lookup checksums match");
    const run_result full_snapshot{run_tracking(true, fixture_count, group_size, iterations)};
    const run_result touched_snapshot{run_tracking(false, fixture_count, group_size, iterations)};
    require(full_snapshot.checksum == touched_snapshot.checksum, "full and touched tracking checksums match");
    require(full_snapshot.checksum != 0, "checksum non-zero");

    std::cout << "bbb_dmx_track_perf_smoke fixtures=" << fixture_count
              << " group_size=" << group_size
              << " iterations=" << iterations
              << " linear_lookup_elapsed_us=" << linear_lookup.elapsed_microseconds
              << " indexed_lookup_elapsed_us=" << indexed_lookup.elapsed_microseconds
              << " full_snapshot_elapsed_us=" << full_snapshot.elapsed_microseconds
              << " touched_snapshot_elapsed_us=" << touched_snapshot.elapsed_microseconds
              << " checksum=" << touched_snapshot.checksum
              << std::endl;
    return 0;
}
