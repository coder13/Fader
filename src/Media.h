// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <fader/Engine.h>
#include <fader/Playback.h>
#include <functional>
#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_cryptography/juce_cryptography.h>

namespace fader {
Clip loadClip(const juce::File &);
void saveProject(const Project &, const juce::File &);
struct IncompleteTake {
    Project original;
    std::vector<Event> events;
    Frame in{}, out{};
    PlaybackStats stats;
};
void saveIncompleteTake(const IncompleteTake &, const juce::File &);
Project loadProject(const juce::File &);
void exportWav(const Project &, const juce::File &, bool floatingPoint = false,
               std::function<bool(double)> progress = {});
juce::var inspectDevices();
} // namespace fader
