// SPDX-License-Identifier: AGPL-3.0-or-later
#include "Media.h"
#include "DecodedCache.h"
#include "FilePaths.h"
#include "FileSource.h"
#include "ProjectStorage.h"
#include "WavExport.h"
#include <algorithm>
#include <bit>
#include <charconv>
#include <cmath>
#include <filesystem>
#include <set>
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
juce::String text(const juce::var &v, std::size_t maxBytes = 4096) {
    if (!v.isString() || v.toString().getNumBytesAsUTF8() > maxBytes || v.toString().containsChar(0))
        fail("Invalid project text");
    return v.toString();
}
bool boolean(const juce::var &v) {
    if (!v.isBool())
        fail("Project flags must be booleans");
    return static_cast<bool>(v);
}
Curve readCurve(const juce::var &v) {
    Curve curve;
    curve.initial = static_cast<float>(number(required(v, "initial")));
    const auto mode = text(required(v, "interpolation"));
    if (mode != "step" && mode != "linear")
        fail("Unknown automation interpolation");
    curve.linear = mode == "linear";
    const auto points = required(v, "points");
    if (!points.isArray() || points.size() > 200000)
        fail("Invalid automation points");
    for (const auto &p : *points.getArray())
        curve.points.push_back({integer(required(p, "frame")),
                                static_cast<float>(number(required(p, "value"))),
                                p.hasProperty("hold") ? boolean(p["hold"]) : false});
    return curve;
}
} // namespace
Clip loadClip(const juce::File &file, bool cacheCompressed) {
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
    audio->sourceChannels = reader->numChannels;
    audio->decoder = reader->getFormatName().toStdString();
    const auto length = static_cast<std::size_t>(reader->lengthInSamples);
    const auto projectLength =
        static_cast<Frame>(std::floor(double(length) * projectRate / audio->sampleRate));
    if (projectLength > maxProjectFrames)
        fail("Source exceeds the prototype's 24-hour timeline limit");
    constexpr std::size_t maxPeaks = 131072;
    audio->peakStride = std::max(std::size_t{512}, ((length + maxPeaks - 1) / maxPeaks + 511) / 512 * 512);
    audio->peaks.resize((length + audio->peakStride - 1) / audio->peakStride);
    juce::AudioBuffer<float> scratch(2, 8192);
    const bool sequentialReplay = reader->getFormatName() == "MP3 file";
    // CoreAudio precedes JUCE's MP3 reader when the native codec is available.
    // Preserve that reader's exact PCM domain too; decoder priming/padding can differ.
    const bool cacheCandidate = sequentialReplay || reader->getFormatName() == "CoreAudio supported file";
    auto decoded =
        cacheCandidate && cacheCompressed ? DecodedCache::create(reader->lengthInSamples) : nullptr;
    // Fixed-block SHA-256 chain of canonical stereo float PCM. Bounded scratch;
    // catches decoder-origin/padding/sample differences despite identical file SHA.
    auto pcmDigest = juce::SHA256(juce::String("fader-pcm-v1").toUTF8()).getRawData();
    juce::MemoryBlock pcmBlock(32 + 8192 * 2 * sizeof(float));
    for (std::size_t offset = 0; offset < length; offset += 8192) {
        const int count = static_cast<int>(std::min(std::size_t{8192}, length - offset));
        if (!reader->read(scratch.getArrayOfWritePointers(), 2, static_cast<juce::int64>(offset), count))
            fail("Audio decoding failed with " + reader->getFormatName() + " at source frame " +
                 juce::String(static_cast<juce::int64>(offset)));
        if (reader->numChannels == 1)
            scratch.copyFrom(1, 0, scratch, 0, 0, count);
        for (int channel = 0; channel < 2; ++channel)
            for (int i = 0; i < count; ++i)
                if (!std::isfinite(scratch.getSample(channel, i)))
                    fail("Audio contains non-finite samples");
        for (int i = 0; i < count; ++i) {
            auto &peak = audio->peaks[(offset + static_cast<std::size_t>(i)) / audio->peakStride];
            peak = std::max(peak, std::abs(scratch.getSample(0, i)));
        }
        auto *bytes = static_cast<unsigned char *>(pcmBlock.getData());
        std::copy_n(static_cast<const unsigned char *>(pcmDigest.getData()), 32, bytes);
        for (int frame = 0; frame < count; ++frame)
            for (int channel = 0; channel < 2; ++channel) {
                const auto bits = std::bit_cast<std::uint32_t>(scratch.getSample(channel, frame));
                const auto index = 32 + (frame * 2 + channel) * 4;
                for (int byte = 0; byte < 4; ++byte)
                    bytes[index + byte] = static_cast<unsigned char>(bits >> (byte * 8));
            }
        pcmDigest = juce::SHA256(bytes, 32 + count * 2 * sizeof(float)).getRawData();
        // A full/unavailable cache must not turn readable audio into an import failure.
        if (decoded && !decoded->append(scratch, count))
            decoded.reset();
    }
    audio->pcmHash = juce::String::toHexString(pcmDigest.getData(), static_cast<int>(pcmDigest.getSize()))
                         .removeCharacters(" ")
                         .toStdString();
    if (decoded && !decoded->finish())
        decoded.reset();
    audio->stream =
        makeFileSource(file, reader->lengthInSamples, reader->sampleRate, reader->numChannels, sourceBytes,
                       sourceModified, sequentialReplay, cacheCandidate, std::move(decoded));
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
void writeDocument(const juce::var &root, const juce::File &file) {
    const auto content = juce::JSON::toString(root, true);
    if (content.getNumBytesAsUTF8() > maxManifestBytes)
        fail("Project exceeds the 8 MiB manifest limit");
    juce::TemporaryFile temporary(file);
    if (!temporary.getFile().replaceWithText(content))
        fail("Could not write project");
    if (!temporary.overwriteTargetFileWithTemporary())
        fail("Could not replace project file");
}
juce::var readManifest(const juce::File &file) {
    auto input = file.createInputStream();
    if (!input)
        fail("Missing project file");
    juce::MemoryBlock block;
    input->readIntoMemoryBlock(block, maxManifestBytes + 1);
    if (block.getSize() > static_cast<std::size_t>(maxManifestBytes))
        fail("Oversized project file");
    const std::string bytes(static_cast<const char *>(block.getData()), block.getSize());
    if (bytes.find('\0') != std::string::npos ||
        !juce::CharPointer_UTF8::isValidString(bytes.data(), static_cast<int>(bytes.size())))
        fail("Project must contain valid UTF-8 text");
    struct Level {
        char close;
        bool key;
        std::set<std::string> keys;
    };
    std::vector<Level> levels;
    bool rootClosed = false;
    // Bound parser recursion and reject duplicate keys before JUCE coalesces them.
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        const auto c = bytes[i];
        if (rootClosed && c != ' ' && c != '\t' && c != '\r' && c != '\n')
            fail("Unexpected text after project object");
        if (c == '\'')
            fail("Project JSON requires double-quoted text");
        if (c == '"') {
            const auto first = i;
            bool ended = false;
            while (++i < bytes.size()) {
                if (bytes[i] == '\\') {
                    if (i + 1 >= bytes.size() ||
                        std::string_view("\"\\/bfnrtu").find(bytes[i + 1]) == std::string_view::npos)
                        fail("Invalid project text escape");
                    if (bytes.compare(i, 6, "\\u0000") == 0)
                        fail("NUL in project text");
                    ++i;
                } else if (bytes[i] == '"') {
                    ended = true;
                    break;
                } else if (static_cast<unsigned char>(bytes[i]) < 0x20)
                    fail("Invalid control character in project text");
            }
            if (!ended)
                fail("Unterminated project text");
            if (!levels.empty() && levels.back().key) {
                const auto key = juce::JSON::fromString(
                    juce::String::fromUTF8(bytes.data() + first, static_cast<int>(i - first + 1)));
                if (!key.isString() || !levels.back().keys.insert(key.toString().toStdString()).second)
                    fail("Duplicate project field");
                levels.back().key = false;
            }
        } else if (c == '{' || c == '[') {
            if (levels.size() == 64)
                fail("Project nesting exceeds limit");
            levels.push_back({c == '{' ? '}' : ']', c == '{', {}});
        } else if (c == '}' || c == ']') {
            if (levels.empty() || levels.back().close != c)
                fail("Invalid project nesting");
            levels.pop_back();
            rootClosed = levels.empty();
        } else if (c == ',' && !levels.empty() && levels.back().close == '}')
            levels.back().key = true;
    }
    if (!levels.empty())
        fail("Unterminated project object");
    juce::var result;
    if (juce::JSON::parse(juce::String::fromUTF8(bytes.data(), static_cast<int>(bytes.size())), result)
            .failed())
        fail("Invalid project JSON");
    return result;
}
} // namespace
void saveProject(const Project &project, const juce::File &file) {
    const auto document = projectDocument(project, file);
    readProjectDocument(document, file, false); // same admission rules on writes, before replacement
    writeDocument(document, file);
}
void saveIncompleteTake(const IncompleteTake &take, const juce::File &file) {
    if (!take.stats.failures || !take.stats.segment || take.events.size() > 200000 || take.in < 0 ||
        take.out <= take.in || take.out > take.original.end())
        fail("Invalid incomplete capture");
    auto root = object();
    put(root, "format", "fader-incomplete-capture");
    put(root, "schemaVersion", 1);
    put(root, "complete", false);
    const auto baseline = projectDocument(take.original, file);
    readProjectDocument(baseline, file, false);
    put(root, "project", baseline);
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
        if (take.original.perClip) {
            put(item, "clipId", juce::String(take.original.clips[event.clip].id));
            put(item, "parameterId", clipParameters[static_cast<unsigned>(event.param)]);
        }
        events.add(item); // ordered array preserves same-frame application order
    }
    put(root, "appliedActions", events);
    writeDocument(root, file);
}
Project loadProject(const juce::File &file) {
    if (!file.existsAsFile() || file.getSize() > 8 * 1024 * 1024)
        fail("Missing or oversized project file");
    const auto root = readManifest(file);
    const auto version = number(required(root, "schemaVersion"));
    if (version == 3)
        return readProjectDocument(root, file);
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
    p.loadedSchemaVersion = static_cast<int>(version);
    p.clips.resize(static_cast<std::size_t>(clips.size()));
    // Validate the manifest before any media decode.
    for (int i = 0; i < clips.size(); ++i) {
        const auto c = clips[i];
        auto &clip = p.clips[static_cast<std::size_t>(i)];
        clip.path = text(required(c, "path")).toStdString();
        clip.hash = text(required(c, "sha256")).toStdString();
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
            clip.name = text(required(c, "name")).toStdString();
            clip.artist = text(required(c, "artist")).toStdString();
            clip.key = text(required(c, "key")).toStdString();
            clip.hasGrid = c.hasProperty("hasGrid") ? boolean(c["hasGrid"]) : false;
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
        const auto interpolation = text(required(l, "interpolation"));
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
        const auto base =
            fileFromPath(std::filesystem::weakly_canonical(filesystemPath(file.getParentDirectory())));
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
    WavExportOptions options;
    options.floatingPoint = floatingPoint;
    options.progress = std::move(progress);
    exportWavFile(project, file, std::move(options));
}
} // namespace fader
