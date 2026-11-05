// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <fader/Playback.h>
#include <juce_audio_devices/juce_audio_devices.h>

namespace fader {
// Own on the control thread. Playback outlives this bridge. Only attach/detach
// take the framework callback lock; none of the project-owned callbacks do.
class AudioPlayback final : public juce::AudioIODeviceCallback {
  public:
    explicit AudioPlayback(Playback &playback) : playback(playback) {}
    ~AudioPlayback() override {
        detach();
    }
    void attach(juce::AudioDeviceManager &);
    void detach();
    void audioDeviceIOCallbackWithContext(const float *const *, int, float *const *, int, int,
                                          const juce::AudioIODeviceCallbackContext &) override;
    void audioDeviceAboutToStart(juce::AudioIODevice *) override;
    void audioDeviceStopped() override;
    void audioDeviceError(const juce::String &) override;

  private:
    Playback &playback;
    juce::AudioDeviceManager *manager = nullptr;
    bool detaching = false; // protected by the manager callback lock
};
} // namespace fader
