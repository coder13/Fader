// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "Media.h"
#include <map>

namespace fader {
struct TrackInfo {
    juce::File file;
    juce::String title, artist, key, album, genre, source = "File";
    double bpm = 0, duration = 0, firstBeat = 0;
    int cueCount = 0;
    bool hasGrid = false, missing = false;
};
struct LibraryResult {
    std::vector<TrackInfo> tracks;
    int traktorEntries = 0, matched = 0, missing = 0;
    juce::String warning;
};
struct Preferences {
    juce::File musicFolder, collection;
    bool importTraktor = true;
    static juce::File defaultFile();
    static Preferences load(const juce::File &file = defaultFile());
    void save(const juce::File &file = defaultFile()) const;
};
std::map<juce::String, TrackInfo> readTraktor(const juce::File &);
LibraryResult scanLibrary(const Preferences &);
juce::String traktorKey(int code);
} // namespace fader
