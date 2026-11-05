// SPDX-License-Identifier: AGPL-3.0-or-later
#include <algorithm>
#include <array>
#include <cmath>
#include <fader/Engine.h>
#include <stdexcept>

namespace {
using namespace fader;
void near(float actual, float expected, const char *message) {
    if (!std::isfinite(actual) || std::abs(actual - expected) > 2e-6f)
        throw std::runtime_error(message);
}
Clip constant(float left, float right, float gain) {
    Clip clip;
    auto audio = std::make_shared<AudioData>();
    audio->samples[0].assign(1024, left);
    audio->samples[1].assign(1024, right);
    clip.audio = audio;
    clip.length = 1024;
    clip.automation[0] = {gain, true, {{0, gain}}};
    return clip;
}
using Block = std::array<std::array<float, 128>, 6>;
void process(Engine &engine, Block &out, int channels = 6) {
    for (auto &channel : out)
        channel.fill(123.0f);
    float *pointers[]{out[0].data(), out[1].data(), out[2].data(),
                      out[3].data(), out[4].data(), out[5].data()};
    engine.process(pointers, channels, 128);
}
} // namespace

void testMixer() {
    Project project;
    project.perClip = true;
    project.clips = {constant(0.2f, -0.1f, 0.25f), constant(0.4f, 0.3f, 0.5f)};
    project.clips[0].cue = true;
    Engine mixed(project);
    Block out{};
    process(mixed, out);
    for (int i = 0; i < 128; ++i) {
        near(out[0][i], 0.25f, "independent left gains did not sum");
        near(out[1][i], 0.125f, "independent right gains did not sum");
        near(out[2][i], 0.1f, "cue gain follows master volume");
        near(out[3][i], -0.05f, "cue stereo channels changed");
        near(out[4][i], 0, "unused output was not cleared");
        near(out[5][i], 0, "unused output was not cleared");
    }
    near(mixed.peak(), 0.25f, "meter does not report master peak");

    auto muted = project;
    for (auto &clip : muted.clips) {
        clip.cue = true;
        clip.automation[0] = {0, true, {{0, 0}}};
        clip.automation[1] = {1, true, {{0, 1}}};
        clip.automation[2] = {0, true, {{0, 0}}};
    }
    Engine monitor(muted);
    process(monitor, out);
    for (int i = 0; i < 128; ++i) {
        near(out[0][i], 0, "muted master is audible");
        near(out[1][i], 0, "muted master is audible");
        near(out[2][i], 0.3f, "cue is not a dry pre-filter sum");
        near(out[3][i], 0.1f, "cue is not a dry pre-filter sum");
    }
    near(monitor.peak(), 0, "cue leaked into master meter");

    auto loud = project;
    loud.clips = {constant(0.75f, -0.75f, 1), constant(0.75f, -0.75f, 1)};
    Engine unclipped(loud);
    process(unclipped, out);
    near(out[0][0], 1.5f, "master silently clips or limits floating-point mix");
    near(out[1][0], -1.5f, "negative master silently clips");
    near(unclipped.peak(), 1.5f, "overload was not metered");

    // Known affine signal: the declared linear interpolator must preserve the
    // slope at fractional source positions, and hold the final source sample.
    for (double sourceRate : {8000.0, 44100.0, 48000.0, 96000.0}) {
        for (double rate : {0.5, 1.0, 2.0}) {
            auto audio = std::make_shared<AudioData>();
            audio->sampleRate = sourceRate;
            for (int i = 0; i < 300; ++i) {
                audio->samples[0].push_back(float(i) / 1024);
                audio->samples[1].push_back(-float(i) / 2048);
            }
            Project ramp;
            ramp.perClip = true;
            ramp.clips = {constant(0, 0, 1)};
            auto &clip = ramp.clips[0];
            clip.audio = audio;
            clip.start = 7;
            clip.sourceOffset = 13;
            clip.rate = rate;
            clip.length = Frame(std::floor(287 * double(projectRate) / sourceRate / rate));
            Engine engine(ramp);
            Frame frame = 0;
            while (frame < ramp.end() + 128) {
                process(engine, out);
                for (int i = 0; i < 128; ++i, ++frame) {
                    const bool active = frame >= clip.start && frame < ramp.end();
                    const auto position = 13 + double(frame - clip.start) * sourceRate / projectRate * rate;
                    const auto expected = active ? float(std::min(position, 299.0)) / 1024 : 0.0f;
                    near(out[0][i], expected, "source-rate interpolation, offset or duration differs");
                    near(out[1][i], -expected * 0.5f, "resampling changed stereo separation");
                }
            }
            if (engine.position() != ramp.end())
                throw std::runtime_error("transport did not stop at the set end");
        }
    }

    // Legacy projects keep their equal-power crossfader and dry cue routing.
    for (float crossfade : {-1.0f, 0.0f, 1.0f}) {
        auto legacy = project;
        legacy.perClip = false;
        legacy.curves[0] = {0.25f, false, {}};
        legacy.curves[1] = {0.5f, false, {}};
        legacy.curves[2] = {crossfade, false, {}};
        Engine engine(legacy);
        process(engine, out);
        const float a = crossfade < 0 ? 1 : crossfade > 0 ? 0 : std::sqrt(0.5f);
        const float b = crossfade > 0 ? 1 : crossfade < 0 ? 0 : std::sqrt(0.5f);
        near(out[0][0], 0.05f * a + 0.2f * b, "legacy crossfader law changed");
        near(out[1][0], -0.025f * a + 0.15f * b, "legacy stereo mix changed");
        near(out[2][0], 0.1f, "legacy cue follows crossfader");
    }
}
