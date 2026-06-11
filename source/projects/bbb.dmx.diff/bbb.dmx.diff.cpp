#include "c74_min.h"

#include <bbb/dmx/frame_set.hpp>
#include <bbb/dmx/max_external_utils.hpp>

#include <cstdlib>
#include <string>

class bbb_dmx_diff : public c74::min::object<bbb_dmx_diff> {
private:
    bbb::dmx::dmx_frame_set before_frames_{};
    bbb::dmx::dmx_frame_set after_frames_{};
    int universe_value_{1};
    bool changed_only_value_{true};

public:
    MIN_DESCRIPTION{"Compare two multi-universe DMX frame sets and report channel deltas."};
    MIN_TAGS{"dmx, lighting, diff, compare, universe"};
    MIN_AUTHOR{"2bit"};
    MIN_RELATED{"bbb.dmx.monitor, bbb.dmx.assert, bbb.dmx.record"};

    c74::min::inlet<> input{this, "(a/b/list/universe/compare/clear) DMX diff input"};
    c74::min::outlet<> output{this, "(anything) changed channel records and summary"};
    c74::min::outlet<> status_output{this, "(anything) status and error messages"};

    c74::min::attribute<int> universe{this, "universe", 1,
        c74::min::description{"Default universe for bare list input."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(args.empty() || !bbb::dmx::maxutil::finite_atom(args[0])) {
                return {universe_value_};
            }
            universe_value_ = bbb::dmx::sanitize_universe_id((int)args[0]);
            return {universe_value_};
        }}
    };

    c74::min::attribute<bool> changed_only{this, "changed_only", true,
        c74::min::description{"Output per-channel records only for changed channels."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            changed_only_value_ = args.empty() || ((int)args[0] != 0);
            return {changed_only_value_};
        }}
    };

    c74::min::message<> list_message{this, "list", "Set after frame for default universe.",
        MIN_FUNCTION {
            write_frame(after_frames_, universe_value_, args, 0, "list requires 512 numeric values");
            return {};
        }
    };

    c74::min::message<> a_message{this, "a", "a universe value1 ... value512",
        MIN_FUNCTION {
            write_universe_message(before_frames_, args, "a requires universe id and 512 values");
            return {};
        }
    };

    c74::min::message<> b_message{this, "b", "b universe value1 ... value512",
        MIN_FUNCTION {
            write_universe_message(after_frames_, args, "b requires universe id and 512 values");
            return {};
        }
    };

    c74::min::message<> before_message{this, "before", "before universe value1 ... value512",
        MIN_FUNCTION {
            write_universe_message(before_frames_, args, "before requires universe id and 512 values");
            return {};
        }
    };

    c74::min::message<> after_message{this, "after", "after universe value1 ... value512",
        MIN_FUNCTION {
            write_universe_message(after_frames_, args, "after requires universe id and 512 values");
            return {};
        }
    };

    c74::min::message<> universe_message{this, "universe", "Set after frame: universe id value1 ... value512",
        MIN_FUNCTION {
            write_universe_message(after_frames_, args, "universe requires id and 512 values");
            return {};
        }
    };

    c74::min::message<> compare_message{this, "compare", "Compare all known universes or a selected universe.",
        MIN_FUNCTION {
            if(args.empty()) {
                compare_all();
                return {};
            }
            if(!bbb::dmx::maxutil::finite_atom(args[0])) {
                report_error("compare requires numeric universe id");
                return {};
            }
            compare_universe((int)args[0]);
            return {};
        }
    };

    c74::min::message<> bang_message{this, "bang", "Compare all known universes.",
        MIN_FUNCTION {
            compare_all();
            return {};
        }
    };

    c74::min::message<> clear_message{this, "clear", "Clear before and after frame sets.",
        MIN_FUNCTION {
            before_frames_.clear();
            after_frames_.clear();
            report_status("clear");
            return {};
        }
    };

private:
    void write_universe_message(bbb::dmx::dmx_frame_set &frames, const c74::min::atoms &args, const std::string &error_message) {
        const bbb::dmx::maxutil::frame_write_result result{bbb::dmx::maxutil::write_universe_message(frames, args, error_message)};
        handle_write_result(result);
    }

    void write_frame(bbb::dmx::dmx_frame_set &frames, int universe_id, const c74::min::atoms &args, std::size_t start, const std::string &error_message) {
        const bbb::dmx::maxutil::frame_write_result result{bbb::dmx::maxutil::write_universe_from_atoms(frames, universe_id, args, start, error_message)};
        handle_write_result(result);
    }

    void handle_write_result(const bbb::dmx::maxutil::frame_write_result &result) {
        if(!result.ok) {
            report_error(result.message);
            return;
        }
        c74::min::atoms atoms;
        atoms.push_back(c74::min::symbol("stored"));
        atoms.push_back(result.universe);
        status_output.send(atoms);
    }

    void compare_all() {
        bbb::dmx::dmx_frame_set universe_union{};
        for(const int universe_id : before_frames_.universe_ids()) {
            universe_union.ensure_universe(universe_id);
        }
        for(const int universe_id : after_frames_.universe_ids()) {
            universe_union.ensure_universe(universe_id);
        }
        int total_changed{0};
        for(const int universe_id : universe_union.universe_ids()) {
            total_changed += compare_universe_internal(universe_id);
        }
        c74::min::atoms atoms;
        atoms.push_back(c74::min::symbol("summary"));
        atoms.push_back(c74::min::symbol("universes"));
        atoms.push_back((int)universe_union.universe_ids().size());
        atoms.push_back(c74::min::symbol("changed"));
        atoms.push_back(total_changed);
        output.send(atoms);
    }

    void compare_universe(int universe_id) {
        const int changed_count{compare_universe_internal(universe_id)};
        c74::min::atoms atoms;
        atoms.push_back(c74::min::symbol("summary"));
        atoms.push_back(bbb::dmx::sanitize_universe_id(universe_id));
        atoms.push_back(c74::min::symbol("changed"));
        atoms.push_back(changed_count);
        output.send(atoms);
    }

    int compare_universe_internal(int universe_id) {
        const int sanitized_universe{bbb::dmx::sanitize_universe_id(universe_id)};
        const bbb::dmx::dmx_universe &before = before_frames_.universe(sanitized_universe);
        const bbb::dmx::dmx_universe &after = after_frames_.universe(sanitized_universe);
        int changed_count{0};
        int max_delta{0};
        for(int address = 1; address <= bbb::dmx::universe_channel_count; address++) {
            const int before_value{before.channel(address)};
            const int after_value{after.channel(address)};
            const int delta{after_value - before_value};
            if(delta != 0) {
                changed_count++;
                max_delta = std::max(max_delta, std::abs(delta));
            }
            if(!changed_only_value_ || delta != 0) {
                c74::min::atoms atoms;
                atoms.push_back(c74::min::symbol("channel"));
                atoms.push_back(sanitized_universe);
                atoms.push_back(address);
                atoms.push_back(before_value);
                atoms.push_back(after_value);
                atoms.push_back(delta);
                output.send(atoms);
            }
        }
        c74::min::atoms atoms;
        atoms.push_back(c74::min::symbol("universe"));
        atoms.push_back(sanitized_universe);
        atoms.push_back(c74::min::symbol("changed"));
        atoms.push_back(changed_count);
        atoms.push_back(c74::min::symbol("max_delta"));
        atoms.push_back(max_delta);
        output.send(atoms);
        return changed_count;
    }

    void report_status(const char *message) {
        bbb::dmx::maxutil::send_status(status_output, "status", message);
    }

    void report_error(const std::string &message) {
        cerr << "bbb.dmx.diff: " << message << c74::min::endl;
        bbb::dmx::maxutil::send_status(status_output, "error", message);
    }
};

MIN_EXTERNAL(bbb_dmx_diff);
