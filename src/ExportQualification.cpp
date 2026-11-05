// SPDX-License-Identifier: AGPL-3.0-or-later
#include "ExportQualification.h"
#include "ProjectStorage.h"
#include <chrono>
#include <iostream>
#include <thread>
#if defined(__APPLE__) || defined(__linux__)
#include <sys/resource.h>
#endif
namespace {
using namespace fader;
using Clock = std::chrono::steady_clock;
struct DelayDecoder final : SourceDecoder {
    DelayDecoder(std::unique_ptr<SourceDecoder> inner, std::shared_ptr<std::atomic<std::uint64_t>> reads)
        : inner(std::move(inner)), reads(std::move(reads)) {}
    bool read(SourceFrame first, int count, float *const *out, const std::atomic<bool> &stop) override {
        if (stop.load())
            return false;
        ++*reads;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        return !stop.load() && inner->read(first, count, out, stop);
    }
    std::unique_ptr<SourceDecoder> inner;
    std::shared_ptr<std::atomic<std::uint64_t>> reads;
};
struct DelaySource final : StreamSource {
    DelaySource(std::shared_ptr<const StreamSource> source, std::shared_ptr<std::atomic<std::uint64_t>> reads)
        : source(std::move(source)), reads(std::move(reads)) {}
    SourceFrame frames() const noexcept override {
        return source->frames();
    }
    std::unique_ptr<SourceDecoder> open() const override {
        return std::make_unique<DelayDecoder>(source->open(), reads);
    }
    std::shared_ptr<const StreamSource> source;
    std::shared_ptr<std::atomic<std::uint64_t>> reads;
};
double seconds(Clock::duration duration) {
    return std::chrono::duration<double>(duration).count();
}
} // namespace
juce::var qualifyLongExport(const juce::File &a, const juce::File &b, const juce::File &output) {
    using namespace fader;
    constexpr Frame duration = Frame{60} * 60 * projectRate;
    const auto begin = Clock::now();
    Project project;
    project.perClip = true;
    project.clips = {loadClip(a), loadClip(b)};
    if (project.clips[0].length < duration || project.clips[1].length < duration)
        throw std::runtime_error("Long export needs sources at least 60 minutes long");
    project.clips[0].length = duration;
    project.clips[1].start = 15 * 60 * projectRate;
    project.clips[1].length = duration - project.clips[1].start;
    project.clips[1].row = 1;
    for (auto &clip : project.clips) {
        clip.cue = true;
        clip.automation[0] = {.25f, true, {{0, .25f}, {clip.length / 2, .6f}, {clip.length, .3f}}};
        clip.automation[1] = {0, true, {{0, 0}, {clip.length / 2, .3f}, {clip.length, 0}}};
        clip.automation[2] = {1, true, {{0, 1}, {clip.length / 2, .2f}, {clip.length, 1}}};
    }
    const auto imported = Clock::now();
    const auto snapshot = [&] {
        return juce::JSON::toString(projectDocument(project, output.withFileExtension("faderset")), true);
    };
    const auto before = snapshot();
    auto delayed = project;
    auto reads = std::make_shared<std::atomic<std::uint64_t>>(0);
    for (auto &clip : delayed.clips) {
        auto data = std::make_shared<AudioData>(*clip.audio);
        data->stream = std::make_shared<DelaySource>(clip.audio->stream, reads);
        clip.audio = data;
    }
    std::cerr << "Import complete; rendering 60 minutes with 1 ms delayed source reads\n";
    WavExportOptions options;
    options.floatingPoint = true;
    const auto result = exportWavFile(delayed, output, options);
    const auto rendered = Clock::now();
    std::cerr << "Export complete; comparing entire WAV with independently hosted Playback\n";
    auto reader = std::unique_ptr<juce::AudioFormatReader>(
        juce::WavAudioFormat{}.createReaderFor(output.createInputStream().release(), true));
    if (!reader || reader->lengthInSamples != duration || reader->numChannels != 2 ||
        reader->sampleRate != projectRate || !reader->usesFloatingPointData)
        throw std::runtime_error("Long output format/duration mismatch");
    auto live = std::make_unique<Playback>(project, 0, 91);
    live->deviceStarted(projectRate);
    juce::AudioBuffer<float> expected(4, 1024), actual(2, 1024);
    double error = 0, checksum = 0, cue = 0;
    for (Frame at = 0; at < duration; at += 1024) {
        const int count = int(std::min(Frame{1024}, duration - at));
        live->prepareSources(count);
        live->process(expected.getArrayOfWritePointers(), 4, count,
                      1000000000 + HostTime((at * 1000000000 + projectRate / 2) / projectRate));
        if (!reader->read(actual.getArrayOfWritePointers(), 2, at, count))
            throw std::runtime_error("Long output read failed");
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < count; ++i) {
                error =
                    std::max(error, std::abs(double(expected.getSample(ch, i)) - actual.getSample(ch, i)));
                checksum += std::abs(actual.getSample(ch, i));
                cue += std::abs(expected.getSample(ch + 2, i));
            }
        CallbackMetric metric;
        while (live->metric(metric)) {
        }
    }
    const auto end = Clock::now();
    if (error > 1e-6 || checksum <= 0 || cue <= 0 || live->starvationFrames() ||
        live->streamStats().failedSources || before != snapshot())
        throw std::runtime_error("Long offline parity, source readiness or project immutability failed");
    auto *out = new juce::DynamicObject();
    out->setProperty("frames", juce::String(result.frames));
    out->setProperty("durationSeconds", double(result.frames) / projectRate);
    out->setProperty("fileBytes", juce::String(output.getSize()));
    out->setProperty("pcmCacheBytes", juce::String(result.sources.pcmBytes));
    out->setProperty("delayedReads", juce::String(reads->load()));
    out->setProperty("artificialReadDelayMs", 1);
    out->setProperty("maxAbsolutePCMError", error);
    out->setProperty("absoluteSampleSum", checksum);
    out->setProperty("cueAbsoluteSampleSum", cue);
    out->setProperty("projectUnchanged", true);
    out->setProperty("failedSources", int(result.sources.failedSources));
    out->setProperty("importSeconds", seconds(imported - begin));
    out->setProperty("exportSeconds", seconds(rendered - imported));
    out->setProperty("verificationSeconds", seconds(end - rendered));
#if defined(__APPLE__) || defined(__linux__)
    rusage usage{};
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        auto bytes = static_cast<juce::int64>(usage.ru_maxrss);
#if defined(__linux__)
        bytes *= 1024;
#endif
        out->setProperty("peakResidentBytes", juce::String(bytes));
    }
#endif
    out->setProperty("scope",
                     "Full 60-minute real WAV export and full independent pre-device PCM comparison; "
                     "accelerated software hosts, synthetic sparse WAV sources, no audio "
                     "device/controller/UI. Not real-time deadline or acoustic qualification.");
    return out;
}
