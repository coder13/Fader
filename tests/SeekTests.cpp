// SPDX-License-Identifier: AGPL-3.0-or-later
#include <algorithm>
#include <cmath>
#include <fader/Engine.h>
#include <future>
#include <iostream>
#include <stdexcept>
#include <thread>
void testPreparationService();

namespace {
using namespace fader;
void check(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
struct Control {
    std::atomic<bool> entered{false};
    bool blocked = false;
};
class Decoder final : public SourceDecoder {
  public:
    explicit Decoder(std::shared_ptr<Control> control) : control(std::move(control)) {}
    bool read(SourceFrame first, int count, float *const *out, const std::atomic<bool> &cancelled) override {
        control->entered.store(true);
        while (control->blocked && !cancelled.load())
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        if (cancelled.load())
            return false;
        for (int i = 0; i < count; ++i) {
            const auto frame = first + i;
            out[0][i] = 0.2f * std::sin(float(frame % 9137) * 0.017f);
            out[1][i] = 0.15f * std::cos(float(frame % 10717) * 0.031f);
        }
        return true;
    }

  private:
    std::shared_ptr<Control> control;
};
class Source final : public StreamSource {
  public:
    explicit Source(std::shared_ptr<Control> control = std::make_shared<Control>())
        : control(std::move(control)) {}
    SourceFrame frames() const noexcept override {
        return SourceFrame{4} * 3600 * 48000;
    }
    std::unique_ptr<SourceDecoder> open() const override {
        return std::make_unique<Decoder>(control);
    }

  private:
    std::shared_ptr<Control> control;
};
Project fixture(double firstRate, double secondRate, double firstSpeed, double secondSpeed) {
    Project p;
    p.perClip = true;
    for (unsigned i = 0; i < 2; ++i) {
        auto audio = std::make_shared<AudioData>();
        audio->sampleRate = i == 0 ? firstRate : secondRate;
        audio->stream = std::make_shared<Source>();
        auto &clip = p.clips[i];
        clip.audio = audio;
        clip.start = i * 379;
        clip.sourceOffset = 97 + i * 197;
        clip.length = 20 * projectRate - clip.start;
        clip.rate = i == 0 ? firstSpeed : secondSpeed;
        clip.cue = true;
        clip.automation[0] = {
            0.4f,
            true,
            {{0, 0.9f}, {13121, 0.2f}, {480017, 0.8f, true}, {481001, 0.1f}, {720037, 1.0f}, {900000, 0.5f}}};
        clip.automation[1] = {
            0.1f,
            true,
            {{0, 0}, {19121, 0.8f}, {310021, 0.01f, true}, {400000, 0.7f}, {730071, 0}, {900001, 0.9f}}};
        clip.automation[2] = {
            0.8f,
            true,
            {{0, 1}, {91121, 0.001f}, {500027, 0.6f, true}, {650000, 1}, {730073, 0.2f}, {900001, 0}}};
    }
    return p;
}
void parity() {
    double maxPCM = 0, maxParameter = 0;
    for (const auto rates :
         {std::array<double, 4>{44100, 96000, 0.75, 1.3}, std::array<double, 4>{8000, 384000, 0.25, 4}}) {
        const auto project = fixture(rates[0], rates[1], rates[2], rates[3]);
        auto reference = std::make_unique<Engine>(project);
        std::array<std::array<float, 1024>, 4> actual{}, expected{};
        float *a[]{actual[0].data(), actual[1].data(), actual[2].data(), actual[3].data()};
        float *b[]{expected[0].data(), expected[1].data(), expected[2].data(), expected[3].data()};
        const std::array<Frame, 11> targets{0,      379,    48000,  95999,  96257, 310021,
                                            480017, 481001, 650000, 730073, 950001};
        for (const auto target : targets) {
            while (reference->position() < target) {
                const int count = static_cast<int>(std::min(Frame{1024}, target - reference->position()));
                reference->prepareSources(count);
                reference->process(b, 4, count);
            }
            auto seek = std::make_unique<Engine>(project, target);
            check(seek->position() == target && seek->preparationFrames() <= seekPrerollFrames,
                  "seek position or bounded preroll contract failed");
            for (unsigned clip = 0; clip < 2; ++clip) {
                const auto x = seek->preparedParameters(clip), y = reference->preparedParameters(clip);
                for (unsigned parameter = 0; parameter < 3; ++parameter)
                    maxParameter = std::max(maxParameter, std::abs(double(x[parameter]) - y[parameter]));
            }
            seek->prepareSources(128);
            reference->prepareSources(128);
            seek->process(a, 4, 128);
            reference->process(b, 4, 128);
            for (unsigned channel = 0; channel < 4; ++channel)
                for (unsigned sample = 0; sample < 128; ++sample)
                    maxPCM = std::max(maxPCM,
                                      std::abs(double(actual[channel][sample]) - expected[channel][sample]));
            check(seek->starvationFrames() == 0 && reference->starvationFrames() == 0,
                  "prepared seek/reference starved");
        }
    }
    std::cout << "Seek max absolute PCM error=" << maxPCM << ", parameter error=" << maxParameter << '\n';
    check(maxPCM <= 1e-5 && maxParameter <= 1e-5, "bounded seek differs from the sequential reference");
}
void cancellation() {
    auto control = std::make_shared<Control>();
    control->blocked = true;
    auto data = std::make_shared<AudioData>();
    data->stream = std::make_shared<Source>(control);
    Project project;
    project.perClip = true;
    project.clips.resize(1);
    project.clips[0].audio = data;
    project.clips[0].length = Frame{4} * 3600 * projectRate;
    auto stop = std::make_shared<std::atomic<bool>>(false);
    auto result = std::async(std::launch::async, [=] {
        try {
            auto engine = std::make_unique<Engine>(project, Frame{3} * 3600 * projectRate, Preparation{stop});
            return false;
        } catch (const std::runtime_error &) {
            return stop->load();
        }
    });
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (!control->entered.load() && std::chrono::steady_clock::now() < deadline)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    stop->store(true);
    check(control->entered.load() && result.wait_for(std::chrono::seconds(1)) == std::future_status::ready &&
              result.get(),
          "cancellation did not stop blocked seek preparation and its worker");
}
} // namespace
int main() {
    try {
        parity();
        cancellation();
        testPreparationService();
        std::cout << "PASS: bounded seek state/PCM, cancellable readiness, latest request, close and worker "
                     "retirement\n";
    } catch (const std::exception &error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
