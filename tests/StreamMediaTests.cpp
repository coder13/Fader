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
        check(int(mp3Result["seekWindows"]) > 0 && !bool(mp3Result["sequentialSeek"]) &&
                  mp3Result["diskCacheBytes"].toString().getLargeIntValue() > 0,
              "MP3 qualification did not use its disk cache");
        const auto vbrResult = qualifyStream(juce::File(FADER_TEST_VBR_PATH));
        check(int(vbrResult["seekWindows"]) == 14 && double(vbrResult["seekMaxError"]) <= 1e-6,
              "VBR MP3 seek/tail qualification failed");
        check(!bool(vbrResult["sequentialSeek"]) &&
                  vbrResult["diskCacheBytes"].toString().getLargeIntValue() > 0,
              "VBR MP3 disk cache was silently bypassed");
        const auto fallback = qualifyStream(juce::File(FADER_TEST_VBR_PATH), false);
        check(bool(fallback["sequentialSeek"]) == (fallback["format"].toString() == "MP3 file") &&
                  fallback["diskCacheBytes"].toString().getLargeIntValue() == 0 &&
                  double(fallback["seekMaxError"]) <= 1e-6,
              "Uncached MP3 fallback lost exact seek support");
        const auto copiedMP3 = dir.getChildFile("cached.mp3");
        check(juce::File(FADER_TEST_VBR_PATH).copyFileTo(copiedMP3), "Cannot copy synthetic MP3");
        const auto cachedClip = loadClip(copiedMP3);
        check(cachedClip.audio->stream->diskCacheBytes() > 0, "MP3 source has no cache");
        auto decoder = cachedClip.audio->stream->open();
        std::array<float, 128> left{}, right{};
        float *stereo[]{left.data(), right.data()};
        std::atomic<bool> cancel{false};
        check(decoder && decoder->read(0, 128, stereo, cancel), "Cached source cannot read");
        copiedMP3.appendText("source changed");
        check(!decoder->read(0, 128, stereo, cancel), "Cache hid an original-source change");
        copiedMP3.deleteFile();
        bool missingCachedSource = false;
        try {
            cachedClip.audio->stream->open();
        } catch (...) {
            missingCachedSource = true;
        }
        check(missingCachedSource, "Cache hid a missing original source");
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
