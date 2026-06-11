#include "c74_min.h"

#include <bbb/dmx/frame_set.hpp>
#include <bbb/dmx/max_external_utils.hpp>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <map>
#include <vector>

class bbb_dmx_safety : public c74::min::object<bbb_dmx_safety> {
private:
    bbb::dmx::dmx_frame_set output_frames_{};
    int universe_value_{1};
    int max_value_{255};
    int max_delta_{255};
    int timeout_ms_value_{0};
    bool blackout_value_{false};
    bool freeze_value_{false};
    double last_input_ms_{0.0};

public:
    MIN_DESCRIPTION{"Apply DMX safety limits, freeze, blackout, slew limiting, and deadman timeout across universes."};
    MIN_TAGS{"dmx, lighting, safety, blackout, freeze, universe"};
    MIN_AUTHOR{"2bit"};
    MIN_RELATED{"bbb.dmx.generator, bbb.dmx.merge"};

    c74::min::inlet<> input{this, "(list/universe/channel/channels/blackout/freeze/bang) protected DMX input"};
    c74::min::outlet<> output{this, "(anything) protected universe data"};
    c74::min::outlet<> status_output{this, "(anything) status and error messages"};

    c74::min::timer<c74::min::timer_options::defer_delivery> timer{this,
        MIN_FUNCTION {
            check_timeout();
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

    c74::min::attribute<int> max_value{this, "max_value", 255,
        c74::min::description{"Maximum allowed DMX byte value."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(args.empty() || !bbb::dmx::maxutil::finite_atom(args[0])) {
                return {max_value_};
            }
            max_value_ = bbb::dmx::maxutil::clamp_int((int)args[0], 0, 255);
            return {max_value_};
        }}
    };

    c74::min::attribute<int> max_delta{this, "max_delta", 255,
        c74::min::description{"Maximum per-update channel delta. 255 disables slew limiting."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(args.empty() || !bbb::dmx::maxutil::finite_atom(args[0])) {
                return {max_delta_};
            }
            max_delta_ = bbb::dmx::maxutil::clamp_int((int)args[0], 0, 255);
            return {max_delta_};
        }}
    };

    c74::min::attribute<int> timeout_ms{this, "timeout_ms", 0,
        c74::min::description{"Deadman timeout in ms. 0 disables timeout. Timeout outputs blackout."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(args.empty() || !bbb::dmx::maxutil::finite_atom(args[0])) {
                return {timeout_ms_value_};
            }
            timeout_ms_value_ = std::max(0, (int)args[0]);
            schedule_timeout_check();
            return {timeout_ms_value_};
        }}
    };

    c74::min::attribute<bool> blackout{this, "blackout", false,
        c74::min::description{"Force all output channels to 0."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            blackout_value_ = !args.empty() && ((int)args[0] != 0);
            output_all();
            return {blackout_value_};
        }}
    };

    c74::min::attribute<bool> freeze{this, "freeze", false,
        c74::min::description{"Hold current protected output and ignore incoming value changes."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            freeze_value_ = !args.empty() && ((int)args[0] != 0);
            return {freeze_value_};
        }}
    };

    c74::min::message<> list_message{this, "list", "512 values for default universe." ,
        MIN_FUNCTION {
            handle_universe(universe_value_, args, 0);
            return {};
        }
    };

    c74::min::message<> universe_message{this, "universe", "universe id value1 ... value512",
        MIN_FUNCTION {
            if(args.size() < 513 || !bbb::dmx::maxutil::finite_atom(args[0])) {
                report_error("universe requires id and 512 values");
                return {};
            }
            handle_universe((int)args[0], args, 1);
            return {};
        }
    };

    c74::min::message<> channel_message{this, "channel", "channel universe address value",
        MIN_FUNCTION {
            if(args.size() < 3 || !bbb::dmx::maxutil::finite_atoms(args, 0, 3)) {
                report_error("channel requires universe address value");
                return {};
            }
            mark_input();
            if(!freeze_value_) {
                set_protected_channel((int)args[0], (int)args[1], (int)args[2]);
            }
            output_universe((int)args[0]);
            return {};
        }
    };

    c74::min::message<> channels_message{this, "channels", "channels universe address value ...",
        MIN_FUNCTION {
            if(args.size() < 3 || ((args.size() - 1) % 2) != 0 || !bbb::dmx::maxutil::finite_atom(args[0])) {
                report_error("channels requires universe and address/value pairs");
                return {};
            }
            const int universe_id{(int)args[0]};
            mark_input();
            if(!freeze_value_) {
                for(std::size_t index = 1; index < args.size(); index += 2) {
                    if(!bbb::dmx::maxutil::finite_atoms(args, index, 2)) {
                        report_error("channels pair must be numeric");
                        return {};
                    }
                    set_protected_channel(universe_id, (int)args[index], (int)args[index + 1]);
                }
            }
            output_universe(universe_id);
            return {};
        }
    };

    c74::min::message<> blackout_message{this, "blackout", "blackout 0|1",
        MIN_FUNCTION {
            blackout_value_ = !args.empty() && ((int)args[0] != 0);
            output_all();
            return {};
        }
    };

    c74::min::message<> freeze_message{this, "freeze", "freeze 0|1",
        MIN_FUNCTION {
            freeze_value_ = !args.empty() && ((int)args[0] != 0);
            return {};
        }
    };

    c74::min::message<> bang_message{this, "bang", "Output default universe." ,
        MIN_FUNCTION {
            output_universe(universe_value_);
            return {};
        }
    };

    c74::min::message<> bangall_message{this, "bangall", "Output all universes." ,
        MIN_FUNCTION {
            output_all();
            return {};
        }
    };

private:
    static double now_ms() {
        using clock = std::chrono::steady_clock;
        return (double)std::chrono::duration_cast<std::chrono::milliseconds>(clock::now().time_since_epoch()).count();
    }

    void mark_input() {
        last_input_ms_ = now_ms();
        schedule_timeout_check();
    }

    void schedule_timeout_check() {
        if(0 < timeout_ms_value_) {
            timer.delay(std::max(1, timeout_ms_value_ / 2));
        }
    }

    void check_timeout() {
        if(0 < timeout_ms_value_ && 0.0 < last_input_ms_ && (double)timeout_ms_value_ < now_ms() - last_input_ms_) {
            blackout_value_ = true;
            status_output.send(bbb::dmx::maxutil::status_atoms("status", "timeout_blackout"));
            output_all();
            return;
        }
        schedule_timeout_check();
    }

    void handle_universe(int universe_id, const c74::min::atoms &args, std::size_t start) {
        if(args.size() < start + (std::size_t)bbb::dmx::universe_channel_count || !bbb::dmx::maxutil::finite_atoms(args, start, bbb::dmx::universe_channel_count)) {
            report_error("universe input requires 512 numeric values");
            return;
        }
        mark_input();
        const int sanitized_universe{bbb::dmx::sanitize_universe_id(universe_id)};
        if(!freeze_value_) {
            for(int address = 1; address <= bbb::dmx::universe_channel_count; address++) {
                set_protected_channel(sanitized_universe, address, (int)args[start + (std::size_t)(address - 1)]);
            }
        }
        output_universe(sanitized_universe);
    }

    int protected_value(int current_value, int requested_value) const {
        if(blackout_value_) {
            return 0;
        }
        int value{bbb::dmx::maxutil::clamp_int(requested_value, 0, max_value_)};
        if(max_delta_ < 255) {
            const int minimum{std::max(0, current_value - max_delta_)};
            const int maximum{std::min(max_value_, current_value + max_delta_)};
            value = bbb::dmx::maxutil::clamp_int(value, minimum, maximum);
        }
        return value;
    }

    void set_protected_channel(int universe_id, int address, int value) {
        const int sanitized_universe{bbb::dmx::sanitize_universe_id(universe_id)};
        const int current_value{output_frames_.universe(sanitized_universe).channel(address)};
        const bbb::dmx::write_result result{output_frames_.set_channel(sanitized_universe, address, protected_value(current_value, value))};
        if(!result.ok) {
            report_error(result.message);
        }
    }

    bbb::dmx::dmx_universe output_view(int universe_id) const {
        if(!blackout_value_) {
            return output_frames_.universe(universe_id);
        }
        bbb::dmx::dmx_universe universe{};
        universe.clear(0);
        return universe;
    }

    void output_universe(int universe_id) {
        const int sanitized_universe{bbb::dmx::sanitize_universe_id(universe_id)};
        output.send(bbb::dmx::maxutil::universe_atoms(sanitized_universe, output_view(sanitized_universe)));
    }

    void output_all() {
        const std::vector<int> ids{output_frames_.universe_ids()};
        if(ids.empty()) {
            output_universe(universe_value_);
            return;
        }
        for(const int universe_id : ids) {
            output_universe(universe_id);
        }
    }

    void report_error(const char *message) {
        cerr << "bbb.dmx.safety: " << message << c74::min::endl;
        status_output.send(bbb::dmx::maxutil::status_atoms("error", message));
    }
};

MIN_EXTERNAL(bbb_dmx_safety);
