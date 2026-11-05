// SPDX-License-Identifier: AGPL-3.0-or-later
#include <algorithm>
#include <chrono>
#include <cmath>
#include <fader/Playback.h>
#include <stdexcept>

namespace fader {
static_assert(std::atomic<HostTime>::is_always_lock_free && std::atomic<unsigned>::is_always_lock_free);
HostTime monotonicTime() noexcept {
    return static_cast<HostTime>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                                     std::chrono::steady_clock::now().time_since_epoch())
                                     .count());
}
Playback::Playback(Project project, Frame start, std::uint64_t serial)
    : engine(std::move(project), start), projectOrigin(start), segmentId(serial) {
    if (serial == 0)
        throw std::runtime_error("A playback segment needs a nonzero identity");
}
void Playback::arm(Frame in, Frame out, unsigned mask, int clip) noexcept {
    engine.arm(in, out, mask, clip);
}
void Playback::markIncomplete(unsigned reason) noexcept {
    failures.fetch_or(reason, std::memory_order_relaxed);
}
bool Playback::submit(InputSource source, ControlInput input) noexcept {
    if (input.segment != segmentId) {
        stale.fetch_add(1, std::memory_order_relaxed);
        return false; // belongs to an old take; does not invalidate the new take
    }
    const auto lane = static_cast<unsigned>(source);
    if (lane >= inputs.size() || static_cast<unsigned>(input.param) >= 3 ||
        input.clip >= engine.plan().clips.size() || !std::isfinite(input.value)) {
        markIncomplete(invalidInput);
        return false;
    }
    if (!inputs[lane].push({input, sequence[lane]++})) {
        dropped.fetch_add(1, std::memory_order_relaxed);
        markIncomplete(inputOverflow);
        return false;
    }
    return true;
}
void Playback::deviceStarted(double sampleRate) noexcept {
    if (started || !std::isfinite(sampleRate) || std::abs(sampleRate - projectRate) > 0.1) {
        markIncomplete(deviceClockChanged);
        running = false;
    } else {
        running = true;
    }
    started = true;
}
void Playback::deviceStopped() noexcept {
    running = false;
    markIncomplete(deviceInterrupted);
}
Frame Playback::elapsedFrames(HostTime eventTime, HostTime blockTime) noexcept {
    const bool positive = eventTime >= blockTime;
    const auto gap = positive ? eventTime - blockTime : blockTime - eventTime;
    // Split before multiplication so even hostile uint64 timestamps cannot overflow.
    const auto whole = (gap / 1000000000) * projectRate;
    const auto fraction = (gap % 1000000000) * projectRate;
    const auto rounded = Frame(whole + (fraction + 500000000) / 1000000000);
    return positive ? rounded : -rounded; // nearest frame; half-frame ties away from zero
}
bool Playback::later(const Pending &a, const Pending &b) noexcept {
    if (a.frame != b.frame)
        return a.frame > b.frame;
    if (a.source != b.source)
        return a.source > b.source; // equal frame: UI first, controller second
    return a.queued.sequence > b.queued.sequence;
}
void Playback::schedule(HostTime blockTime) noexcept {
    // Bound ingestion even if a producer refills while the callback runs.
    for (unsigned budget = 4096; budget > 0; --budget) {
        QueuedInput queued;
        unsigned lane = nextSource;
        if (!inputs[lane].pop(queued)) {
            lane = 1 - lane;
            if (!inputs[lane].pop(queued))
                break;
        }
        nextSource = 1 - lane;
        const auto delta = elapsedFrames(queued.input.receivedAt, blockTime);
        if (delta > 2 * projectRate) {
            markIncomplete(invalidInput); // live input, at most two seconds ahead
            continue;
        }
        if (pendingCount == pending.size()) {
            dropped.fetch_add(1, std::memory_order_relaxed);
            markIncomplete(schedulerOverflow);
            continue;
        }
        pending[pendingCount++] = {elapsed + delta, static_cast<InputSource>(lane), queued};
        std::push_heap(pending.begin(), pending.begin() + pendingCount, later);
    }
}
void Playback::process(float *const *output, int channels, int count, HostTime blockTime) noexcept {
    const auto began = monotonicTime();
    const auto blockStart = elapsed;
    const auto positionStart = engine.position();
    if (running && !clockFailed() && count > 0) {
        schedule(blockTime);
        unsigned budget = 4096;
        while (budget > 0 && pendingCount > 0 && pending[0].frame < elapsed + count) {
            --budget;
            std::pop_heap(pending.begin(), pending.begin() + pendingCount, later);
            const auto next = pending[--pendingCount];
            if (next.frame < elapsed) {
                late.fetch_add(1, std::memory_order_relaxed);
                maximumLate.store(std::max(maximumLate.load(std::memory_order_relaxed), elapsed - next.frame),
                                  std::memory_order_relaxed);
            }
            const auto &input = next.queued.input;
            // Engine capture records the actual project frame after late clamping.
            const Frame projectFrame = std::max(Frame{0}, projectOrigin + next.frame);
            if (!engine.command({projectFrame, input.param, input.value, input.clip})) {
                dropped.fetch_add(1, std::memory_order_relaxed);
                markIncomplete(schedulerOverflow);
            }
        }
        engine.process(output, channels, count);
        elapsed += count; // device frames continue independently of project-end clamping
        if (engine.captureFailed())
            markIncomplete(captureOverflow);
    } else {
        for (int c = 0; c < channels; ++c)
            if (output[c])
                std::fill_n(output[c], std::max(0, count), 0.0f);
    }
    const auto duration = monotonicTime() - began;
    maximumCallback.store(std::max(maximumCallback.load(std::memory_order_relaxed), duration),
                          std::memory_order_relaxed);
    publishedElapsed.store(elapsed, std::memory_order_relaxed);
    blocks.fetch_add(1, std::memory_order_relaxed);
    if (!metrics.push({segmentId, blockStart, positionStart, count, duration, pendingCount}))
        metricDropped.fetch_add(1, std::memory_order_relaxed);
}
PlaybackStats Playback::stats() const noexcept {
    return {segmentId,          blocks.load(),          dropped.load(),          stale.load(),
            late.load(),        metricDropped.load(),   publishedElapsed.load(), engine.position(),
            maximumLate.load(), maximumCallback.load(), failures.load()};
}
} // namespace fader
