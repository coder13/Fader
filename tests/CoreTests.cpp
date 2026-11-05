// SPDX-License-Identifier: AGPL-3.0-or-later
#include <algorithm>
#include <cmath>
#include <fader/Engine.h>
#include <iostream>
#include <stdexcept>

using namespace fader;
void require(bool condition, const char *message) {
    if (!condition)
        throw std::runtime_error(message);
}
Project fixture() {
    Project p;
    for (unsigned deck = 0; deck < 2; ++deck) {
        auto audio = std::make_shared<AudioData>();
        for (unsigned c = 0; c < 2; ++c) {
            audio->samples[c].resize(12000);
            for (int i = 0; i < 12000; ++i)
                audio->samples[c][static_cast<std::size_t>(i)] =
                    0.2f * std::sin(float(i) * (deck ? 0.07f : 0.031f));
        }
        p.clips[deck].audio = audio;
        p.clips[deck].length = 12000;
    }
    p.curves[0].points = {{128, 0.1f}, {769, 0.9f}, {4000, 0.3f}};
    p.curves[2].points = {{0, -1}, {351, 0}, {8000, 1}};
    return p;
}
std::vector<float> render(Engine &engine, int block, Frame end) {
    std::vector<float> result;
    std::vector<float> left(static_cast<std::size_t>(block)), right(left.size());
    float *channels[]{left.data(), right.data()};
    while (engine.position() < end) {
        const int n = static_cast<int>(std::min(Frame{block}, end - engine.position()));
        engine.process(channels, 2, n);
        result.insert(result.end(), left.begin(), left.begin() + n);
    }
    return result;
}
int main() {
    try {
        const auto project = fixture();
        Engine reference(project);
        const auto baseline = render(reference, 64, project.end());
        for (int block : {1, 127, 128, 256, 512, 1024}) {
            Engine e(project);
            require(render(e, block, project.end()) == baseline, "block-dependent render");
        }
        Engine seek(project, 5311);
        auto suffix = render(seek, 128, project.end());
        require(std::equal(suffix.begin(), suffix.end(), baseline.begin() + 5311),
                "seek did not reconstruct smoother");
        auto p = fixture();
        p.curves[0].points.clear();
        Engine live(p);
        live.arm(100, 9000);
        require(live.command({128, Param::gainA, 0.1f}), "queue push");
        require(live.command({769, Param::gainA, 0.9f}), "queue push");
        auto actual = render(live, 256, 9000);
        std::vector<Event> capture;
        Event event;
        while (live.captured(event))
            capture.push_back(event);
        require(capture.size() == 2 && capture[0].frame == 128 && capture[1].frame == 769,
                "capture timestamps wrong");
        require(!live.captureFailed(), "unexpected capture overflow");
        commitTake(p, capture, 100, 9000);
        Engine replay(p);
        require(render(replay, 127, 9000) == actual, "captured target replay differs");
        require(p.curves[0].at(127) == 0.7f, "target interpolated before its timestamp");
        auto unchanged = p.curves[1].points;
        commitTake(p, {{4000, Param::gainA, 0.2f}}, 3900, 5000);
        require(p.curves[1].points == unchanged && p.curves[0].at(128) == 0.1f,
                "retake changed outside region");
        auto boundary = fixture();
        boundary.curves[0].points = {{128, 0.1f}, {4980, 0.6f}, {5100, 0.3f}};
        Engine performing(boundary, 4000);
        performing.arm(4000, 5000);
        performing.command({4001, Param::gainA, 0.8f});
        performing.command({4500, Param::gainA, 0.4f});
        const auto performed = render(performing, 128, boundary.end());
        std::vector<Event> retake;
        while (performing.captured(event))
            retake.push_back(event);
        commitTake(boundary, retake, 4000, 5000);
        Engine boundaryReplay(boundary, 4000);
        require(render(boundaryReplay, 127, boundary.end()) == performed, "retake punch-out replay differs");
        require(boundary.curves[0].at(5100) == 0.3f, "retake erased a following target");
        auto atomic = fixture();
        atomic.curves[1].linear = true;
        const auto before = atomic.curves[0].points;
        bool failed = false;
        try {
            commitTake(atomic, {{10, Param::gainA, 0.1f}, {10, Param::gainB, 0.1f}}, 0, 20);
        } catch (...) {
            failed = true;
        }
        require(failed && atomic.curves[0].points == before, "failed take partially changed project");
        Engine pressure(project);
        pressure.arm(0, project.end());
        float a{}, b{};
        float *output[]{&a, &b};
        for (int batch = 0; batch < 5; ++batch) {
            for (int i = 0; i < 4096; ++i)
                require(pressure.command({0, Param::gainA, 0.2f}), "unexpected input overflow");
            pressure.process(output, 2, 1);
        }
        require(pressure.captureFailed() && pressure.lateEvents() > 0, "capture pressure was not reported");
        Queue<int, 2> queue;
        int value = 0;
        require(queue.push(1) && queue.push(2) && !queue.push(3), "queue overflow not bounded");
        require(queue.pop(value) && value == 1 && queue.push(3), "queue ordering");
        auto bad = project;
        bad.curves[0].points = {{1, 0.1f}, {1, 0.2f}};
        bool rejected = false;
        try {
            bad.validate();
        } catch (...) {
            rejected = true;
        }
        require(rejected, "duplicate timestamps accepted");
        auto cueProject = fixture();
        cueProject.clips[0].cue = true;
        Engine cue(cueProject), noCue(project);
        require(render(cue, 128, 12000) == render(noCue, 128, 12000), "cue leaked to master");
        std::cout << "PASS: block independence, exact seek state, applied-event capture/replay, retake, "
                     "bounded queue, validation, cue isolation\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
