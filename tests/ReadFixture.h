// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "Media.h"
#include <stdexcept>

// Only the small synthetic test outputs are collected in full for comparison.
// Production imports retain source metadata/peaks and stream their PCM.
inline std::array<std::vector<float>, 2> readImportedSamples(const fader::Clip &clip) {
    if (!clip.audio->stream)
        return clip.audio->samples;
    fader::StreamBank bank({clip.audio->stream});
    std::array<std::vector<float>, 2> result;
    const auto frames = static_cast<fader::SourceFrame>(clip.audio->frames());
    for (auto &channel : result)
        channel.resize(static_cast<std::size_t>(frames));
    for (fader::SourceFrame first = 0; first < frames; first += 4096) {
        const auto end = std::min(frames, first + 4096);
        bank.prepare(0, first, std::min(frames - 1, end));
        for (auto frame = first; frame < end; ++frame) {
            std::array<float, 2> current{}, next{};
            if (!bank.sample(0, frame, current, next))
                throw std::runtime_error("Prepared fixture PCM unavailable");
            for (unsigned channel = 0; channel < 2; ++channel)
                result[channel][static_cast<std::size_t>(frame)] = current[channel];
        }
        bank.release();
    }
    return result;
}
inline void renderPrepared(fader::Engine &engine, juce::AudioBuffer<float> &output) {
    for (int first = 0; first < output.getNumSamples(); first += 1024) {
        const auto count = std::min(1024, output.getNumSamples() - first);
        float *pointers[]{output.getWritePointer(0, first), output.getWritePointer(1, first)};
        engine.prepareSources(count);
        engine.process(pointers, 2, count);
    }
}
