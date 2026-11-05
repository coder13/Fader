// SPDX-License-Identifier: AGPL-3.0-or-later
#include "AudioPlayback.h"

namespace fader {
void AudioPlayback::attach(juce::AudioDeviceManager &deviceManager) {
    detach();
    manager = &deviceManager;
    manager->addAudioCallback(this);
}
void AudioPlayback::detach() {
    if (!manager)
        return;
    // Serialize with device-stop notification. Suppress only the synthetic
    // stopped() emitted by removeAudioCallback, never an earlier real fault.
    const juce::ScopedLock lock(manager->getAudioCallbackLock());
    detaching = true;
    manager->removeAudioCallback(this);
    detaching = false;
    manager = nullptr;
}
void AudioPlayback::audioDeviceIOCallbackWithContext(const float *const *, int, float *const *output,
                                                     int channels, int frames,
                                                     const juce::AudioIODeviceCallbackContext &) {
    playback.process(output, channels, frames, monotonicTime());
}
void AudioPlayback::audioDeviceAboutToStart(juce::AudioIODevice *device) {
    playback.deviceStarted(device ? device->getCurrentSampleRate() : 0);
}
void AudioPlayback::audioDeviceStopped() {
    if (!detaching)
        playback.deviceStopped();
}
void AudioPlayback::audioDeviceError(const juce::String &) {
    playback.deviceStopped();
}
} // namespace fader
