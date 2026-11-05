// SPDX-License-Identifier: AGPL-3.0-or-later
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <fader/Engine.h>
#include <fader/Playback.h>
#ifdef _WIN32
#include <malloc.h>
#endif
#include <iostream>
#include <new>

namespace {
thread_local bool checking = false;
thread_local unsigned heapCalls = 0;
} // namespace
void *operator new(std::size_t size) {
    if (checking)
        ++heapCalls;
    if (auto *p = std::malloc(std::max(size, std::size_t{1})))
        return p;
    throw std::bad_alloc();
}
void *operator new[](std::size_t size) {
    return ::operator new(size);
}
void operator delete(void *p) noexcept {
    if (checking)
        ++heapCalls;
    std::free(p);
}
void operator delete[](void *p) noexcept {
    ::operator delete(p);
}
void operator delete(void *p, std::size_t) noexcept {
    ::operator delete(p);
}
void operator delete[](void *p, std::size_t) noexcept {
    ::operator delete(p);
}

void *operator new(std::size_t size, std::align_val_t alignment) {
    if (checking)
        ++heapCalls;
    void *p = nullptr;
#ifdef _WIN32
    p = _aligned_malloc(std::max(size, std::size_t{1}), static_cast<std::size_t>(alignment));
#else
    if (posix_memalign(&p, static_cast<std::size_t>(alignment), std::max(size, std::size_t{1})) != 0)
        p = nullptr;
#endif
    if (p)
        return p;
    throw std::bad_alloc();
}
void *operator new[](std::size_t size, std::align_val_t alignment) {
    return ::operator new(size, alignment);
}
void operator delete(void *p, std::align_val_t) noexcept {
    if (checking)
        ++heapCalls;
#ifdef _WIN32
    _aligned_free(p);
#else
    std::free(p);
#endif
}
void operator delete[](void *p, std::align_val_t alignment) noexcept {
    ::operator delete(p, alignment);
}
void operator delete(void *p, std::size_t, std::align_val_t alignment) noexcept {
    ::operator delete(p, alignment);
}
void operator delete[](void *p, std::size_t, std::align_val_t alignment) noexcept {
    ::operator delete(p, alignment);
}

namespace {
class ConstantDecoder final : public fader::SourceDecoder {
  public:
    bool read(fader::SourceFrame, int frames, float *const *stereo,
              const std::atomic<bool> &cancelled) override {
        if (cancelled.load())
            return false;
        for (int channel = 0; channel < 2; ++channel)
            std::fill_n(stereo[channel], frames, 0.1f);
        return true;
    }
};
class ConstantSource final : public fader::StreamSource {
  public:
    fader::SourceFrame frames() const noexcept override {
        return 3000000;
    }
    std::unique_ptr<fader::SourceDecoder> open() const override {
        return std::make_unique<ConstantDecoder>();
    }
};
} // namespace

int main() {
    using namespace fader;
    checking = true;
    auto *ordinary = ::operator new(8);
    ::operator delete(ordinary);
    auto *aligned = ::operator new(64, std::align_val_t{64});
    ::operator delete(aligned, std::align_val_t{64});
    checking = false;
    if (heapCalls != 4) {
        std::cerr << "FAIL: allocation instrumentation self-check\n";
        return 1;
    }
    heapCalls = 0;
    Project project;
    project.perClip = true;
    auto audio = std::make_shared<AudioData>();
    for (auto &channel : audio->samples) {
        channel.resize(48000);
        for (std::size_t i = 0; i < channel.size(); ++i)
            channel[i] = 0.1f * std::sin(float(i) * 0.03f);
    }
    for (auto &clip : project.clips) {
        clip.audio = audio;
        clip.length = 48000;
        clip.cue = true;
        for (auto &curve : clip.automation) {
            curve.points.clear();
            for (int i = 0; i < 400; ++i)
                curve.points.push_back({i * 120, float(i % 10) / 10});
        }
    }
    for (auto &curve : project.curves)
        for (int i = 0; i < 400; ++i)
            curve.points.push_back({i * 120, float(i % 10) / 10});
    std::array<std::array<float, 128>, 4> output{};
    float *pointers[]{output[0].data(), output[1].data(), output[2].data(), output[3].data()};
    std::vector<double> times;
    times.reserve(20000);
    double sum = 0;
    for (int batch = 0; batch < 200; ++batch) {
        Engine engine(project);
        engine.arm(0, 48000);
        for (int block = 0; block < 100; ++block) {
            engine.command({engine.position() + 17, Param::gainA, float(block % 10) / 10});
            const auto start = std::chrono::steady_clock::now();
            checking = true;
            engine.process(pointers, 4, 128);
            checking = false;
            const auto elapsed = std::chrono::steady_clock::now() - start;
            times.push_back(std::chrono::duration<double, std::micro>(elapsed).count());
            Event event;
            while (engine.captured(event)) {
            }
            sum += std::abs(output[0][50]);
        }
    }

    unsigned captured = 0;
    for (int batch = 0; batch < 200; ++batch) {
        auto playback = std::make_unique<Playback>(project, 4800, 1);
        playback->arm(4800, 48000);
        playback->deviceStarted(projectRate);
        for (int block = 0; block < 100; ++block) {
            const auto now = HostTime(1000000000) + HostTime(block * 128) * 1000000000 / projectRate;
            playback->submit(InputSource::ui, {1, now, Param::volume, 0.6f, 0});
            playback->submit(InputSource::controller, {1, now + 200000, Param::lowPass, 0.8f, 1});
            checking = true;
            playback->process(pointers, 4, 128, now);
            checking = false;
            Event event;
            while (playback->captured(event))
                ++captured;
            CallbackMetric metric;
            while (playback->metric(metric)) {
            }
            sum += std::abs(output[0][50]);
        }
        if (playback->captureFailed()) {
            std::cerr << "FAIL: playback fixture lost actions\n";
            return 1;
        }
    }
    // Saturated fixed scheduler/capture/telemetry paths must stay allocation-free too.
    auto pressure = std::make_unique<Playback>(project, 0, 2);
    pressure->arm(0, 48000);
    pressure->deviceStarted(projectRate);
    for (int batch = 0; batch < 1100; ++batch) {
        if (batch < 6)
            for (unsigned i = 0; i < 4096; ++i)
                pressure->submit(InputSource::controller, {2, 1000000000, Param::volume, 0.5f, 0});
        checking = true;
        pressure->process(pointers, 4, 1, 1000000000);
        checking = false;
    }
    checking = true;
    pressure->deviceStopped();
    pressure->process(pointers, 4, 128, 1000000000);
    checking = false;
    auto streamingProject = project;
    auto streamingData = std::make_shared<AudioData>();
    streamingData->stream = std::make_shared<ConstantSource>();
    for (auto &clip : streamingProject.clips) {
        clip.audio = streamingData;
        clip.length = 20001 * 128;
    }
    auto streamed = std::make_unique<Engine>(streamingProject);
    for (int block = 0; block < 20000; ++block) {
        streamed->prepareSources(128); // control/offline side, deliberately outside the callback guard
        checking = true;
        streamed->process(pointers, 4, 128);
        checking = false;
        sum += std::abs(output[0][50]);
    }
    if (streamed->starvationFrames() != 0) {
        std::cerr << "FAIL: prepared streaming fixture starved\n";
        return 1;
    }
    streamed->prepareSources(128);
    streamed->cancelSourceReads(); // availability can change after readiness, including the final span
    checking = true;
    streamed->process(pointers, 4, 128); // missing-source concealment also stays allocation-free
    checking = false;
    if (streamed->starvationFrames() != 256 || streamed->position() != streamingProject.end()) {
        std::cerr << "FAIL: source loss after final-span readiness was not reported\n";
        return 1;
    }
    if (heapCalls || sum == 0 || captured != 40000) {
        std::cerr << "FAIL: C++ heap use, missing capture or silent timing fixture\n";
        return 1;
    }
    std::sort(times.begin(), times.end());
    std::cout << "PASS: 20000 Engine + 20000 Playback + 20000 streaming calls, pressure and concealment, no "
                 "ordinary/aligned "
                 "new/delete; two clips + volume/filters + cue\n"
              << "microseconds p50=" << times[10000] << " p95=" << times[19000] << " p99.9=" << times[19980]
              << " max=" << times.back() << '\n'
              << "Synthetic offline timing only; not device latency or sustained audio qualification.\n";
}
