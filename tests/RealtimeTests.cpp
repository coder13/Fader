// SPDX-License-Identifier: AGPL-3.0-or-later
#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <fader/Engine.h>
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

int main() {
    using namespace fader;
    Project project;
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
    if (heapCalls || sum == 0) {
        std::cerr << "FAIL: ordinary heap use or silent timing fixture\n";
        return 1;
    }
    std::sort(times.begin(), times.end());
    std::cout << "PASS: 20000 core process calls, no ordinary new/delete; two sources + automation + cue\n"
              << "microseconds p50=" << times[10000] << " p95=" << times[19000] << " p99.9=" << times[19980]
              << " max=" << times.back() << '\n'
              << "Synthetic offline timing only; not device latency or sustained audio qualification.\n";
}
