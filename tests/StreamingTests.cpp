// SPDX-License-Identifier: AGPL-3.0-or-later
#include <cmath>
#include <fader/Engine.h>
#include <fader/Streaming.h>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <thread>

namespace {
using namespace fader;
void check(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
template <class Predicate> void eventually(Predicate predicate) {
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (!predicate()) {
        if (std::chrono::steady_clock::now() >= deadline)
            throw std::runtime_error("Worker fixture timed out");
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
struct Control {
    std::atomic<bool> allowed{true}, entered{false};
    std::atomic<unsigned> completed{0}, destroyed{0};
    std::atomic<bool> blocked{false};
    SourceFrame blockFrom = 0;
    bool fail = false, nonFinite = false;
};
float expected(SourceFrame frame, float base) {
    return base + float(frame % 997) * 0.0001f;
}
class Decoder final : public SourceDecoder {
  public:
    Decoder(std::shared_ptr<Control> control, float base) : control(std::move(control)), base(base) {}
    ~Decoder() override {
        control->destroyed.fetch_add(1);
    }
    bool read(SourceFrame first, int count, float *const *out, const std::atomic<bool> &cancelled) override {
        control->entered.store(true);
        while (first >= control->blockFrom && !control->allowed.load() && !cancelled.load()) {
            control->blocked.store(true);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        control->completed.fetch_add(1);
        if (control->fail || cancelled.load())
            return false;
        for (int i = 0; i < count; ++i) {
            out[0][i] = expected(first + i, base);
            out[1][i] = -out[0][i];
        }
        if (control->nonFinite)
            out[0][0] = std::numeric_limits<float>::quiet_NaN();
        return true;
    }

  private:
    std::shared_ptr<Control> control;
    float base;
};
class Source final : public StreamSource {
  public:
    Source(std::shared_ptr<Control> control, SourceFrame length, float base = 0.1f)
        : control(std::move(control)), length(length), base(base) {}
    SourceFrame frames() const noexcept override {
        return length;
    }
    std::unique_ptr<SourceDecoder> open() const override {
        return std::make_unique<Decoder>(control, base);
    }

  private:
    std::shared_ptr<Control> control;
    SourceFrame length;
    float base;
};
void paging() {
    constexpr SourceFrame length = SourceFrame(8) * 3600 * 48000 + 17;
    auto a = std::make_shared<Source>(std::make_shared<Control>(), length, 0.1f);
    auto b = std::make_shared<Source>(std::make_shared<Control>(), length, 0.3f);
    StreamBank bank({a, b});
    check(bank.stats().pcmBytes == 2 * StreamBank::pcmBytesPerSource && bank.stats().pcmBytes == 1024 * 1024,
          "cache memory depends on source duration");
    for (const SourceFrame index : {SourceFrame{0}, SourceFrame{8191}, SourceFrame{8192}, SourceFrame{65535},
                                    SourceFrame{300000}, SourceFrame{17}, length - 1}) {
        for (unsigned clip = 0; clip < 2; ++clip) {
            bank.prepare(clip, index, std::min(length - 1, index + 1));
            std::array<float, 2> current{}, next{};
            check(bank.sample(clip, index, current, next), "prepared PCM unavailable");
            const auto base = clip == 0 ? 0.1f : 0.3f;
            check(current[0] == expected(index, base) && current[1] == -current[0] &&
                      next[0] == expected(std::min(length - 1, index + 1), base) && next[1] == -next[0],
                  "paging, source ownership or terminal-frame alignment changed");
        }
        bank.release();
    }
    check(bank.stats().failedSources == 0, "healthy source failed");
}
void slowAndCancelled() {
    auto control = std::make_shared<Control>();
    control->allowed.store(false);
    auto source = std::make_shared<Source>(control, 480000);
    {
        StreamBank slow({source});
        slow.request(0, 0);
        eventually([&] { return control->entered.load(); });
        std::array<float, 2> current{}, next{};
        for (int i = 0; i < 10000; ++i)
            check(!slow.sample(0, i % 128, current, next), "unpublished PCM exposed");
        check(!control->allowed.load() && control->completed.load() == 0,
              "render fixture did not run while the decoder was blocked");
        bool timedOut = false;
        try {
            slow.prepare(0, 0, 128, std::chrono::milliseconds(20));
        } catch (const std::runtime_error &) {
            timedOut = true;
        }
        check(timedOut, "offline readiness silently accepted missing PCM");
        slow.cancel();
        auto replacement = std::make_shared<Source>(std::make_shared<Control>(), 480000, 0.8f);
        StreamBank fresh({replacement});
        fresh.prepare(0, 0, 128);
        check(fresh.stats().generation != slow.stats().generation && fresh.sample(0, 17, current, next) &&
                  current[0] == expected(17, 0.8f),
              "stale source read contaminated the new generation");
        fresh.release();
        eventually([&] { return slow.stats().discardedReads > 0; });
        check(slow.stats().cancelled && !slow.sample(0, 0, current, next),
              "cancelled generation continued serving audio");
    }
    check(control->destroyed.load() == 1, "decoder reclaimed before/without worker shutdown");
}
void staleSeek() {
    auto control = std::make_shared<Control>();
    control->allowed.store(false);
    StreamBank bank({std::make_shared<Source>(control, 480000)});
    bank.request(0, 0);
    eventually([&] { return control->blocked.load(); });
    bank.request(0, 200000); // the previous read is still in flight
    control->allowed.store(true);
    bank.prepare(0, 200000, 200128);
    std::array<float, 2> current{}, next{};
    check(bank.stats().discardedReads > 0 && bank.sample(0, 200017, current, next) &&
              current[0] == expected(200017, 0.1f),
          "stale seek request published into the new cache window");
    bank.release();
}
void engineParity() {
    constexpr int length = 60000;
    auto streamed = std::make_shared<AudioData>();
    streamed->sampleRate = 44100;
    streamed->stream = std::make_shared<Source>(std::make_shared<Control>(), length);
    auto dense = std::make_shared<AudioData>();
    dense->sampleRate = streamed->sampleRate;
    for (int channel = 0; channel < 2; ++channel) {
        dense->samples[channel].resize(length);
        for (int frame = 0; frame < length; ++frame)
            dense->samples[channel][frame] = expected(frame, 0.1f) * (channel == 0 ? 1 : -1);
    }
    Project p;
    p.perClip = true;
    p.clips.resize(1);
    auto &clip = p.clips[0];
    clip.audio = streamed;
    clip.start = 179;
    clip.sourceOffset = 17;
    clip.length = 20000;
    clip.rate = 1.25;
    clip.automation[1] = {0.2f, true, {{0, 0.2f}, {19000, 0.5f}}};
    clip.automation[2] = {0.8f, true, {{0, 0.8f}, {19000, 0.3f}}};
    auto baseline = p;
    baseline.clips[0].audio = dense;
    for (const int block : {64, 127, 1024}) {
        auto actual = std::make_unique<Engine>(p, 4001);
        auto reference = std::make_unique<Engine>(baseline, 4001);
        std::array<std::array<float, 1024>, 2> a{}, b{};
        float *outA[]{a[0].data(), a[1].data()}, *outB[]{b[0].data(), b[1].data()};
        while (actual->position() < p.end()) {
            actual->prepareSources(block);
            actual->process(outA, 2, block);
            reference->process(outB, 2, block);
            for (int channel = 0; channel < 2; ++channel)
                for (int sample = 0; sample < block; ++sample)
                    check(a[channel][sample] == b[channel][sample],
                          "streaming changed source interpolation, automation or reconstructed filter state");
        }
        check(actual->starvationFrames() == 0, "offline preparation starved");
    }
}
void liveFade() {
    auto control = std::make_shared<Control>();
    control->allowed.store(false);
    control->blockFrom = 65536;
    auto audio = std::make_shared<AudioData>();
    audio->stream = std::make_shared<Source>(control, 100000);
    Project project;
    project.perClip = true;
    project.clips.resize(1);
    project.clips[0].audio = audio;
    project.clips[0].length = 100000;
    project.clips[0].cue = true;
    project.clips[0].automation[0] = {1, true, {{0, 1}}};
    auto engine = std::make_unique<Engine>(project);
    engine->prepareSources(65535);
    std::array<std::vector<float>, 4> output;
    for (auto &channel : output)
        channel.resize(65535);
    float *out[]{output[0].data(), output[1].data(), output[2].data(), output[3].data()};
    engine->process(out, 4, 65535);
    check(engine->starvationFrames() == 0, "prefetched playback starved early");
    const auto held = output[0].back();
    eventually([&] { return control->blocked.load(); });
    engine->process(out, 4, rampFrames);
    check(engine->starvationFrames() == rampFrames && output[0][rampFrames - 1] == 0,
          "live starvation did not count/fade to silence");
    check(std::abs(output[0][0] - held * float(rampFrames - 1) / rampFrames) < 1e-7f &&
              output[2][0] == output[0][0] * 0.5f,
          "master/cue starvation fade has a discontinuity or wrong gain");
    control->allowed.store(true);
    engine->prepareSources(rampFrames);
    const auto resumedAt = engine->position();
    engine->process(out, 4, rampFrames);
    check(std::abs(output[0][0] - expected(resumedAt, 0.1f) / rampFrames) < 1e-7f &&
              output[0][rampFrames - 1] == expected(resumedAt + rampFrames - 1, 0.1f) &&
              engine->starvationFrames() == rampFrames,
          "live recovery did not crossfade smoothly back to source audio");
}
void failures() {
    for (bool nonFinite : {false, true}) {
        auto control = std::make_shared<Control>();
        control->fail = !nonFinite;
        control->nonFinite = nonFinite;
        StreamBank bank({std::make_shared<Source>(control, 48000)});
        bool failed = false;
        try {
            bank.prepare(0, 0, 128);
        } catch (const std::runtime_error &) {
            failed = true;
        }
        check(failed && bank.stats().failedSources == 1 &&
                  bank.error(0) == (nonFinite ? SourceError::nonFinite : SourceError::read),
              "decode failure or non-finite PCM was accepted as complete audio");
    }
}
} // namespace
int main() {
    try {
        paging();
        slowAndCancelled();
        staleSeek();
        failures();
        engineParity();
        liveFade();
        std::cout << "PASS: bounded PCM pages, boundary/seek alignment, blocked-read nonblocking access, "
                     "offline timeout/failure and generation cancellation\n";
    } catch (const std::exception &error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
