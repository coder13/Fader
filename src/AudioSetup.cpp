// SPDX-License-Identifier: AGPL-3.0-or-later
#include "AudioSetup.h"
#include <cmath>
#include <fader/Engine.h>

namespace fader {
juce::String configureAudioOutput(juce::AudioDeviceManager &manager, bool preserveBuffer) {
    juce::String error;
    auto setup = manager.getAudioDeviceSetup();
    setup.sampleRate = projectRate;
    if (!preserveBuffer || setup.bufferSize <= 0)
        setup.bufferSize = 128;
    if (!manager.getCurrentAudioDevice()) {
        error = manager.initialise(0, 4, nullptr, true, {}, &setup);
        if (error.isNotEmpty()) {
            setup.bufferSize = 256;
            error = manager.initialise(0, 4, nullptr, true, {}, &setup);
        }
    } else {
        error = manager.setAudioDeviceSetup(setup, true);
        if (error.isNotEmpty()) {
            setup.bufferSize = 256;
            error = manager.setAudioDeviceSetup(setup, true);
        }
    }
    if (error.isNotEmpty())
        return "Could not open audio output: " + error;
    const auto device = manager.getCurrentAudioDevice();
    if (!device || !device->isOpen())
        return "No audio output is available. Choose an output in Preferences.";
    if (!std::isfinite(device->getCurrentSampleRate()) ||
        std::abs(device->getCurrentSampleRate() - projectRate) > 0.1)
        return "The selected output did not open at 48 kHz. Choose a compatible output in Preferences.";
    if (device->getActiveOutputChannels().countNumberOfSetBits() < 2)
        return "Stereo output is required. Enable at least two output channels in Preferences.";
    return {};
}
} // namespace fader
