// SPDX-License-Identifier: AGPL-3.0-or-later
#include "FilePaths.h"
#include "Library.h"
#include "ProjectStorage.h"
#include "ReadFixture.h"
#include <fader/OfflineRender.h>
#include <iostream>

namespace {
using namespace fader;
constexpr int frames = 8192;
constexpr std::array<int, 8> levels{-32768, -16384, -8192, 0, 8192, 16384, 24576, 32767};
void check(bool pass, const char *message) {
    if (!pass)
        throw std::runtime_error(message);
}
void fixture(const juce::File &file) {
    auto out = file.createOutputStream();
    check(bool(out), "Cannot create portable WAV fixture");
    check(out->write("RIFF", 4) && out->writeInt(36 + frames * 4) && out->write("WAVEfmt ", 8) &&
              out->writeInt(16) && out->writeShort(1) && out->writeShort(2) && out->writeInt(48000) &&
              out->writeInt(192000) && out->writeShort(4) && out->writeShort(16) && out->write("data", 4) &&
              out->writeInt(frames * 4),
          "Portable WAV header write failed");
    for (int i = 0; i < frames; ++i)
        check(out->writeShort(static_cast<short>(levels[i % 8])) &&
                  out->writeShort(static_cast<short>(levels[(i * 3 + 1) % 8])),
              "Portable WAV body write failed");
    out->flush();
    check(out->getStatus().wasOk(), "Portable WAV flush failed");
}
void pcm(const Project &project) {
    OfflineRender renderer(project);
    std::array<std::array<float, 128>, 2> output{};
    float *channels[]{output[0].data(), output[1].data()};
    Frame at = 0;
    while (const auto count = renderer.read(channels, 128)) {
        for (int i = 0; i < count; ++i) {
            check(output[0][i] == float(levels[(at + i) % 8]) / 32768.0f &&
                      output[1][i] == float(levels[((at + i) * 3 + 1) % 8]) / 32768.0f,
                  "Portable fixture differs from independently defined PCM");
        }
        at += count;
    }
    check(at == frames, "Portable render duration changed");
}
} // namespace

void testPortability() {
    using namespace fader;
    const auto root = juce::File::getSpecialLocation(juce::File::tempDirectory)
                          .getNonexistentChildFile("fader-portability", {}, false);
    struct Cleanup {
        juce::File root;
        ~Cleanup() {
            root.deleteRecursively();
        }
    } cleanup{root};
    const auto folder = root.getChildFile(juce::String::fromUTF8("DJ café α 音楽 🎛"));
    const auto media = folder.getChildFile(juce::String::fromUTF8("素材"));
    check(media.createDirectory().wasOk(), "Cannot create Unicode media folder");
    const auto audio = media.getChildFile(juce::String::fromUTF8("Track déjà-vu α 音楽 🎵.wav"));
    fixture(audio);
    check(fileFromPath(filesystemPath(audio)) == audio &&
              std::filesystem::equivalent(
                  filesystemPath(fileFromPath(std::filesystem::canonical(filesystemPath(audio)))),
                  filesystemPath(audio)),
          "UTF-8/native filesystem conversion changed the Unicode path");
    check(juce::SHA256(audio).toHexString() ==
              "9274aec9d580f2434ac82f1f602a052a4c48c6f5a09165fbb38b5a47ce841977",
          "Portable encoded WAV bytes changed");
    auto clip = loadClip(audio);
    check(clip.audio->pcmHash == "3f3cd41159de8c77db90c35d1fee239429edac6b0b75458878da3ac277828c96" &&
              clip.audio->sampleRate == 48000 && clip.audio->sourceChannels == 2 && clip.length == frames,
          "Portable WAV decoded domain differs across the host");
    clip.name = "Prepared café α 音楽";
    clip.artist = "Synthetic 🎵";
    clip.automation[0] = {1, true, {{0, 1}}};
    clip.automation[1] = {0, true, {{0, 0}}};
    clip.automation[2] = {1, true, {{0, 1}}};
    Project project;
    project.perClip = true;
    project.clips = {clip};
    const auto set = folder.getChildFile(juce::String::fromUTF8("Set été α 音楽.faderset"));
    saveProject(project, set);
    const auto document = juce::JSON::parse(set);
    const auto relative = document["assets"][0]["path"].toString();
    check(relative.containsChar('/') && !relative.containsChar('\\') && !juce::File::isAbsolutePath(relative),
          "Stored media path is not portable and relative");
    const auto reopened = loadProject(set);
    check(reopened.id == project.id && reopened.clips[0].id == clip.id &&
              reopened.clips[0].name == clip.name && reopened.clips[0].artist == clip.artist,
          "Unicode metadata or stable identities did not survive reopen");
    pcm(reopened);
    const auto output = folder.getChildFile(juce::String::fromUTF8("Export été 音楽.wav"));
    exportWav(reopened, output, true);
    const auto samples = readImportedSamples(loadClip(output));
    for (int i = 0; i < frames; ++i)
        check(samples[0][i] == float(levels[i % 8]) / 32768.0f &&
                  samples[1][i] == float(levels[(i * 3 + 1) % 8]) / 32768.0f,
              "Portable exported PCM changed");

    Preferences prefs;
    prefs.musicFolder = media;
    prefs.importTraktor = false;
    const auto preferences = folder.getChildFile(juce::String::fromUTF8("Préférences α.json"));
    prefs.save(preferences);
    const auto storedPrefs = Preferences::load(preferences);
    check(storedPrefs.musicFolder == media && !storedPrefs.importTraktor,
          "Unicode library preferences did not survive reopen");
    auto library = scanLibrary(storedPrefs);
    check(library.tracks.size() == 1 &&
              library.tracks.front().file.getFileIdentifier() == audio.getFileIdentifier(),
          "Filesystem library scan lost the Unicode song");
    juce::XmlElement nml("NML");
    auto *entry = nml.createNewChildElement("COLLECTION")->createNewChildElement("ENTRY");
    entry->setAttribute("TITLE", juce::String::fromUTF8(clip.name.c_str()));
    entry->setAttribute("ARTIST", juce::String::fromUTF8(clip.artist.c_str()));
    auto *location = entry->createNewChildElement("LOCATION");
    location->setAttribute("DIR",
                           media.getFullPathName().replaceCharacter('\\', '/').replace("/", "/:") + "/:");
    location->setAttribute("FILE", audio.getFileName());
    entry->createNewChildElement("TEMPO")->setAttribute("BPM", 128.0);
    entry->createNewChildElement("MUSICAL_KEY")->setAttribute("VALUE", 21);
    prefs.collection = folder.getChildFile(juce::String::fromUTF8("Collection été 音楽.nml"));
    check(prefs.collection.replaceWithText(nml.toString()), "Cannot write Unicode Traktor fixture");
    prefs.importTraktor = true;
    library = scanLibrary(prefs);
    check(library.tracks.size() == 1 && library.matched == 1 &&
              library.tracks.front().title == juce::String(clip.name) && library.tracks.front().key == "1m" &&
              library.tracks.front().bpm == 128,
          "Unicode Traktor metadata failed to match its source");

    const auto relocated = root.getChildFile(juce::String::fromUTF8("Relocated été α 音楽"));
    check(
        relocated.getChildFile(media.getFileName()).createDirectory().wasOk() &&
            audio.copyFileTo(relocated.getChildFile(media.getFileName()).getChildFile(audio.getFileName())) &&
            set.copyFileTo(relocated.getChildFile(set.getFileName())),
        "Cannot relocate the portable fixture");
    const auto moved = loadProject(relocated.getChildFile(set.getFileName()));
    check(moved.id == project.id && moved.clips[0].hash == clip.hash,
          "Unicode relocation changed asset or project identity");
    pcm(moved);
    const auto mp3 = loadClip(juce::File(FADER_TEST_VBR_PATH));
    check(mp3.length > 7 * projectRate && mp3.audio->stream && mp3.audio->samples[0].empty(),
          "Representative MP3 decoder is unavailable or unbounded");
    std::cout << "DECODERS: WAV=" << clip.audio->decoder << "; MP3=" << mp3.audio->decoder
              << "; MP3 source frames=" << mp3.audio->frames() << "; rate=" << mp3.audio->sampleRate << '\n';
    std::cout << "PASS: portable WAV SHA/decoded-domain golden, exact PCM/export, Unicode paths/metadata, "
                 "library/Traktor/preferences and relocated project\n";
}
