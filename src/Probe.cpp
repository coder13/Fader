// SPDX-License-Identifier: AGPL-3.0-or-later
#include "Library.h"
#include "Media.h"
#include <algorithm>
#include <chrono>
#include <iostream>
namespace {
class SilentDeviceProbe final : public juce::AudioIODeviceCallback {
  public:
    SilentDeviceProbe() : engine(makeProject()) {}
    void audioDeviceIOCallbackWithContext(const float *const *, int, float *const *out, int channels,
                                          int frames, const juce::AudioIODeviceCallbackContext &) override {
        const auto start = std::chrono::steady_clock::now();
        engine.process(out, channels, frames);
        if (count < times.size())
            times[count++] =
                std::chrono::duration<double, std::micro>(std::chrono::steady_clock::now() - start).count();
    }
    void audioDeviceAboutToStart(juce::AudioIODevice *) override {}
    void audioDeviceStopped() override {}
    static fader::Project makeProject() {
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
    fader::Engine engine;
    std::array<double, 4096> times{};
    std::size_t count = 0;
};
void audioSmoke() {
    juce::AudioDeviceManager manager;
    auto error = manager.initialise(0, 2, nullptr, true);
    if (error.isNotEmpty())
        throw std::runtime_error(error.toStdString());
    auto settings = manager.getAudioDeviceSetup();
    settings.sampleRate = 48000;
    settings.bufferSize = 128;
    error = manager.setAudioDeviceSetup(settings, true);
    if (error.isNotEmpty())
        throw std::runtime_error(error.toStdString());
    auto *device = manager.getCurrentAudioDevice();
    if (!device || device->getCurrentSampleRate() != 48000)
        throw std::runtime_error("48 kHz device unavailable");
    SilentDeviceProbe callback;
    const int before = device->getXRunCount();
    manager.addAudioCallback(&callback);
    juce::Thread::sleep(2000);
    manager.removeAudioCallback(&callback);
    const int after = device->getXRunCount();
    if (callback.count == 0)
        throw std::runtime_error("No device callbacks received");
    std::sort(callback.times.begin(), callback.times.begin() + static_cast<std::ptrdiff_t>(callback.count));
    auto *result = new juce::DynamicObject();
    juce::var json(result);
    result->setProperty("sampleRate", device->getCurrentSampleRate());
    result->setProperty("bufferFrames", device->getCurrentBufferSizeSamples());
    result->setProperty("callbacks", static_cast<int>(callback.count));
    result->setProperty("xrunDelta", before < 0 || after < 0 ? -1 : after - before);
    result->setProperty("reportedOutputLatencyFrames", device->getOutputLatencyInSamples());
    result->setProperty("coreP99Microseconds", callback.times[callback.count * 99 / 100]);
    result->setProperty("coreMaxMicroseconds", callback.times[callback.count - 1]);
    result->setProperty("scope",
                        "2-second silent device smoke test; no acoustic or sustained-load qualification");
    manager.closeAudioDevice();
    std::cout << juce::JSON::toString(json, true) << '\n';
}
} // namespace
int main(int argc, char **argv) {
    try {
        juce::ScopedJuceInitialiser_GUI juce;
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
               "<set.faderset> <output.wav>\n";
        return 2;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
