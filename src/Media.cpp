// SPDX-License-Identifier: AGPL-3.0-or-later
#include "Media.h"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <stdexcept>

namespace fader {
namespace {
[[noreturn]] void fail(const juce::String &s) {
    throw std::runtime_error(s.toStdString());
}
juce::var object() {
    return juce::var(new juce::DynamicObject());
}
void put(juce::var &v, const juce::Identifier &key, juce::var value) {
    v.getDynamicObject()->setProperty(key, value);
}
juce::var required(const juce::var &v, const juce::Identifier &key) {
    if (!v.getDynamicObject() || !v.getDynamicObject()->hasProperty(key))
        fail("Missing project field: " + key.toString());
    return v[key];
}
Frame integer(const juce::var &v) {
    if (!v.isString())
        fail("Project frame positions must be integer strings");
    const auto s = v.toString().toStdString();
    Frame result{};
    const auto parsed = std::from_chars(s.data(), s.data() + s.size(), result);
    if (parsed.ec != std::errc{} || parsed.ptr != s.data() + s.size())
        fail("Invalid project frame position");
    return result;
}
double number(const juce::var &v) {
    if (!v.isInt() && !v.isInt64() && !v.isDouble())
        fail("Invalid numeric project value");
    const double d = v;
    if (!std::isfinite(d))
        fail("Non-finite project value");
    return d;
}
} // namespace
Clip loadClip(const juce::File &file) {
    if (!file.existsAsFile())
        fail("Audio file is missing");
    if (file.hasFileExtension("crdownload;part;download"))
        fail("This download is incomplete");
    const auto sourceHash = juce::SHA256(file).toHexString();
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    // Stream overload probes content; File overload can reject a mismatched extension.
    auto reader = std::unique_ptr<juce::AudioFormatReader>(formats.createReaderFor(file.createInputStream()));
    if (!reader)
        fail("Unsupported or unreadable audio file");
    if (reader->numChannels < 1 || reader->numChannels > 2)
        fail("This POC supports mono or stereo source files");
    if (reader->lengthInSamples <= 0 || reader->lengthInSamples > 33554432)
        fail("Source exceeds the POC's 256 MiB decoded-audio limit per deck");
    if (!std::isfinite(reader->sampleRate) || reader->sampleRate < 8000 || reader->sampleRate > 384000)
        fail("Unsupported source sample rate");
    auto audio = std::make_shared<AudioData>();
    audio->sampleRate = reader->sampleRate;
    const auto length = static_cast<std::size_t>(reader->lengthInSamples);
    for (auto &channel : audio->samples)
        channel.resize(length);
    for (std::size_t offset = 0; offset < length; offset += 8192) {
        const int count = static_cast<int>(std::min(std::size_t{8192}, length - offset));
        float *destinations[]{audio->samples[0].data() + offset, audio->samples[1].data() + offset};
        if (!reader->read(destinations, 2, static_cast<juce::int64>(offset), count))
            fail("Audio decoding failed with " + reader->getFormatName() + " at source frame " +
                 juce::String(static_cast<juce::int64>(offset)));
        if (reader->numChannels == 1)
            std::copy_n(destinations[0], count, destinations[1]);
        for (int c = 0; c < 2; ++c)
            for (int i = 0; i < count; ++i)
                if (!std::isfinite(destinations[c][i]))
                    fail("Audio contains non-finite samples");
    }
    constexpr std::size_t peakWidth = 512;
    audio->peaks.resize((length + peakWidth - 1) / peakWidth);
    for (std::size_t i = 0; i < length; ++i)
        audio->peaks[i / peakWidth] = std::max(audio->peaks[i / peakWidth], std::abs(audio->samples[0][i]));
    Clip clip;
    clip.audio = audio;
    clip.path = file.getFullPathName().toStdString();
    clip.name = file.getFileNameWithoutExtension().toStdString();
    clip.hash = juce::SHA256(file).toHexString().toStdString();
    if (clip.hash != sourceHash.toStdString())
        fail("Audio file changed during decoding; try again after the file is stable");
    clip.length =
        static_cast<Frame>(std::floor(static_cast<double>(length) * projectRate / audio->sampleRate));
    return clip;
}
void saveProject(const Project &project, const juce::File &file) {
    project.validate();
    auto root = object();
    put(root, "schemaVersion", 1);
    put(root, "engineVersion", 1);
    put(root, "sampleRate", projectRate);
    juce::Array<juce::var> clips;
    for (const auto &clip : project.clips) {
        if (clip.length > 0 && clip.path.empty())
            fail("Cannot save a clip without a source file");
        if (!clip.path.empty() && juce::File(clip.path) == file)
            fail("A set cannot replace a source audio file");
        auto v = object();
        put(v, "path",
            clip.path.empty() ? juce::String{}
                              : juce::File(clip.path).getRelativePathFrom(file.getParentDirectory()));
        put(v, "sha256", juce::String(clip.hash));
        put(v, "start", juce::String(clip.start));
        put(v, "sourceOffset", juce::String(clip.sourceOffset));
        put(v, "length", juce::String(clip.length));
        put(v, "rate", clip.rate);
        put(v, "bpm", clip.bpm);
        put(v, "firstBeatSeconds", clip.firstBeatSeconds);
        clips.add(v);
    }
    put(root, "clips", clips);
    juce::Array<juce::var> lanes;
    for (const auto &curve : project.curves) {
        auto v = object();
        put(v, "initial", curve.initial);
        put(v, "interpolation", curve.linear ? "linear" : "step");
        juce::Array<juce::var> points;
        for (const auto &point : curve.points) {
            auto p = object();
            put(p, "frame", juce::String(point.frame));
            put(p, "value", point.value);
            points.add(p);
        }
        put(v, "points", points);
        lanes.add(v);
    }
    put(root, "automation", lanes);
    juce::TemporaryFile temporary(file);
    if (!temporary.getFile().replaceWithText(juce::JSON::toString(root, true)))
        fail("Could not write project");
    if (!temporary.overwriteTargetFileWithTemporary())
        fail("Could not replace project file");
}
Project loadProject(const juce::File &file) {
    if (!file.existsAsFile() || file.getSize() > 8 * 1024 * 1024)
        fail("Missing or oversized project file");
    const auto root = juce::JSON::parse(file);
    if (number(required(root, "schemaVersion")) != 1 || number(required(root, "engineVersion")) != 1 ||
        number(required(root, "sampleRate")) != projectRate)
        fail("Unsupported project or engine version");
    const auto clips = required(root, "clips");
    const auto lanes = required(root, "automation");
    if (!clips.isArray() || clips.size() != 2 || !lanes.isArray() || lanes.size() != 3)
        fail("Invalid project lane count");
    Project p;
    // Validate the manifest before any media decode.
    for (int i = 0; i < 2; ++i) {
        const auto c = clips[i];
        auto &clip = p.clips[static_cast<std::size_t>(i)];
        clip.path = required(c, "path").toString().toStdString();
        clip.hash = required(c, "sha256").toString().toStdString();
        clip.start = integer(required(c, "start"));
        clip.sourceOffset = integer(required(c, "sourceOffset"));
        clip.length = integer(required(c, "length"));
        clip.rate = number(required(c, "rate"));
        clip.bpm = number(required(c, "bpm"));
        clip.firstBeatSeconds = number(required(c, "firstBeatSeconds"));
    }
    for (int i = 0; i < 3; ++i) {
        const auto l = lanes[i];
        auto &curve = p.curves[static_cast<std::size_t>(i)];
        curve.initial = static_cast<float>(number(required(l, "initial")));
        const auto interpolation = required(l, "interpolation").toString();
        if (interpolation != "step" && interpolation != "linear")
            fail("Unknown interpolation mode");
        curve.linear = interpolation == "linear";
        const auto points = required(l, "points");
        if (!points.isArray() || points.size() > 200000)
            fail("Invalid automation points");
        for (const auto &point : *points.getArray())
            curve.points.push_back(
                {integer(required(point, "frame")), static_cast<float>(number(required(point, "value")))});
    }
    p.validate();
    for (auto &clip : p.clips) {
        if (clip.path.empty()) {
            if (clip.length > 0)
                fail("Clip has no source file");
            continue;
        }
        const auto source = file.getParentDirectory().getChildFile(clip.path);
        auto loaded = loadClip(source);
        if (loaded.hash != clip.hash)
            fail("Source content changed. Restore the original file before opening this set.");
        clip.audio = loaded.audio;
        clip.path = loaded.path;
        clip.name = loaded.name;
    }
    p.validate();
    return p;
}
void exportWav(const Project &project, const juce::File &file, bool floatingPoint,
               std::function<bool(double)> progress) {
    project.validate();
    if (project.end() <= 0)
        fail("The set is empty");
    for (const auto &clip : project.clips) {
        if (!clip.path.empty() && juce::File(clip.path) == file)
            fail("Export cannot replace a source audio file");
        if (clip.length > 0 && !clip.audio)
            fail("Missing source audio");
    }
    juce::TemporaryFile temporary(file);
    std::unique_ptr<juce::OutputStream> stream = temporary.getFile().createOutputStream();
    if (!stream)
        fail("Could not create export file");
    auto options =
        juce::AudioFormatWriterOptions{}
            .withSampleRate(projectRate)
            .withNumChannels(2)
            .withBitsPerSample(floatingPoint ? 32 : 24)
            .withSampleFormat(floatingPoint ? juce::AudioFormatWriterOptions::SampleFormat::floatingPoint
                                            : juce::AudioFormatWriterOptions::SampleFormat::integral);
    auto writer = juce::WavAudioFormat{}.createWriterFor(stream, options);
    if (!writer)
        fail("Could not create WAV writer");
    Engine engine(project);
    juce::AudioBuffer<float> buffer(2, 1024);
    while (engine.position() < project.end()) {
        const int count = static_cast<int>(std::min(Frame{1024}, project.end() - engine.position()));
        engine.process(buffer.getArrayOfWritePointers(), 2, count);
        if (!floatingPoint && engine.peak() > 1.0f)
            fail("Mix exceeds 0 dBFS. Lower the deck gains before exporting PCM WAV.");
        if (!writer->writeFromFloatArrays(buffer.getArrayOfReadPointers(), 2, count))
            fail("WAV export failed (check available disk space)");
        if (progress &&
            !progress(static_cast<double>(engine.position()) / static_cast<double>(project.end())))
            fail("Export cancelled");
    }
    if (!writer->flush())
        fail("Could not flush WAV export");
    writer.reset();
    if (!temporary.overwriteTargetFileWithTemporary())
        fail("Could not finish WAV export");
}
} // namespace fader
