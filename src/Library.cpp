// SPDX-License-Identifier: AGPL-3.0-or-later
#include "Library.h"
#include "FilePaths.h"
#include <cmath>
#include <filesystem>
#include <stdexcept>
#include <unordered_map>

namespace fader {
namespace {
juce::String pathKey(const juce::File &f) {
    return f.getFullPathName();
}
juce::String textFrame(const unsigned char *data, std::size_t size) {
    if (size < 2)
        return {};
    const auto encoding = *data++;
    --size;
    if (encoding == 3)
        return juce::String::fromUTF8(reinterpret_cast<const char *>(data), static_cast<int>(size))
            .trimCharactersAtEnd(juce::String::charToString(0))
            .trim();
    juce::String result;
    if (encoding == 0) {
        for (std::size_t i = 0; i < size && data[i]; ++i)
            result += juce::String::charToString(data[i]);
    } else if (encoding == 1 || encoding == 2) {
        bool little = false;
        std::size_t i = 0;
        if (size >= 2 && data[0] == 255 && data[1] == 254) {
            little = true;
            i = 2;
        } else if (size >= 2 && data[0] == 254 && data[1] == 255)
            i = 2;
        for (; i + 1 < size; i += 2) {
            auto value = static_cast<juce::juce_wchar>(little ? (data[i] | data[i + 1] << 8)
                                                              : (data[i] << 8 | data[i + 1]));
            if (!value)
                break;
            result += juce::String::charToString(value);
        }
    }
    return result.trim();
}
void readTags(TrackInfo &track) {
    if (!track.file.hasFileExtension("mp3"))
        return;
    auto input = track.file.createInputStream();
    if (!input)
        return;
    unsigned char header[10]{};
    if (input->read(header, 10) != 10 || std::string(reinterpret_cast<char *>(header), 3) != "ID3")
        return;
    if ((header[3] != 3 && header[3] != 4) || (header[5] & 0xc0))
        return; // compressed/extended/unsynchronised tags stay unknown.
    const auto size = (header[6] << 21) | (header[7] << 14) | (header[8] << 7) | header[9];
    if (size <= 0 || size > 2 * 1024 * 1024)
        return;
    std::vector<unsigned char> data(static_cast<std::size_t>(size));
    if (input->read(data.data(), size) != size)
        return;
    for (std::size_t at = 0; at + 10 < data.size();) {
        const auto *p = data.data() + at;
        const std::string id(reinterpret_cast<const char *>(p), 4);
        const std::size_t count =
            header[3] == 4
                ? static_cast<std::size_t>((p[4] << 21) | (p[5] << 14) | (p[6] << 7) | p[7])
                : (std::size_t{p[4]} << 24) | (std::size_t{p[5]} << 16) | (std::size_t{p[6]} << 8) | p[7];
        if (count == 0 || count > data.size() - at - 10)
            break;
        if (p[8] == 0 && p[9] == 0) {
            const auto value = textFrame(p + 10, count);
            if (value.isNotEmpty()) {
                if (id == "TIT2")
                    track.title = value;
                else if (id == "TPE1")
                    track.artist = value;
                else if (id == "TKEY")
                    track.key = value;
                else if (id == "TBPM")
                    track.bpm = value.getDoubleValue();
                else if (id == "TALB")
                    track.album = value;
                else if (id == "TCON")
                    track.genre = value;
                if (id == "TIT2" || id == "TPE1" || id == "TKEY" || id == "TBPM")
                    track.source = "File tags";
            }
        }
        at += 10 + count;
    }
    if (!std::isfinite(track.bpm) || track.bpm < 20 || track.bpm > 400)
        track.bpm = 0;
}
} // namespace
juce::String traktorKey(int code) {
    // Traktor's 0..11 major / 12..23 minor chromatic codes, shown as Open Key.
    constexpr const char *names[]{"1d", "8d", "3d",  "10d", "5d",  "12d", "7d",  "2d",
                                  "9d", "4d", "11d", "6d",  "10m", "5m",  "12m", "7m",
                                  "2m", "9m", "4m",  "11m", "6m",  "1m",  "8m",  "3m"};
    return code >= 0 && code < 24 ? names[code] : juce::String{};
}
std::map<juce::String, TrackInfo> readTraktor(const juce::File &file) {
    if (!file.existsAsFile() || file.getSize() > 64 * 1024 * 1024)
        throw std::runtime_error("Missing or oversized Traktor collection");
    auto root = juce::XmlDocument::parse(file);
    if (!root || !root->hasTagName("NML"))
        throw std::runtime_error("This is not a valid Traktor NML collection");
    const auto *collection = root->getChildByName("COLLECTION");
    if (!collection)
        throw std::runtime_error("Traktor collection contains no COLLECTION element");
    std::map<juce::String, TrackInfo> result;
    for (auto *e : collection->getChildIterator()) {
        if (!e->hasTagName("ENTRY"))
            continue;
        const auto *loc = e->getChildByName("LOCATION");
        if (!loc)
            continue;
        auto directory = loc->getStringAttribute("DIR").replace("/:", "/");
        const auto filename = loc->getStringAttribute("FILE");
        if (directory.isEmpty() || filename.isEmpty())
            continue;
        if (!juce::File::isAbsolutePath(directory))
            continue;
        juce::File path(directory + filename);
#if JUCE_MAC
        if (!path.existsAsFile() && !directory.startsWith("/Volumes/")) {
            const auto external = juce::File("/Volumes")
                                      .getChildFile(loc->getStringAttribute("VOLUME"))
                                      .getChildFile(directory.trimCharactersAtStart("/") + filename);
            if (external.existsAsFile())
                path = external;
        }
#endif
        TrackInfo t;
        t.file = path;
        t.title = e->getStringAttribute("TITLE", path.getFileNameWithoutExtension());
        t.artist = e->getStringAttribute("ARTIST");
        t.source = "Traktor";
        t.missing = !path.existsAsFile();
        if (const auto *info = e->getChildByName("INFO")) {
            t.key = info->getStringAttribute("KEY");
            t.genre = info->getStringAttribute("GENRE");
            t.duration = info->getDoubleAttribute("PLAYTIME_FLOAT", info->getDoubleAttribute("PLAYTIME"));
        }
        if (const auto *tempo = e->getChildByName("TEMPO"))
            t.bpm = tempo->getDoubleAttribute("BPM");
        if (const auto *key = e->getChildByName("MUSICAL_KEY")) {
            auto value = traktorKey(key->getIntAttribute("VALUE", -1));
            if (value.isNotEmpty())
                t.key = value;
        }
        if (const auto *album = e->getChildByName("ALBUM"))
            t.album = album->getStringAttribute("TITLE");
        for (auto *cue : e->getChildIterator())
            if (cue->hasTagName("CUE_V2")) {
                ++t.cueCount;
                const auto time = cue->getDoubleAttribute("START", -1) / 1000.0;
                if (cue->getIntAttribute("TYPE") == 4 && std::isfinite(time) && time >= 0 &&
                    (!t.hasGrid || time < t.firstBeat)) {
                    t.firstBeat = time;
                    t.hasGrid = true;
                }
            }
        if (!std::isfinite(t.bpm) || t.bpm < 20 || t.bpm > 400)
            t.bpm = 0;
        if (!std::isfinite(t.duration) || t.duration < 0)
            t.duration = 0;
        result[pathKey(path)] = std::move(t);
    }
    return result;
}
juce::File Preferences::defaultFile() {
    auto root = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
#if JUCE_MAC
    root = root.getChildFile("Application Support");
#endif
    return root.getChildFile("Fader/preferences.json");
}
Preferences Preferences::load(const juce::File &file) {
    Preferences p;
    const auto music = juce::File::getSpecialLocation(juce::File::userMusicDirectory);
    p.musicFolder = music.getChildFile("DJ").isDirectory() ? music.getChildFile("DJ") : music;
    if (file.existsAsFile()) {
        auto json = juce::JSON::parse(file);
        if (json.isObject()) {
            auto folder = json["musicFolder"].toString();
            if (juce::File::isAbsolutePath(folder))
                p.musicFolder = juce::File(folder);
            auto nml = json["traktorCollection"].toString();
            if (juce::File::isAbsolutePath(nml))
                p.collection = juce::File(nml);
            if (json.hasProperty("importTraktor"))
                p.importTraktor = static_cast<bool>(json["importTraktor"]);
            return p;
        }
    }
    const auto ni =
        juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("Native Instruments");
    std::error_code error;
    std::filesystem::directory_iterator entries(
        filesystemPath(ni), std::filesystem::directory_options::skip_permission_denied, error),
        end;
    for (; entries != end; entries.increment(error)) {
        if (error) {
            error.clear();
            continue;
        }
        const auto dir = fileFromPath(entries->path());
        if (!dir.getFileName().startsWithIgnoreCase("Traktor"))
            continue;
        const auto nml = dir.getChildFile("collection.nml");
        if (nml.existsAsFile() && (!p.collection.existsAsFile() ||
                                   nml.getLastModificationTime() > p.collection.getLastModificationTime()))
            p.collection = nml;
    }
    return p;
}
void Preferences::save(const juce::File &file) const {
    if (file.getParentDirectory().createDirectory().failed())
        throw std::runtime_error("Could not create preferences folder");
    auto *o = new juce::DynamicObject();
    juce::var json(o);
    o->setProperty("musicFolder", musicFolder.getFullPathName());
    o->setProperty("traktorCollection", collection.getFullPathName());
    o->setProperty("importTraktor", importTraktor);
    juce::TemporaryFile temporary(file);
    if (!temporary.getFile().replaceWithText(juce::JSON::toString(json, true)) ||
        !temporary.overwriteTargetFileWithTemporary())
        throw std::runtime_error("Could not save preferences");
}
LibraryResult scanLibrary(const Preferences &p) {
    LibraryResult result;
    std::map<juce::String, TrackInfo> traktor;
    if (p.importTraktor && p.collection != juce::File{}) {
        try {
            traktor = readTraktor(p.collection);
            result.traktorEntries = static_cast<int>(traktor.size());
        } catch (const std::exception &e) {
            result.warning = e.what();
        }
    }
    if (!p.musicFolder.isDirectory())
        throw std::runtime_error("Music folder is unavailable. Choose a folder in Preferences.");
    // IDs narrow the search; equivalent() verifies identity even across volumes.
    // This also handles case and Unicode spelling differences on macOS.
    std::unordered_map<juce::uint64, std::vector<decltype(traktor)::const_iterator>> identities;
    for (auto entry = traktor.cbegin(); entry != traktor.cend(); ++entry)
        if (!entry->second.missing)
            if (const auto id = entry->second.file.getFileIdentifier())
                identities[id].push_back(entry);
    std::error_code error;
    std::filesystem::recursive_directory_iterator it(
        filesystemPath(p.musicFolder), std::filesystem::directory_options::skip_permission_denied, error),
        end;
    std::map<juce::String, bool> seen;
    for (; it != end; it.increment(error)) {
        if (error) {
            error.clear();
            continue;
        }
        if (!it->is_regular_file(error))
            continue;
        const auto file = fileFromPath(it->path());
        if (!file.hasFileExtension("mp3;wav;wave;aif;aiff;flac;m4a;aac;ogg;opus;alac"))
            continue;
        auto found = traktor.cend();
        found = traktor.find(pathKey(file));
        if (found == traktor.cend()) {
            const auto candidates = identities.find(file.getFileIdentifier());
            if (candidates != identities.end())
                for (const auto candidate : candidates->second) {
                    std::error_code identityError;
                    if (std::filesystem::equivalent(it->path(), filesystemPath(candidate->second.file),
                                                    identityError) &&
                        !identityError) {
                        found = candidate;
                        break;
                    }
                }
        }
        TrackInfo track;
        if (found != traktor.end()) {
            track = found->second;
            track.file = file;
            ++result.matched;
        } else {
            track.file = file;
            track.title = file.getFileNameWithoutExtension();
            readTags(track);
        }
        seen[pathKey(file)] = true;
        result.tracks.push_back(std::move(track));
        if (result.tracks.size() >= 100000) {
            result.warning = "Library scan stopped at the prototype's 100,000-file limit";
            break;
        }
    }
    for (const auto &[path, track] : traktor)
        if (track.file.isAChildOf(p.musicFolder) && !seen.contains(path) && track.missing) {
            result.tracks.push_back(track);
            ++result.missing;
        }
    std::sort(result.tracks.begin(), result.tracks.end(),
              [](const auto &a, const auto &b) { return a.title.compareNatural(b.title) < 0; });
    return result;
}
} // namespace fader
