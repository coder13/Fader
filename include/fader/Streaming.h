// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <vector>

namespace fader {
using SourceFrame = std::int64_t;

// Used only by a decode worker. Implementations should observe cancellation
// between reads; an OS read already in progress may still have to return first.
class SourceDecoder {
  public:
    virtual ~SourceDecoder() = default;
    virtual bool read(SourceFrame first, int frames, float *const *stereo,
                      const std::atomic<bool> &cancelled) = 0;
};
class StreamSource {
  public:
    virtual ~StreamSource() = default;
    virtual SourceFrame frames() const noexcept = 0;
    virtual std::unique_ptr<SourceDecoder> open() const = 0;
};
enum class SourceError : unsigned { none, open, read, nonFinite };
struct StreamStats {
    std::uint64_t generation{}, decodedFrames{}, discardedReads{}, pcmBytes{};
    unsigned failedSources{};
    bool cancelled{};
};

// One render consumer, with a separate cache per clip/position. Slot pinning
// protects PCM from reuse; no ownership transfer/reclamation occurs on render.
// Construct/prepare/cancel/destroy off the audio callback. release() ends a block.
class StreamBank {
  public:
    static constexpr int pageFrames = 8192, pagesPerSource = 8;
    static constexpr unsigned maxSources = 64;
    static constexpr std::uint64_t pcmBytesPerSource = 2 * sizeof(float) * pageFrames * pagesPerSource;
    explicit StreamBank(std::vector<std::shared_ptr<const StreamSource>>);
    ~StreamBank();
    StreamBank(const StreamBank &) = delete;
    StreamBank &operator=(const StreamBank &) = delete;
    // request/sample/release/prepare share one exclusive consumer; prepare only
    // while detached or on the offline owner. cancel/stats may run concurrently.
    // Nonblocking render methods. Sources may be null for in-memory clips.
    void request(unsigned clip, SourceFrame first) noexcept;
    bool sample(unsigned clip, SourceFrame index, std::array<float, 2> &current,
                std::array<float, 2> &next) noexcept;
    void release() noexcept;
    SourceError error(unsigned clip) const noexcept;
    StreamStats stats() const noexcept;
    // Worker/offline-only readiness barrier. Includes both endpoints. Throws on
    // decode failure, cancellation, timeout, or a range larger than the cache.
    void prepare(unsigned clip, SourceFrame first, SourceFrame last,
                 std::chrono::milliseconds timeout = std::chrono::seconds(30));
    void cancel() noexcept;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl;
};
} // namespace fader
