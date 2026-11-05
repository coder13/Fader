// SPDX-License-Identifier: AGPL-3.0-or-later
#include <algorithm>
#include <cmath>
#include <fader/OfflineRender.h>
#include <fader/Playback.h>
#include <future>
#include <iostream>
#include <stdexcept>
#include <thread>

namespace {
using namespace fader;
void check(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}
template <class F> void rejects(F action) {
    try {
        action();
    } catch (const std::exception &) {
        return;
    }
    throw std::runtime_error("Invalid offline render was accepted");
}
struct State {
    std::atomic<unsigned> reads{0};
    std::atomic<bool> blocked{false}, entered{false};
    bool fail = false;
};
struct Decoder final : SourceDecoder {
    explicit Decoder(std::shared_ptr<State> s) : s(std::move(s)) {}
    bool read(SourceFrame first, int frames, float *const *out, const std::atomic<bool> &cancel) override {
        s->entered.store(true);
        while (s->blocked.load() && !cancel.load())
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        if (cancel.load() || s->fail)
            return false;
        ++s->reads;
        for (int i = 0; i < frames; ++i) {
            out[0][i] = 0.15f * std::sin(float((first + i) % 7919) * 0.043f);
            out[1][i] = 0.1f * std::cos(float((first + i) % 8179) * 0.019f);
        }
        return true;
    }
    std::shared_ptr<State> s;
};
struct Source final : StreamSource {
    explicit Source(std::shared_ptr<State> s) : s(std::move(s)) {}
    SourceFrame frames() const noexcept override {
        return 200000;
    }
    std::unique_ptr<SourceDecoder> open() const override {
        return std::make_unique<Decoder>(s);
    }
    std::shared_ptr<State> s;
};
Project fixture(bool stream = false, std::shared_ptr<State> state = std::make_shared<State>()) {
    Project p;
    p.perClip = true;
    for (unsigned clip = 0; clip < 2; ++clip) {
        auto a = std::make_shared<AudioData>();
        a->sampleRate = clip ? 96000 : 44100;
        if (stream)
            a->stream = std::make_shared<Source>(state);
        else {
            for (auto &ch : a->samples)
                ch.resize(200000);
            Decoder reader(state);
            float *out[]{a->samples[0].data(), a->samples[1].data()};
            std::atomic<bool> cancel{false};
            for (int first = 0; first < 200000; first += 8192) {
                float *at[]{out[0] + first, out[1] + first};
                reader.read(first, std::min(8192, 200000 - first), at, cancel);
            }
        }
        auto &c = p.clips[clip];
        c.audio = a;
        c.start = clip ? 379 : 0;
        c.length = 48017 - c.start;
        c.sourceOffset = 123 + clip * 17;
        c.rate = clip ? 0.75 : 1.3;
        c.cue = true;
        c.automation[0] = {0.4f, true, {{0, 0.4f}, {20000, 0.8f}, {40000, 0.2f, true}}};
        c.automation[1] = {0, true, {{0, 0}, {16000, 0.7f}, {30000, 0, true}}};
        c.automation[2] = {1, true, {{0, 1}, {23000, 0.02f}, {45000, 1}}};
    }
    return p;
}
constexpr HostTime origin = 1000000000;
HostTime timeAt(Frame frame) {
    return origin + HostTime((frame * 1000000000 + projectRate / 2) / projectRate);
}
void parity() {
    double maxError = 0;
    const std::vector<Event> events{
        {0, Param::volume, .1f, 0},    {63, Param::highPass, .2f, 0}, {64, Param::lowPass, .7f, 0},
        {127, Param::volume, .8f, 0},  {128, Param::volume, .3f, 0},  {255, Param::highPass, .7f, 0},
        {256, Param::lowPass, .1f, 0}, {380, Param::volume, .6f, 1},  {511, Param::highPass, .1f, 1},
        {512, Param::lowPass, .9f, 1}, {512, Param::volume, .2f, 1},  {513, Param::volume, .7f, 1},
        {20000, Param::volume, .4f, 0}};
    for (bool streamed : {false, true})
        for (int block : {64, 128, 256, 512}) {
            const auto p = fixture(streamed);
            const auto original = p.clips[0].automation[0].points;
            auto live = std::make_unique<Playback>(p, 0, 23);
            live->arm(0, p.end());
            live->deviceStarted(projectRate);
            for (const auto &e : events)
                check(live->submit(InputSource::ui, {23, timeAt(e.frame), e.param, e.value, e.clip}),
                      "Cannot submit live parity event");
            std::array<std::vector<float>, 2> reference;
            for (auto &ch : reference)
                ch.resize(static_cast<std::size_t>(p.end()));
            std::array<std::array<float, 1024>, 4> buffer{};
            float *out[]{buffer[0].data(), buffer[1].data(), buffer[2].data(), buffer[3].data()};
            double cue = 0;
            for (Frame at = 0; at < p.end(); at += block) {
                const int n = int(std::min(Frame(block), p.end() - at));
                live->prepareSources(n);
                live->process(out, 4, n, timeAt(at));
                for (int c = 0; c < 2; ++c)
                    std::copy_n(out[c], n, reference[c].data() + at);
                for (int i = 0; i < n; ++i)
                    cue += std::abs(out[2][i]) + std::abs(out[3][i]);
                CallbackMetric metric;
                while (live->metric(metric)) {
                }
            }
            check(!live->captureFailed() && !live->starvationFrames() && cue > 0,
                  "Live baseline lost events, starved or omitted cue");
            std::vector<Event> applied;
            Event event;
            while (live->captured(event))
                applied.push_back(event);
            check(applied.size() == events.size(), "Applied event count changed");
            for (std::size_t i = 0; i < events.size(); ++i)
                check(applied[i].frame == events[i].frame && applied[i].param == events[i].param &&
                          applied[i].clip == events[i].clip && applied[i].value == events[i].value,
                      "Applied frame/target differs from submission");
            auto compiled = p;
            commitTake(compiled, applied, 0, p.end());
            OfflineRender offline(compiled);
            for (Frame at = 0; at < p.end();) {
                const int n = offline.read(out, block);
                for (int c = 0; c < 2; ++c)
                    for (int i = 0; i < n; ++i)
                        maxError = std::max(maxError, std::abs(double(out[c][i]) - reference[c][at + i]));
                at += n;
            }
            check(offline.read(out, block) == 0 && offline.streamStats().failedSources == 0 &&
                      p.clips[0].automation[0].points == original,
                  "Offline host overran or mutated active plan");
            // Exact nonzero range reconstruction, with no appended tail or alignment shift.
            OfflineRender range(compiled, {19001, 30009});
            Frame at = 19001;
            while (at < 30009) {
                const int n = range.read(out, block);
                for (int c = 0; c < 2; ++c)
                    for (int i = 0; i < n; ++i)
                        maxError = std::max(maxError, std::abs(double(out[c][i]) - reference[c][at + i]));
                at += n;
            }
            check(range.frames() == 11008, "Range duration changed");
        }
    std::cout << "Live/offline master max absolute error=" << maxError
              << " at 64/128/256/512 frames, resident+streamed, applied events+nonzero ranges\n";
    check(maxError <= 1e-6, "Live/offline PCM parity failed");
}
void cancellationAndWait() {
    auto state = std::make_shared<State>();
    state->blocked.store(true);
    auto p = fixture(true, state);
    auto stop = std::make_shared<std::atomic<bool>>(false);
    auto result = std::async(std::launch::async, [=] {
        try {
            OfflineRender render(p, {}, stop);
            return false;
        } catch (const std::exception &) {
            return stop->load();
        }
    });
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (!state->entered.load() && std::chrono::steady_clock::now() < deadline)
        std::this_thread::yield();
    const bool entered = state->entered.load();
    stop->store(true);
    check(entered, "Delayed decoder did not start");
    check(result.wait_for(std::chrono::seconds(1)) == std::future_status::ready && result.get(),
          "Offline cancellation failed while waiting for source");
    state = std::make_shared<State>();
    state->blocked.store(true);
    p = fixture(true, state);
    auto ready = std::async(std::launch::async, [p] {
        OfflineRender render(p);
        std::array<float, 128> l{}, r{};
        float *out[]{l.data(), r.data()};
        return render.read(out, 128) == 128 && render.streamStats().failedSources == 0;
    });
    const auto readyDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(3);
    while (!state->entered.load() && std::chrono::steady_clock::now() < readyDeadline)
        std::this_thread::yield();
    const bool waited = ready.wait_for(std::chrono::milliseconds(20)) == std::future_status::timeout;
    state->blocked.store(false);
    check(state->entered.load() && waited, "Offline host skipped delayed readiness");
    check(ready.get(), "Delayed readiness did not produce complete audio");
    state = std::make_shared<State>();
    state->fail = true;
    p = fixture(true, state);
    rejects([&] { OfflineRender render(p); });
    p = fixture();
    rejects([&] { OfflineRender render(p, {-1, 100}); });
    rejects([&] { OfflineRender render(p, {0, p.end() + 1}); });
    OfflineRender bounded(p);
    std::array<float, 1024> left{}, right{};
    float *output[]{left.data(), right.data()};
    rejects([&] { bounded.read(output, 1025); });
    rejects([&] { bounded.read(output, 0); });
    rejects([&] { bounded.read(nullptr, 128); });
}
} // namespace
int main() {
    try {
        parity();
        cancellationAndWait();
        std::cout << "PASS: offline applied-event/range parity, readiness wait, cancellation, source failure "
                     "and bounded blocks\n";
    } catch (const std::exception &e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
