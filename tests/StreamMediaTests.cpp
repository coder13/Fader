// SPDX-License-Identifier: AGPL-3.0-or-later
#include "StreamQualification.h"
#include <cmath>
#include <stdexcept>

namespace {
void check(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
} // namespace
void testStreamMedia() {
    using namespace fader;
    const auto dir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                         .getNonexistentChildFile("fader-stream-media", {}, false);
    dir.createDirectory();
    try {
        Project project;
        project.perClip = true;
        project.clips.resize(1);
        auto audio = std::make_shared<AudioData>();
        for (unsigned channel = 0; channel < 2; ++channel) {
            audio->samples[channel].resize(240000);
            for (unsigned frame = 0; frame < 240000; ++frame)
                audio->samples[channel][frame] =
                    0.2f * std::sin(float(frame) * (channel == 0 ? 0.013f : 0.019f));
        }
        project.clips[0].audio = audio;
        project.clips[0].length = 240000;
        const auto file = dir.getChildFile("synthetic.wav");
        exportWav(project, file);
        const auto waveResult = qualifyStream(file);
        check(double(waveResult["sequentialMaxError"]) == 0 && double(waveResult["seekMaxError"]) == 0,
              "WAV PCM paging is not exact");
        // Redistributable MP3 fixture from the exact pinned JUCE checkout; no
        // network, external encoder or private library required for CI.
        const auto mp3Result = qualifyStream(juce::File(FADER_TEST_MP3_PATH));
        check(int(mp3Result["seekWindows"]) > 0, "MP3 seek qualification did not run");
        const auto vbrResult = qualifyStream(juce::File(FADER_TEST_VBR_PATH));
        check(int(vbrResult["seekWindows"]) == 14 && double(vbrResult["seekMaxError"]) <= 1e-6,
              "VBR MP3 seek/tail qualification failed");
        const auto imported = loadClip(file);
        StreamBank bank({imported.audio->stream});
        bank.prepare(0, 0, 128);
        file.appendText("source changed");
        bool changed = false;
        try {
            bank.prepare(0, 200000, 200128);
        } catch (...) {
            changed = true;
        }
        check(changed && bank.stats().failedSources == 1, "changed streaming source was accepted");
        project.clips[0] = imported;
        const auto destination = dir.getChildFile("keep.wav");
        destination.replaceWithText("keep destination");
        bool failed = false;
        try {
            exportWav(project, destination);
        } catch (...) {
            failed = true;
        }
        check(failed && destination.loadFileAsString() == "keep destination",
              "offline source failure replaced the existing destination");
        file.deleteFile();
        StreamBank missing({imported.audio->stream});
        failed = false;
        try {
            missing.prepare(0, 0, 128);
        } catch (...) {
            failed = true;
        }
        check(failed && missing.error(0) == SourceError::open, "missing source opened as valid audio");
        dir.deleteRecursively();
    } catch (...) {
        dir.deleteRecursively();
        throw;
    }
}
