// SPDX-License-Identifier: AGPL-3.0-or-later
#include "Media.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
using namespace fader;
void check(bool pass, const char *message) {
    if (!pass)
        throw std::runtime_error(message);
}
int main() {
    juce::ScopedJuceInitialiser_GUI juce;
    const auto dir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                         .getNonexistentChildFile("fader-test", {}, false);
    dir.createDirectory();
    try {
        auto data = std::make_shared<AudioData>();
        for (auto &c : data->samples) {
            c.resize(48000);
            for (int i = 0; i < 48000; ++i)
                c[static_cast<std::size_t>(i)] = 0.2f * std::sin(float(i) * 0.02f);
        }
        Project source;
        source.clips[0].audio = data;
        source.clips[0].length = 48000;
        const auto wave = dir.getChildFile("source.audio");
        exportWav(source, wave, true);
        auto clip = loadClip(wave);
        check(clip.audio->frames() == 48000, "header-based decoding failed");
        Project p;
        p.clips[0] = clip;
        p.curves[0].points = {{128, 0.2f}, {12000, 0.8f}};
        const auto file = dir.getChildFile("set.faderset");
        saveProject(p, file);
        const auto reopened = loadProject(file);
        check(reopened.curves[0].points == p.curves[0].points, "automation did not round-trip");
        check(reopened.clips[0].hash == p.clips[0].hash, "source identity not preserved");
        const auto output = dir.getChildFile("output.wav");
        exportWav(reopened, output, true);
        auto rendered = loadClip(output);
        Engine e(p);
        juce::AudioBuffer<float> expected(2, 48000);
        e.process(expected.getArrayOfWritePointers(), 2, 48000);
        for (int i = 0; i < 48000; ++i)
            check(std::abs(expected.getSample(0, i) -
                           rendered.audio->samples[0][static_cast<std::size_t>(i)]) < 1e-6f,
                  "offline/playback mismatch");
        const auto preserved = dir.getChildFile("preserved.wav");
        preserved.replaceWithText("keep me");
        bool cancelled = false;
        try {
            exportWav(p, preserved, false, [](double) { return false; });
        } catch (...) {
            cancelled = true;
        }
        check(cancelled && preserved.loadFileAsString() == "keep me", "cancel damaged existing destination");
        auto loud = std::make_shared<AudioData>();
        for (auto &c : loud->samples)
            c.assign(1024, 2.0f);
        Project clipping;
        clipping.clips[0].audio = loud;
        clipping.clips[0].length = 1024;
        clipping.curves[0].initial = 1;
        clipping.curves[2].initial = -1;
        bool rejectedClipping = false;
        try {
            exportWav(clipping, preserved);
        } catch (...) {
            rejectedClipping = true;
        }
        check(rejectedClipping && preserved.loadFileAsString() == "keep me",
              "clipped export replaced destination");
        wave.appendText("changed");
        bool rejected = false;
        try {
            loadProject(file);
        } catch (...) {
            rejected = true;
        }
        check(rejected, "changed source hash accepted");
        auto json = juce::JSON::parse(file);
        json.getDynamicObject()->setProperty("schemaVersion", 99);
        file.replaceWithText(juce::JSON::toString(json));
        rejected = false;
        try {
            loadProject(file);
        } catch (...) {
            rejected = true;
        }
        check(rejected, "future schema accepted");
        dir.deleteRecursively();
        std::cout << "PASS: content probing, full decode, project round-trip, source identity, export "
                     "parity, atomic cancellation, schema validation\n";
        return 0;
    } catch (const std::exception &e) {
        dir.deleteRecursively();
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
