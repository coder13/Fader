// SPDX-License-Identifier: AGPL-3.0-or-later
#include <array>
#include <fader/Playback.h>
#include <limits>
#include <stdexcept>
#include <thread>

namespace {
using namespace fader;
void check(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
Project fixture() {
    Project project;
    project.perClip = true;
    project.clips.resize(1);
    auto pcm = std::make_shared<AudioData>();
    for (auto &channel : pcm->samples)
        channel.assign(48000, 0.1f);
    project.clips[0].audio = pcm;
    project.clips[0].length = 48000;
    return project;
}
constexpr HostTime origin = 1000000000;
HostTime timestamp(Frame frame) {
    return origin + HostTime((frame * 1000000000 + projectRate / 2) / projectRate);
}
void process(Playback &playback, int frames, Frame elapsed) {
    std::array<float, 512> left{}, right{};
    float *out[]{left.data(), right.data()};
    playback.process(out, 2, frames, timestamp(elapsed));
}
std::vector<Event> drain(Playback &playback) {
    std::vector<Event> events;
    Event event;
    while (playback.captured(event))
        events.push_back(event);
    return events;
}
} // namespace

void testPlayback() {
    using namespace fader;
    for (int block : {1, 64, 128, 256}) {
        auto playback = std::make_unique<Playback>(fixture(), 4800, 11);
        playback->arm(4800, 48000);
        playback->deviceStarted(projectRate);
        auto send = [&](InputSource source, Frame frame, float value) {
            check(playback->submit(source, {11, timestamp(frame), Param::volume, value, 0}), "submit failed");
        };
        send(InputSource::ui, 128, 0.2f); // future input must not hide a later-enqueued earlier timestamp
        send(InputSource::ui, 0, 0.1f);
        send(InputSource::controller, 127, 0.3f);
        send(InputSource::controller, 127, 0.4f);
        send(InputSource::ui, 127, 0.5f);
        send(InputSource::ui, 256, 0.6f);
        for (Frame elapsed = 0; elapsed < 512; elapsed += block)
            process(*playback, block, elapsed);
        const auto events = drain(*playback);
        const std::array<Frame, 6> frames{4800, 4927, 4927, 4927, 4928, 5056};
        const std::array<float, 6> values{0.1f, 0.5f, 0.3f, 0.4f, 0.2f, 0.6f};
        check(events.size() == frames.size(), "events missing at block boundary");
        for (std::size_t i = 0; i < events.size(); ++i)
            check(events[i].frame == frames[i] && events[i].value == values[i],
                  "engine/project mapping or same-frame order changed across blocks");
        const auto stats = playback->stats();
        check(stats.engineFrame == 512 && stats.projectFrame == 5312 && stats.segment == 11,
              "engine clock confused with nonzero project position");
        check(stats.lateInputs == 0 && !playback->captureFailed(), "on-time input marked incomplete");
    }
    auto shortProject = fixture();
    shortProject.clips[0].length = 100;
    auto ended = std::make_unique<Playback>(shortProject, 0, 10);
    ended->deviceStarted(projectRate);
    process(*ended, 128, 0);
    process(*ended, 128, 128);
    check(ended->stats().engineFrame == 256 && ended->position() == 100,
          "project-end clamping incorrectly stopped the engine frame clock");

    auto restarted = std::make_unique<Playback>(fixture(), 12000, 12);
    restarted->arm(12000, 48000);
    restarted->deviceStarted(projectRate);
    check(!restarted->submit(InputSource::controller, {11, timestamp(0), Param::volume, 0.1f, 0}),
          "stale segment input applied after restart");
    check(!restarted->captureFailed() && restarted->stats().staleInputs == 1,
          "stale input incorrectly invalidated new take");
    process(*restarted, 128, 0);
    check(restarted->submit(InputSource::controller, {12, timestamp(5), Param::volume, 0.3f, 0}),
          "late input was not accepted for explicit late clamping");
    process(*restarted, 128, 128);
    auto events = drain(*restarted);
    check(events.size() == 1 && events[0].frame == 12128 && restarted->stats().lateInputs == 1 &&
              restarted->stats().maxLateFrames == 123,
          "late input was not captured at the actual applied project frame");

    auto pressure = std::make_unique<Playback>(fixture(), 0, 20);
    pressure->arm(0, 48000);
    pressure->deviceStarted(projectRate);
    for (int i = 0; i < 4096; ++i)
        check(pressure->submit(InputSource::ui, {20, timestamp(0), Param::volume, 0.2f, 0}),
              "early overflow");
    check(!pressure->submit(InputSource::ui, {20, timestamp(0), Param::volume, 0.9f, 0}), "input queue grew");
    process(*pressure, 128, 0);
    check((pressure->stats().failures & inputOverflow) && drain(*pressure).size() == 4096,
          "input loss was hidden or valid queued data was discarded");

    auto capture = std::make_unique<Playback>(fixture(), 0, 21);
    capture->arm(0, 48000);
    capture->deviceStarted(projectRate);
    for (int batch = 0; batch < 5; ++batch) {
        for (int i = 0; i < 4096; ++i)
            check(capture->submit(InputSource::controller,
                                  {21, timestamp(batch * 128), Param::volume, 0.4f, 0}),
                  "input overflow");
        process(*capture, 128, batch * 128);
    }
    check((capture->stats().failures & captureOverflow) && drain(*capture).size() == 16384,
          "capture overflow lost valid prefix or was hidden");

    auto scheduled = std::make_unique<Playback>(fixture(), 0, 22);
    scheduled->deviceStarted(projectRate);
    for (int batch = 0; batch < 2; ++batch) {
        for (int i = 0; i < 4096; ++i)
            check(scheduled->submit(InputSource::ui, {22, timestamp(48000), Param::volume, 0.1f, 0}),
                  "input queue did not drain");
        process(*scheduled, 128, batch * 128);
    }
    check((scheduled->stats().failures & schedulerOverflow) && scheduled->stats().inputDrops == 4096,
          "future scheduler overflow was not bounded/reported");
    check(scheduled->submit(InputSource::controller,
                            {22, std::numeric_limits<HostTime>::max(), Param::volume, 0.1f, 0}),
          "timestamp validation should occur against callback clock");
    process(*scheduled, 128, 256);
    check(scheduled->stats().failures & invalidInput, "host timestamp overflow accepted");

    // Two actual producers feed separate lanes while the consumer runs without
    // any UI timer. The session remains alive until both producers have joined.
    auto concurrent = std::make_unique<Playback>(fixture(), 0, 30);
    concurrent->arm(0, 48000);
    concurrent->deviceStarted(projectRate);
    std::atomic<int> done{0};
    auto producer = [&](InputSource source) {
        for (int i = 0; i < 1000; ++i)
            concurrent->submit(source, {30, timestamp(0), Param::volume, 0.5f, 0});
        done.fetch_add(1);
    };
    std::thread ui(producer, InputSource::ui), controller(producer, InputSource::controller);
    Frame elapsed = 0;
    while (done.load() != 2 && elapsed < 10000) {
        process(*concurrent, 1, elapsed++);
        std::this_thread::yield();
    }
    ui.join();
    controller.join();
    process(*concurrent, 128, elapsed);
    check(drain(*concurrent).size() == 2000 && !concurrent->captureFailed(),
          "producer concurrency required UI timing or lost controls");

    auto interrupted = std::make_unique<Playback>(fixture(), 0, 40);
    interrupted->arm(0, 48000);
    interrupted->deviceStarted(projectRate);
    interrupted->submit(InputSource::ui, {40, timestamp(0), Param::volume, 0.3f, 0});
    process(*interrupted, 128, 0);
    interrupted->deviceStopped();
    interrupted->deviceStarted(44100);
    process(*interrupted, 128, 128);
    check(interrupted->clockFailed() && interrupted->position() == 128 && drain(*interrupted).size() == 1,
          "clock change silently resumed or discarded applied capture");
    auto reset = std::make_unique<Playback>(fixture(), 0, 41);
    reset->deviceStarted(projectRate);
    reset->deviceStarted(projectRate);
    check(reset->clockFailed(), "same-rate device restart was not a new clock segment");

    auto telemetry = std::make_unique<Playback>(fixture(), 0, 50);
    telemetry->deviceStarted(projectRate);
    for (int i = 0; i < 1100; ++i)
        process(*telemetry, 1, i);
    check(telemetry->stats().metricDrops == 76 && !telemetry->captureFailed(),
          "telemetry pressure grew its queue or invalidated musical capture");
    CallbackMetric metric;
    std::size_t count = 0;
    while (telemetry->metric(metric)) {
        check(metric.segment == 50, "wrong metric segment");
        ++count;
    }
    check(count == 1024, "telemetry capacity changed");
}
