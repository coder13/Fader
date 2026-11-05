// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <juce_audio_devices/juce_audio_devices.h>

namespace fader {
// Message/control-thread operation only. A successful return means the actual device
// is open at 48 kHz with stereo output, not that requested buffer/latency was achieved.
juce::String configureAudioOutput(juce::AudioDeviceManager &, bool preserveBuffer = false);
} // namespace fader
