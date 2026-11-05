// SPDX-License-Identifier: AGPL-3.0-or-later
#include "Media.h"
#include <cmath>
#include <stdexcept>

namespace {
void check(bool pass, const char *message) {
    if (!pass)
        throw std::runtime_error(message);
}
struct FixtureDirectory {
    juce::File file = juce::File::getSpecialLocation(juce::File::tempDirectory)
                          .getNonexistentChildFile("fader-source-rates", {}, false);
    FixtureDirectory() {
        check(file.createDirectory().wasOk(), "could not create source-rate fixture directory");
    }
    ~FixtureDirectory() {
        file.deleteRecursively();
    }
};
void writeSource(const juce::File &file, int rate, int channels, int frames) {
    std::unique_ptr<juce::OutputStream> stream = file.createOutputStream();
    check(stream != nullptr, "could not create fixture source");
    auto options = juce::AudioFormatWriterOptions{}
                       .withSampleRate(rate)
                       .withNumChannels(channels)
                       .withBitsPerSample(32)
                       .withSampleFormat(juce::AudioFormatWriterOptions::SampleFormat::floatingPoint);
    auto writer = juce::WavAudioFormat{}.createWriterFor(stream, options);
    check(writer != nullptr, "could not create fixture writer");
    juce::AudioBuffer<float> samples(channels, frames);
    for (int c = 0; c < channels; ++c)
        for (int i = 0; i < frames; ++i)
            samples.setSample(c, i, c == 0 ? 0.25f : -0.125f);
    check(writer->writeFromAudioSampleBuffer(samples, 0, frames) && writer->flush(),
          "could not write source-rate fixture");
}
} // namespace

void testSourceRates() {
    using namespace fader;
    FixtureDirectory directory;
    for (const int rate : {44100, 48000, 96000}) {
        for (const int channels : {1, 2}) {
            const auto source =
                directory.file.getChildFile(juce::String(rate) + "-" + juce::String(channels) + ".wav");
            const int frames = rate + 17;
            writeSource(source, rate, channels, frames);
            auto clip = loadClip(source);
            check(clip.audio->sampleRate == rate && clip.audio->frames() == std::size_t(frames),
                  "decode changed source rate or frame count");
            check(clip.length == Frame(std::floor(double(frames) * projectRate / rate)),
                  "import duration has wrong project-frame rounding");
            for (std::size_t i = 0; i < clip.audio->frames(); ++i) {
                check(clip.audio->samples[0][i] == 0.25f, "left source level changed");
                check(clip.audio->samples[1][i] == (channels == 1 ? 0.25f : -0.125f),
                      "mono duplication or stereo decoding changed the right level");
            }
            Project project;
            project.perClip = true;
            project.clips = {clip};
            project.clips[0].cue = true;
            project.clips[0].automation[0] = {0.5f, true, {{0, 0.5f}}};
            const auto output = directory.file.getChildFile("cue-on.wav");
            exportWav(project, output, true);
            juce::AudioFormatManager formats;
            formats.registerBasicFormats();
            auto reader = std::unique_ptr<juce::AudioFormatReader>(formats.createReaderFor(output));
            check(reader && reader->numChannels == 2 && reader->sampleRate == projectRate &&
                      reader->lengthInSamples == clip.length,
                  "export did not retain stereo master format and duration");
            reader.reset();
            const auto rendered = loadClip(output);
            for (std::size_t i = 0; i < rendered.audio->frames(); ++i) {
                check(std::abs(rendered.audio->samples[0][i] - 0.125f) < 1e-6f,
                      "import/resample/export changed left gain");
                check(std::abs(rendered.audio->samples[1][i] - (channels == 1 ? 0.125f : -0.0625f)) < 1e-6f,
                      "import/resample/export changed right gain");
            }
            project.clips[0].cue = false;
            const auto withoutCue = directory.file.getChildFile("cue-off.wav");
            exportWav(project, withoutCue, true);
            check(juce::SHA256(output) == juce::SHA256(withoutCue), "cue state changed exported master");
        }
    }
    Project overload;
    overload.perClip = true;
    auto loud = std::make_shared<AudioData>();
    loud->samples[0].assign(512, 0.75f);
    loud->samples[1].assign(512, -0.75f);
    for (auto &clip : overload.clips) {
        clip.audio = loud;
        clip.length = 512;
        clip.automation[0] = {1, true, {{0, 1}}};
    }
    const auto pcm = directory.file.getChildFile("overload.wav");
    check(pcm.replaceWithText("preserve destination"), "could not create export destination fixture");
    bool rejected = false;
    try {
        exportWav(overload, pcm, false);
    } catch (const std::exception &error) {
        rejected = std::string(error.what()).find("0 dBFS") != std::string::npos;
    }
    check(rejected && pcm.loadFileAsString() == "preserve destination",
          "PCM overload was not rejected atomically");
    exportWav(overload, pcm, true);
    const auto floating = loadClip(pcm);
    for (std::size_t i = 0; i < floating.audio->frames(); ++i) {
        check(floating.audio->samples[0][i] == 1.5f && floating.audio->samples[1][i] == -1.5f,
              "float export silently clipped overloaded mix");
    }
}
