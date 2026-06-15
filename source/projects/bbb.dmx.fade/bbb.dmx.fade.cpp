#include "c74_min.h"

#include <bbb/dmx/build_info.hpp>

#include <bbb/dmx/frame_set.hpp>
#include <bbb/dmx/max_external_utils.hpp>

#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <map>
#include <vector>

namespace {

struct fade_universe {
public:
    std::array<double, bbb::dmx::universe_channel_count> current{};
    std::array<double, bbb::dmx::universe_channel_count> start{};
    std::array<double, bbb::dmx::universe_channel_count> target{};
    std::array<double, bbb::dmx::universe_channel_count> start_ms{};
    std::array<double, bbb::dmx::universe_channel_count> duration_ms{};
    std::array<bool, bbb::dmx::universe_channel_count> active{};
};

} // namespace

class bbb_dmx_fade : public c74::min::object<bbb_dmx_fade> {
private:
    std::map<int, fade_universe> universes_{};
    int universe_value_{1};
    double default_time_ms_value_{1000.0};
    int fps_value_{40};

public:
    MIN_DESCRIPTION{"Fade DMX channels and universes over time across multiple universes."};
    MIN_TAGS{"dmx, lighting, fade, interpolation, universe"};
    MIN_AUTHOR{"2bit"};
    MIN_RELATED{"bbb.dmx.merge, bbb.dmx.monitor"};

    bbb_dmx_fade() {
        bbb::dmx::report_external_build_info(cout, "bbb.dmx.fade");
    }

    c74::min::inlet<> input{this, "(list/universe/channel/channels/stop/bang) fade target input"};
    c74::min::outlet<> output{this, "(anything) universe id and 512 faded bytes"};
    c74::min::outlet<> status_output{this, "(anything) status and error messages"};

    c74::min::timer<c74::min::timer_options::defer_delivery> timer{this,
        MIN_FUNCTION {
            tick();
            return {};
        }
    };

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

    c74::min::attribute<double> time_ms{this, "time_ms", 1000.0,
        c74::min::description{"Default fade time in milliseconds."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(args.empty() || !bbb::dmx::maxutil::finite_atom(args[0])) {
                return {default_time_ms_value_};
            }
            default_time_ms_value_ = std::max(0.0, (double)args[0]);
            return {default_time_ms_value_};
        }}
    };

    c74::min::attribute<int> fps{this, "fps", 40,
        c74::min::description{"Fade output frame rate."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(args.empty() || !bbb::dmx::maxutil::finite_atom(args[0])) {
                return {fps_value_};
            }
            fps_value_ = bbb::dmx::maxutil::clamp_int((int)args[0], 1, 120);
            return {fps_value_};
        }}
    };

    c74::min::message<> list_message{this, "list", "512 target values for default universe using @time_ms.",
        MIN_FUNCTION {
            set_universe_target(universe_value_, default_time_ms_value_, args, 0);
            return {};
        }
    };

    c74::min::message<> universe_message{this, "universe", "universe id time_ms value1 ... value512",
        MIN_FUNCTION {
            if(args.size() < 514 || !bbb::dmx::maxutil::finite_atoms(args, 0, 2)) {
                report_error("universe requires id, time_ms, and 512 values");
                return {};
            }
            set_universe_target((int)args[0], std::max(0.0, (double)args[1]), args, 2);
            return {};
        }
    };

    c74::min::message<> channel_message{this, "channel", "channel universe address value time_ms",
        MIN_FUNCTION {
            if(args.size() < 4 || !bbb::dmx::maxutil::finite_atoms(args, 0, 4)) {
                report_error("channel requires universe address value time_ms");
                return {};
            }
            set_channel_target((int)args[0], (int)args[1], (int)args[2], std::max(0.0, (double)args[3]));
            return {};
        }
    };

    c74::min::message<> channels_message{this, "channels", "channels universe time_ms address value ...",
        MIN_FUNCTION {
            if(args.size() < 4 || ((args.size() - 2) % 2) != 0 || !bbb::dmx::maxutil::finite_atoms(args, 0, 2)) {
                report_error("channels requires universe time_ms and address/value pairs");
                return {};
            }
            const int universe_id{(int)args[0]};
            const double duration{std::max(0.0, (double)args[1])};
            for(std::size_t index = 2; index < args.size(); index += 2) {
                if(!bbb::dmx::maxutil::finite_atoms(args, index, 2)) {
                    report_error("channels pair must be numeric");
                    return {};
                }
                set_channel_target_no_output(universe_id, (int)args[index], (int)args[index + 1], duration);
            }
            output_universe(bbb::dmx::sanitize_universe_id(universe_id));
            schedule_timer_if_needed();
            return {};
        }
    };

    c74::min::message<> bang_message{this, "bang", "Output the default universe." ,
        MIN_FUNCTION {
            output_universe(universe_value_);
            return {};
        }
    };

    c74::min::message<> bangall_message{this, "bangall", "Output all known universes." ,
        MIN_FUNCTION {
            output_all();
            return {};
        }
    };

    c74::min::message<> stop_message{this, "stop", "Stop all active fades at the current values." ,
        MIN_FUNCTION {
            for(auto &universe_entry : universes_) {
                universe_entry.second.active.fill(false);
            }
            timer.stop();
            output_all();
            return {};
        }
    };

    c74::min::message<> clear_message{this, "clear", "Clear all universe state." ,
        MIN_FUNCTION {
            universes_.clear();
            timer.stop();
            status_output.send(bbb::dmx::maxutil::status_atoms("status", "clear"));
            return {};
        }
    };

private:
    static double now_ms() {
        using clock = std::chrono::steady_clock;
        const auto now = clock::now().time_since_epoch();
        return (double)std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
    }

    fade_universe &ensure_universe(int universe_id) {
        return universes_[bbb::dmx::sanitize_universe_id(universe_id)];
    }

    void set_universe_target(int universe_id, double duration, const c74::min::atoms &args, std::size_t start) {
        if(args.size() < start + (std::size_t)bbb::dmx::universe_channel_count || !bbb::dmx::maxutil::finite_atoms(args, start, bbb::dmx::universe_channel_count)) {
            report_error("universe target requires 512 numeric values");
            return;
        }
        const int sanitized_universe{bbb::dmx::sanitize_universe_id(universe_id)};
        for(int address = 1; address <= bbb::dmx::universe_channel_count; address++) {
            set_channel_target_no_output(sanitized_universe, address, (int)args[start + (std::size_t)(address - 1)], duration);
        }
        output_universe(sanitized_universe);
        schedule_timer_if_needed();
    }

    bool set_channel_target_no_output(int universe_id, int address, int value, double duration) {
        if(address < 1 || bbb::dmx::universe_channel_count < address) {
            report_error("channel address outside 1..512");
            return false;
        }
        fade_universe &universe = ensure_universe(universe_id);
        const std::size_t index{(std::size_t)(address - 1)};
        const double timestamp{now_ms()};
        update_channel(universe, index, timestamp);
        universe.start[index] = universe.current[index];
        universe.target[index] = (double)bbb::dmx::maxutil::clamp_int(value, 0, 255);
        universe.start_ms[index] = timestamp;
        universe.duration_ms[index] = duration;
        universe.active[index] = 0.0 < duration && std::fabs(universe.start[index] - universe.target[index]) > 0.0001;
        if(!universe.active[index]) {
            universe.current[index] = universe.target[index];
        }
        return true;
    }

    void set_channel_target(int universe_id, int address, int value, double duration) {
        const int sanitized_universe{bbb::dmx::sanitize_universe_id(universe_id)};
        if(set_channel_target_no_output(sanitized_universe, address, value, duration)) {
            output_universe(sanitized_universe);
            schedule_timer_if_needed();
        }
    }

    static bool update_channel(fade_universe &universe, std::size_t index, double timestamp) {
        if(!universe.active[index]) {
            return false;
        }
        const double elapsed{timestamp - universe.start_ms[index]};
        const double duration{std::max(1.0, universe.duration_ms[index])};
        const double t{std::max(0.0, std::min(1.0, elapsed / duration))};
        universe.current[index] = universe.start[index] + (universe.target[index] - universe.start[index]) * t;
        if(1.0 <= t) {
            universe.current[index] = universe.target[index];
            universe.active[index] = false;
        }
        return true;
    }

    bool tick_universe(fade_universe &universe, double timestamp) {
        bool changed{false};
        for(std::size_t index = 0; index < (std::size_t)bbb::dmx::universe_channel_count; index++) {
            changed = update_channel(universe, index, timestamp) || changed;
        }
        return changed;
    }

    bool any_active() const {
        for(const auto &universe_entry : universes_) {
            for(const bool active : universe_entry.second.active) {
                if(active) {
                    return true;
                }
            }
        }
        return false;
    }

    void tick() {
        const double timestamp{now_ms()};
        for(auto &universe_entry : universes_) {
            if(tick_universe(universe_entry.second, timestamp)) {
                output_universe(universe_entry.first);
            }
        }
        schedule_timer_if_needed();
    }

    void schedule_timer_if_needed() {
        if(any_active()) {
            timer.delay(std::max(1, 1000 / std::max(1, fps_value_)));
        }
    }

    bbb::dmx::dmx_universe build_universe(int universe_id) const {
        bbb::dmx::dmx_universe result{};
        const auto found = universes_.find(bbb::dmx::sanitize_universe_id(universe_id));
        if(found == universes_.end()) {
            return result;
        }
        for(int address = 1; address <= bbb::dmx::universe_channel_count; address++) {
            const std::size_t index{(std::size_t)(address - 1)};
            result.set_channel(address, (int)std::round(found->second.current[index]));
        }
        return result;
    }

    void output_universe(int universe_id) {
        const int sanitized_universe{bbb::dmx::sanitize_universe_id(universe_id)};
        output.send(bbb::dmx::maxutil::universe_atoms(sanitized_universe, build_universe(sanitized_universe)));
    }

    void output_all() {
        if(universes_.empty()) {
            output_universe(universe_value_);
            return;
        }
        for(const auto &universe_entry : universes_) {
            output_universe(universe_entry.first);
        }
    }

    void report_error(const char *message) {
        cerr << "bbb.dmx.fade: " << message << c74::min::endl;
        status_output.send(bbb::dmx::maxutil::status_atoms("error", message));
    }
};

MIN_EXTERNAL(bbb_dmx_fade);
