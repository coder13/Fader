// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <cstdio>
#include <fader/Streaming.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <mutex>

namespace fader {
// Session-owned native float PCM. Original source identity stays authoritative.
// Build on the import worker; publish only after finish(). No path is serialized.
class DecodedCache {
  public:
    static constexpr std::uint64_t budgetBytes = 8ULL * 1024 * 1024 * 1024;
    static std::shared_ptr<DecodedCache> create(SourceFrame);
    static std::uint64_t reservedBytes() noexcept;
    ~DecodedCache();
    bool append(const juce::AudioBuffer<float> &, int frames);
    bool finish();
    // Decoder-worker only. The FILE position lock never reaches the callback.
    bool read(SourceFrame first, int frames, float *const *stereo, const std::atomic<bool> &) const;
    std::uint64_t bytes() const noexcept {
        return reservation;
    }

  private:
    explicit DecodedCache(std::uint64_t);
    const std::uint64_t reservation;
    std::FILE *file = nullptr;
    mutable std::mutex filePosition;
    mutable std::array<float, 2 * 8192> interleaved{};
    std::uint64_t written = 0;
    bool ready = false;
};
} // namespace fader
