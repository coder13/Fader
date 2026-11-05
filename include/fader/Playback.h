// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <fader/Engine.h>

namespace fader {
using HostTime = std::uint64_t; // steady-clock nanoseconds, not wall-clock time
HostTime monotonicTime() noexcept;
enum class InputSource : unsigned { ui, controller, count };
enum PlaybackFailure : unsigned {
    inputOverflow = 1,
    schedulerOverflow = 2,
    captureOverflow = 4,
    invalidInput = 8,
    deviceInterrupted = 16,
    deviceClockChanged = 32,
    captureStorageFull = 64
};
struct ControlInput {
    std::uint64_t segment{};
    HostTime receivedAt{};
    Param param{};
    float value{};
    unsigned clip{};
};
struct CallbackMetric {
    std::uint64_t segment{};
    Frame engineFrame{}, projectFrame{};
    int frames{};
    HostTime executionNs{};
    std::size_t scheduled{};
};
struct PlaybackStats {
    std::uint64_t segment{}, blocks{}, inputDrops{}, staleInputs{}, lateInputs{}, metricDrops{};
    Frame engineFrame{}, projectFrame{}, maxLateFrames{};
    HostTime maxCallbackNs{};
    unsigned failures{};
};

// One producer per input lane; the controller lane does not visit the UI thread.
// The owner stops producers and detaches the audio callback before destruction.
class Playback {
  public:
    Playback(Project, Frame projectStart, std::uint64_t segment);
    void arm(Frame in, Frame out, unsigned mask = 7, int clip = -1) noexcept;
    bool submit(InputSource, ControlInput) noexcept;
    // Device lifecycle callbacks are serialized with process by the native host.
    // A restart/reconfiguration invalidates this segment; construct a new one.
    void deviceStarted(double sampleRate) noexcept;
    void deviceStopped() noexcept;
    void process(float *const *output, int channels, int frames, HostTime blockTime) noexcept;
    void markIncomplete(unsigned reason) noexcept;
    bool captured(Event &event) noexcept {
        return engine.captured(event);
    }
    bool metric(CallbackMetric &value) noexcept {
        return metrics.pop(value);
    }
    Frame position() const noexcept {
        return engine.position();
    }
    std::uint64_t starvationFrames() const noexcept {
        return engine.starvationFrames();
    }
    StreamStats streamStats() const noexcept {
        return engine.streamStats();
    }
    void cancelSourceReads() noexcept {
        engine.cancelSourceReads();
    }
    float peak() const noexcept {
        return engine.peak();
    }
    bool captureFailed() const noexcept {
        return failures.load(std::memory_order_relaxed) != 0;
    }
    bool clockFailed() const noexcept {
        return (failures.load(std::memory_order_relaxed) & (deviceInterrupted | deviceClockChanged)) != 0;
    }
    std::uint64_t segment() const noexcept {
        return segmentId;
    }
    const Project &plan() const noexcept {
        return engine.plan();
    }
    PlaybackStats stats() const noexcept; // approximate atomic snapshot, no reader retry

  private:
    struct QueuedInput {
        ControlInput input;
        std::uint64_t sequence{};
    };
    struct Pending {
        Frame frame{};
        InputSource source{};
        QueuedInput queued;
    };
    static bool later(const Pending &, const Pending &) noexcept;
    void schedule(HostTime blockTime) noexcept;
    static Frame elapsedFrames(HostTime eventTime, HostTime blockTime) noexcept;
    Engine engine;
    const Frame projectOrigin;
    const std::uint64_t segmentId;
    std::array<Queue<QueuedInput, 4096>, 2> inputs;
    std::array<std::uint64_t, 2> sequence{}; // each slot belongs to its producer
    std::array<Pending, 4096> pending{};
    std::size_t pendingCount = 0;
    unsigned nextSource = 0;
    Queue<CallbackMetric, 1024> metrics;
    Frame elapsed = 0;
    bool started = false, running = false;
    std::atomic<unsigned> failures{0};
    std::atomic<std::uint64_t> blocks{0}, dropped{0}, stale{0}, late{0}, metricDropped{0};
    std::atomic<Frame> publishedElapsed{0}, maximumLate{0};
    std::atomic<HostTime> maximumCallback{0};
};
} // namespace fader
