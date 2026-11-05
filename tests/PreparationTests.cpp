// SPDX-License-Identifier: AGPL-3.0-or-later
#include <chrono>
#include <fader/PlaybackPreparation.h>
#include <stdexcept>
#include <thread>

namespace {
using namespace fader;
using Clock = std::chrono::steady_clock;
void check(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
template <typename Predicate> void until(Predicate ready) {
    const auto deadline = Clock::now() + std::chrono::seconds(5);
    while (!ready()) {
        check(Clock::now() < deadline, "Preparation worker did not reach the expected state");
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
std::shared_ptr<const Project> resident(const char *name) {
    auto p = std::make_shared<Project>();
    p->perClip = true;
    p->clips.resize(1);
    auto audio = std::make_shared<AudioData>();
    for (auto &channel : audio->samples)
        channel.assign(48000, 0.1f);
    p->clips[0].audio = audio;
    p->clips[0].length = 48000;
    p->clips[0].name = name;
    return p;
}
struct Gate {
    std::atomic<bool> entered{false}, released{false};
    void block() {
        entered.store(true);
        // Failure paths must not hang the test process during stack unwinding.
        const auto deadline = Clock::now() + std::chrono::seconds(5);
        while (!released.load() && Clock::now() < deadline)
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
};
void latestAndClose() {
    Gate gate;
    std::atomic<unsigned> builds{0};
    PlaybackPreparation service(
        [&](const Project &p, Frame at, std::uint64_t segment, std::shared_ptr<std::atomic<bool>>) {
            if (++builds == 1)
                gate.block();
            // Deliberately return a stale success despite cancellation. Publication
            // must be protected even if a decoder/factory does not cooperate.
            return std::make_unique<Playback>(p, at, segment);
        });
    auto oldProject = resident("old source");
    service.request(oldProject, 0, 1);
    until([&] { return gate.entered.load(); });
    const auto begin = Clock::now();
    for (unsigned i = 0; i < 1000; ++i)
        service.request(oldProject, i, i + 2);
    service.cancel();
    auto latest = resident("replacement source");
    const auto generation = service.request(latest, 12345, 1002);
    check(Clock::now() - begin < std::chrono::seconds(1), "Rapid seeks blocked on decoder work");
    check(!service.poll(), "Stale preparation was published before it finished");
    gate.released.store(true);
    std::optional<PlaybackPreparation::Result> ready;
    until([&] {
        ready = service.poll();
        return ready.has_value();
    });
    check(builds == 2 && ready->generation == generation && ready->playback &&
              ready->playback->position() == 12345 &&
              ready->playback->plan().clips[0].name == "replacement source",
          "Rapid seeks did not coalesce to the latest project and position");
    ready.reset(); // end the borrow before invalidating
    service.cancel();
    check(!service.poll(), "Cancel left a completion available");

    Gate closing;
    auto closed = std::make_unique<PlaybackPreparation>(
        [&](const Project &, Frame, std::uint64_t,
            std::shared_ptr<std::atomic<bool>>) -> std::unique_ptr<Playback> {
            closing.block();
            throw std::runtime_error("late failure after close");
        });
    closed->request(latest, 0, 1);
    until([&] { return closing.entered.load(); });
    const auto closeAt = Clock::now();
    closed->close();
    check(Clock::now() - closeAt < std::chrono::seconds(1) && !closed->poll(),
          "Close waited for or published a stale job");
    closing.released.store(true);
    closed.reset(); // owner joins only after its UI client is gone
}
struct RetiringDecoder final : SourceDecoder {
    explicit RetiringDecoder(std::shared_ptr<Gate> gate) : gate(std::move(gate)) {}
    ~RetiringDecoder() override {
        gate->block();
    }
    bool read(SourceFrame, int count, float *const *out, const std::atomic<bool> &cancelled) override {
        for (int c = 0; c < 2; ++c)
            std::fill_n(out[c], count, 0.1f);
        return !cancelled.load();
    }
    std::shared_ptr<Gate> gate;
};
struct RetiringSource final : StreamSource {
    explicit RetiringSource(std::shared_ptr<Gate> gate) : gate(std::move(gate)) {}
    SourceFrame frames() const noexcept override {
        return 48000;
    }
    std::unique_ptr<SourceDecoder> open() const override {
        return std::make_unique<RetiringDecoder>(gate);
    }
    std::shared_ptr<Gate> gate;
};
void retirement() {
    auto gate = std::make_shared<Gate>();
    auto p = std::make_shared<Project>();
    p->perClip = true;
    p->clips.resize(1);
    auto audio = std::make_shared<AudioData>();
    audio->stream = std::make_shared<RetiringSource>(gate);
    p->clips[0].audio = audio;
    p->clips[0].length = 48000;
    PlaybackPreparation service;
    service.request(p, 0, 1);
    std::optional<PlaybackPreparation::Result> ready;
    until([&] {
        ready = service.poll();
        return ready.has_value();
    });
    check(ready->playback != nullptr, "Initial streamed preparation failed");
    ready.reset();
    service.cancel();
    until([&] { return gate->entered.load(); });
    const auto begin = Clock::now();
    const auto id = service.request(resident("new project"), 100, 2);
    check(Clock::now() - begin < std::chrono::seconds(1) && !service.poll(),
          "Retiring source blocked a new UI request or admitted a second engine");
    gate->released.store(true);
    until([&] {
        ready = service.poll();
        return ready.has_value();
    });
    check(ready->generation == id && ready->playback && ready->playback->position() == 100,
          "Newest request did not complete after old readers retired");
    ready.reset();
    service.cancel();
}
} // namespace
void testPreparationService() {
    latestAndClose();
    retirement();
}
