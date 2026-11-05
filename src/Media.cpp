// SPDX-License-Identifier: AGPL-3.0-or-later
#include "Media.h"
#include "FileSource.h"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <filesystem>
#include <stdexcept>
#include <unordered_map>

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
juce::var writeCurve(const Curve &curve) {
    auto v = object();
    put(v, "initial", curve.initial);
    put(v, "interpolation", curve.linear ? "linear" : "step");
    juce::Array<juce::var> points;
    for (const auto &point : curve.points) {
        auto p = object();
        put(p, "frame", juce::String(point.frame));
        put(p, "value", point.value);
        put(p, "hold", point.hold);
        points.add(p);
    }
    put(v, "points", points);
    return v;
}
Curve readCurve(const juce::var &v) {
    Curve curve;
    curve.initial = static_cast<float>(number(required(v, "initial")));
    const auto mode = required(v, "interpolation").toString();
    if (mode != "step" && mode != "linear")
        fail("Unknown automation interpolation");
    curve.linear = mode == "linear";
    const auto points = required(v, "points");
    if (!points.isArray() || points.size() > 200000)
        fail("Invalid automation points");
    for (const auto &p : *points.getArray())
        curve.points.push_back({integer(required(p, "frame")),
                                static_cast<float>(number(required(p, "value"))),
                                static_cast<bool>(p["hold"])});
    return curve;
}
} // namespace
Clip loadClip(const juce::File &file) {
    if (!file.existsAsFile())
        fail("Audio file is missing");
    if (file.hasFileExtension("crdownload;part;download"))
        fail("This download is incomplete");
    const auto sourceBytes = file.getSize();
    const auto sourceModified = file.getLastModificationTime().toMilliseconds();
    const auto sourceHash = juce::SHA256(file).toHexString();
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    // Stream overload probes content; File overload can reject a mismatched extension.
    auto reader = std::unique_ptr<juce::AudioFormatReader>(formats.createReaderFor(file.createInputStream()));
    if (!reader)
        fail("Unsupported or unreadable audio file");
    if (reader->numChannels < 1 || reader->numChannels > 2)
        fail("This POC supports mono or stereo source files");
    if (reader->lengthInSamples <= 0 || reader->lengthInSamples > maxSourceFrames)
        fail("Invalid or unsupported source length");
    if (!std::isfinite(reader->sampleRate) || reader->sampleRate < 8000 || reader->sampleRate > 384000)
        fail("Unsupported source sample rate");
    auto audio = std::make_shared<AudioData>();
    audio->sampleRate = reader->sampleRate;
    const auto length = static_cast<std::size_t>(reader->lengthInSamples);
    const auto projectLength =
        static_cast<Frame>(std::floor(double(length) * projectRate / audio->sampleRate));
    if (projectLength > maxProjectFrames)
        fail("Source exceeds the prototype's 24-hour timeline limit");
    constexpr std::size_t maxPeaks = 131072;
    audio->peakStride = std::max(std::size_t{512}, ((length + maxPeaks - 1) / maxPeaks + 511) / 512 * 512);
    audio->peaks.resize((length + audio->peakStride - 1) / audio->peakStride);
    juce::AudioBuffer<float> scratch(2, 8192);
    for (std::size_t offset = 0; offset < length; offset += 8192) {
        const int count = static_cast<int>(std::min(std::size_t{8192}, length - offset));
        if (!reader->read(scratch.getArrayOfWritePointers(), 2, static_cast<juce::int64>(offset), count))
            fail("Audio decoding failed with " + reader->getFormatName() + " at source frame " +
                 juce::String(static_cast<juce::int64>(offset)));
        for (int channel = 0; channel < 2; ++channel)
            for (int i = 0; i < count; ++i)
                if (!std::isfinite(scratch.getSample(channel, i)))
                    fail("Audio contains non-finite samples");
        for (int i = 0; i < count; ++i) {
            auto &peak = audio->peaks[(offset + static_cast<std::size_t>(i)) / audio->peakStride];
            peak = std::max(peak, std::abs(scratch.getSample(0, i)));
        }
    }
    audio->stream = makeFileSource(file, reader->lengthInSamples, reader->sampleRate, reader->numChannels,
                                   sourceBytes, sourceModified);
    Clip clip;
    clip.audio = audio;
    clip.path = file.getFullPathName().toStdString();
    clip.name = file.getFileNameWithoutExtension().toStdString();
    clip.hash = juce::SHA256(file).toHexString().toStdString();
    if (clip.hash != sourceHash.toStdString() || file.getSize() != sourceBytes ||
        file.getLastModificationTime().toMilliseconds() != sourceModified)
        fail("Audio file changed during decoding; try again after the file is stable");
    clip.length =
        static_cast<Frame>(std::floor(static_cast<double>(length) * projectRate / audio->sampleRate));
    return clip;
}
namespace {
juce::var projectDocument(const Project &project, const juce::File &file) {
    project.validate();
    // Relative paths must share the same physical base, even if a native chooser
    // returns a directory alias such as /tmp instead of /private/tmp on macOS.
    const auto base = juce::File(
        std::filesystem::weakly_canonical(file.getParentDirectory().getFullPathName().toStdString())
            .string());
    auto root = object();
    put(root, "schemaVersion", project.perClip ? 2 : 1);
    put(root, "engineVersion", project.perClip ? 2 : 1);
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
                              : juce::File(std::filesystem::weakly_canonical(clip.path).string())
                                    .getRelativePathFrom(base));
        put(v, "sha256", juce::String(clip.hash));
        put(v, "start", juce::String(clip.start));
        put(v, "sourceOffset", juce::String(clip.sourceOffset));
        put(v, "length", juce::String(clip.length));
        put(v, "rate", clip.rate);
        put(v, "bpm", clip.bpm);
        put(v, "firstBeatSeconds", clip.firstBeatSeconds);
        if (project.perClip) {
            put(v, "row", clip.row);
            put(v, "name", juce::String(clip.name));
            put(v, "artist", juce::String(clip.artist));
            put(v, "key", juce::String(clip.key));
            put(v, "hasGrid", clip.hasGrid);
            juce::Array<juce::var> automation;
            for (const auto &c : clip.automation)
                automation.add(writeCurve(c));
            put(v, "automation", automation);
        }
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
    return root;
}
void writeDocument(const juce::var &root, const juce::File &file) {
    juce::TemporaryFile temporary(file);
    if (!temporary.getFile().replaceWithText(juce::JSON::toString(root, true)))
        fail("Could not write project");
    if (!temporary.overwriteTargetFileWithTemporary())
        fail("Could not replace project file");
}
} // namespace
void saveProject(const Project &project, const juce::File &file) {
    writeDocument(projectDocument(project, file), file);
}
void saveIncompleteTake(const IncompleteTake &take, const juce::File &file) {
    if (!take.stats.failures || !take.stats.segment || take.events.size() > 200000 || take.in < 0 ||
        take.out <= take.in || take.out > take.original.end())
        fail("Invalid incomplete capture");
    auto root = object();
    put(root, "format", "fader-incomplete-capture");
    put(root, "schemaVersion", 1);
    put(root, "complete", false);
    put(root, "project", projectDocument(take.original, file));
    put(root, "segment", juce::String(take.stats.segment));
    put(root, "rangeIn", juce::String(take.in));
    put(root, "rangeOut", juce::String(take.out));
    put(root, "actualProjectEnd", juce::String(take.stats.projectFrame));
    put(root, "engineFrames", juce::String(take.stats.engineFrame));
    put(root, "failureFlags", static_cast<int>(take.stats.failures));
    put(root, "inputDrops", juce::String(take.stats.inputDrops));
    put(root, "lateInputs", juce::String(take.stats.lateInputs));
    juce::Array<juce::var> events;
    Frame previous = -1;
    for (const auto &event : take.events) {
        if (event.frame < previous || event.frame < take.in || event.frame >= take.out ||
            static_cast<unsigned>(event.param) >= 3 || event.clip >= take.original.clips.size() ||
            !std::isfinite(event.value) || event.value < 0 || event.value > 1)
            fail("Invalid recovered action");
        previous = event.frame;
        auto item = object();
        put(item, "frame", juce::String(event.frame));
        put(item, "parameter", static_cast<int>(event.param));
        put(item, "value", event.value);
        put(item, "clip", static_cast<int>(event.clip));
        events.add(item); // ordered array preserves same-frame application order
    }
    put(root, "appliedActions", events);
    writeDocument(root, file);
}
Project loadProject(const juce::File &file) {
    if (!file.existsAsFile() || file.getSize() > 8 * 1024 * 1024)
        fail("Missing or oversized project file");
    const auto root = juce::JSON::parse(file);
    const auto version = number(required(root, "schemaVersion"));
    if ((version != 1 && version != 2) || number(required(root, "engineVersion")) != version ||
        number(required(root, "sampleRate")) != projectRate)
        fail("Unsupported project or engine version");
    const auto clips = required(root, "clips");
    const auto lanes = required(root, "automation");
    if (!clips.isArray() || clips.size() > 64 || (version == 1 && clips.size() != 2) || !lanes.isArray() ||
        lanes.size() != 3)
        fail("Invalid project lane count");
    Project p;
    p.perClip = version == 2;
    p.clips.resize(static_cast<std::size_t>(clips.size()));
    // Validate the manifest before any media decode.
    for (int i = 0; i < clips.size(); ++i) {
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
        clip.row = i;
        if (p.perClip) {
            const auto row = number(required(c, "row"));
            if (row < 0 || row > 4095 || std::floor(row) != row)
                fail("Invalid arrangement row");
            clip.row = static_cast<int>(row);
            clip.name = required(c, "name").toString().toStdString();
            clip.artist = required(c, "artist").toString().toStdString();
            clip.key = required(c, "key").toString().toStdString();
            clip.hasGrid = static_cast<bool>(c["hasGrid"]);
            const auto a = required(c, "automation");
            if (!a.isArray() || a.size() != 3)
                fail("Invalid clip automation count");
            for (int j = 0; j < 3; ++j)
                clip.automation[static_cast<std::size_t>(j)] = readCurve(a[j]);
        }
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
    std::unordered_map<std::string, Clip> media;
    std::size_t peakBytes = 0;
    for (auto &clip : p.clips) {
        if (clip.path.empty()) {
            if (clip.length > 0)
                fail("Clip has no source file");
            continue;
        }
        const auto base = juce::File(
            std::filesystem::weakly_canonical(file.getParentDirectory().getFullPathName().toStdString())
                .string());
        const auto source = base.getChildFile(clip.path);
        const auto path = source.getFullPathName().toStdString();
        if (!media.contains(path)) {
            auto loaded = loadClip(source);
            peakBytes += loaded.audio->peaks.size() * sizeof(float);
            if (peakBytes > 32ULL * 1024 * 1024)
                fail("Set exceeds the prototype's 32 MiB waveform-cache budget");
            media.emplace(path, std::move(loaded));
        }
        const auto &loaded = media.at(path);
        if (loaded.hash != clip.hash)
            fail("Source content changed. Restore the original file before opening this set.");
        clip.audio = loaded.audio;
        clip.path = loaded.path;
        if (clip.name.empty())
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
        engine.prepareSources(count); // offline may wait/fail; the live callback never calls this
        engine.process(buffer.getArrayOfWritePointers(), 2, count);
        // A concurrent prefetch failure can become visible after readiness but
        // during this span, including the final span with no following prepare.
        if (engine.starvationFrames() || engine.streamStats().failedSources)
            fail("Audio source became unavailable during export");
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
