// SPDX-License-Identifier: AGPL-3.0-or-later
#include "StreamQualification.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

juce::var qualifyStream(const juce::File &file, bool cacheCompressed) {
    using namespace fader;
    const auto clip = loadClip(file, cacheCompressed);
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    std::unique_ptr<juce::AudioFormatReader> reference(formats.createReaderFor(file.createInputStream()));
    if (!reference || !clip.audio->stream || !clip.audio->samples[0].empty())
        throw std::runtime_error("Qualification requires a streaming source and reference decoder");
    const auto length = reference->lengthInSamples;
    struct Window {
        SourceFrame first;
        std::array<std::array<float, 128>, 2> pcm{};
    };
    std::vector<Window> windows;
    for (SourceFrame first : {SourceFrame{0}, SourceFrame{8191}, SourceFrame{65530}, length / 4, length / 2,
                              length * 3 / 4, length - 128})
        if (first >= 0 && first + 128 <= length)
            windows.push_back({first});
    StreamBank bank({clip.audio->stream});
    juce::AudioBuffer<float> block(2, 4096);
    double sequentialError = 0, seekError = 0;
    for (SourceFrame first = 0; first < length; first += 4096) {
        const int count = static_cast<int>(std::min(SourceFrame{4096}, length - first));
        if (!reference->read(block.getArrayOfWritePointers(), 2, first, count))
            throw std::runtime_error("Sequential reference read failed");
        if (reference->numChannels == 1)
            block.copyFrom(1, 0, block, 0, 0, count);
        bank.prepare(0, first, std::min(length - 1, first + count));
        for (int frame = 0; frame < count; ++frame) {
            std::array<float, 2> current{}, next{};
            if (!bank.sample(0, first + frame, current, next))
                throw std::runtime_error("Prepared sequential source unavailable");
            for (unsigned channel = 0; channel < 2; ++channel)
                sequentialError = std::max(
                    sequentialError, std::abs(double(current[channel]) - block.getSample(channel, frame)));
        }
        bank.release();
        for (auto &window : windows)
            for (auto frame = std::max(first, window.first);
                 frame < std::min(first + count, window.first + 128); ++frame)
                for (unsigned channel = 0; channel < 2; ++channel)
                    window.pcm[channel][static_cast<std::size_t>(frame - window.first)] =
                        block.getSample(channel, static_cast<int>(frame - first));
    }
    // Deliberately alternate distant forward/backward requests, including cold
    // opens. Windows straddle codec/PCM-page boundaries and the track tail.
    std::reverse(windows.begin(), windows.end());
    unsigned seekWindows = 0;
    for (unsigned pass = 0; pass < 2; ++pass) {
        StreamBank fresh({clip.audio->stream});
        auto &reader = pass == 0 ? bank : fresh;
        for (const auto &window : windows) {
            reader.prepare(0, window.first, std::min(length - 1, window.first + 128));
            for (int frame = 0; frame < 128; ++frame) {
                std::array<float, 2> current{}, next{};
                if (!reader.sample(0, window.first + frame, current, next))
                    throw std::runtime_error("Prepared seek source unavailable");
                for (unsigned channel = 0; channel < 2; ++channel)
                    seekError =
                        std::max(seekError, std::abs(double(current[channel]) - window.pcm[channel][frame]));
            }
            reader.release();
            ++seekWindows;
        }
        std::reverse(windows.begin(), windows.end());
    }
    auto *result = new juce::DynamicObject();
    juce::var json(result);
    result->setProperty("format", reference->getFormatName());
    result->setProperty("sampleRate", reference->sampleRate);
    result->setProperty("sourceFrames", juce::String(length));
    result->setProperty("durationSeconds", double(clip.length) / projectRate);
    result->setProperty("peakStride", static_cast<int>(clip.audio->peakStride));
    result->setProperty("sequentialMaxError", sequentialError);
    result->setProperty("seekMaxError", seekError);
    result->setProperty("seekWindows", static_cast<int>(seekWindows));
    result->setProperty("pcmCacheBytes", static_cast<int>(bank.stats().pcmBytes));
    result->setProperty("diskCacheBytes", juce::String(clip.audio->stream->diskCacheBytes()));
    result->setProperty("sequentialSeek", clip.audio->stream->sequentialSeek());
    result->setProperty("uncachedSeek", clip.audio->stream->uncachedSeek());
    if (sequentialError > 1e-6 || seekError > 1e-6)
        throw std::runtime_error("Streaming differs from sequential reference: " +
                                 juce::JSON::toString(json, true).toStdString());
    return json;
}
