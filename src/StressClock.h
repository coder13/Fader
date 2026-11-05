// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <fader/Playback.h>

namespace fader::qualification {
// The CPU reading occurs somewhere inside [before, after]. Keep that wall
// bracket rather than pretending the two clocks are sampled simultaneously.
struct CpuReading {
    HostTime before{}, after{}, cpu{};
    unsigned error = 0;
    bool valid = false;
};
struct CpuObservation {
    CpuReading first, last;
    bool valid() const noexcept {
        return first.valid && last.valid && first.before <= first.after && first.after <= last.before &&
               last.before <= last.after && first.cpu <= last.cpu;
    }
    HostTime elapsed() const noexcept {
        return last.cpu - first.cpu; // call only after valid()
    }
};
struct CpuClockInfo {
    const char *name;
    HostTime unitNanoseconds{}, reportedResolutionNanoseconds{};
    unsigned resolutionError = 0;
};
CpuReading readThreadCpu() noexcept;
CpuClockInfo threadCpuClockInfo() noexcept;
} // namespace fader::qualification
