// SPDX-License-Identifier: AGPL-3.0-or-later
#include "AudioSetup.h"
#include <memory>
#include <stdexcept>

namespace {
struct DeviceOptions {
    bool visible = true, failOpen = false, fail128 = false;
    int channels = 2;
    double actualRate = 48000;
};
// Only these fake backends are registered. The test never opens a system device.
class Device final : public juce::AudioIODevice {
  public:
    explicit Device(std::shared_ptr<DeviceOptions> options)
        : AudioIODevice("Fixture output", "Fixture backend"), options(std::move(options)) {}
    ~Device() override {
        stop();
    }
    juce::StringArray getOutputChannelNames() override {
        juce::StringArray names;
        for (int i = 0; i < options->channels; ++i)
            names.add("Channel " + juce::String(i + 1));
        return names;
    }
    juce::StringArray getInputChannelNames() override {
        return {};
    }
    juce::Array<double> getAvailableSampleRates() override {
        return {44100, 48000};
    }
    juce::Array<int> getAvailableBufferSizes() override {
        return {128, 256, 512};
    }
    int getDefaultBufferSize() override {
        return 512;
    }
    juce::String open(const juce::BigInteger &, const juce::BigInteger &out, double, int size) override {
        close();
        if (options->failOpen || (options->fail128 && size == 128))
            return "fixture driver refused the buffer";
        opened = true;
        block = size;
        outputs = out;
        outputs.setRange(options->channels, std::max(0, out.getHighestBit() + 1 - options->channels), false);
        return {};
    }
    void close() override {
        stop();
        opened = false;
    }
    bool isOpen() override {
        return opened;
    }
    void start(juce::AudioIODeviceCallback *next) override {
        stop();
        callback = next;
        if (callback)
            callback->audioDeviceAboutToStart(this);
    }
    void stop() override {
        const auto previous = callback;
        callback = nullptr;
        if (previous)
            previous->audioDeviceStopped();
    }
    bool isPlaying() override {
        return opened && callback;
    }
    juce::String getLastError() override {
        return {};
    }
    int getCurrentBufferSizeSamples() override {
        return block;
    }
    double getCurrentSampleRate() override {
        return options->actualRate;
    }
    int getCurrentBitDepth() override {
        return 32;
    }
    juce::BigInteger getActiveOutputChannels() const override {
        return outputs;
    }
    juce::BigInteger getActiveInputChannels() const override {
        return {};
    }
    int getOutputLatencyInSamples() override {
        return 0;
    }
    int getInputLatencyInSamples() override {
        return 0;
    }

  private:
    std::shared_ptr<DeviceOptions> options;
    bool opened = false;
    int block = 0;
    juce::BigInteger outputs;
    juce::AudioIODeviceCallback *callback = nullptr;
};
class Type final : public juce::AudioIODeviceType {
  public:
    explicit Type(std::shared_ptr<DeviceOptions> options)
        : AudioIODeviceType("Fixture backend"), options(std::move(options)) {}
    void scanForDevices() override {}
    juce::StringArray getDeviceNames(bool input) const override {
        return !input && options->visible ? juce::StringArray{"Fixture output"} : juce::StringArray{};
    }
    int getDefaultDeviceIndex(bool) const override {
        return 0;
    }
    int getIndexOfDevice(juce::AudioIODevice *, bool input) const override {
        return input ? -1 : 0;
    }
    bool hasSeparateInputsAndOutputs() const override {
        return true;
    }
    juce::AudioIODevice *createDevice(const juce::String &out, const juce::String &) override {
        return out.isNotEmpty() ? new Device(options) : nullptr;
    }

  private:
    std::shared_ptr<DeviceOptions> options;
};
class Manager final : public juce::AudioDeviceManager {
  public:
    explicit Manager(DeviceOptions value = {}) : options(std::make_shared<DeviceOptions>(value)) {}
    void createAudioDeviceTypes(juce::OwnedArray<juce::AudioIODeviceType> &types) override {
        types.add(new Type(options));
    }

  private:
    std::shared_ptr<DeviceOptions> options;
};
void require(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
} // namespace
void testAudioSetup() {
    Manager stereo;
    require(fader::configureAudioOutput(stereo).isEmpty(), "stereo setup failed");
    require(stereo.getCurrentAudioDevice()->getCurrentBufferSizeSamples() == 128,
            "initial setup did not request 128 frames");
    auto setup = stereo.getAudioDeviceSetup();
    setup.bufferSize = 512;
    require(stereo.setAudioDeviceSetup(setup, true).isEmpty(), "fixture buffer change failed");
    require(fader::configureAudioOutput(stereo, true).isEmpty() &&
                stereo.getCurrentAudioDevice()->getCurrentBufferSizeSamples() == 512,
            "user-selected buffer was overwritten");
    Manager absent(DeviceOptions{.visible = false});
    require(fader::configureAudioOutput(absent).contains("No audio output"),
            "missing output reported success");
    Manager denied(DeviceOptions{.failOpen = true});
    require(fader::configureAudioOutput(denied).contains("fixture driver refused"),
            "initialization error was hidden");
    Manager fallback(DeviceOptions{.fail128 = true});
    require(fader::configureAudioOutput(fallback).isEmpty() &&
                fallback.getCurrentAudioDevice()->getCurrentBufferSizeSamples() == 256,
            "128-frame failure did not fall back to 256");
    Manager wrongRate(DeviceOptions{.actualRate = 44100});
    require(fader::configureAudioOutput(wrongRate).contains("48 kHz"),
            "negotiated unsupported sample rate accepted");
    Manager mono(DeviceOptions{.channels = 1});
    require(fader::configureAudioOutput(mono).contains("Stereo output"),
            "mono-only device accepted for stereo engine");
}
