// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "Media.h"

// Software-only qualification. Never opens devices or reads user preferences.
juce::var qualifySoftwareStress(const juce::File &folder, int seconds, int blockFrames,
                                bool realtimeThread = false);
juce::var qualifyStressFaults(const juce::File &folder);
juce::var stressSelfTest();
