// SPDX-License-Identifier: AGPL-3.0-or-later
#include "Library.h"
#include "Media.h"
#include "ReadFixture.h"
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
using namespace fader;
void testArrangement();
void testAudioSetup();
void testAudioPlayback();
void testSourceRates();
void testStreamMedia();
void testDecodedCache();
void testProjectModel();
void testExportFailures();
void testPortability();
void check(bool pass, const char *message) {
    if (!pass)
        throw std::runtime_error(message);
}
int main() {
    juce::ScopedJuceInitialiser_GUI juce;
    testArrangement();
    testAudioSetup();
    testAudioPlayback();
    const auto dir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                         .getNonexistentChildFile("fader-test", {}, false);
    dir.createDirectory();
    try {
        testPortability();
        testSourceRates();
        testStreamMedia();
        testDecodedCache();
        testProjectModel();
        testExportFailures();
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
        const auto directoryAlias = dir.getChildFile("directory-alias");
        std::filesystem::create_directory_symlink(dir.getFullPathName().toStdString(),
                                                  directoryAlias.getFullPathName().toStdString());
        saveProject(p, directoryAlias.getChildFile("saved-via-alias.faderset"));
        const auto aliasReopened = loadProject(dir.getChildFile("saved-via-alias.faderset"));
        check(aliasReopened.clips[0].hash == clip.hash,
              "native chooser directory alias broke saved media references");
        const auto output = dir.getChildFile("output.wav");
        exportWav(reopened, output, true);
        auto rendered = loadClip(output);
        const auto renderedSamples = readImportedSamples(rendered);
        auto eOwned = std::make_unique<Engine>(p);
        auto &e = *eOwned;
        juce::AudioBuffer<float> expected(2, 48000);
        renderPrepared(e, expected);
        for (int i = 0; i < 48000; ++i)
            check(std::abs(expected.getSample(0, i) - renderedSamples[0][static_cast<std::size_t>(i)]) <
                      1e-6f,
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
        auto modern = p;
        modern.perClip = true;
        modern.clips[0].row = 7;
        modern.clips[0].artist = "Test artist";
        modern.clips[0].key = "1m";
        modern.clips[0].hasGrid = true;
        modern.clips[0].automation[0].points = {{0, 0.5f}, {12000, 0.2f, true}, {18000, 0.8f}};
        modern.clips[0].automation[1].points = {{0, 0}, {16000, 0.4f}};
        const auto modernFile = dir.getChildFile("modern.faderset");
        saveProject(modern, modernFile);
        const auto modernReopened = loadProject(modernFile);
        check(modernReopened.perClip && modernReopened.clips[0].row == 7 &&
                  modernReopened.clips[0].automation[0].points == modern.clips[0].automation[0].points &&
                  modernReopened.clips[0].artist == "Test artist" && modernReopened.clips[0].hasGrid,
              "clip project did not round-trip");
        const auto originalBytes = modernFile.loadFileAsString();
        IncompleteTake incomplete{
            modern,
            {{128, Param::volume, 0.2f, 0}, {128, Param::volume, 0.3f, 0}, {256, Param::highPass, 0.4f, 0}},
            0,
            48000,
            PlaybackStats{.segment = 7,
                          .inputDrops = 1,
                          .engineFrame = 512,
                          .projectFrame = 512,
                          .failures = inputOverflow}};
        const auto archiveFile = dir.getChildFile("interrupted.fadercapture");
        saveIncompleteTake(incomplete, archiveFile);
        const auto archive = juce::JSON::parse(archiveFile);
        const auto actions = archive["appliedActions"];
        check(archive["format"].toString() == "fader-incomplete-capture" && !bool(archive["complete"]) &&
                  actions.size() == 3 && actions[0]["frame"].toString() == "128" &&
                  double(actions[1]["value"]) > double(actions[0]["value"]) &&
                  archive["project"]["assets"][0]["sha256"].toString() == juce::String(clip.hash),
              "incomplete capture lost ordered actions or original source identity");
        const auto baseline = dir.getChildFile("archive-baseline.faderset");
        baseline.replaceWithText(juce::JSON::toString(archive["project"]));
        check(loadProject(baseline).clips[0].automation[0].points == modern.clips[0].automation[0].points &&
                  modernFile.loadFileAsString() == originalBytes && archiveFile.getSize() < 20000,
              "capture archive changed baseline automation or embedded media");
        bool rejectedArchive = false;
        try {
            loadProject(archiveFile);
        } catch (...) {
            rejectedArchive = true;
        }
        check(rejectedArchive, "incomplete archive opened as a complete set");
        const auto archiveBytes = archiveFile.loadFileAsString();
        incomplete.events.back().frame = 127; // invalid ordering must not replace good archive
        bool rejectedCapture = false;
        try {
            saveIncompleteTake(incomplete, archiveFile);
        } catch (...) {
            rejectedCapture = true;
        }
        check(rejectedCapture && archiveFile.loadFileAsString() == archiveBytes,
              "invalid capture replaced saved actions");
        const auto modernWave = dir.getChildFile("modern.wav");
        exportWav(modernReopened, modernWave, true);
        auto modernAudio = loadClip(modernWave);
        const auto modernSamples = readImportedSamples(modernAudio);
        auto modernEngineOwned = std::make_unique<Engine>(modern);
        auto &modernEngine = *modernEngineOwned;
        renderPrepared(modernEngine, expected);
        for (int i = 0; i < 48000; ++i)
            check(std::abs(expected.getSample(0, i) - modernSamples[0][static_cast<std::size_t>(i)]) < 1e-6f,
                  "clip/filter export differs from live core");
        juce::XmlElement nml("NML");
        nml.setAttribute("VERSION", 20);
        auto *collection = nml.createNewChildElement("COLLECTION");
        auto *entry = collection->createNewChildElement("ENTRY");
        entry->setAttribute("TITLE", "Synthetic song");
        entry->setAttribute("ARTIST", "Test artist");
        auto *location = entry->createNewChildElement("LOCATION");
        location->setAttribute("DIR", dir.getFullPathName().replace("/", "/:") + "/:");
        location->setAttribute("FILE", "modern.wav");
        location->setAttribute("VOLUME", "Macintosh HD");
        entry->createNewChildElement("TEMPO")->setAttribute("BPM", 123.45);
        entry->createNewChildElement("MUSICAL_KEY")->setAttribute("VALUE", 21);
        entry->createNewChildElement("INFO")->setAttribute("PLAYTIME_FLOAT", 1.0);
        auto *cue = entry->createNewChildElement("CUE_V2");
        cue->setAttribute("TYPE", 4);
        cue->setAttribute("START", 250.0);
        const auto collectionFile = dir.getChildFile("collection.nml");
        collectionFile.replaceWithText(nml.toString());
        const auto collectionHash = juce::SHA256(collectionFile).toHexString();
        auto imported = readTraktor(collectionFile);
        check(imported.size() == 1 && imported.begin()->second.file == modernWave &&
                  imported.begin()->second.key == "1m" && imported.begin()->second.bpm == 123.45 &&
                  imported.begin()->second.firstBeat == 0.25,
              "Traktor path/key/BPM/grid import failed");
        check(traktorKey(0) == "1d" && traktorKey(12) == "10m" && traktorKey(23) == "3m" &&
                  traktorKey(24).isEmpty(),
              "Traktor musical key mapping incorrect");
        Preferences prefs;
        prefs.musicFolder = dir;
        prefs.collection = collectionFile;
        const auto prefsFile = dir.getChildFile("preferences.json");
        prefs.save(prefsFile);
        const auto readPrefs = Preferences::load(prefsFile);
        check(readPrefs.musicFolder == dir && readPrefs.collection == collectionFile &&
                  readPrefs.importTraktor,
              "preferences did not persist");
        const auto library = scanLibrary(readPrefs);
        check(library.matched == 1 && juce::SHA256(collectionFile).toHexString() == collectionHash,
              "library import altered collection or missed exact path");
        const auto alias = dir.getChildFile("same-audio-alias.wav");
        std::filesystem::create_hard_link(modernWave.getFullPathName().toStdString(),
                                          alias.getFullPathName().toStdString());
        const auto aliases = scanLibrary(readPrefs);
        check(aliases.matched == 2, "library failed to match the same physical file under another path");
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
        std::cout << "PASS: mono/stereo source rates, cue-free export, overload policy, "
                     "content probing, streamed WAV/MP3 sequential/seek parity, changed/missing sources, "
                     "project round-trip, source identity, export "
                     "parity, atomic cancellation, schema validation, stable identities, legacy migration "
                     "and decoder-domain checks, export write/header/flush/replace failure retention\n";
        return 0;
    } catch (const std::exception &e) {
        dir.deleteRecursively();
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
