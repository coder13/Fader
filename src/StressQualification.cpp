// SPDX-License-Identifier: AGPL-3.0-or-later
#include "StressQualification.h"
#include "Arrangement.h"
#include "Library.h"
#include <algorithm>
#include <bit>
#include <chrono>
#include <cmath>
#include <fader/PlaybackPreparation.h>
#include <functional>
#include <iostream>
#include <map>
#include <thread>
#if defined(__APPLE__) || defined(__linux__)
#include <sys/resource.h>
#endif
namespace {
using namespace fader;
using Clock = std::chrono::steady_clock;
constexpr std::size_t takeLimit = 200000;
constexpr HostTime second = 1000000000;
HostTime frameTime(Frame frame) {
    return HostTime((frame * 1000000000 + projectRate / 2) / projectRate);
}
Frame timeFrames(HostTime event, HostTime anchor) {
    const bool positive = event >= anchor;
    const auto gap = positive ? event - anchor : anchor - event;
    const auto frames =
        Frame((gap / second) * projectRate + ((gap % second) * projectRate + second / 2) / second);
    return positive ? frames : -frames;
}
void sleepUntil(HostTime target, HostTime spin = 0) {
    const auto now = monotonicTime();
    if (target > now + spin)
        std::this_thread::sleep_until(Clock::time_point(std::chrono::nanoseconds(target - spin)));
    while (monotonicTime() < target) {
    }
}
void require(bool pass, const char *message) {
    if (!pass)
        throw std::runtime_error(message);
}
struct DelayDecoder final : SourceDecoder {
    DelayDecoder(std::unique_ptr<SourceDecoder> inner, std::atomic<std::uint64_t> &reads)
        : inner(std::move(inner)), reads(reads) {}
    bool read(SourceFrame first, int count, float *const *out, const std::atomic<bool> &stop) override {
        ++reads;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        return !stop.load() && inner->read(first, count, out, stop);
    }
    std::unique_ptr<SourceDecoder> inner;
    std::atomic<std::uint64_t> &reads;
};
struct DelaySource final : StreamSource {
    DelaySource(std::shared_ptr<const StreamSource> source, std::atomic<std::uint64_t> &reads)
        : source(std::move(source)), reads(reads) {}
    SourceFrame frames() const noexcept override {
        return source->frames();
    }
    std::unique_ptr<SourceDecoder> open() const override {
        return std::make_unique<DelayDecoder>(source->open(), reads);
    }
    std::shared_ptr<const StreamSource> source;
    std::atomic<std::uint64_t> &reads;
};
juce::var distribution(std::vector<HostTime> &values) {
    auto *out = new juce::DynamicObject();
    out->setProperty("samples", juce::int64(values.size()));
    if (!values.empty()) {
        std::sort(values.begin(), values.end());
        const auto percentile = [&](double p) {
            return double(
                       values[std::min(values.size() - 1, std::size_t(std::ceil(p * values.size())) - 1)]) /
                   1000;
        };
        out->setProperty("p50Microseconds", percentile(.5));
        out->setProperty("p95Microseconds", percentile(.95));
        out->setProperty("p99Microseconds", percentile(.99));
        out->setProperty("p999Microseconds", percentile(.999));
        out->setProperty("maximumMicroseconds", double(values.back()) / 1000);
    }
    return out;
}
struct InputRecord {
    HostTime intended{}, received{};
    Param param{};
    float value{};
    bool attempted = false, accepted = false;
};
std::vector<InputRecord> inputPlan(int seconds, InputSource source) {
    std::vector<InputRecord> records;
    const bool controller = source == InputSource::controller;
    const int frequency = controller ? 120 : 10;
    for (int tick = 0; tick < seconds * frequency; ++tick) {
        const auto time = HostTime(tick) * second / unsigned(frequency);
        if (time + second / 10 >= HostTime(seconds) * second || (controller && (time / second) % 30 >= 5))
            continue;
        const auto index = records.size();
        // Distinct values correlate measurements, not physical controller resolution.
        records.push_back({time, 0, static_cast<Param>(index % 3),
                           .2f + float(index * 2 + unsigned(controller)) * .000001f});
    }
    return records;
}
struct HostMetric {
    HostTime wake{}, outer{};
    bool missed = false;
};
class VirtualCallbackThread final : public juce::Thread {
  public:
    VirtualCallbackThread(std::function<void()> body, std::atomic<bool> &done)
        : juce::Thread("Fader virtual callback"), body(std::move(body)), done(done) {}
    ~VirtualCallbackThread() override {
        done.store(true);
        stopThread(-1);
    }
    void run() override {
        body();
    }

  private:
    std::function<void()> body;
    std::atomic<bool> &done;
};
} // namespace

juce::var qualifySoftwareStress(const juce::File &folder, int seconds, int blockFrames, bool realtimeThread) {
    require(seconds >= 1 && seconds <= 3600 && (blockFrames == 128 || blockFrames == 256),
            "Stress duration must be 1..3600 seconds and block size 128 or 256");
    const auto importing = monotonicTime();
    Project project;
    project.perClip = true;
    project.clips = {loadClip(folder.getChildFile("A.wav")), loadClip(folder.getChildFile("B.wav"))};
    const Frame frames = (Frame(seconds) * projectRate + blockFrames - 1) / blockFrames * blockFrames;
    for (unsigned index = 0; index < 2; ++index) {
        auto &clip = project.clips[index];
        require(clip.length >= frames, "Stress sources are shorter than the declared interval");
        clip.length = frames;
        clip.row = int(index);
        clip.cue = true;
        for (unsigned p = 0; p < 3; ++p) {
            auto &curve = clip.automation[p];
            curve.points.clear();
            for (Frame at = 0; at <= frames; at += projectRate / 4) {
                const auto step = at / (projectRate / 4);
                const float value = p == 0   ? .3f + .1f * float(step % 3)
                                    : p == 1 ? .05f + .03f * float(step % 4)
                                             : .7f + .05f * float(step % 5);
                curve.points.push_back({at, value, step % 7 == 0});
            }
            curve.initial = curve.points.front().value;
            curve.linear = true;
        }
    }
    const auto analysis = folder.getChildFile("analysis.wav");
    require(analysis.existsAsFile(), "Stress analysis fixture is missing");
    Preferences prefs;
    prefs.musicFolder = folder.getChildFile("library");
    prefs.importTraktor = false;
    require(scanLibrary(prefs).tracks.size() == 1000, "Stress library must contain exactly 1000 files");
    std::atomic<std::uint64_t> reads{0};
    auto delayed = project;
    for (auto &clip : delayed.clips) {
        auto data = std::make_shared<AudioData>(*clip.audio);
        data->stream = std::make_shared<DelaySource>(data->stream, reads);
        clip.audio = data;
    }
    auto playback = std::make_unique<Playback>(delayed, 0, 9001, Preparation{}, true);
    playback->arm(0, frames);
    playback->deviceStarted(projectRate); // Virtual lifecycle, no device manager.
    const auto imported = monotonicTime();
    auto uiInputs = inputPlan(seconds, InputSource::ui);
    auto controllerInputs = inputPlan(seconds, InputSource::controller);
    const auto blocks = std::size_t(frames / blockFrames);
    Project editor = project;
    Arrangement arrangement;
    arrangement.project = &editor;
    arrangement.selected = 0;
    arrangement.setSize(1200, 700);
    arrangement.refresh();
    juce::Image image(juce::Image::ARGB, 1200, 700, true);
    {
        juce::Graphics graphics(image);
        arrangement.paintEntireComponent(graphics, true);
    }
    const auto begin = monotonicTime() + second / 5;
    const auto endTime = begin + frameTime(frames);
    std::atomic<bool> done{false};
    std::atomic<std::size_t> capturedCount{0};
    auto hostQueue = std::make_unique<Queue<HostMetric, 4096>>();
    std::vector<Event> take;
    std::vector<ControlMetric> controls;
    std::vector<HostTime> callbacks, wakes, outer;
    take.reserve(takeLimit);
    controls.reserve(takeLimit);
    callbacks.reserve(blocks);
    wakes.reserve(blocks);
    outer.reserve(blocks);
    std::size_t hostDrops = 0, virtualMisses = 0, nonFinite = 0, quietBlocks = 0, retainedDrops = 0;
    std::size_t libraryJobs = 0, analysisJobs = 0, seekCancellations = 0;
    double checksum = 0, cueChecksum = 0;
    std::string backgroundError;
    std::exception_ptr consumerError;
    std::vector<std::thread> workers;
    struct StopOnExit {
        std::atomic<bool> &done;
        std::vector<std::thread> &workers;
        ~StopOnExit() {
            done.store(true);
            for (auto &worker : workers)
                if (worker.joinable())
                    worker.join();
        }
    } stop{done, workers}; // Stop loops and join on exceptions, including older libc++.
    workers.emplace_back([&] {
        try {
            const auto drain = [&] {
                Event e;
                while (playback->captured(e)) {
                    if (take.size() < takeLimit)
                        take.push_back(e);
                    else {
                        ++retainedDrops;
                        playback->markIncomplete(captureStorageFull);
                    }
                }
                capturedCount.store(take.size());
                CallbackMetric metric;
                while (playback->metric(metric))
                    callbacks.push_back(metric.executionNs);
                ControlMetric control;
                while (playback->controlMetric(control)) {
                    if (controls.size() < takeLimit)
                        controls.push_back(control);
                    else
                        ++retainedDrops;
                }
                HostMetric host;
                while (hostQueue->pop(host)) {
                    wakes.push_back(host.wake);
                    outer.push_back(host.outer);
                    virtualMisses += unsigned(host.missed);
                }
            };
            while (!done.load()) {
                drain();
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            drain();
        } catch (...) {
            consumerError = std::current_exception();
            done.store(true);
        }
    });
    const auto producer = [&](InputSource source, std::vector<InputRecord> &records) {
        for (auto &record : records) {
            if (done.load())
                break;
            sleepUntil(begin + record.intended);
            if (done.load())
                break;
            record.received = monotonicTime();
            record.attempted = true;
            record.accepted =
                playback->submit(source, {9001, record.received, record.param, record.value, 0});
        }
    };
    workers.emplace_back([&] { producer(InputSource::ui, uiInputs); });
    workers.emplace_back([&] { producer(InputSource::controller, controllerInputs); });
    workers.emplace_back([&] {
        try {
            PlaybackPreparation preparation;
            auto snapshot = std::make_shared<const Project>(project);
            auto nextLibrary = begin, nextSeek = begin;
            while (!done.load()) {
                const auto now = monotonicTime();
                if (now >= nextLibrary) {
                    require(scanLibrary(prefs).tracks.size() == 1000, "Library workload changed");
                    ++libraryJobs;
                    auto clip = loadClip(analysis);
                    require(clip.length > 0, "Analysis fixture decoded no frames");
                    ++analysisJobs;
                    nextLibrary += 10 * second;
                }
                if (now >= nextSeek) {
                    preparation.request(snapshot,
                                        std::min(frames - 1, Frame(seekCancellations % 30) * projectRate),
                                        10000 + seekCancellations);
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    preparation.cancel();
                    ++seekCancellations;
                    nextSeek += 30 * second;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
            preparation.close();
        } catch (const std::exception &e) {
            backgroundError = e.what();
            done.store(true);
        }
    });
    const auto callbackBody = [&] {
        std::array<std::array<float, 256>, 4> output{};
        float *pointers[]{output[0].data(), output[1].data(), output[2].data(), output[3].data()};
        for (Frame at = 0; at < frames && !done.load(); at += blockFrames) {
            const auto target = begin + frameTime(at);
            sleepUntil(target, 100000);
            const auto woke = monotonicTime();
            playback->process(pointers, 4, blockFrames, woke);
            double blockSum = 0;
            for (int ch = 0; ch < 4; ++ch)
                for (int i = 0; i < blockFrames; ++i) {
                    const auto value = output[ch][i];
                    nonFinite += unsigned(!std::isfinite(value));
                    if (ch < 2)
                        blockSum += std::abs(value);
                    else
                        cueChecksum += std::abs(value);
                }
            checksum += blockSum;
            quietBlocks += unsigned(blockSum == 0);
            const auto finished = monotonicTime();
            if (!hostQueue->push({woke > target ? woke - target : 0, finished - woke,
                                  finished > begin + frameTime(at + blockFrames)}))
                ++hostDrops;
        }
        if (!done.load())
            sleepUntil(endTime); // Include the final virtual sink buffer.
        done.store(true);
    };
    VirtualCallbackThread realtime(callbackBody, done);
    const auto intervalMs = double(blockFrames) * 1000 / projectRate;
    if (realtimeThread) {
        const auto options = juce::Thread::RealtimeOptions{}
                                 .withPeriodMs(intervalMs)
                                 .withProcessingTimeMs(intervalMs / 10)
                                 .withMaximumProcessingTimeMs(intervalMs / 2);
        require(realtime.startRealtimeThread(options),
                "JUCE could not start the requested real-time virtual host");
    } else {
        workers.emplace_back(callbackBody);
    }
    std::size_t paints = 0, missedPaintIntervals = 0;
    auto nextPaint = begin, nextProgress = begin + 30 * second;
    while (!done.load()) {
        sleepUntil(nextPaint);
        if (done.load())
            break;
        const auto now = monotonicTime();
        if (now > nextPaint + second / 50)
            ++missedPaintIntervals;
        arrangement.parameter = int(paints % 3);
        arrangement.playhead = playback->position();
        arrangement.viewport.setViewPosition(int((paints % 12000) * 3), 0);
        editor.clips[0].start = paints % 2 ? projectRate : 0;
        {
            juce::Graphics graphics(image);
            arrangement.paintEntireComponent(graphics, true);
        }
        editor.clips[0].start = 0;
        ++paints;
        nextPaint += second / 50;
        if (now >= nextProgress) {
            std::cerr << "Software stress " << (now - begin) / second << '/' << seconds
                      << " s; callbacks=" << playback->stats().blocks << "; captured=" << capturedCount.load()
                      << "; source gaps=" << playback->starvationFrames() << '\n';
            nextProgress += 30 * second;
        }
    }
    for (auto &worker : workers)
        worker.join();
    realtime.waitForThreadToExit(-1);
    const auto finished = monotonicTime();
    if (consumerError)
        std::rethrow_exception(consumerError);
    std::map<std::uint32_t, const InputRecord *> inputByValue;
    std::vector<HostTime> receiptJitter, outputLatency;
    std::size_t attempted = 0, accepted = 0, correlationErrors = 0;
    for (const auto *records : {&uiInputs, &controllerInputs})
        for (const auto &record : *records) {
            attempted += unsigned(record.attempted);
            accepted += unsigned(record.accepted);
            require(inputByValue.emplace(std::bit_cast<std::uint32_t>(record.value), &record).second,
                    "Stress correlation values are not unique");
            if (record.attempted)
                receiptJitter.push_back(record.received > begin + record.intended
                                            ? record.received - begin - record.intended
                                            : 0);
        }
    std::map<std::uint32_t, const ControlMetric *> controlByValue;
    for (const auto &control : controls)
        if (!controlByValue.emplace(std::bit_cast<std::uint32_t>(control.value), &control).second)
            ++correlationErrors;
    std::map<std::uint32_t, bool> seen;
    juce::Array<juce::var> rows;
    for (const auto &event : take) {
        const auto key = std::bit_cast<std::uint32_t>(event.value);
        const auto input = inputByValue.find(key);
        const auto metric = controlByValue.find(key);
        if (input == inputByValue.end() || metric == controlByValue.end() ||
            !seen.emplace(key, true).second) {
            ++correlationErrors;
            continue;
        }
        const auto &record = *input->second;
        const auto &control = *metric->second;
        const auto scheduled = control.callbackFrame + timeFrames(record.received, control.callbackTime);
        if (!record.accepted || event.clip != 0 || event.param != record.param ||
            control.receivedAt != record.received || control.param != record.param ||
            control.scheduledFrame != scheduled || control.commandFrame != std::max(Frame{0}, scheduled) ||
            event.frame != std::max(control.callbackFrame, control.commandFrame))
            ++correlationErrors;
        const auto predicted =
            control.callbackTime + frameTime(event.frame - control.callbackFrame) + frameTime(blockFrames);
        if (predicted < record.received)
            ++correlationErrors;
        else
            outputLatency.push_back(predicted - record.received);
        if (rows.size() < 12) {
            auto *row = new juce::DynamicObject();
            row->setProperty("source", control.source == InputSource::ui ? "ui" : "controller");
            row->setProperty("receiptMicrosecondsFromStart", double(record.received - begin) / 1000);
            row->setProperty("scheduledFrame", juce::int64(control.scheduledFrame));
            row->setProperty("commandFrame", juce::int64(control.commandFrame));
            row->setProperty("appliedFrame", juce::int64(event.frame));
            row->setProperty("predictedOutputMicrosecondsFromStart", double(predicted - begin) / 1000);
            rows.add(row);
        }
    }
    const auto stats = playback->stats();
    const auto sources = playback->streamStats();
    const auto planned = uiInputs.size() + controllerInputs.size();
    const bool integrity =
        stats.blocks == blocks && attempted == planned && accepted == planned && take.size() == accepted &&
        controls.size() == accepted && correlationErrors == 0 && retainedDrops == 0 &&
        stats.inputDrops == 0 && stats.failures == 0 && stats.metricDrops == 0 &&
        stats.controlMetricDrops == 0 && hostDrops == 0 && callbacks.size() == blocks &&
        wakes.size() == blocks && playback->starvationFrames() == 0 && sources.failedSources == 0 &&
        nonFinite == 0 && checksum > 0 && cueChecksum > 0 && quietBlocks == 0 && backgroundError.empty() &&
        libraryJobs > 0 && analysisJobs > 0 && seekCancellations > 0 && paints > 0;
    const auto maximum = callbacks.empty() ? second : *std::max_element(callbacks.begin(), callbacks.end());
    auto callbackDistribution = distribution(callbacks);
    const auto budget = double(blockFrames) * 1e9 / projectRate;
    const bool headroom =
        maximum < budget && double(callbackDistribution["p999Microseconds"]) * 1000 < budget / 2;
    auto *out = new juce::DynamicObject();
    out->setProperty("scope",
                     "Real-clock software sink, production arrangement painter without native input/display, "
                     "synthetic library and virtual lifecycle. No audio/MIDI device or acoustic timing.");
    out->setProperty("requestedSeconds", seconds);
    out->setProperty("observedSeconds", double(finished - begin) / second);
    out->setProperty("importSeconds", double(imported - importing) / second);
    out->setProperty("sampleRate", projectRate);
    out->setProperty("requestedScheduler", realtimeThread ? "JUCE real-time thread" : "default std::thread");
    out->setProperty("realtimeThreadReported", realtimeThread && realtime.isRealtime());
    if (realtimeThread) {
        out->setProperty("requestedPeriodMs", intervalMs);
        out->setProperty("requestedProcessingMs", intervalMs / 10);
        out->setProperty("requestedMaximumProcessingMs", intervalMs / 2);
    }
    out->setProperty("blockFrames", blockFrames);
    out->setProperty("callbacks", juce::int64(stats.blocks));
    out->setProperty("callbackExecution", callbackDistribution);
    out->setProperty("virtualHostWakeLateness", distribution(wakes));
    out->setProperty("callbackAndSinkExecution", distribution(outer));
    out->setProperty("virtualDeadlineMisses", juce::int64(virtualMisses));
    out->setProperty("headroomTargetPassed", headroom);
    out->setProperty("softwareIntegrityPassed", integrity);
    out->setProperty("hourTargetPassed", false); // Set only when the memory measurement is available below.
    out->setProperty("memoryMeasurementAvailable", false);
    out->setProperty("sourceStarvationFrames", juce::int64(playback->starvationFrames()));
    out->setProperty("sourceFailures", int(sources.failedSources));
    out->setProperty("pcmCacheBytes", juce::int64(sources.pcmBytes));
    out->setProperty("delayedReads", juce::int64(reads.load()));
    out->setProperty("artificialReadDelayMs", 1);
    out->setProperty("plannedInputs", juce::int64(planned));
    out->setProperty("attemptedInputs", juce::int64(attempted));
    out->setProperty("acceptedInputs", juce::int64(accepted));
    out->setProperty("capturedInputs", juce::int64(take.size()));
    out->setProperty("controlMetrics", juce::int64(controls.size()));
    out->setProperty("correlationErrors", juce::int64(correlationErrors));
    out->setProperty("inputDrops", juce::int64(stats.inputDrops));
    out->setProperty("lateInputs", juce::int64(stats.lateInputs));
    out->setProperty("maximumLateFrames", juce::int64(stats.maxLateFrames));
    out->setProperty("callbackMetricDrops", juce::int64(stats.metricDrops));
    out->setProperty("controlMetricDrops", juce::int64(stats.controlMetricDrops));
    out->setProperty("hostMetricDrops", juce::int64(hostDrops));
    out->setProperty("retentionDrops", juce::int64(retainedDrops));
    out->setProperty("failureFlags", int(stats.failures));
    out->setProperty("producerWakeToReceipt", distribution(receiptJitter));
    auto latencyDistribution = distribution(outputLatency);
    out->setProperty("receiptToPredictedSoftwareOutput", latencyDistribution);
    const bool latencyPassed =
        !outputLatency.empty() && double(latencyDistribution["p95Microseconds"]) < 10000;
    out->setProperty("predictedSoftwareLatencyTargetPassed", latencyPassed);
    out->setProperty("virtualOutputBufferFrames", blockFrames);
    out->setProperty("timingExamples", rows);
    out->setProperty("masterAbsoluteSampleSum", checksum);
    out->setProperty("cueAbsoluteSampleSum", cueChecksum);
    out->setProperty("nonFiniteSamples", juce::int64(nonFinite));
    out->setProperty("quietBlocks", juce::int64(quietBlocks));
    out->setProperty("arrangementPaints", juce::int64(paints));
    out->setProperty("missedPaintIntervals", juce::int64(missedPaintIntervals));
    out->setProperty("libraryJobs", juce::int64(libraryJobs));
    out->setProperty("analysisJobs", juce::int64(analysisJobs));
    out->setProperty("seekCancellations", juce::int64(seekCancellations));
    out->setProperty("backgroundError", juce::String(backgroundError));
#if defined(__APPLE__) || defined(__linux__)
    rusage beforeCommit{};
    if (getrusage(RUSAGE_SELF, &beforeCommit) == 0) {
        auto bytes = juce::int64(beforeCommit.ru_maxrss);
#if defined(__linux__)
        bytes *= 1024;
#endif
        out->setProperty("peakResidentBytesBeforeCaptureCommit", bytes);
    }
#endif
    bool persisted = false;
    const auto persistenceStart = monotonicTime();
    try {
        if (!take.empty()) {
            auto capturedProject = project;
            commitTake(capturedProject, take, 0, frames);
            saveProject(capturedProject,
                        folder.getChildFile("stress-captured-" + juce::String(blockFrames) + ".faderset"));
            persisted = true;
        }
    } catch (const std::exception &e) {
        out->setProperty("capturePersistenceError", juce::String(e.what()));
    }
    out->setProperty("capturePersistencePassed", persisted);
    out->setProperty("capturePersistenceSeconds", double(monotonicTime() - persistenceStart) / second);
    out->setProperty("softwareIntegrityPassed", integrity && persisted);
#if defined(__APPLE__) || defined(__linux__)
    rusage usage{};
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        auto bytes = juce::int64(usage.ru_maxrss);
#if defined(__linux__)
        bytes *= 1024;
#endif
        out->setProperty("peakResidentBytes", bytes);
        out->setProperty("memoryMeasurementAvailable", true);
        out->setProperty("memoryBudgetPassed", bytes <= 128 * 1024 * 1024);
        out->setProperty("hourTargetPassed", seconds == 3600 && integrity && persisted && headroom &&
                                                 latencyPassed && virtualMisses == 0 &&
                                                 bytes <= 128 * 1024 * 1024 &&
                                                 sources.pcmBytes <= 1024 * 1024);
    }
#endif
    return out;
}
