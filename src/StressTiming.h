// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <algorithm>
#include <fader/Playback.h>
#include <limits>
#include <optional>
#include <vector>

namespace fader::qualification {
struct HostMetric {
    Frame frame{};
    HostTime target{}, woke{}, returned{}, finished{}, deadline{};
};

// Consumer-thread history. Index by frame before any percentile calculation so
// a dropped or duplicated metric cannot pair a tail with the wrong callback.
// The three exact distributions replace the old append-only vectors. Their
// pages are touched before the timed interval, not in the virtual callback.
class StressTiming {
  public:
    static constexpr std::size_t tailLimit = 256;
    static constexpr HostTime missing = std::numeric_limits<HostTime>::max();
    StressTiming(std::size_t blocks, int blockFrames)
        : callbacks(blocks, missing), wakes(blocks, missing), outer(blocks, missing),
          blockFrames(blockFrames) {
        tails.reserve(tailLimit);
    }
    void callback(const CallbackMetric &metric) {
        const auto index = indexFor(metric.engineFrame);
        if (!index || metric.segment != 9001 || metric.frames != blockFrames ||
            metric.executionNs == missing || callbacks[*index] != missing) {
            ++callbackErrors;
            return;
        }
        callbacks[*index] = metric.executionNs;
        ++callbackSamples;
    }
    void host(const HostMetric &metric) {
        const auto index = indexFor(metric.frame);
        if (!index || metric.target > metric.woke || metric.woke > metric.returned ||
            metric.returned > metric.finished || metric.deadline <= metric.target ||
            wakes[*index] != missing) {
            ++hostErrors;
            return;
        }
        const auto wake = metric.woke - metric.target;
        const auto call = metric.returned - metric.woke;
        wakes[*index] = wake;
        outer[*index] = metric.finished - metric.woke;
        ++hostSamples;
        virtualMisses += unsigned(metric.finished > metric.deadline);
        if (!maximumWake || wake > maximumWake->woke - maximumWake->target)
            maximumWake = metric;
        if (!maximumCall || call > maximumCall->returned - maximumCall->woke)
            maximumCall = metric;
        if (metric.finished > metric.deadline &&
            (!maximumMiss ||
             metric.finished - metric.deadline > maximumMiss->finished - maximumMiss->deadline))
            maximumMiss = metric;
        const auto threshold = (metric.deadline - metric.target) / 2;
        if (wake >= threshold || call >= threshold || metric.finished > metric.deadline) {
            ++tailCount;
            if (tails.size() < tailLimit)
                tails.push_back(metric);
            else
                ++omittedTails;
        }
    }
    std::optional<HostTime> callbackFor(Frame frame) const {
        const auto index = indexFor(frame);
        if (!index || callbacks[*index] == missing)
            return {};
        return callbacks[*index];
    }
    bool allSamplesPresent() const {
        return callbackSamples == callbacks.size() && hostSamples == wakes.size() && callbackErrors == 0 &&
               hostErrors == 0;
    }
    // This is separate from musical capture integrity. A bounded trace can be
    // incomplete even when all exact timing samples and musical events survive.
    bool traceComplete() const {
        return allSamplesPresent() && omittedTails == 0;
    }
    std::vector<HostTime> callbacks, wakes, outer;
    std::vector<HostMetric> tails;
    std::optional<HostMetric> maximumWake, maximumCall, maximumMiss;
    std::size_t callbackSamples = 0, hostSamples = 0, callbackErrors = 0, hostErrors = 0;
    std::size_t tailCount = 0, omittedTails = 0, virtualMisses = 0;

  private:
    std::optional<std::size_t> indexFor(Frame frame) const {
        if (frame < 0 || frame % blockFrames != 0)
            return {};
        const auto index = std::size_t(frame / blockFrames);
        return index < callbacks.size() ? std::optional(index) : std::nullopt;
    }
    const int blockFrames;
};
} // namespace fader::qualification
