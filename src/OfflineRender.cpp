// SPDX-License-Identifier: AGPL-3.0-or-later
#include <algorithm>
#include <cmath>
#include <fader/OfflineRender.h>
#include <stdexcept>

namespace fader {
OfflineRender::OfflineRender(Project project, RenderRange requested, std::shared_ptr<std::atomic<bool>> stop)
    : range(requested), cancellation(stop ? std::move(stop) : std::make_shared<std::atomic<bool>>(false)) {
    project.validate();
    if (range.end == -1)
        range.end = project.end();
    if (range.first < 0 || range.end <= range.first || range.end > project.end())
        throw std::runtime_error("Invalid or empty export range");
    for (const auto &clip : project.clips)
        if (clip.length > 0 && !clip.audio)
            throw std::runtime_error("Missing source audio");
    // Arbitrary ranges reconstruct uninterrupted history, rather than use the
    // approximate interactive seek policy. No extra tail is appended.
    engine = std::make_unique<Engine>(std::move(project), range.first, Preparation{cancellation, true});
}
int OfflineRender::read(float *const *stereo, int requested) {
    if (!stereo || !stereo[0] || !stereo[1] || requested < 1 || requested > maxBlockFrames)
        throw std::runtime_error("Invalid offline output block");
    if (cancellation->load())
        throw std::runtime_error("Export cancelled");
    const auto count = static_cast<int>(std::min(Frame(requested), range.end - engine->position()));
    if (count <= 0)
        return 0;
    engine->prepareSources(count);
    engine->process(stereo, 2, count);
    if (cancellation->load())
        throw std::runtime_error("Export cancelled");
    if (engine->starvationFrames() || engine->streamStats().failedSources)
        throw std::runtime_error("Audio source became unavailable during export");
    for (int channel = 0; channel < 2; ++channel)
        for (int frame = 0; frame < count; ++frame)
            if (!std::isfinite(stereo[channel][frame]))
                throw std::runtime_error("Non-finite export audio");
    return count;
}
} // namespace fader
