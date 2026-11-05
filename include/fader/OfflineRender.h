// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <fader/Engine.h>

namespace fader {
struct RenderRange {
    Frame first = 0;
    Frame end = -1; // exclusive; -1 means the arrangement end
};
// Worker-only host. Owns an independent immutable engine, waits for sources
// before processing, outputs stereo master only, never opens a device.
class OfflineRender {
  public:
    static constexpr int maxBlockFrames = 1024;
    explicit OfflineRender(Project, RenderRange = {}, std::shared_ptr<std::atomic<bool>> cancellation = {});
    int read(float *const *stereo, int requestedFrames);
    Frame frames() const noexcept {
        return range.end - range.first;
    }
    Frame completed() const noexcept {
        return engine->position() - range.first;
    }
    float peak() const noexcept {
        return engine->peak();
    }
    StreamStats streamStats() const noexcept {
        return engine->streamStats();
    }
    void cancel() noexcept {
        cancellation->store(true);
    }

  private:
    RenderRange range;
    std::shared_ptr<std::atomic<bool>> cancellation;
    std::unique_ptr<Engine> engine;
};
} // namespace fader
