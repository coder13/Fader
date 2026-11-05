// SPDX-License-Identifier: AGPL-3.0-or-later
#include "StressClock.h"
#include <cerrno>
#include <limits>
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <time.h>
#endif

namespace fader::qualification {
namespace {
constexpr unsigned unsupported = 0xffffffffu, overflow = 0xfffffffeu;
#if !defined(_WIN32) && defined(CLOCK_THREAD_CPUTIME_ID)
#if defined(__APPLE__)
__attribute__((availability(macos, introduced = 10.12)))
#endif
bool posixClock(bool resolution, HostTime &value, unsigned &error) noexcept {
    timespec time{};
    const int status = resolution ? clock_getres(CLOCK_THREAD_CPUTIME_ID, &time)
                                  : clock_gettime(CLOCK_THREAD_CPUTIME_ID, &time);
    if (status != 0) {
        error = unsigned(errno);
        return false;
    }
    if (time.tv_sec < 0 || time.tv_nsec < 0 || time.tv_nsec >= 1000000000 ||
        HostTime(time.tv_sec) >
            (std::numeric_limits<HostTime>::max() - HostTime(time.tv_nsec)) / 1000000000) {
        error = overflow;
        return false;
    }
    value = HostTime(time.tv_sec) * 1000000000 + HostTime(time.tv_nsec);
    return true;
}
#endif
bool cpuValue(bool resolution, HostTime &value, unsigned &error) noexcept {
#if defined(_WIN32)
    if (resolution) {
        error = unsupported; // Units are not a guarantee of sampling precision.
        return false;
    }
    FILETIME created{}, exited{}, kernel{}, user{};
    if (!GetThreadTimes(GetCurrentThread(), &created, &exited, &kernel, &user)) {
        error = unsigned(GetLastError());
        return false;
    }
    const auto units = [](const FILETIME &time) {
        return (HostTime(time.dwHighDateTime) << 32) | time.dwLowDateTime;
    };
    const auto k = units(kernel), u = units(user);
    if (k > std::numeric_limits<HostTime>::max() - u || k + u > std::numeric_limits<HostTime>::max() / 100) {
        error = overflow;
        return false;
    }
    value = (k + u) * 100;
    return true;
#elif defined(CLOCK_THREAD_CPUTIME_ID)
#if defined(__APPLE__)
    if (__builtin_available(macOS 10.12, *))
        return posixClock(resolution, value, error);
    error = unsupported;
    return false;
#else
    return posixClock(resolution, value, error);
#endif
#else
    (void)resolution;
    (void)value;
    error = unsupported;
    return false;
#endif
}
} // namespace
CpuReading readThreadCpu() noexcept {
    CpuReading result;
    result.before = monotonicTime();
    result.valid = cpuValue(false, result.cpu, result.error);
    result.after = monotonicTime();
    return result;
}
CpuClockInfo threadCpuClockInfo() noexcept {
#if defined(_WIN32)
    CpuClockInfo info{"GetThreadTimes user+kernel; 100 ns units, precision not guaranteed", 100};
#elif defined(CLOCK_THREAD_CPUTIME_ID)
    CpuClockInfo info{"clock_gettime(CLOCK_THREAD_CPUTIME_ID); user+system thread CPU", 1};
#else
    CpuClockInfo info{"Thread CPU clock unsupported", 0};
#endif
    cpuValue(true, info.reportedResolutionNanoseconds, info.resolutionError);
    return info;
}
} // namespace fader::qualification
