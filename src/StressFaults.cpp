// SPDX-License-Identifier: AGPL-3.0-or-later
#include "StressQualification.h"
#include "StressTiming.h"
#include <chrono>
#include <cmath>
#include <fader/PlaybackPreparation.h>
#include <thread>
#include <utility>
namespace {
using namespace fader;
void require(bool pass, const char *message) {
    if (!pass)
        throw std::runtime_error(message);
}
void timingModelTests() {
    using qualification::HostMetric;
    using qualification::StressTiming;
    constexpr int block = 128;
    constexpr std::size_t count = StressTiming::tailLimit + 3;
    StressTiming timing(count, block);
    // Independent drains need not see the two streams in matching order.
    for (std::size_t i = count; i-- > 0;)
        timing.callback({9001, Frame(i * block), Frame(i * block), block, HostTime(100 + i), 0});
    for (std::size_t i = 0; i < count; ++i) {
        const HostTime target = 1000000 + i * 10000;
        const bool last = i == count - 1;
        timing.host({Frame(i * block), target, target + (last ? 1800 : 600), target + (last ? 3500 : 750),
                     target + (last ? 3600 : 800), target + 1000});
    }
    require(timing.allSamplesPresent() && !timing.traceComplete() && timing.tailCount == count &&
                timing.tails.size() == StressTiming::tailLimit && timing.omittedTails == 3 &&
                timing.virtualMisses == 1,
            "Bounded timing history hid samples, truncation or a missed deadline");
    require(timing.maximumWake && timing.maximumCall && timing.maximumMiss &&
                timing.maximumWake->frame == Frame((count - 1) * block) &&
                timing.maximumCall->frame == timing.maximumWake->frame &&
                timing.maximumMiss->frame == timing.maximumWake->frame &&
                timing.callbackFor(timing.maximumMiss->frame) == HostTime(100 + count - 1),
            "Timing maxima after tail capacity lost their frame-correlated callback");
    for (const auto &tail : timing.tails)
        require(timing.callbackFor(tail.frame) == HostTime(100 + tail.frame / block),
                "Out-of-order drains paired a tail with the wrong callback");

    StressTiming missing(2, block);
    missing.callback({9001, block, block, block, 17, 0});
    const HostMetric first{0, 1000, 1000, 1010, 1020, 2000};
    missing.host(first);
    missing.host({block, 2000, 2000, 2010, 2020, 3000});
    require(!missing.allSamplesPresent() && !missing.traceComplete() && !missing.callbackFor(0) &&
                missing.callbackFor(block) == 17,
            "A dropped callback metric shifted later frame associations or hid incompleteness");
    missing.callback({9001, block, block, block, 99, 0});
    missing.callback({9001, 1, 1, block, 99, 0});
    missing.callback({1, 0, 0, block, 99, 0});
    missing.host(first);
    missing.host({0, 1000, 999, 1010, 1020, 2000});
    require(missing.callbackErrors == 3 && missing.hostErrors == 2 && missing.callbackFor(block) == 17,
            "Invalid/duplicate timing records overwrote valid history or escaped counters");

    StressTiming complete(1, block);
    complete.callback({9001, 0, 0, block, 5, 0});
    complete.host(first);
    require(complete.traceComplete() && complete.tails.empty(),
            "A complete timing trace needs no outliers to be valid");
    require(timing.boundaryCallbacks.samples == count / 32 &&
                timing.boundaryCallbacks.samples + timing.otherCallbacks.samples == count &&
                timing.boundaryCallbacks.maximum == 355 && timing.otherCallbacks.maximum == 358,
            "Boundary populations lost samples or their correctly indexed maximum");
}
void cpuModelTests() {
    using namespace qualification;
    CpuObservation cpu{{1002, 1004, 10, 0, true}, {1011, 1013, 18, 0, true}};
    require(cpu.valid() && cpu.elapsed() == 8, "CPU sample brackets lost their delta");
    StressTiming timing(3, 128, true);
    for (int i = 0; i < 3; ++i)
        timing.callback({9001, i * 128, i * 128, 128, 5, 0});
    timing.host({0, 1000, 1001, 1010, 1020, 2000, cpu});
    auto bad = cpu;
    bad.first.valid = false;
    bad.first.error = 5;
    timing.host({128, 1000, 1001, 1010, 1020, 2000, bad});
    bad = cpu;
    bad.last.cpu = 9;
    require(!bad.valid(), "Backwards CPU accounting was treated as a valid delta");
    timing.host({256, 1000, 1001, 1010, 1020, 2000, bad});
    require(timing.allSamplesPresent() && timing.cpuSamples == 1 && timing.cpuErrors == 2 &&
                timing.cpu[0] == 8 && timing.cpu[1] == StressTiming::missing &&
                timing.cpu[2] == StressTiming::missing && timing.maximumCpu && timing.maximumCpu->frame == 0,
            "Optional CPU clock errors hid wall samples or became zero CPU measurements");
    bad = cpu;
    bad.last.before = 1003;
    require(!bad.valid(), "Overlapping/reversed CPU reading brackets were accepted");

    PaintTiming paints;
    for (std::size_t i = 0; i < PaintTiming::tailLimit + 3; ++i) {
        const auto begin = HostTime(1000 + i * 10000);
        const bool last = i == PaintTiming::tailLimit + 2;
        paints.add({i,
                    begin,
                    begin + (last ? 2000 : 1),
                    begin + (last ? 12000 : 1001),
                    Frame(i * 128),
                    i % 2 ? projectRate : 0,
                    int(i * 3),
                    int(i % 3),
                    {}},
                   1000);
    }
    require(paints.samples == PaintTiming::tailLimit + 3 && paints.tails.size() == PaintTiming::tailLimit &&
                paints.omittedTails == 3 && paints.maximumExecution && paints.maximumWake &&
                paints.maximumExecution->index == PaintTiming::tailLimit + 2 &&
                paints.maximumWake->index == PaintTiming::tailLimit + 2 && paints.errors == 0,
            "Painter outlier capacity lost context, truncation or later global maxima");
    const auto &last = *paints.maximumExecution;
    require(last.viewportX == int(last.index * 3) && last.parameter == int(last.index % 3) &&
                last.playhead == Frame(last.index * 128),
            "Painter maxima were paired with another frame");
    paints.add(last, 1000);
    require(paints.errors == 1, "Duplicate painter metadata was not rejected");
}
std::vector<Event> captured(Playback &playback) {
    std::vector<Event> events;
    Event e;
    while (playback.captured(e))
        events.push_back(e);
    return events;
}
void process(Playback &playback, Frame frame) {
    std::array<std::array<float, 128>, 2> pcm{};
    float *out[]{pcm[0].data(), pcm[1].data()};
    playback.process(out, 2, 128, 1000000000 + HostTime(frame) * 1000000000 / projectRate);
}
void prefix(const Project &source, const std::vector<Event> &events, const juce::File &file) {
    require(!events.empty(), "Fault lost the valid captured prefix");
    auto saved = source;
    commitTake(saved, events, 0, source.end());
    saveProject(saved, file);
    auto reopened = loadProject(file);
    require(reopened.clips[0].automation[0].points == saved.clips[0].automation[0].points,
            "Incomplete prefix did not survive save/reopen");
}
struct Gate {
    std::atomic<bool> blocked{false}, entered{false};
};
struct GateDecoder final : SourceDecoder {
    GateDecoder(std::unique_ptr<SourceDecoder> inner, std::shared_ptr<Gate> gate)
        : inner(std::move(inner)), gate(std::move(gate)) {}
    bool read(SourceFrame at, int count, float *const *out, const std::atomic<bool> &stop) override {
        gate->entered.store(true);
        while (gate->blocked.load() && !stop.load())
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        return !stop.load() && inner->read(at, count, out, stop);
    }
    std::unique_ptr<SourceDecoder> inner;
    std::shared_ptr<Gate> gate;
};
struct GateSource final : StreamSource {
    GateSource(std::shared_ptr<const StreamSource> inner, std::shared_ptr<Gate> gate)
        : inner(std::move(inner)), gate(std::move(gate)) {}
    SourceFrame frames() const noexcept override {
        return inner->frames();
    }
    std::unique_ptr<SourceDecoder> open() const override {
        return std::make_unique<GateDecoder>(inner->open(), gate);
    }
    std::shared_ptr<const StreamSource> inner;
    std::shared_ptr<Gate> gate;
};
Project gated(Project project, std::shared_ptr<Gate> gate) {
    auto data = std::make_shared<AudioData>(*project.clips[0].audio);
    data->stream = std::make_shared<GateSource>(data->stream, std::move(gate));
    project.clips[0].audio = data;
    return project;
}
void writeWave(const juce::File &file, int seconds, int phase) {
    std::unique_ptr<juce::OutputStream> stream = file.createOutputStream();
    require(bool(stream), "Cannot create synthetic stress fixture");
    const auto options =
        juce::AudioFormatWriterOptions{}.withSampleRate(projectRate).withNumChannels(2).withBitsPerSample(16);
    auto writer = juce::WavAudioFormat{}.createWriterFor(stream, options);
    require(bool(writer), "Cannot create fixture WAV writer");
    juce::AudioBuffer<float> pcm(2, 4096);
    for (int at = 0; at < seconds * projectRate; at += pcm.getNumSamples()) {
        const auto count = std::min(pcm.getNumSamples(), seconds * projectRate - at);
        for (int ch = 0; ch < 2; ++ch)
            for (int i = 0; i < count; ++i) {
                const double t = double(at + i) / projectRate;
                pcm.setSample(
                    ch, i,
                    float(.2 * std::sin(t * (1400 + phase * 100 + ch * 200)) + .08 * std::cos(t * 11000)));
            }
        require(writer->writeFromAudioSampleBuffer(pcm, 0, count), "Fixture write failed");
    }
}
} // namespace

juce::var qualifyStressFaults(const juce::File &folder) {
    Project project;
    project.perClip = true;
    project.clips = {loadClip(folder.getChildFile("A.wav"))};
    project.clips[0].length = std::min(Frame{4} * projectRate, project.clips[0].length);
    require(project.end() >= 3 * projectRate, "Fault fixture needs at least three seconds");
    auto root = folder.getChildFile("fault-results");
    require(root.createDirectory().wasOk(), "Cannot create fault evidence folder");
    auto *out = new juce::DynamicObject();
    juce::var result(out);
    {
        auto playback = std::make_unique<Playback>(project, 0, 1, Preparation{}, true);
        playback->arm(0, project.end());
        playback->deviceStarted(projectRate);
        for (int i = 0; i < 4096; ++i)
            require(playback->submit(InputSource::ui, {1, 1000000000, Param::volume, .3f, 0}),
                    "Input pressure overflowed before capacity");
        require(!playback->submit(InputSource::ui, {1, 1000000000, Param::volume, .7f, 0}),
                "Input pressure did not overflow");
        process(*playback, 0);
        auto take = captured(*playback);
        require(take.size() == 4096 && (playback->stats().failures & inputOverflow),
                "Input overflow discarded its prefix or hid failure");
        prefix(project, take, root.getChildFile("input-overflow-incomplete-prefix.faderset"));
        out->setProperty("inputOverflowRetained", int(take.size()));
        // Telemetry loss does not invalidate otherwise valid musical capture.
        require(playback->submit(InputSource::ui, {1, 1002666667, Param::volume, .4f, 0}),
                "Cannot submit control after pressure");
        process(*playback, 128);
        require(playback->stats().controlMetricDrops == 1 && captured(*playback).size() == 1,
                "Control metric pressure lost musical capture or hid metric loss");
        out->setProperty("controlTelemetryDropReported", true);
    }
    {
        auto playback = std::make_unique<Playback>(project, 0, 2);
        playback->arm(0, project.end());
        playback->deviceStarted(projectRate);
        for (int batch = 0; batch < 5; ++batch) {
            for (int i = 0; i < 4096; ++i)
                require(playback->submit(InputSource::controller,
                                         {2, 1000000000 + HostTime(batch * 128) * 1000000000 / projectRate,
                                          Param::volume, .25f, 0}),
                        "Input pressure obscured capture pressure");
            process(*playback, batch * 128);
        }
        auto take = captured(*playback);
        require(take.size() == 16384 && (playback->stats().failures & captureOverflow),
                "Capture pressure discarded its prefix or hid failure");
        prefix(project, take, root.getChildFile("capture-overflow-incomplete-prefix.faderset"));
        out->setProperty("captureOverflowRetained", int(take.size()));
    }
    {
        auto playback = std::make_unique<Playback>(project, 0, 3);
        playback->arm(0, project.end());
        playback->deviceStarted(projectRate);
        require(playback->submit(InputSource::ui, {3, 1000000000, Param::volume, .31f, 0}),
                "Cannot submit interruption prefix");
        process(*playback, 0);
        playback->deviceStopped();
        playback->deviceStarted(projectRate);
        process(*playback, 128);
        auto take = captured(*playback);
        require(playback->clockFailed() && playback->position() == 128 && take.size() == 1,
                "Simulated device restart resumed invalid segment or lost prefix");
        prefix(project, take, root.getChildFile("device-loss-incomplete-prefix.faderset"));
        out->setProperty("virtualDeviceLossRetained", int(take.size()));
    }
    {
        auto gate = std::make_shared<Gate>();
        gate->blocked.store(true);
        PlaybackPreparation preparation;
        preparation.request(std::make_shared<const Project>(gated(project, gate)), 0, 4);
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
        while (!gate->entered.load() && std::chrono::steady_clock::now() < deadline)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        const bool entered = gate->entered.load();
        const auto before = monotonicTime();
        preparation.cancel();
        const auto cancelNs = monotonicTime() - before;
        const auto generation = preparation.request(std::make_shared<const Project>(project), 0, 5);
        std::optional<PlaybackPreparation::Result> ready;
        while (!(ready = preparation.poll()) && std::chrono::steady_clock::now() < deadline)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        require(entered && ready && ready->generation == generation && ready->playback &&
                    ready->error.empty(),
                "Cancelled blocked decode prevented the next prepared segment");
        require(cancelNs < 1000000000, "Cancellation joined a blocked decoder on the control thread");
        preparation.cancel();
        preparation.close();
        out->setProperty("blockedDecodeCancelMicroseconds", double(cancelNs) / 1000);
        out->setProperty("latestPreparationRecovered", true);
    }
    {
        auto gate = std::make_shared<Gate>();
        auto playback = std::make_unique<Playback>(gated(project, gate), 0, 6);
        gate->blocked.store(true);
        playback->arm(0, project.end());
        playback->deviceStarted(projectRate);
        require(playback->submit(InputSource::ui, {6, 1000000000, Param::volume, .32f, 0}),
                "Cannot submit source gap prefix");
        for (Frame at = 0; at < 3 * projectRate; at += 128)
            process(*playback, at);
        auto take = captured(*playback);
        playback->cancelSourceReads();
        require(playback->starvationFrames() > 0 && take.size() == 1 && !playback->captureFailed(),
                "Blocked source did not expose a gap independently of capture integrity");
        prefix(project, take, root.getChildFile("source-gap-valid-prefix.faderset"));
        out->setProperty("injectedStarvationFrames", juce::int64(playback->starvationFrames()));
        out->setProperty("sourceGapPreservedCapture", true);
    }
    {
        const auto media = root.getChildFile("missing-media.wav");
        require(folder.getChildFile("analysis.wav").copyFileTo(media), "Cannot create missing-media fixture");
        Project missing;
        missing.perClip = true;
        missing.clips = {loadClip(media)};
        const auto file = root.getChildFile("missing-media-preserved.faderset");
        saveProject(missing, file);
        const auto before = juce::SHA256(file).toHexString();
        const auto hidden = root.getChildFile("missing-media.hidden");
        require(media.moveFileTo(hidden), "Cannot hide synthetic missing source");
        bool rejected = false;
        try {
            loadProject(file);
        } catch (const std::exception &) {
            rejected = true;
        }
        const bool restored = hidden.moveFileTo(media);
        require(rejected && restored && before == juce::SHA256(file).toHexString(),
                "Missing-media load changed a saved project or accepted missing content");
        out->setProperty("missingMediaReportedAndProjectRetained", true);
    }
    out->setProperty(
        "scope",
        "Isolated software faults with explicit incomplete-prefix files. No physical device or UI claim.");
    return result;
}
juce::var stressSelfTest() {
    timingModelTests();
    cpuModelTests();
    const auto root = juce::File::getSpecialLocation(juce::File::tempDirectory)
                          .getNonexistentChildFile("fader-stress-self-test", {}, false);
    require(root.createDirectory().wasOk(), "Cannot create stress self-test folder");
    struct Cleanup {
        juce::File root;
        ~Cleanup() {
            root.deleteRecursively();
        }
    } cleanup{root};
    writeWave(root.getChildFile("A.wav"), 4, 0);
    writeWave(root.getChildFile("B.wav"), 4, 1);
    writeWave(root.getChildFile("analysis.wav"), 1, 2);
    const auto library = root.getChildFile("library");
    require(library.createDirectory().wasOk(), "Cannot create self-test library");
    for (int i = 0; i < 1000; ++i)
        require(root.getChildFile("analysis.wav").copyFileTo(library.getChildFile(juce::String(i) + ".wav")),
                "Cannot populate self-test library");
    auto *result = new juce::DynamicObject();
    juce::var json(result);
    result->setProperty("faults", qualifyStressFaults(root));
    result->setProperty("timingModelPassed", true);
    auto timed = qualifySoftwareStress(root, 2, 128);
    // Smoke checks integrity only. CI scheduling jitter cannot qualify or fail a
    // one-hour performance target, and every timing measurement remains visible.
    require(bool(timed["softwareIntegrityPassed"]),
            "Software stress smoke lost capture, stream data or timing metrics");
    require(bool(timed["timingTrace"]["allSamplesPresent"]) &&
                juce::int64(timed["arrangementUpdateAndPaintExecution"]["samples"]) ==
                    juce::int64(timed["arrangementPaints"]) &&
                juce::int64(timed["arrangementPaintStartSpacing"]["samples"]) ==
                    juce::int64(timed["arrangementPaints"]) - 1,
            "Stress timing or painter telemetry did not cover its actual samples");
    result->setProperty("timedSmoke", timed);
    auto diagnostic = qualifySoftwareStress(root, 2, 128, false, true);
    require(bool(diagnostic["softwareIntegrityPassed"]) && bool(diagnostic["cpuDiagnosticsEnabled"]) &&
                !bool(diagnostic["performanceQualificationEligible"]) &&
                !bool(diagnostic["hourTargetPassed"]),
            "Instrumented capture lost integrity or could be called a performance qualification");
    const auto cpu = diagnostic["cpuDiagnostics"];
    require(bool(cpu["complete"]) &&
                juce::int64(cpu["callbackCpuSamples"]) == juce::int64(diagnostic["callbacks"]) &&
                juce::int64(cpu["callbackCpuErrors"]) == 0 && juce::int64(cpu["paintCpuErrors"]) == 0 &&
                juce::int64(cpu["enclosingPaintThreadCpu"]["samples"]) ==
                    juce::int64(diagnostic["arrangementPaints"]),
            "Native CPU clock reads failed or did not cover every instrumented observation");
    require(juce::int64(diagnostic["paintTrace"]["samples"]) ==
                    juce::int64(diagnostic["arrangementPaints"]) &&
                juce::int64(diagnostic["paintTrace"]["invalidOrDuplicateMetrics"]) == 0,
            "Painter context did not cover its actual frames");
    result->setProperty("cpuDiagnosticSmoke", diagnostic);
    result->setProperty("cpuAndPaintModelPassed", true);
    // The project remains longer than the timed host. In particular the
    // untouched second clip retains every point beyond the two-second run.
    for (const auto &[cpuMode, timeline] :
         {std::pair{false, 4}, std::pair{true, -1}, std::pair{true, 1}, std::pair{true, 3601}}) {
        bool rejectedBeforeImport = false;
        try {
            qualifySoftwareStress(root.getChildFile("not-a-fixture-folder"), 2, 128, false, cpuMode,
                                  timeline);
        } catch (const std::exception &error) {
            rejectedBeforeImport = juce::String(error.what()).contains("Diagnostic timeline requires");
        }
        require(rejectedBeforeImport,
                "Invalid expanded timeline reached source import or could qualify performance");
    }
    auto expanded = qualifySoftwareStress(root, 2, 128, false, true, 4);
    require(bool(expanded["softwareIntegrityPassed"]) &&
                juce::int64(expanded["projectFrames"]) == 4 * projectRate &&
                int(expanded["diagnosticTimelineSeconds"]) == 4 &&
                juce::int64(expanded["storedAutomationPoints"]) == 6 * 17 &&
                !bool(expanded["performanceQualificationEligible"]) && !bool(expanded["hourTargetPassed"]) &&
                bool(expanded["cpuDiagnostics"]["complete"]),
            "Expanded timeline changed its density, lost integrity or could qualify an hour");
    const auto saved = loadProject(root.getChildFile("stress-captured-128.faderset"));
    require(saved.clips.size() == 2 && saved.clips[0].length == 4 * projectRate &&
                saved.clips[1].length == 4 * projectRate,
            "Short diagnostic resized the full project clips");
    for (const auto &curve : saved.clips[1].automation)
        require(curve.points.size() == 17 && curve.points.back().frame == 4 * projectRate,
                "Short diagnostic discarded untouched points after its timed interval");
    const auto policy = expanded["threadPolicySnapshots"];
    for (const auto *name : {"painterBeforeActivity", "painterDuringActivity", "callbackAtStart",
                             "callbackAtEnd", "painterAtEnd"}) {
        const auto sample = policy[name];
        require(juce::int64(sample["beforeNanosecondsFromStart"]) <=
                    juce::int64(sample["afterNanosecondsFromStart"]),
                "Thread policy query has a backwards wall bracket");
#if JUCE_MAC
        require(bool(sample["supported"]) && int(sample["pthreadQosStatus"]) == 0 &&
                    int(sample["machPortReleaseStatus"]) == 0,
                "Mac thread policy snapshot failed or leaked its thread port");
#else
        require(!bool(sample["supported"]), "Non-Mac host claimed Mac policy queries");
#endif
    }
    result->setProperty("fullTimelineDiagnosticSmoke", expanded);
    result->setProperty("expandedTimelineGuardsPassed", true);
    return json;
}
