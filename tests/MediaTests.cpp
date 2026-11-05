// SPDX-License-Identifier: AGPL-3.0-or-later
#include "Library.h"
#include "Media.h"
#include <cmath>
#include <filesystem>
#include <iostream>
#include <stdexcept>
using namespace fader;
void testArrangement();
void testAudioSetup();
void check(bool pass, const char *message) {
    if (!pass)
        throw std::runtime_error(message);
}
int main() {
    juce::ScopedJuceInitialiser_GUI juce;
    testArrangement();
    testAudioSetup();
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
        const auto modernWave = dir.getChildFile("modern.wav");
        exportWav(modernReopened, modernWave, true);
        auto modernAudio = loadClip(modernWave);
        Engine modernEngine(modern);
        modernEngine.process(expected.getArrayOfWritePointers(), 2, 48000);
        for (int i = 0; i < 48000; ++i)
            check(std::abs(expected.getSample(0, i) -
                           modernAudio.audio->samples[0][static_cast<std::size_t>(i)]) < 1e-6f,
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
        std::cout << "PASS: content probing, full decode, project round-trip, source identity, export "
                     "parity, atomic cancellation, schema validation\n";
        return 0;
    } catch (const std::exception &e) {
        dir.deleteRecursively();
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
