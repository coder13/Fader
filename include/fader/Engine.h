// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <fader/Streaming.h>
#include <memory>
#include <string>
#include <vector>

namespace fader {
using Frame = std::int64_t;
constexpr int projectRate = 48000;
constexpr Frame maxProjectFrames = Frame{24} * 3600 * projectRate;
constexpr Frame maxSourceFrames = Frame{24} * 3600 * 384000;
constexpr int rampFrames = 240; // 5 ms, serialized as engine version 1 behavior.
enum class Param : unsigned { volume, highPass, lowPass, gainA = 0, gainB = 1, crossfader = 2 };
struct Point {
    Frame frame{};
    float value{};
    bool hold = false;
    bool operator==(const Point &) const = default;
};
struct Curve {
    float initial = 0.7f;
    bool linear = false;
    std::vector<Point> points;
    float at(Frame) const noexcept;
};
struct AudioData {
    double sampleRate = projectRate;
    std::array<std::vector<float>, 2> samples;
    std::vector<float> peaks;
    std::size_t peakStride = 512;
    std::shared_ptr<const StreamSource> stream;
    std::size_t frames() const noexcept {
        return stream ? static_cast<std::size_t>(stream->frames()) : samples[0].size();
    }
};
struct Clip {
    std::shared_ptr<const AudioData> audio;
    std::string path, hash, name, artist, key;
    Frame start = 0, sourceOffset = 0, length = 0;
    double rate = 1.0;
    double bpm = 0.0, firstBeatSeconds = 0.0;
    bool hasGrid = false;
    bool cue = false;
    int row = 0;
    std::array<Curve, 3> automation{
        {Curve{0.7f, true, {{0, 0.7f}}}, Curve{0.0f, true, {{0, 0.0f}}}, Curve{1.0f, true, {{0, 1.0f}}}}};
};
struct Project {
    std::vector<Clip> clips = std::vector<Clip>(2);
    bool perClip = false; // Version 1 remains a separate playback-compatible path.
    std::array<Curve, 3> curves{{Curve{}, Curve{}, Curve{0.0f, false, {}}}};
    Frame end() const noexcept;
    void validate() const;
};
struct Event {
    Frame frame{};
    Param param{};
    float value{};
    unsigned clip = 0;
};

// One producer and one consumer only. No allocation, lock, or reference reclamation.
template <class T, std::size_t N> class Queue {
  public:
    static_assert(N > 0);
    static_assert(std::atomic<std::size_t>::is_always_lock_free);
    bool push(const T &value) noexcept {
        const auto w = write.load(std::memory_order_relaxed);
        if (w - read.load(std::memory_order_acquire) == N)
            return false;
        data[w % N] = value;
        write.store(w + 1, std::memory_order_release);
        return true;
    }
    bool peek(T &value) const noexcept {
        const auto r = read.load(std::memory_order_relaxed);
        if (r == write.load(std::memory_order_acquire))
            return false;
        value = data[r % N];
        return true;
    }
    bool pop(T &value) noexcept {
        const auto r = read.load(std::memory_order_relaxed);
        if (r == write.load(std::memory_order_acquire))
            return false;
        value = data[r % N];
        read.store(r + 1, std::memory_order_release);
        return true;
    }

  private:
    std::array<T, N> data{};
    alignas(64) std::atomic<std::size_t> read{0};
    alignas(64) std::atomic<std::size_t> write{0};
};

class Engine {
  public:
    static_assert(std::atomic<Frame>::is_always_lock_free && std::atomic<float>::is_always_lock_free &&
                  std::atomic<bool>::is_always_lock_free && std::atomic<std::uint64_t>::is_always_lock_free);
    // Construct/seek on control thread while callback detached. Project immutable in process().
    explicit Engine(Project, Frame start = 0);
    // Worker/offline only. Never call this readiness barrier from an audio callback.
    void prepareSources(int frames);
    void cancelSourceReads() noexcept;
    StreamStats streamStats() const noexcept;
    std::uint64_t starvationFrames() const noexcept {
        return publishedStarvation.load(std::memory_order_relaxed);
    }
    bool command(Event e) noexcept {
        return incoming.push(e);
    }
    bool captured(Event &e) noexcept {
        return capture.pop(e);
    }
    void arm(Frame in, Frame out, unsigned mask = 7, int clip = -1) noexcept;
    void process(float *const *output, int channels, int frames) noexcept;
    Frame position() const noexcept {
        return publishedFrame.load(std::memory_order_relaxed);
    }
    bool captureFailed() const noexcept {
        return overflow.load(std::memory_order_relaxed);
    }
    std::uint64_t lateEvents() const noexcept {
        return late.load(std::memory_order_relaxed);
    }
    float peak() const noexcept {
        return publishedPeak.load(std::memory_order_relaxed);
    }
    const Project &plan() const noexcept {
        return project;
    }

  private:
    struct Ramp {
        float current{}, target{}, increment{};
        int left{};
        float next(float value) noexcept;
    };
    void targets(Frame) noexcept;
    bool sampleClip(unsigned clip, double source, std::array<float, 2> &stereo) noexcept;
    struct SourceState {
        std::size_t length = 0;
        std::array<float, 2> last{}, held{};
        bool missing = false;
        int fade = 0;
    };
    std::vector<SourceState> sourceStates;
    std::unique_ptr<StreamBank> streams;
    std::uint64_t missingFrames = 0;
    std::atomic<std::uint64_t> publishedStarvation{0};
    void processClips(float *const *, int, int) noexcept;
    struct ChannelFilter {
        std::array<float, 2> hpIn{}, hpOut{}, lp{};
    };
    struct ClipState {
        std::array<Ramp, 3> ramps;
        std::array<float, 3> values{};
        unsigned overrides = 0;
        std::array<ChannelFilter, 2> filters;
        float hpAlpha = 0, lpAlpha = 0;
    };
    std::vector<ClipState> clipStates;
    int armedClip = -1;
    Project project;
    Frame cursor = 0, punchIn = 0, punchOut = 0;
    unsigned armedMask = 0, overrides = 0;
    std::array<float, 3> overrideValues{};
    std::array<Ramp, 3> ramps;
    Queue<Event, 4096> incoming;
    Queue<Event, 16384> capture;
    std::atomic<Frame> publishedFrame{0};
    std::atomic<bool> overflow{false};
    std::atomic<std::uint64_t> late{0};
    std::atomic<float> publishedPeak{0};
};

// Preserve unarmed lanes and stored targets outside the touched interval. At
// punch-out, restore the original target through the normal 5 ms smoother.
// This preserves live/replay parity, not baseline PCM identity at the boundary.
void commitTake(Project &, const std::vector<Event> &, Frame in, Frame out);
float cutoffHz(float normalized) noexcept;
} // namespace fader
