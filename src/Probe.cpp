// SPDX-License-Identifier: AGPL-3.0-or-later
#include "AudioPlayback.h"
#include "AudioSetup.h"
#include "Library.h"
#include "Media.h"
#include "SeekBenchmark.h"
#include "StreamQualification.h"
#include <algorithm>
#include <chrono>
#include <iostream>
#if defined(__APPLE__) || defined(__linux__)
#include <sys/resource.h>
#endif
namespace {
void streamingSoak(const juce::File &a, const juce::File &b) {
    using namespace fader;
    const auto begin = std::chrono::steady_clock::now();
    Project project;
    project.perClip = true;
    project.clips = {loadClip(a), loadClip(b)};
    project.clips[1].start = 30 * 60 * projectRate;
    project.clips[1].row = 1;
    std::uint64_t peaks = 0;
    for (auto &clip : project.clips) {
        clip.automation[0] = {0.25f, true, {{0, 0.25f}}};
        peaks += clip.audio->peaks.size() * sizeof(float);
    }
    const auto imported = std::chrono::steady_clock::now();
    auto engine = std::make_unique<Engine>(project);
    std::array<std::array<float, 1024>, 2> output{};
    float *pointers[]{output[0].data(), output[1].data()};
    double checksum = 0;
    while (engine->position() < project.end()) {
        const int count = static_cast<int>(std::min(Frame{1024}, project.end() - engine->position()));
        engine->prepareSources(count);
        engine->process(pointers, 2, count);
        for (int frame = 0; frame < count; ++frame)
            checksum += std::abs(output[0][frame]) + std::abs(output[1][frame]);
    }
    const auto finish = std::chrono::steady_clock::now();
    const auto stats = engine->streamStats();
    if (engine->starvationFrames() || stats.failedSources || checksum <= 0)
        throw std::runtime_error("Streaming soak had missing/failed/empty output");
    auto *result = new juce::DynamicObject();
    juce::var json(result);
    result->setProperty("projectFrames", juce::String(project.end()));
    result->setProperty("durationSeconds", double(project.end()) / projectRate);
    result->setProperty("importSeconds", std::chrono::duration<double>(imported - begin).count());
    result->setProperty("renderSeconds", std::chrono::duration<double>(finish - imported).count());
    result->setProperty("pcmCacheBytes", juce::String(stats.pcmBytes));
    result->setProperty("waveformBytes", juce::String(peaks));
    result->setProperty("decodedFrames", juce::String(stats.decodedFrames));
    result->setProperty("starvationFrames", juce::String(engine->starvationFrames()));
    result->setProperty("absoluteSampleSum", checksum);
#if defined(__APPLE__) || defined(__linux__)
    rusage usage{};
    if (getrusage(RUSAGE_SELF, &usage) == 0) {
        auto bytes = static_cast<juce::int64>(usage.ru_maxrss);
#if defined(__linux__)
        bytes *= 1024;
#endif
        result->setProperty("peakResidentBytes", juce::String(bytes));
    }
#endif
    result->setProperty("scope", "Entire arrangement, accelerated prepared reads; no device, wall-clock "
                                 "deadline or acoustic qualification");
    std::cout << juce::JSON::toString(json, true) << '\n';
}
fader::Project silentProject() {
    fader::Project project;
    project.perClip = true;
    auto audio = std::make_shared<fader::AudioData>();
    for (auto &channel : audio->samples)
        channel.assign(48000 * 4, 0.1f);
    for (auto &clip : project.clips) {
        clip.audio = audio;
        clip.length = 48000 * 4;
        clip.automation[0].initial = 0;
        clip.automation[0].points = {{0, 0}}; // silent physical output
        clip.automation[1].points = {{0, 0.1f}, {192000, 0.6f}};
        clip.automation[2].points = {{0, 0.9f}, {192000, 0.4f}};
    }
    project.curves[0].initial = project.curves[1].initial = 0; // silent physical output
    return project;
}
void audioSmoke() {
    juce::AudioDeviceManager manager;
    const auto error = fader::configureAudioOutput(manager);
    if (error.isNotEmpty())
        throw std::runtime_error(error.toStdString());
    auto *device = manager.getCurrentAudioDevice();
    if (!device || device->getCurrentSampleRate() != 48000)
        throw std::runtime_error("48 kHz device unavailable");
    auto playback = std::make_unique<fader::Playback>(silentProject(), 0, 1);
    fader::AudioPlayback callback(*playback);
    const int before = device->getXRunCount();
    callback.attach(manager);
    juce::Thread::sleep(2000);
    callback.detach();
    const int after = device->getXRunCount();
    const auto stats = playback->stats();
    std::vector<double> times;
    fader::CallbackMetric metric;
    while (playback->metric(metric))
        times.push_back(double(metric.executionNs) / 1000);
    if (times.empty())
        throw std::runtime_error("No device callbacks received");
    std::sort(times.begin(), times.end());
    auto *result = new juce::DynamicObject();
    juce::var json(result);
    result->setProperty("sampleRate", device->getCurrentSampleRate());
    result->setProperty("bufferFrames", device->getCurrentBufferSizeSamples());
    result->setProperty("callbacks", static_cast<int>(stats.blocks));
    result->setProperty("xrunDelta", before < 0 || after < 0 ? -1 : after - before);
    result->setProperty("reportedOutputLatencyFrames", device->getOutputLatencyInSamples());
    result->setProperty("retainedMetricP99Microseconds", times[times.size() * 99 / 100]);
    result->setProperty("playbackMaxMicroseconds", double(stats.maxCallbackNs) / 1000);
    result->setProperty("droppedMetrics", static_cast<int>(stats.metricDrops));
    result->setProperty("failureFlags", static_cast<int>(stats.failures));
    result->setProperty("scope",
                        "2-second silent device smoke test; no acoustic or sustained-load qualification");
    manager.closeAudioDevice();
    std::cout << juce::JSON::toString(json, true) << '\n';
}
} // namespace
int main(int argc, char **argv) {
    try {
        juce::ScopedJuceInitialiser_GUI juce;
        if (argc == 4 && std::string(argv[1]) == "--resave-project") {
            const auto cwd = juce::File::getCurrentWorkingDirectory();
            fader::saveProject(fader::loadProject(cwd.getChildFile(argv[2])), cwd.getChildFile(argv[3]));
            std::cout << "Project saved with current storage schema\n";
            return 0;
        }
        if ((argc == 4 || argc == 5) && std::string(argv[1]) == "--seek-benchmark") {
            const auto cwd = juce::File::getCurrentWorkingDirectory();
            if (argc == 5 && std::string(argv[4]) != "--uncached")
                throw std::runtime_error("Expected --uncached");
            std::cout << juce::JSON::toString(
                             benchmarkSeeks(cwd.getChildFile(argv[2]), cwd.getChildFile(argv[3]), argc == 4),
                             true)
                      << '\n';
            return 0;
        }
        if ((argc == 3 || argc == 4) && std::string(argv[1]) == "--qualify-stream") {
            if (argc == 4 && std::string(argv[3]) != "--uncached")
                throw std::runtime_error("Expected --uncached");
            std::cout << juce::JSON::toString(
                             qualifyStream(juce::File::getCurrentWorkingDirectory().getChildFile(argv[2]),
                                           argc == 3),
                             true)
                      << '\n';
            return 0;
        }
        if (argc == 4 && std::string(argv[1]) == "--streaming-soak") {
            const auto cwd = juce::File::getCurrentWorkingDirectory();
            streamingSoak(cwd.getChildFile(argv[2]), cwd.getChildFile(argv[3]));
            return 0;
        }
        if (argc == 2 && std::string(argv[1]) == "--library") {
            const auto library = fader::scanLibrary(fader::Preferences::load());
            auto *o = new juce::DynamicObject();
            juce::var json(o);
            int bpm = 0, key = 0, grids = 0;
            for (const auto &t : library.tracks) {
                bpm += t.bpm > 0;
                key += t.key.isNotEmpty();
                grids += t.hasGrid;
            }
            o->setProperty("tracks", static_cast<int>(library.tracks.size()));
            o->setProperty("traktorEntries", library.traktorEntries);
            o->setProperty("matchedTraktor", library.matched);
            o->setProperty("missing", library.missing);
            o->setProperty("withBPM", bpm);
            o->setProperty("withKey", key);
            o->setProperty("withGrid", grids);
            o->setProperty("warning", library.warning);
            std::cout << juce::JSON::toString(json, true) << '\n';
            return 0;
        }
        if (argc == 2 && std::string(argv[1]) == "--audio-smoke") {
            audioSmoke();
            return 0;
        }
        if (argc == 2 && std::string(argv[1]) == "--devices") {
            std::cout << juce::JSON::toString(fader::inspectDevices(), true) << '\n';
            return 0;
        }
        if (argc == 3 && std::string(argv[1]) == "--inspect") {
            const auto clip = fader::loadClip(juce::File::getCurrentWorkingDirectory().getChildFile(argv[2]));
            auto *result = new juce::DynamicObject();
            juce::var v(result);
            result->setProperty("sampleRate", clip.audio->sampleRate);
            result->setProperty("decodedFrames",
                                juce::String(static_cast<juce::int64>(clip.audio->frames())));
            result->setProperty("durationSeconds", static_cast<double>(clip.length) / fader::projectRate);
            result->setProperty("status", "decoded to end; source unchanged");
            std::cout << juce::JSON::toString(v, true) << '\n';
            return 0;
        }
        if (argc == 4 && std::string(argv[1]) == "--render") {
            const auto cwd = juce::File::getCurrentWorkingDirectory();
            fader::exportWav(fader::loadProject(cwd.getChildFile(argv[2])), cwd.getChildFile(argv[3]));
            std::cout << "WAV export complete\n";
            return 0;
        }
        std::cerr
            << "Usage: fader_probe --devices | --library | --audio-smoke | --inspect <audio> | --render "
               "<set.faderset> <output.wav> | --qualify-stream <audio> | --streaming-soak <A> <B>\n";
        return 2;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
