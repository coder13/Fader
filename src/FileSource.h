// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <fader/Streaming.h>
#include <juce_audio_formats/juce_audio_formats.h>

namespace fader {
std::shared_ptr<const StreamSource> makeFileSource(const juce::File &, SourceFrame frames, double sampleRate,
                                                   unsigned channels, juce::int64 bytes,
                                                   juce::int64 modifiedMillis);
} // namespace fader
