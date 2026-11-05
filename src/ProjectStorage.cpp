// SPDX-License-Identifier: AGPL-3.0-or-later
#include "ProjectStorage.h"
#include <charconv>
#include <cmath>
#include <filesystem>
#include <map>
#include <set>
#include <stdexcept>

namespace fader {
namespace {
[[noreturn]] void bad(const char *message) {
    throw std::runtime_error(message);
}
juce::var object() {
    return new juce::DynamicObject();
}
void put(juce::var &v, const juce::Identifier &key, juce::var value) {
    v.getDynamicObject()->setProperty(key, value);
}
juce::var required(const juce::var &v, const juce::Identifier &key) {
    if (!v.hasProperty(key))
        bad("Missing project field");
    return v[key];
}
void fields(const juce::var &v, std::initializer_list<const char *> names) {
    if (!v.getDynamicObject())
        bad("Project object expected");
    for (const auto &entry : v.getDynamicObject()->getProperties()) {
        bool found = false;
        for (const auto *name : names)
            found |= entry.name == juce::Identifier(name);
        if (!found)
            bad("Unsupported project field");
    }
    for (const auto *name : names)
        required(v, name);
}
std::string text(const juce::var &v, std::size_t size = 4096) {
    if (!v.isString() || v.toString().getNumBytesAsUTF8() > size || v.toString().containsChar(0))
        bad("Invalid project text");
    return v.toString().toStdString();
}
bool boolean(const juce::var &v) {
    if (!v.isBool())
        bad("Project flag must be boolean");
    return bool(v);
}
double number(const juce::var &v) {
    if (!v.isInt() && !v.isInt64() && !v.isDouble())
        bad("Project number expected");
    double value = v;
    if (!std::isfinite(value))
        bad("Non-finite project number");
    return value;
}
Frame frame(const juce::var &v) {
    const auto s = text(v, 24);
    Frame value{};
    auto parsed = std::from_chars(s.data(), s.data() + s.size(), value);
    if (parsed.ec != std::errc{} || parsed.ptr != s.data() + s.size())
        bad("Invalid project integer");
    return value;
}
bool hex(const std::string &s, std::size_t count) {
    return s.size() == count && s.find_first_not_of("0123456789abcdef") == std::string::npos;
}
std::string identity(const juce::var &v) {
    auto s = text(v, 32);
    if (!hex(s, 32))
        bad("Invalid stable identity");
    return s;
}
const std::array<const char *, 3> busParameters{"gainA", "gainB", "crossfader"};
juce::var curve(const Curve &c, const char *parameter) {
    auto out = object();
    put(out, "parameter", parameter);
    put(out, "initial", c.initial);
    put(out, "interpolation", c.linear ? "linear" : "step");
    juce::Array<juce::var> points;
    for (const auto &point : c.points) {
        auto item = object();
        put(item, "frame", juce::String(point.frame));
        put(item, "value", point.value);
        put(item, "hold", point.hold);
        points.add(item);
    }
    put(out, "points", points);
    return out;
}
std::array<Curve, 3> curves(const juce::var &v, bool clip) {
    if (!v.isArray() || v.size() != 3)
        bad("Exactly three parameter curves required");
    std::array<Curve, 3> result;
    std::array<bool, 3> seen{};
    for (const auto &item : *v.getArray()) {
        fields(item, {"parameter", "initial", "interpolation", "points"});
        const auto name = text(item["parameter"], 32);
        unsigned index = 3;
        for (unsigned i = 0; i < 3; ++i)
            if (name == (clip ? clipParameters[i] : busParameters[i]))
                index = i;
        if (index == 3 || seen[index])
            bad("Unknown or duplicate parameter reference");
        seen[index] = true;
        auto &c = result[index];
        c.initial = static_cast<float>(number(item["initial"]));
        const auto mode = text(item["interpolation"], 16);
        if (mode != "linear" && mode != "step")
            bad("Unsupported interpolation");
        c.linear = mode == "linear";
        const auto points = item["points"];
        if (!points.isArray() || points.size() > 200000)
            bad("Invalid automation count");
        for (const auto &p : *points.getArray()) {
            fields(p, {"frame", "value", "hold"});
            c.points.push_back(
                {frame(p["frame"]), static_cast<float>(number(p["value"])), boolean(p["hold"])});
        }
        const auto valid = [=](float x) {
            return std::isfinite(x) && x >= (clip || index != 2 ? 0 : -1) && x <= 1;
        };
        if (!valid(c.initial))
            bad("Invalid initial parameter state");
        Frame previous = -1;
        for (const auto &p : c.points) {
            if (p.frame <= previous || p.frame > maxProjectFrames || !valid(p.value))
                bad("Invalid ordered automation");
            previous = p.frame;
        }
    }
    return result;
}
juce::var render(bool clips) {
    auto out = object();
    put(out, "engineVersion", clips ? 2 : 1);
    put(out, "sampleRate", projectRate);
    put(out, "smoothingFrames", rampFrames);
    put(out, "sourceInterpolation", "linear-v1");
    put(out, "mixLaw", clips ? "sum-v2" : "equal-power-v1");
    put(out, "filter", "one-pole-hp-lp-v2");
    put(out, "cutoffMap", "log-20-20000-v2");
    put(out, "timeBasis", "project-frame");
    put(out, "seekPrerollFrames", juce::String(seekPrerollFrames));
    return out;
}
void checkRender(const juce::var &v, bool clips) {
    const auto expected = render(clips);
    fields(v, {"engineVersion", "sampleRate", "smoothingFrames", "sourceInterpolation", "mixLaw", "filter",
               "cutoffMap", "timeBasis", "seekPrerollFrames"});
    for (const auto &entry : expected.getDynamicObject()->getProperties()) {
        const auto value = v[entry.name];
        if (entry.value.isString()) {
            if (text(value) != entry.value.toString().toStdString())
                bad("Unsupported render policy");
        } else if (number(value) != double(entry.value))
            bad("Unsupported render policy");
    }
}
struct Asset {
    std::string id, path, hash, pcmHash, decoder;
    double sampleRate{};
    Frame frames{};
    unsigned channels{};
};
std::string assetIdentity(const Asset &a) {
    const auto domain = juce::String(a.hash) + "\n" + a.pcmHash + "\n" + juce::String(a.sampleRate, 17) +
                        "\n" + juce::String(a.frames) + "\n" + juce::String(a.channels);
    return "asset-" + juce::SHA256(domain.toUTF8()).toHexString().toStdString();
}
Asset asset(const Clip &clip) {
    if (!clip.audio || !hex(clip.hash, 64) || !hex(clip.audio->pcmHash, 64) || clip.audio->decoder.empty())
        bad("Source identity unavailable; reimport the source before saving");
    Asset result{"",
                 clip.path,
                 clip.hash,
                 clip.audio->pcmHash,
                 clip.audio->decoder,
                 clip.audio->sampleRate,
                 static_cast<Frame>(clip.audio->frames()),
                 clip.audio->sourceChannels};
    result.id = assetIdentity(result);
    return result;
}
void checkAsset(const Asset &a) {
    if (a.path.empty() || !hex(a.hash, 64) || !hex(a.pcmHash, 64) || a.decoder.empty() ||
        !std::isfinite(a.sampleRate) || a.sampleRate < 8000 || a.sampleRate > 384000 || a.frames <= 0 ||
        a.frames > maxSourceFrames || a.channels < 1 || a.channels > 2 || a.id != assetIdentity(a))
        bad("Invalid asset reference or sample domain");
}
juce::File baseDirectory(const juce::File &file) {
    return juce::File(
        std::filesystem::weakly_canonical(file.getParentDirectory().getFullPathName().toStdString())
            .string());
}
void checkText(const std::string &s, std::size_t size = 4096) {
    if (s.size() > size || !juce::CharPointer_UTF8::isValidString(s.data(), static_cast<int>(s.size())))
        bad("Project text exceeds limit");
    text(juce::String::fromUTF8(s.data(), static_cast<int>(s.size())), size);
    if (s.find('\0') != std::string::npos)
        bad("NUL in project text");
}
} // namespace
juce::var projectDocument(const Project &project, const juce::File &file) {
    if (project.storageVersion != 3)
        bad("Unsupported project write version");
    project.validate();
    // Every encoded point occupies more than 32 bytes, even before delimiters.
    // Reject impossible-sized manifests before constructing millions of JSON objects.
    std::size_t points = 0;
    for (const auto &curve : project.curves)
        points += curve.points.size();
    for (const auto &clip : project.clips)
        for (const auto &curve : clip.automation)
            points += curve.points.size();
    if (points > static_cast<std::size_t>(maxManifestBytes / 32))
        bad("Project exceeds the 8 MiB manifest limit");
    if (!hex(project.id, 32))
        bad("Invalid project identity");
    auto root = object();
    put(root, "schemaVersion", 3);
    put(root, "projectId", juce::String(project.id));
    put(root, "render", render(project.perClip));
    juce::Array<juce::var> assets, clips, automation;
    std::set<std::string> ids, assetIds;
    for (const auto &clip : project.clips) {
        if (!hex(clip.id, 32))
            bad("Invalid clip identity");
        if (!ids.insert(clip.id).second)
            bad("Duplicate clip identity");
        for (const auto *s : {&clip.name, &clip.artist, &clip.key})
            checkText(*s);
        std::string ref;
        if (!clip.path.empty()) {
            checkText(clip.path, 8192);
            if (clip.audio)
                checkText(clip.audio->decoder, 128);
            if (juce::File(clip.path) == file)
                bad("A set cannot replace source audio");
            const auto a = asset(clip);
            checkAsset(a);
            ref = a.id;
            if (assetIds.insert(ref).second) {
                auto item = object();
                put(item, "id", juce::String(ref));
                put(item, "path",
                    juce::File(std::filesystem::weakly_canonical(a.path).string())
                        .getRelativePathFrom(baseDirectory(file)));
                put(item, "sha256", juce::String(a.hash));
                put(item, "pcmHash", juce::String(a.pcmHash));
                put(item, "decoder", juce::String(a.decoder));
                put(item, "sampleRate", a.sampleRate);
                put(item, "frames", juce::String(a.frames));
                put(item, "channels", int(a.channels));
                assets.add(item);
            }
        } else if (clip.length > 0)
            bad("Clip has no asset");
        auto item = object();
        put(item, "id", juce::String(clip.id));
        put(item, "assetId", juce::String(ref));
        put(item, "start", juce::String(clip.start));
        put(item, "sourceOffset", juce::String(clip.sourceOffset));
        put(item, "length", juce::String(clip.length));
        put(item, "rate", clip.rate);
        put(item, "row", clip.row);
        put(item, "name", juce::String(clip.name));
        put(item, "artist", juce::String(clip.artist));
        put(item, "key", juce::String(clip.key));
        put(item, "bpm", clip.bpm);
        put(item, "hasGrid", clip.hasGrid);
        put(item, "firstBeatSeconds", clip.firstBeatSeconds);
        juce::Array<juce::var> lanes;
        for (unsigned i = 0; i < 3; ++i)
            lanes.add(curve(clip.automation[i], clipParameters[i]));
        put(item, "automation", lanes);
        clips.add(item);
    }
    for (unsigned i = 0; i < 3; ++i)
        automation.add(curve(project.curves[i], busParameters[i]));
    put(root, "assets", assets);
    put(root, "clips", clips);
    put(root, "automation", automation);
    return root;
}
Project readProjectDocument(const juce::var &root, const juce::File &file, bool resolveMedia) {
    fields(root, {"schemaVersion", "projectId", "render", "assets", "clips", "automation"});
    if (number(root["schemaVersion"]) != 3)
        bad("Unsupported schema");
    Project p;
    p.id = identity(root["projectId"]);
    p.loadedSchemaVersion = 3;
    const auto version = number(required(root["render"], "engineVersion"));
    if (version != 1 && version != 2)
        bad("Unsupported renderer");
    p.perClip = version == 2;
    checkRender(root["render"], p.perClip);
    const auto sources = root["assets"], instances = root["clips"];
    if (!sources.isArray() || sources.size() > 64 || !instances.isArray() || instances.size() > 64)
        bad("Invalid asset/clip count");
    std::map<std::string, Asset> assets;
    for (const auto &item : *sources.getArray()) {
        fields(item, {"id", "path", "sha256", "pcmHash", "decoder", "sampleRate", "frames", "channels"});
        const auto channels = number(item["channels"]);
        if (channels != 1 && channels != 2)
            bad("Invalid source channels");
        Asset a{text(item["id"], 80),      text(item["path"], 8192),   text(item["sha256"], 64),
                text(item["pcmHash"], 64), text(item["decoder"], 128), number(item["sampleRate"]),
                frame(item["frames"]),     unsigned(channels)};
        checkAsset(a);
        if (!assets.emplace(a.id, a).second)
            bad("Duplicate asset identity");
    }
    p.clips.clear();
    std::set<std::string> ids, used;
    std::vector<std::string> references;
    for (const auto &item : *instances.getArray()) {
        fields(item, {"id", "assetId", "start", "sourceOffset", "length", "rate", "row", "name", "artist",
                      "key", "bpm", "hasGrid", "firstBeatSeconds", "automation"});
        Clip c;
        c.id = identity(item["id"]);
        if (!ids.insert(c.id).second)
            bad("Duplicate clip identity");
        const auto ref = text(item["assetId"], 80);
        c.start = frame(item["start"]);
        c.sourceOffset = frame(item["sourceOffset"]);
        c.length = frame(item["length"]);
        c.rate = number(item["rate"]);
        const auto row = number(item["row"]);
        if (row < 0 || row > 4095 || std::floor(row) != row)
            bad("Invalid row");
        c.row = int(row);
        c.name = text(item["name"]);
        c.artist = text(item["artist"]);
        c.key = text(item["key"]);
        c.bpm = number(item["bpm"]);
        c.hasGrid = boolean(item["hasGrid"]);
        c.firstBeatSeconds = number(item["firstBeatSeconds"]);
        c.automation = curves(item["automation"], true);
        if (ref.empty()) {
            if (c.length > 0)
                bad("Missing asset reference");
        } else {
            auto found = assets.find(ref);
            if (found == assets.end())
                bad("Dangling asset reference");
            c.path = found->second.path;
            c.hash = found->second.hash;
            used.insert(ref);
        }
        references.push_back(ref);
        p.clips.push_back(std::move(c));
    }
    if (used.size() != assets.size())
        bad("Unreferenced asset");
    p.curves = curves(root["automation"], false);
    p.validate();
    if (!resolveMedia)
        return p;
    // All types, IDs, policies and references have passed before any file read.
    std::map<std::string, Clip> decoded;
    std::size_t peaks = 0;
    for (std::size_t i = 0; i < p.clips.size(); ++i) {
        const auto &ref = references[i];
        if (ref.empty())
            continue;
        const auto &expected = assets.at(ref);
        if (!decoded.contains(ref)) {
            auto loaded = loadClip(baseDirectory(file).getChildFile(expected.path));
            const auto actual = asset(loaded);
            if (actual.hash != expected.hash || actual.pcmHash != expected.pcmHash ||
                actual.frames != expected.frames || actual.sampleRate != expected.sampleRate ||
                actual.channels != expected.channels)
                bad("Source decoder sample domain changed; restore the original decoder/source or explicitly "
                    "migrate this set");
            peaks += loaded.audio->peaks.size() * sizeof(float);
            if (peaks > 32ULL * 1024 * 1024)
                bad("Set waveform budget exceeded");
            decoded.emplace(ref, std::move(loaded));
        }
        auto &c = p.clips[i];
        const auto &loaded = decoded.at(ref);
        c.audio = loaded.audio;
        c.path = loaded.path;
    }
    p.validate();
    return p;
}
} // namespace fader
