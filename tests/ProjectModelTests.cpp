// SPDX-License-Identifier: AGPL-3.0-or-later
#include "ProjectStorage.h"
#include "ReadFixture.h"
#include <bit>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
#include <stdexcept>

namespace {
using namespace fader;
void check(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
template <class F> std::string rejects(F action) {
    try {
        action();
    } catch (const std::exception &e) {
        return e.what();
    }
    throw std::runtime_error("Invalid project was accepted");
}
juce::var clone(const juce::var &v) {
    return juce::JSON::parse(juce::JSON::toString(v, true));
}
void write(const juce::File &file, const juce::var &v) {
    check(file.replaceWithText(juce::JSON::toString(v, true)), "Cannot write fixture");
}
void numericRoundTrip(Project project, const juce::File &file) {
    project.clips.resize(1);
    auto &clip = project.clips[0];
    clip.rate = double(std::nextafter(.75f, 1.0f));
    clip.firstBeatSeconds = double(std::nextafter(.1723f, 1.0f));
    clip.bpm = 123.456789012345;
    clip.name = "Escapes: \" \\ \n \t café α 音楽";
    const std::vector<float> values{-0.0f,
                                    std::numeric_limits<float>::denorm_min(),
                                    std::numeric_limits<float>::min(),
                                    .2f,
                                    std::nextafter(.2f, 1.0f),
                                    .7f,
                                    std::nextafter(1.0f, 0.0f),
                                    1.0f};
    clip.automation[0].initial = -0.0f;
    clip.automation[0].points.clear();
    for (std::size_t i = 0; i < values.size(); ++i)
        clip.automation[0].points.push_back({Frame(i), values[i], true});
    saveProject(project, file);
    const auto opened = loadProject(file);
    const auto &actual = opened.clips[0];
    check(actual.rate == clip.rate && actual.firstBeatSeconds == clip.firstBeatSeconds &&
              actual.bpm == clip.bpm && actual.name == clip.name,
          "Compact manifest lost binary64 metadata or string escapes");
    check(std::bit_cast<std::uint32_t>(actual.automation[0].initial) ==
              std::bit_cast<std::uint32_t>(clip.automation[0].initial),
          "Initial signed zero changed");
    for (std::size_t i = 0; i < values.size(); ++i)
        check(std::bit_cast<std::uint32_t>(actual.automation[0].points[i].value) ==
                  std::bit_cast<std::uint32_t>(values[i]),
              "Compact manifest did not preserve the exact binary32 target");
}
void denseRoundTrip(Project project, const juce::File &file) {
    // Same declared point density and control count as the hour workload, with
    // short media so this storage regression does not pretend to be a soak.
    constexpr Frame end = Frame{3600} * projectRate;
    for (std::size_t index = 0; index < project.clips.size(); ++index) {
        auto &clip = project.clips[index];
        clip.start = 0;
        clip.length = end;
        clip.row = int(index);
        for (unsigned p = 0; p < 3; ++p) {
            auto &curve = clip.automation[p];
            curve.points.clear();
            for (Frame frame = 0; frame <= end; frame += projectRate / 4) {
                const auto step = frame / (projectRate / 4);
                const auto value = p == 0   ? .3f + .1f * float(step % 3)
                                   : p == 1 ? .05f + .03f * float(step % 4)
                                            : .7f + .05f * float(step % 5);
                curve.points.push_back({frame, value, step % 7 == 0});
            }
            curve.initial = curve.points.front().value;
            curve.linear = true;
        }
    }
    std::vector<Event> actions;
    const auto block = [](Frame frame) { return (frame + 127) / 128 * 128; };
    for (int i = 0; i < 35999; ++i)
        actions.push_back({block(Frame(i) * (projectRate / 10)), static_cast<Param>(i % 3),
                           .2f + float(i * 2) * .000001f, 0});
    int controllerIndex = 0;
    for (int burst = 0; burst < 120; ++burst)
        for (int tick = 0; tick < 600; ++tick, ++controllerIndex)
            actions.push_back({block(Frame(burst * 30) * projectRate + tick * (projectRate / 120)),
                               static_cast<Param>(controllerIndex % 3),
                               .2f + float(controllerIndex * 2 + 1) * .000001f, 0});
    std::stable_sort(actions.begin(), actions.end(),
                     [](const Event &a, const Event &b) { return a.frame < b.frame; });
    check(actions.size() == 107999, "Dense storage workload lost original controls");
    commitTake(project, actions, 0, end);
    saveProject(project, file);
    check(file.getSize() <= maxManifestBytes, "Dense capture cannot fit the unchanged manifest bound");
    const auto opened = loadProject(file);
    check(opened.id == project.id && opened.clips.size() == project.clips.size(),
          "Dense capture changed project identity or clip count");
    std::size_t points = 0;
    for (std::size_t c = 0; c < project.clips.size(); ++c) {
        check(opened.clips[c].id == project.clips[c].id, "Dense capture changed clip identity");
        for (unsigned p = 0; p < 3; ++p) {
            points += project.clips[c].automation[p].points.size();
            check(opened.clips[c].automation[p].points == project.clips[c].automation[p].points,
                  "Compact save removed or changed dense automation targets");
        }
    }
    std::cout << "PASS: dense manifest preserves 107999 original-control compilation, " << points
              << " points in " << file.getSize() << " bytes; float target bits and double metadata\n";
}
void source(const juce::File &file) {
    std::unique_ptr<juce::OutputStream> stream = file.createOutputStream();
    auto writer = juce::WavAudioFormat{}.createWriterFor(
        stream, juce::AudioFormatWriterOptions{}
                    .withSampleRate(44100)
                    .withNumChannels(1)
                    .withBitsPerSample(32)
                    .withSampleFormat(juce::AudioFormatWriterOptions::SampleFormat::floatingPoint));
    check(writer != nullptr, "Cannot create native-rate fixture");
    juce::AudioBuffer<float> samples(1, 88200);
    for (int i = 0; i < 88200; ++i)
        samples.setSample(0, i, 0.1f * std::sin(float(i) * 0.071f));
    check(writer->writeFromAudioSampleBuffer(samples, 0, 88200) && writer->flush(),
          "Cannot write source fixture");
}
void parity(const Project &a, const Project &b) {
    const auto frames = static_cast<int>(a.end());
    check(a.end() == b.end(), "Stored duration changed");
    auto first = std::make_unique<Engine>(a), second = std::make_unique<Engine>(b);
    juce::AudioBuffer<float> x(2, frames), y(2, frames);
    renderPrepared(*first, x);
    renderPrepared(*second, y);
    double error = 0;
    for (int c = 0; c < 2; ++c)
        for (int i = 0; i < frames; ++i)
            error = std::max(error, std::abs(double(x.getSample(c, i)) - y.getSample(c, i)));
    check(error <= 1e-6, "Persistence changed rendered master PCM");
}
juce::var legacy(const Project &p, int version) {
    auto root = juce::var(new juce::DynamicObject());
    auto set = [](juce::var &v, const char *key, juce::var value) {
        v.getDynamicObject()->setProperty(key, value);
    };
    set(root, "schemaVersion", version);
    set(root, "engineVersion", version);
    set(root, "sampleRate", 48000);
    juce::Array<juce::var> clips, lanes;
    auto oldCurve = [&](const Curve &curve) {
        auto out = juce::var(new juce::DynamicObject());
        set(out, "initial", curve.initial);
        set(out, "interpolation", curve.linear ? "linear" : "step");
        juce::Array<juce::var> points;
        for (const auto &p : curve.points) {
            auto item = juce::var(new juce::DynamicObject());
            set(item, "frame", juce::String(p.frame));
            set(item, "value", p.value);
            set(item, "hold", p.hold);
            points.add(item);
        }
        set(out, "points", points);
        return out;
    };
    for (const auto &c : p.clips) {
        auto out = juce::var(new juce::DynamicObject());
        set(out, "path", juce::String(c.path));
        set(out, "sha256", juce::String(c.hash));
        set(out, "start", juce::String(c.start));
        set(out, "sourceOffset", juce::String(c.sourceOffset));
        set(out, "length", juce::String(c.length));
        set(out, "rate", c.rate);
        set(out, "bpm", c.bpm);
        set(out, "firstBeatSeconds", c.firstBeatSeconds);
        if (version == 2) {
            set(out, "row", c.row);
            set(out, "name", juce::String(c.name));
            set(out, "artist", juce::String(c.artist));
            set(out, "key", juce::String(c.key));
            set(out, "hasGrid", c.hasGrid);
            juce::Array<juce::var> automation;
            for (const auto &curve : c.automation)
                automation.add(oldCurve(curve));
            set(out, "automation", automation);
        }
        clips.add(out);
    }
    for (const auto &curve : p.curves)
        lanes.add(oldCurve(curve));
    set(root, "clips", clips);
    set(root, "automation", lanes);
    return root;
}
} // namespace
void testProjectModel() {
    using namespace fader;
    const auto dir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                         .getNonexistentChildFile("fader-model", {}, false);
    dir.createDirectory();
    try {
        const auto audio = dir.getChildFile("source.audio");
        source(audio);
        const auto originalHash = juce::SHA256(audio).toHexString();
        Project p;
        p.perClip = true;
        p.clips.clear();
        auto a = loadClip(audio);
        a.start = 123;
        a.row = 5;
        a.sourceOffset = 4411;
        a.length = 48000;
        a.rate = 0.75;
        a.name = "Synthetic α";
        a.artist = "Test artist";
        a.key = "8m";
        a.bpm = 123.45678;
        a.hasGrid = true;
        a.firstBeatSeconds = 0.1723;
        a.cue = true;
        a.automation[0] = {0.3f, true, {{0, 0.7f}, {1234, 0.1f, true}, {5678, 0.9f}}};
        a.automation[1] = {0.2f, true, {{0, 0.1f}, {2222, 0.6f, true}}};
        a.automation[2] = {0.9f, false, {{0, 1}, {3000, 0.4f, true}}};
        auto b = a;
        b.id = newIdentity();
        b.row = 9;
        b.start = 7000;
        b.automation[0].initial = 0.8f;
        p.clips = {a, b};
        const auto file = dir.getChildFile("set.faderset");
        saveProject(p, file);
        const auto doc = juce::JSON::parse(file);
        check(int(doc["schemaVersion"]) == 3 && doc["assets"].size() == 1,
              "Assets were not deduplicated by identity");
        auto opened = loadProject(file);
        check(opened.id == p.id && opened.clips[0].id == a.id && opened.clips[1].id == b.id,
              "Stable IDs did not round-trip");
        check(opened.clips[0].audio == opened.clips[1].audio && opened.clips[0].audio->sourceChannels == 1 &&
                  opened.clips[0].audio->sampleRate == 44100,
              "Shared source domain was lost");
        check(opened.clips[0].sourceOffset == 4411 && opened.clips[0].rate == 0.75 &&
                  opened.clips[0].name == a.name && opened.clips[0].artist == a.artist &&
                  opened.clips[0].key == a.key && opened.clips[0].bpm == a.bpm && opened.clips[0].hasGrid &&
                  opened.clips[0].firstBeatSeconds == a.firstBeatSeconds,
              "Placement/metadata/grid snapshot did not round-trip");
        for (unsigned i = 0; i < 3; ++i)
            check(opened.clips[0].automation[i].initial == a.automation[i].initial &&
                      opened.clips[0].automation[i].linear == a.automation[i].linear &&
                      opened.clips[0].automation[i].points == a.automation[i].points,
                  "Initial/curve/hold state changed");
        check(!opened.clips[0].cue && !doc["clips"][0].hasProperty("cue") && !doc.hasProperty("device"),
              "Monitoring/device state entered the saved sound model");
        parity(p, opened);
        numericRoundTrip(p, dir.getChildFile("numeric.faderset"));
        denseRoundTrip(p, dir.getChildFile("dense.faderset"));
        const auto before = opened.clips[0].automation;
        opened.clips[0].start += 1234;
        opened.clips[0].row = 40;
        std::swap(opened.clips[0], opened.clips[1]);
        saveProject(opened, file);
        auto moved = loadProject(file);
        check(moved.clips[1].id == a.id && moved.clips[1].start == 1357 && moved.clips[1].row == 40 &&
                  moved.clips[1].automation[0].points == before[0].points,
              "Move/reorder changed clip identity or its curves");
        // Reorder parameter entries on disk: semantic names, not array positions, route them.
        auto shuffled = clone(doc);
        auto *curves = shuffled["clips"][0]["automation"].getArray();
        curves->swap(0, 2);
        write(file, shuffled);
        check(loadProject(file).clips[0].automation[0].points == a.automation[0].points,
              "Parameter names do not resolve independently of array order");
        // Collect and relocate the saved set and media without rewriting its identities.
        const auto collected = dir.getChildFile("collected");
        collected.createDirectory();
        audio.copyFileTo(collected.getChildFile("source.audio"));
        write(collected.getChildFile("set.faderset"), doc);
        auto relocated = loadProject(collected.getChildFile("set.faderset"));
        check(relocated.id == p.id && relocated.clips[0].id == a.id, "Relocation changed identity");
        parity(p, relocated);
        const auto rejectDocument = [&](auto change) {
            auto bad = clone(doc);
            bad["assets"][0].getDynamicObject()->setProperty("path", "missing.audio");
            change(bad);
            write(file, bad);
            const auto error = rejects([&] { loadProject(file); });
            check(error != "Audio file is missing", "Invalid manifest reached media I/O");
        };
        rejectDocument([](auto &d) { d["clips"][0].getDynamicObject()->setProperty("name", 123); });
        rejectDocument([](auto &d) {
            d["clips"][0].getDynamicObject()->setProperty("artist", new juce::DynamicObject());
        });
        rejectDocument([](auto &d) { d["clips"][0].getDynamicObject()->setProperty("hasGrid", "false"); });
        rejectDocument([](auto &d) {
            d["clips"][0]["automation"][0]["points"][0].getDynamicObject()->setProperty("hold", "false");
        });
        rejectDocument([](auto &d) { d["clips"][0].getDynamicObject()->setProperty("sourceOffset", 123); });
        rejectDocument(
            [](auto &d) { d["clips"][0].getDynamicObject()->setProperty("start", "9223372036854775808"); });
        rejectDocument(
            [](auto &d) { d["clips"][0].getDynamicObject()->setProperty("assetId", "asset-dangling"); });
        rejectDocument(
            [](auto &d) { d["clips"][1].getDynamicObject()->setProperty("id", d["clips"][0]["id"]); });
        rejectDocument([](auto &d) { d["assets"].getArray()->add(clone(d["assets"][0])); });
        rejectDocument([](auto &d) {
            d["clips"][0]["automation"][0].getDynamicObject()->setProperty("parameter", "unknown-fx");
        });
        rejectDocument([](auto &d) {
            d["clips"][0]["automation"][1].getDynamicObject()->setProperty("parameter", "volume");
        });
        rejectDocument([](auto &d) { d["render"].getDynamicObject()->setProperty("smoothingFrames", 241); });
        rejectDocument([](auto &d) { d["render"].getDynamicObject()->setProperty("engineVersion", 99); });
        rejectDocument([](auto &d) { d.getDynamicObject()->setProperty("deviceHandle", 123); });
        rejectDocument([](auto &d) {
            d["clips"][0]["automation"][0]["points"][1].getDynamicObject()->setProperty("frame", "0");
        });
        const auto goodBytes = juce::JSON::toString(doc, true);
        file.replaceWithText(goodBytes + " trailing");
        rejects([&] { loadProject(file); });
        file.replaceWithText(goodBytes.replaceFirstOccurrenceOf("{", "{\"schemaVersion\":3,"));
        rejects([&] { loadProject(file); });
        file.replaceWithText(goodBytes.replaceFirstOccurrenceOf("{", "{\"\\u0073chemaVersion\":3,"));
        rejects([&] { loadProject(file); });
        file.replaceWithText(juce::String::repeatedString("[", 65) + "0" +
                             juce::String::repeatedString("]", 65));
        rejects([&] { loadProject(file); });
        file.replaceWithText(juce::String::repeatedString(" ", static_cast<int>(maxManifestBytes + 1)));
        rejects([&] { loadProject(file); });
        // Writer rejection preserves an existing destination.
        file.replaceWithText(goodBytes);
        const auto keep = file.loadFileAsString();
        auto invalid = p;
        invalid.storageVersion = 99;
        rejects([&] { saveProject(invalid, file); });
        check(file.loadFileAsString() == keep, "Unsupported write replaced destination");
        invalid = p;
        invalid.clips[0].rate = std::numeric_limits<double>::infinity();
        rejects([&] { saveProject(invalid, file); });
        invalid = p;
        invalid.clips[0].id = invalid.clips[1].id;
        rejects([&] { saveProject(invalid, file); });
        invalid = p;
        for (auto &c : invalid.clips)
            for (auto &curve : c.automation) {
                curve.points.resize(50000);
                for (std::size_t i = 0; i < curve.points.size(); ++i)
                    curve.points[i] = {Frame(i), 0.5f};
            }
        rejects([&] { saveProject(invalid, file); });
        check(file.loadFileAsString() == keep, "Oversized write replaced destination");
        // A different decoder domain with the same encoded file must not silently reopen.
        auto domain = p;
        auto altered = std::make_shared<AudioData>(*domain.clips[0].audio);
        altered->pcmHash = std::string(64, '0');
        domain.clips[0].audio = altered;
        domain.clips.resize(1);
        saveProject(domain, file);
        check(rejects([&] { loadProject(file); }).find("sample domain changed") != std::string::npos,
              "Changed decoded sample domain was silently accepted");
        for (int version : {1, 2}) {
            auto baseline = p;
            baseline.perClip = version == 2;
            write(file, legacy(baseline, version));
            auto old = loadProject(file);
            check(old.loadedSchemaVersion == version && old.perClip == baseline.perClip,
                  "Legacy renderer selection changed");
            parity(baseline, old);
            saveProject(old, file);
            auto upgraded = loadProject(file);
            check(upgraded.perClip == old.perClip && upgraded.id == old.id &&
                      upgraded.clips[0].id == old.clips[0].id,
                  "Legacy migration lost stable identity");
            parity(old, upgraded);
        }
        Project compressed;
        compressed.perClip = true;
        compressed.clips = {loadClip(juce::File(FADER_TEST_VBR_PATH))};
        compressed.clips[0].sourceOffset = 123;
        compressed.clips[0].length = 8192;
        saveProject(compressed, file);
        const auto mp3 = loadProject(file);
        check(mp3.clips[0].audio->pcmHash == compressed.clips[0].audio->pcmHash &&
                  mp3.clips[0].id == compressed.clips[0].id,
              "MP3 decoded identity did not round-trip");
        parity(compressed, mp3);
        check(juce::SHA256(audio).toHexString() == originalHash, "Project operations modified source audio");
        dir.deleteRecursively();
    } catch (...) {
        dir.deleteRecursively();
        throw;
    }
}
