#include "c74_min.h"

#include <bbb/dmx/build_info.hpp>

#include <bbb/dmx/frame_set.hpp>
#include <bbb/dmx/max_external_utils.hpp>

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

class bbb_dmx_record : public c74::min::object<bbb_dmx_record> {
private:
    bbb::dmx::dmx_frame_set current_{};
    std::vector<bbb::dmx::dmx_frame_set> frames_{};
    int universe_value_{1};
    int fps_value_{30};
    bool loop_value_{false};
    bool recording_{false};
    bool playing_{false};
    std::size_t play_index_{0};

public:
    MIN_DESCRIPTION{"Record and play back multi-universe DMX frames."};
    MIN_TAGS{"dmx, lighting, record, playback, universe"};
    MIN_AUTHOR{"2bit"};
    MIN_RELATED{"bbb.dmx.monitor, bbb.dmx.merge, bbb.dmx.fade"};

    bbb_dmx_record() {
        bbb::dmx::report_external_build_info(cout, "bbb.dmx.record");
    }

    c74::min::inlet<> input{this, "(universe/list/record/play/write/read/bang) recorder input"};
    c74::min::outlet<> output{this, "(anything) universe id and 512 DMX bytes"};
    c74::min::outlet<> status_output{this, "(anything) status and error messages"};

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

    c74::min::attribute<int> fps{this, "fps", 30,
        c74::min::description{"Playback frames per second."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            if(args.empty() || !bbb::dmx::maxutil::finite_atom(args[0])) {
                return {fps_value_};
            }
            fps_value_ = std::max(1, std::min(120, (int)args[0]));
            return {fps_value_};
        }}
    };

    c74::min::attribute<bool> loop{this, "loop", false,
        c74::min::description{"Loop playback when enabled."},
        c74::min::setter{[this](const c74::min::atoms &args, int) -> c74::min::atoms {
            loop_value_ = !args.empty() && ((int)args[0] != 0);
            return {loop_value_};
        }}
    };

    c74::min::timer<c74::min::timer_options::defer_delivery> playback_timer{this,
        MIN_FUNCTION {
            playback_tick();
            return {};
        }
    };

    c74::min::message<> list_message{this, "list", "512 DMX bytes for the default universe",
        MIN_FUNCTION {
            receive_universe(universe_value_, bbb::dmx::maxutil::values_from_atoms(args, 0));
            return {};
        }
    };

    c74::min::message<> universe_message{this, "universe", "universe id value1 ... value512",
        MIN_FUNCTION {
            if(args.size() < 513 || !bbb::dmx::maxutil::finite_atom(args[0])) {
                report_error("universe requires id and 512 values");
                return {};
            }
            receive_universe((int)args[0], bbb::dmx::maxutil::values_from_atoms(args, 1));
            return {};
        }
    };

    c74::min::message<> channel_message{this, "channel", "channel universe address value",
        MIN_FUNCTION {
            if(args.size() < 3 || !bbb::dmx::maxutil::finite_atoms(args, 0, 3)) {
                report_error("channel requires universe address value");
                return {};
            }
            const bbb::dmx::write_result result{current_.set_channel((int)args[0], (int)args[1], (int)args[2])};
            if(!handle_write_result(result)) {
                return {};
            }
            capture_if_recording();
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
            for(std::size_t index = 1; index < args.size(); index += 2) {
                if(!bbb::dmx::maxutil::finite_atom(args[index]) || !bbb::dmx::maxutil::finite_atom(args[index + 1])) {
                    report_error("channels pair must be numeric");
                    return {};
                }
                const bbb::dmx::write_result result{current_.set_channel(universe_id, (int)args[index], (int)args[index + 1])};
                if(!handle_write_result(result)) {
                    return {};
                }
            }
            capture_if_recording();
            output_universe(universe_id);
            return {};
        }
    };

    c74::min::message<> record_message{this, "record", "record 0|1",
        MIN_FUNCTION {
            recording_ = args.empty() || ((int)args[0] != 0);
            report_status(recording_ ? "recording" : "record_stopped");
            return {};
        }
    };

    c74::min::message<> play_message{this, "play", "play 0|1",
        MIN_FUNCTION {
            const bool should_play{args.empty() || ((int)args[0] != 0)};
            if(should_play) {
                start_playback();
            } else {
                stop_playback();
            }
            return {};
        }
    };

    c74::min::message<> clear_message{this, "clear", "Clear recorded frames and current DMX state.",
        MIN_FUNCTION {
            stop_playback();
            frames_.clear();
            current_.clear();
            report_status("clear");
            return {};
        }
    };

    c74::min::message<> bang_message{this, "bang", "Output current default universe.",
        MIN_FUNCTION {
            output_universe(universe_value_);
            return {};
        }
    };

    c74::min::message<> bangall_message{this, "bangall", "Output all current universes.",
        MIN_FUNCTION {
            output_all(current_);
            return {};
        }
    };

    c74::min::message<> frame_message{this, "frame", "frame index",
        MIN_FUNCTION {
            if(args.empty() || !bbb::dmx::maxutil::finite_atom(args[0])) {
                report_error("frame requires index");
                return {};
            }
            const int requested_index{(int)args[0]};
            if(requested_index < 0 || frames_.size() <= (std::size_t)requested_index) {
                report_error("frame index outside recorded range");
                return {};
            }
            output_all(frames_[(std::size_t)requested_index]);
            return {};
        }
    };

    c74::min::message<> write_message{this, "write", "write path",
        MIN_FUNCTION {
            if(args.empty()) {
                report_error("write requires file path");
                return {};
            }
            write_frames(bbb::dmx::maxutil::symbol_arg(args[0]));
            return {};
        }
    };

    c74::min::message<> read_message{this, "read", "read path",
        MIN_FUNCTION {
            if(args.empty()) {
                report_error("read requires file path");
                return {};
            }
            read_frames(bbb::dmx::maxutil::symbol_arg(args[0]));
            return {};
        }
    };

    c74::min::message<> dump_message{this, "dump", "Output recorder status.",
        MIN_FUNCTION {
            c74::min::atoms atoms;
            atoms.push_back(c74::min::symbol("status"));
            atoms.push_back(c74::min::symbol("frames"));
            atoms.push_back((int)frames_.size());
            atoms.push_back(c74::min::symbol("universes"));
            atoms.push_back((int)current_.universes.size());
            atoms.push_back(c74::min::symbol("recording"));
            atoms.push_back(recording_ ? 1 : 0);
            atoms.push_back(c74::min::symbol("playing"));
            atoms.push_back(playing_ ? 1 : 0);
            status_output.send(atoms);
            return {};
        }
    };

private:
    void receive_universe(int universe_id, const std::vector<int> &values) {
        const bbb::dmx::write_result result{current_.set_universe(universe_id, values)};
        if(!handle_write_result(result)) {
            return;
        }
        capture_if_recording();
        output_universe(universe_id);
    }

    void capture_if_recording() {
        if(recording_) {
            frames_.push_back(current_);
        }
    }

    void start_playback() {
        if(frames_.empty()) {
            report_error("play requires at least one recorded frame");
            return;
        }
        playing_ = true;
        play_index_ = 0;
        report_status("playing");
        playback_timer.delay(0);
    }

    void stop_playback() {
        playing_ = false;
        playback_timer.stop();
        report_status("play_stopped");
    }

    void playback_tick() {
        if(!playing_) {
            return;
        }
        if(frames_.empty()) {
            stop_playback();
            return;
        }
        if(frames_.size() <= play_index_) {
            if(loop_value_) {
                play_index_ = 0;
            } else {
                stop_playback();
                return;
            }
        }
        current_ = frames_[play_index_];
        output_all(current_);
        play_index_++;
        if(playing_) {
            playback_timer.delay(std::max(1, 1000 / std::max(1, fps_value_)));
        }
    }

    void output_universe(int universe_id) {
        output.send(bbb::dmx::maxutil::universe_atoms(bbb::dmx::sanitize_universe_id(universe_id), current_.universe(universe_id)));
    }

    void output_all(const bbb::dmx::dmx_frame_set &frame_set) {
        for(const int universe_id : frame_set.universe_ids()) {
            output.send(bbb::dmx::maxutil::universe_atoms(universe_id, frame_set.universe(universe_id)));
        }
    }

    void write_frames(const std::string &path) {
        std::ofstream output_file(path);
        if(!output_file.good()) {
            report_error("failed to open record file for writing");
            return;
        }
        output_file << "bbb.dmx.record 1\n";
        for(std::size_t frame_index = 0; frame_index < frames_.size(); frame_index++) {
            for(const auto &entry : frames_[frame_index].universes) {
                output_file << "frame " << frame_index << " universe " << entry.first;
                for(const int value : entry.second.to_int_vector()) {
                    output_file << ' ' << value;
                }
                output_file << '\n';
            }
        }
        report_status("written");
    }

    void read_frames(const std::string &path) {
        std::ifstream input_file(path);
        if(!input_file.good()) {
            report_error("failed to open record file for reading");
            return;
        }
        std::vector<bbb::dmx::dmx_frame_set> loaded_frames;
        std::string line;
        while(std::getline(input_file, line)) {
            if(line.empty() || line[0] == '#') {
                continue;
            }
            std::istringstream stream(line);
            std::string frame_token;
            stream >> frame_token;
            if(frame_token != "frame") {
                continue;
            }
            int frame_index{0};
            std::string universe_token;
            int universe_id{1};
            stream >> frame_index >> universe_token >> universe_id;
            if(frame_index < 0 || universe_token != "universe") {
                report_error("invalid record line");
                return;
            }
            std::vector<int> values;
            values.reserve((std::size_t)bbb::dmx::universe_channel_count);
            int value{0};
            while(stream >> value) {
                values.push_back(value);
            }
            if(values.size() < (std::size_t)bbb::dmx::universe_channel_count) {
                report_error("record line has fewer than 512 values");
                return;
            }
            if(loaded_frames.size() <= (std::size_t)frame_index) {
                loaded_frames.resize((std::size_t)frame_index + 1);
            }
            const bbb::dmx::write_result result{loaded_frames[(std::size_t)frame_index].set_universe(universe_id, values)};
            if(!result.ok) {
                report_error(result.message);
                return;
            }
        }
        frames_ = loaded_frames;
        if(!frames_.empty()) {
            current_ = frames_.front();
        }
        report_status("read");
    }

    bool handle_write_result(const bbb::dmx::write_result &result) {
        if(result.ok) {
            return true;
        }
        report_error(result.message);
        return false;
    }

    void report_status(const char *message) {
        status_output.send(bbb::dmx::maxutil::status_atoms("status", message));
    }

    void report_error(const char *message) {
        cerr << "bbb.dmx.record: " << message << c74::min::endl;
        status_output.send(bbb::dmx::maxutil::status_atoms("error", message));
    }
};

MIN_EXTERNAL(bbb_dmx_record);
