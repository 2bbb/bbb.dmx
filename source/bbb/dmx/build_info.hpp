#pragma once

#include "c74_min.h"

#include <bbb/dmx/build_info_generated.hpp>

namespace bbb::dmx {

inline void report_external_build_info(c74::min::logger &output, const char *external_name) {
    output << external_name << " bbb.dmx "
        << bbb::dmx::build_info::package_version
        << " commit " << bbb::dmx::build_info::git_commit
        << " core " << bbb::dmx::build_info::core_git_commit
        << " " << bbb::dmx::build_info::git_dirty
        << " built " << bbb::dmx::build_info::build_time_utc
        << c74::min::endl;
}

} // namespace bbb::dmx
