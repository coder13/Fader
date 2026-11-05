// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "Media.h"

// Probe/test utility. Reads the complete fixture sequentially through an
// independent decoder, then checks cached forward/backward seeks against it.
// Retains only bounded blocks and selected reference windows, not the track.
juce::var qualifyStream(const juce::File &, bool cacheCompressed = true);
