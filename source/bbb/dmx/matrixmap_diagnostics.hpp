#pragma once

namespace bbb::dmx::matrixmap {

class autobang_warning_gate {
private:
    bool warning_emitted_{false};

public:
    void reset() {
        warning_emitted_ = false;
    }

    bool should_report(bool autobang_enabled) {
        if(autobang_enabled) {
            reset();
            return false;
        }
        if(warning_emitted_) {
            return false;
        }
        warning_emitted_ = true;
        return true;
    }
};

} // namespace bbb::dmx::matrixmap
