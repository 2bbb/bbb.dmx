#include "c74_min.h"

#include <bbb/dmx/build_info.hpp>

#include <bbb/dmx/frame_set.hpp>
#include <bbb/dmx/max_external_utils.hpp>

#include <string>
#include <vector>

class bbb_dmx_monitor : public c74::min::object<bbb_dmx_monitor> {
private:
    bbb::dmx::dmx_frame_set frames_{};
    int default_universe_value_{1};
    bool changed_only_value_{false};

public:
    MIN_DESCRIPTION{"Monitor multi-universe DMX frames and output full universes or changed channel pairs."};
    MIN_TAGS{"dmx, lighting, monitor, universe, debug"};
    MIN_AUTHOR{"2bit"};
    MIN_RELATED{"bbb.dmx.fixturemap, bbb.dmx.merge"};

    bbb_dmx_monitor() {
        bbb::dmx::report_external_build_info(cout, "bbb.dmx.monitor");
    }

    c74::min::inlet<> input{this, "(list/universe/channel/channels/bang/dump) DMX monitor input"};
    c74::min::outlet<> output{this, "(anything) universe or changed channel data"};
    c74::min::outlet<> status_output{this, "(anything) status and error messages"};

    c74::min::attribute<int> default_universe{this, "default_universe", 1,
        c74::min::description{"Default universe for bare 512-value list input and bang output."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(args.empty() || !bbb::dmx::maxutil::finite_atom(args[0])) {
                return {default_universe_value_};
            }
            default_universe_value_ = bbb::dmx::sanitize_universe_id((int)args[0]);
            return {default_universe_value_};
        }}
    };

    c74::min::attribute<bool> changed_only{this, "changed_only", false,
        c74::min::description{"If non-zero, list/universe input outputs only changed address/value pairs."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            changed_only_value_ = !args.empty() && ((int)args[0] != 0);
            return {changed_only_value_};
        }}
    };

    c74::min::message<> list_message{this, "list", "512 DMX byte values for the default universe.",
        MIN_FUNCTION {
            update_universe(default_universe_value_, args, 0);
            return {};
        }
    };

    c74::min::message<> universe_message{this, "universe", "universe id value1 ... value512",
        MIN_FUNCTION {
            if(args.size() < 513 || !bbb::dmx::maxutil::finite_atom(args[0])) {
                report_error("universe requires id and 512 values");
                return {};
            }
            update_universe(bbb::dmx::sanitize_universe_id((int)args[0]), args, 1);
            return {};
        }
    };

    c74::min::message<> channel_message{this, "channel", "channel universe address value",
        MIN_FUNCTION {
            if(args.size() < 3 || !bbb::dmx::maxutil::finite_atoms(args, 0, 3)) {
                report_error("channel requires universe address value");
                return {};
            }
            const int universe_id{bbb::dmx::sanitize_universe_id((int)args[0])};
            const int address{(int)args[1]};
            const int value{(int)args[2]};
            const bbb::dmx::dmx_universe before{frames_.universe(universe_id)};
            const bbb::dmx::write_result result{frames_.set_channel(universe_id, address, value)};
            if(!result.ok) {
                report_error(result.message);
                return {};
            }
            output_changes_or_universe(universe_id, before);
            return {};
        }
    };

    c74::min::message<> channels_message{this, "channels", "channels universe address value ...",
        MIN_FUNCTION {
            if(args.size() < 3 || ((args.size() - 1) % 2) != 0 || !bbb::dmx::maxutil::finite_atom(args[0])) {
                report_error("channels requires universe and address/value pairs");
                return {};
            }
            const int universe_id{bbb::dmx::sanitize_universe_id((int)args[0])};
            const bbb::dmx::dmx_universe before{frames_.universe(universe_id)};
            for(std::size_t index = 1; index < args.size(); index += 2) {
                if(!bbb::dmx::maxutil::finite_atoms(args, index, 2)) {
                    report_error("channels pair must be numeric");
                    return {};
                }
                const bbb::dmx::write_result result{frames_.set_channel(universe_id, (int)args[index], (int)args[index + 1])};
                if(!result.ok) {
                    report_error(result.message);
                    return {};
                }
            }
            output_changes_or_universe(universe_id, before);
            return {};
        }
    };

    c74::min::message<> bang_message{this, "bang", "Output the default universe." ,
        MIN_FUNCTION {
            output.send(bbb::dmx::maxutil::universe_atoms(default_universe_value_, frames_.universe(default_universe_value_)));
            return {};
        }
    };

    c74::min::message<> bangall_message{this, "bangall", "Output every known universe." ,
        MIN_FUNCTION {
            output_all_universes();
            return {};
        }
    };

    c74::min::message<> dump_message{this, "dump", "Output known universe ids from the status outlet." ,
        MIN_FUNCTION {
            c74::min::atoms atoms;
            atoms.push_back(c74::min::symbol("universes"));
            for(const int universe_id : frames_.universe_ids()) {
                atoms.push_back(universe_id);
            }
            status_output.send(atoms);
            return {};
        }
    };

    c74::min::message<> clear_message{this, "clear", "Clear all stored universe data." ,
        MIN_FUNCTION {
            frames_.clear();
            status_output.send(bbb::dmx::maxutil::status_atoms("status", "clear"));
            return {};
        }
    };

private:
    void update_universe(int universe_id, const c74::min::atoms &args, std::size_t start) {
        if(args.size() < start + (std::size_t)bbb::dmx::universe_channel_count || !bbb::dmx::maxutil::finite_atoms(args, start, bbb::dmx::universe_channel_count)) {
            report_error("universe input requires 512 numeric values");
            return;
        }
        const bbb::dmx::dmx_universe before{frames_.universe(universe_id)};
        const bbb::dmx::write_result result{frames_.set_universe(universe_id, bbb::dmx::maxutil::values_from_atoms(args, start))};
        if(!result.ok) {
            report_error(result.message);
            return;
        }
        output_changes_or_universe(universe_id, before);
    }

    void output_changes_or_universe(int universe_id, const bbb::dmx::dmx_universe &before) {
        if(changed_only_value_) {
            const std::vector<int> changes{bbb::dmx::changed_channels(before, frames_.universe(universe_id))};
            if(!changes.empty()) {
                output.send(bbb::dmx::maxutil::changed_atoms(universe_id, changes));
            }
            return;
        }
        output.send(bbb::dmx::maxutil::universe_atoms(universe_id, frames_.universe(universe_id)));
    }

    void output_all_universes() {
        const std::vector<int> ids{frames_.universe_ids()};
        if(ids.empty()) {
            output.send(bbb::dmx::maxutil::universe_atoms(default_universe_value_, frames_.universe(default_universe_value_)));
            return;
        }
        for(const int universe_id : ids) {
            output.send(bbb::dmx::maxutil::universe_atoms(universe_id, frames_.universe(universe_id)));
        }
    }

    void report_error(const char *message) {
        cerr << "bbb.dmx.monitor: " << message << c74::min::endl;
        status_output.send(bbb::dmx::maxutil::status_atoms("error", message));
    }
};

MIN_EXTERNAL(bbb_dmx_monitor);
