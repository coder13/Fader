// SPDX-License-Identifier: AGPL-3.0-or-later
#include "SeekBenchmark.h"
#include <algorithm>
#include <chrono>
#include <fader/PlaybackPreparation.h>
#include <thread>

namespace {
using Clock = std::chrono::steady_clock;
double millis(Clock::duration elapsed) {
    return std::chrono::duration<double, std::milli>(elapsed).count();
}
juce::var distribution(std::vector<double> values, double target) {
    std::sort(values.begin(), values.end());
    auto *out = new juce::DynamicObject();
    out->setProperty("count", static_cast<int>(values.size()));
    out->setProperty("p50Ms", values[(values.size() - 1) / 2]);
    out->setProperty("p95Ms", values[(values.size() * 95 + 99) / 100 - 1]);
    out->setProperty("maxMs", values.back());
    out->setProperty("targetMs", target);
    out->setProperty("misses", static_cast<int>(std::count_if(values.begin(), values.end(),
                                                              [=](double ms) { return ms > target; })));
    return out;
}
} // namespace
juce::var benchmarkSeeks(const juce::File &a, const juce::File &b, bool cacheCompressed) {
    using namespace fader;
    const auto began = Clock::now();
    auto project = std::make_shared<Project>();
    project->perClip = true;
    project->clips = {loadClip(a, cacheCompressed), loadClip(b, cacheCompressed)};
    const bool longSources = project->clips[0].length > 3600 * projectRate;
    const Frame origin = longSources ? 0 : Frame{2} * 3600 * projectRate;
    project->clips[0].start = origin;
    project->clips[1].start = origin + (longSources ? 30 * 60 : 30) * projectRate;
    juce::Array<juce::var> sources;
    std::uint64_t diskBytes = 0;
    for (auto &clip : project->clips) {
        const auto offset = static_cast<SourceFrame>(clip.audio->sampleRate * 2);
        clip.sourceOffset = offset;
        clip.length -= 2 * projectRate;
        clip.cue = true;
        clip.automation[0] = {0.25f, true, {{0, 0.25f}, {clip.length, 0.65f}}};
        clip.automation[1] = {0, true, {{0, 0}, {clip.length / 2, 0.4f}, {clip.length, 0.01f}}};
        clip.automation[2] = {1, true, {{0, 1}, {clip.length / 2, 0.02f}, {clip.length, 0.9f}}};
        auto *info = new juce::DynamicObject();
        const auto &stream = *clip.audio->stream;
        info->setProperty("sampleRate", clip.audio->sampleRate);
        info->setProperty("sourceFrames", juce::String(stream.frames()));
        info->setProperty("sequentialSeek", stream.sequentialSeek());
        info->setProperty("uncachedSeek", stream.uncachedSeek());
        info->setProperty("diskCacheBytes", juce::String(stream.diskCacheBytes()));
        info->setProperty("sourceOffset", juce::String(offset));
        sources.add(info);
        diskBytes += stream.diskCacheBytes();
    }
    project->validate();
    const auto imported = Clock::now();
    PlaybackPreparation service;
    std::vector<double> cold, warm;
    juce::Array<juce::var> measurements;
    const Frame from = project->clips[1].start + 10 * projectRate;
    const Frame to = project->clips[0].start + project->clips[0].length - 2 * projectRate;
    if (to <= from)
        throw std::runtime_error("Seek benchmark needs longer overlapping sources");
    std::uint64_t segment = 0, maxPCMBytes = 0;
    Frame maxPreroll = 0;
    for (unsigned i = 0; i < 40; ++i) {
        // Coprime stride visits distant destinations in a reproducible order.
        const auto at = from + (to - from) * ((i * 17) % 40) / 39;
        for (unsigned repeat = 0; repeat < 2; ++repeat) {
            const auto begin = Clock::now();
            const auto generation = service.request(project, at, ++segment);
            std::optional<PlaybackPreparation::Result> result;
            while (!(result = service.poll())) {
                if (Clock::now() - begin > std::chrono::seconds(60))
                    throw std::runtime_error("Seek benchmark timed out");
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            const auto ms = millis(Clock::now() - begin);
            if (!result->playback || result->generation != generation)
                throw std::runtime_error(result->error.empty() ? "Stale benchmark result" : result->error);
            auto &playback = *result->playback;
            if (playback.position() != at)
                throw std::runtime_error("Seek returned the wrong position");
            maxPreroll = std::max(maxPreroll, playback.preparationFrames());
            maxPCMBytes = std::max(maxPCMBytes, playback.streamStats().pcmBytes);
            playback.deviceStarted(projectRate);
            std::array<float, 128> left{}, right{};
            float *output[]{left.data(), right.data()};
            playback.process(output, 2, 128, monotonicTime());
            if (playback.starvationFrames() || playback.streamStats().failedSources)
                throw std::runtime_error("Prepared benchmark playback had a source gap");
            (repeat ? warm : cold).push_back(ms);
            auto *sample = new juce::DynamicObject();
            sample->setProperty("frame", juce::String(at));
            sample->setProperty("repeat", repeat != 0);
            sample->setProperty("readyMs", ms);
            measurements.add(sample);
            result.reset();
            service.cancel();
        }
    }
    auto *out = new juce::DynamicObject();
    out->setProperty("importAndCacheMs", millis(imported - began));
    out->setProperty("projectSeconds", double(project->end()) / projectRate);
    out->setProperty("sources", sources);
    out->setProperty("diskCacheBytes", juce::String(diskBytes));
    out->setProperty("maxPCMBytes", juce::String(maxPCMBytes));
    out->setProperty("maxPrerollFrames", juce::String(maxPreroll));
    out->setProperty("cold", distribution(cold, 500));
    out->setProperty("warm", distribution(warm, 150));
    out->setProperty("samples", measurements);
    out->setProperty(
        "scope", "40 first-visit/repeat pairs; fresh engine/readers every time. Cold is a new destination, "
                 "warm is its immediate repeat. OS cache is not purged; import/cache construction is "
                 "separate. 1 ms polling; native UI adds up to one 30 Hz tick. No audio device.");
    return out;
}
