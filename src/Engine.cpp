// SPDX-License-Identifier: AGPL-3.0-or-later
#include <algorithm>
#include <cmath>
#include <fader/Engine.h>
#include <limits>
#include <stdexcept>

namespace fader {
float Curve::at(Frame frame) const noexcept {
    auto next = std::upper_bound(points.begin(), points.end(), frame,
                                 [](Frame f, const Point &p) { return f < p.frame; });
    if (next == points.begin())
        return initial;
    const auto &previous = *std::prev(next);
    if (!linear || previous.hold || next == points.end())
        return previous.value;
    const auto fraction =
        static_cast<double>(frame - previous.frame) / static_cast<double>(next->frame - previous.frame);
    return previous.value + static_cast<float>(fraction) * (next->value - previous.value);
}
Frame Project::end() const noexcept {
    Frame result = 0;
    for (const auto &c : clips)
        result = std::max(result, c.start + c.length);
    return result;
}
void Project::validate() const {
    constexpr Frame maxTime = 2 * 3600 * projectRate;
    if (clips.size() > 64 || (!perClip && clips.size() != 2))
        throw std::runtime_error("This prototype supports up to 64 clips");
    for (const auto &clip : clips) {
        if (clip.start < 0 || clip.length < 0 || clip.start > maxTime || clip.length > maxTime - clip.start ||
            clip.sourceOffset < 0 || !std::isfinite(clip.rate) || clip.rate < 0.25 || clip.rate > 4.0 ||
            !std::isfinite(clip.bpm) || (clip.bpm != 0 && clip.bpm < 20) || clip.bpm > 400 ||
            !std::isfinite(clip.firstBeatSeconds) || clip.firstBeatSeconds < 0 || clip.row < 0 ||
            clip.row > 4095)
            throw std::runtime_error("Invalid clip timing or playback rate");
        if (clip.audio && (clip.audio->samples[0].size() != clip.audio->samples[1].size() ||
                           !std::isfinite(clip.audio->sampleRate) || clip.audio->sampleRate < 8000 ||
                           clip.audio->sampleRate > 384000))
            throw std::runtime_error("Invalid source audio");
        if (perClip)
            for (const auto &curve : clip.automation) {
                if (!std::isfinite(curve.initial) || curve.initial < 0 || curve.initial > 1 ||
                    curve.points.size() > 200000)
                    throw std::runtime_error("Invalid clip automation");
                Frame previous = -1;
                for (const auto &p : curve.points) {
                    if (p.frame <= previous || p.frame > maxTime || !std::isfinite(p.value) || p.value < 0 ||
                        p.value > 1)
                        throw std::runtime_error("Invalid clip automation point");
                    previous = p.frame;
                }
            }
    }
    for (std::size_t i = 0; i < curves.size(); ++i) {
        const auto validValue = [i](float v) {
            return std::isfinite(v) && v >= (i == 2 ? -1.0f : 0.0f) && v <= 1.0f;
        };
        if (!validValue(curves[i].initial) || curves[i].points.size() > 200000)
            throw std::runtime_error("Invalid automation lane");
        Frame previous = -1;
        for (const auto &p : curves[i].points) {
            if (p.frame < 0 || p.frame > maxTime || p.frame <= previous || !validValue(p.value))
                throw std::runtime_error("Automation must contain ordered, unique, valid points");
            previous = p.frame;
        }
    }
}
float Engine::Ramp::next(float value) noexcept {
    if (value != target) {
        target = value;
        left = rampFrames;
        increment = (target - current) / static_cast<float>(rampFrames);
    }
    if (left > 0) {
        current += increment;
        if (--left == 0)
            current = target;
    }
    return current;
}
Engine::Engine(Project p, Frame start) : project(std::move(p)) {
    project.validate();
    if (start < 0 || start > project.end())
        throw std::runtime_error("Seek outside set");
    for (std::size_t i = 0; i < ramps.size(); ++i)
        ramps[i].current = ramps[i].target = project.curves[i].initial;
    if (project.perClip) {
        clipStates.resize(project.clips.size());
        for (std::size_t c = 0; c < clipStates.size(); ++c)
            for (std::size_t p = 0; p < 3; ++p)
                clipStates[c].ramps[p].current = clipStates[c].ramps[p].target =
                    project.clips[c].automation[p].initial;
        // Filter history must be reconstructed as well as the control values.
        // The host prepares this engine on a worker, never the UI/audio thread.
        while (cursor < start)
            processClips(nullptr, 0, static_cast<int>(std::min(Frame{1024}, start - cursor)));
        publishedFrame.store(cursor);
        return;
    }
    // Exact automation history reconstruction, off the audio thread.
    for (; cursor < start; ++cursor)
        targets(cursor);
    publishedFrame.store(cursor);
}
void Engine::arm(Frame in, Frame out, unsigned mask, int clip) noexcept {
    punchIn = in;
    punchOut = out;
    armedMask = mask & 7;
    armedClip = clip;
}
void Engine::targets(Frame frame) noexcept {
    for (std::size_t i = 0; i < ramps.size(); ++i)
        ramps[i].next((overrides & (1u << i)) ? overrideValues[i] : project.curves[i].at(frame));
}
void Engine::process(float *const *output, int channels, int count) noexcept {
    if (project.perClip) {
        processClips(output, channels, count);
        return;
    }
    float maxPeak = 0;
    unsigned commandsRemaining = 4096; // Bound work even if the producer keeps refilling.
    for (int s = 0; s < count; ++s) {
        for (int c = 0; c < channels; ++c)
            if (output[c])
                output[c][s] = 0;
        if (cursor >= project.end())
            continue;
        if (armedMask && cursor == punchOut)
            overrides = 0;
        Event event;
        while (commandsRemaining && incoming.peek(event) && event.frame <= cursor) {
            --commandsRemaining;
            incoming.pop(event);
            auto i = static_cast<unsigned>(event.param);
            if (i >= 3 || !std::isfinite(event.value))
                continue;
            if (event.frame < cursor)
                late.fetch_add(1, std::memory_order_relaxed);
            event.frame = cursor;
            event.value = std::clamp(event.value, i == 2 ? -1.0f : 0.0f, 1.0f);
            const bool writing =
                armedMask && cursor >= punchIn && cursor < punchOut && (armedMask & (1u << i));
            if (!armedMask || writing) {
                overrides |= (1u << i);
                overrideValues[i] = event.value;
                if (writing && !capture.push(event))
                    overflow.store(true, std::memory_order_relaxed);
            }
        }
        targets(cursor);
        const float angle = (ramps[2].current + 1.0f) * 0.7853981633974483f;
        const std::array<float, 2> cross{std::cos(angle), std::sin(angle)};
        for (std::size_t deck = 0; deck < 2; ++deck) {
            const auto &clip = project.clips[deck];
            if (!clip.audio || cursor < clip.start || cursor >= clip.start + clip.length)
                continue;
            const double source =
                static_cast<double>(clip.sourceOffset) +
                static_cast<double>(cursor - clip.start) * clip.rate * clip.audio->sampleRate / projectRate;
            if (source < 0 || source >= static_cast<double>(clip.audio->frames()))
                continue;
            const auto index = static_cast<std::size_t>(source);
            const auto next = std::min(index + 1, clip.audio->frames() - 1);
            const float fraction = static_cast<float>(source - static_cast<double>(index));
            for (int c = 0; c < 2; ++c) {
                const auto &audio = clip.audio->samples[static_cast<std::size_t>(c)];
                const float sample = audio[index] + fraction * (audio[next] - audio[index]);
                if (c < channels && output[c])
                    output[c][s] += sample * ramps[deck].current * cross[deck];
                if (clip.cue && c + 2 < channels && output[c + 2])
                    output[c + 2][s] += sample * 0.5f;
            }
        }
        for (int c = 0; c < std::min(channels, 2); ++c)
            if (output[c])
                maxPeak = std::max(maxPeak, std::abs(output[c][s]));
        ++cursor;
    }
    publishedFrame.store(cursor, std::memory_order_relaxed);
    publishedPeak.store(maxPeak, std::memory_order_relaxed);
}
void commitTake(Project &project, const std::vector<Event> &events, Frame in, Frame out) {
    if (in < 0 || out <= in)
        return;
    auto replacement = project;
    if (project.perClip) {
        for (std::size_t clip = 0; clip < project.clips.size(); ++clip)
            for (unsigned param = 0; param < 3; ++param) {
                const auto &originalClip = project.clips[clip];
                std::vector<Point> take;
                for (const auto &e : events)
                    if (e.clip == clip && static_cast<unsigned>(e.param) == param && e.frame >= in &&
                        e.frame < out && e.frame >= originalClip.start &&
                        e.frame < originalClip.start + originalClip.length)
                        take.push_back({e.frame - originalClip.start, e.value, true});
                if (take.empty())
                    continue;
                const auto original = originalClip.automation[param];
                auto &curve = replacement.clips[clip].automation[param];
                const auto first = take.front().frame,
                           end = std::min(out - originalClip.start, originalClip.length);
                std::erase_if(curve.points,
                              [first, end](const Point &p) { return p.frame >= first && p.frame <= end; });
                // A boundary point prevents a captured step from pulling an earlier
                // linear segment towards the new value before the gesture happened.
                if (first > 0) {
                    std::erase_if(curve.points, [first](const Point &p) { return p.frame == first - 1; });
                    curve.points.push_back({first - 1, original.at(first - 1)});
                }
                curve.points.insert(curve.points.end(), take.begin(), take.end());
                bool hold = !original.linear;
                for (const auto &p : original.points) {
                    if (p.frame > end)
                        break;
                    hold = p.hold || !original.linear;
                }
                curve.points.push_back({end, original.at(end), hold});
                std::stable_sort(curve.points.begin(), curve.points.end(),
                                 [](auto a, auto b) { return a.frame < b.frame; });
                std::vector<Point> unique;
                for (auto p : curve.points) {
                    if (!unique.empty() && unique.back().frame == p.frame)
                        unique.back() = p;
                    else
                        unique.push_back(p);
                }
                curve.points = std::move(unique);
            }
        replacement.validate();
        project = std::move(replacement);
        return;
    }
    for (unsigned lane = 0; lane < 3; ++lane) {
        std::vector<Point> take;
        for (const auto &e : events)
            if (static_cast<unsigned>(e.param) == lane && e.frame >= in && e.frame < out)
                take.push_back({e.frame, e.value});
        if (take.empty())
            continue;
        const auto original = project.curves[lane];
        if (original.linear)
            throw std::runtime_error("Convert the lane to Step before recording a replacement take");
        auto &curve = replacement.curves[lane];
        const auto first = take.front().frame;
        // Restore the saved target at punch-out; retain every later target so the
        // committed result follows the same return smoothing as the live take.
        std::erase_if(curve.points,
                      [first, out](const Point &p) { return p.frame >= first && p.frame <= out; });
        curve.points.insert(curve.points.end(), take.begin(), take.end());
        curve.points.push_back({out, original.at(out)});
        std::stable_sort(curve.points.begin(), curve.points.end(),
                         [](const Point &a, const Point &b) { return a.frame < b.frame; });
        std::vector<Point> unique;
        for (const auto &point : curve.points) {
            if (!unique.empty() && unique.back().frame == point.frame)
                unique.back() = point;
            else
                unique.push_back(point);
        }
        curve.points = std::move(unique);
    }
    replacement.validate();
    project = std::move(replacement);
}
float cutoffHz(float normalized) noexcept {
    return 20.0f * std::pow(1000.0f, std::clamp(normalized, 0.0f, 1.0f));
}
void Engine::processClips(float *const *output, int channels, int count) noexcept {
    float maxPeak = 0;
    unsigned budget = 4096;
    const auto end = project.end();
    for (int sampleIndex = 0; sampleIndex < count; ++sampleIndex) {
        for (int ch = 0; ch < channels; ++ch)
            if (output[ch])
                output[ch][sampleIndex] = 0;
        if (cursor >= end)
            continue;
        if (armedMask && cursor == punchOut)
            for (auto &state : clipStates)
                state.overrides = 0;
        Event e;
        while (budget && incoming.peek(e) && e.frame <= cursor) {
            --budget;
            incoming.pop(e);
            const auto p = static_cast<unsigned>(e.param);
            if (p >= 3 || e.clip >= clipStates.size() || !std::isfinite(e.value))
                continue;
            if (e.frame < cursor)
                late.fetch_add(1, std::memory_order_relaxed);
            e.frame = cursor;
            e.value = std::clamp(e.value, 0.0f, 1.0f);
            const auto &clip = project.clips[e.clip];
            const bool active = cursor >= clip.start && cursor < clip.start + clip.length;
            const bool writing = armedMask && (armedMask & (1u << p)) && cursor >= punchIn &&
                                 cursor < punchOut &&
                                 (armedClip < 0 || static_cast<unsigned>(armedClip) == e.clip) && active;
            if (!armedMask || writing) {
                auto &state = clipStates[e.clip];
                state.overrides |= 1u << p;
                state.values[p] = e.value;
                if (writing && !capture.push(e))
                    overflow.store(true, std::memory_order_relaxed);
            }
        }
        for (std::size_t i = 0; i < project.clips.size(); ++i) {
            const auto &clip = project.clips[i];
            if (!clip.audio || cursor < clip.start || cursor >= clip.start + clip.length)
                continue;
            const auto local = cursor - clip.start;
            auto &state = clipStates[i];
            for (unsigned p = 0; p < 3; ++p)
                state.ramps[p].next(state.overrides & (1u << p) ? state.values[p]
                                                                : clip.automation[p].at(local));
            const float hp = state.ramps[1].current, lp = state.ramps[2].current;
            if (local % 16 == 0) {
                state.hpAlpha = std::exp(-6.28318530718f * cutoffHz(hp) / projectRate);
                state.lpAlpha = std::exp(-6.28318530718f * cutoffHz(lp) / projectRate);
            }
            const double source =
                static_cast<double>(clip.sourceOffset) +
                static_cast<double>(local) * clip.rate * clip.audio->sampleRate / projectRate;
            if (source < 0 || source >= static_cast<double>(clip.audio->frames()))
                continue;
            const auto index = static_cast<std::size_t>(source),
                       next = std::min(index + 1, clip.audio->frames() - 1);
            const float fraction = static_cast<float>(source - static_cast<double>(index));
            for (int ch = 0; ch < 2; ++ch) {
                const auto &pcm = clip.audio->samples[static_cast<std::size_t>(ch)];
                const float dry = pcm[index] + fraction * (pcm[next] - pcm[index]);
                auto &f = state.filters[static_cast<std::size_t>(ch)];
                float high = dry;
                for (unsigned stage = 0; stage < 2; ++stage) {
                    const auto value = state.hpAlpha * (f.hpOut[stage] + high - f.hpIn[stage]);
                    f.hpIn[stage] = high;
                    f.hpOut[stage] = std::abs(value) < 1.0e-20f ? 0 : value;
                    high = value;
                }
                high = dry + std::min(1.0f, hp * 50) * (high - dry);
                float low = high;
                for (unsigned stage = 0; stage < 2; ++stage) {
                    f.lp[stage] = (1 - state.lpAlpha) * low + state.lpAlpha * f.lp[stage];
                    if (std::abs(f.lp[stage]) < 1.0e-20f)
                        f.lp[stage] = 0;
                    low = f.lp[stage];
                }
                const float wet = high + std::min(1.0f, (1 - lp) * 50) * (low - high);
                if (ch < channels && output[ch])
                    output[ch][sampleIndex] += wet * state.ramps[0].current;
                if (clip.cue && ch + 2 < channels && output[ch + 2])
                    output[ch + 2][sampleIndex] += dry * 0.5f;
            }
        }
        for (int ch = 0; ch < std::min(2, channels); ++ch)
            if (output[ch])
                maxPeak = std::max(maxPeak, std::abs(output[ch][sampleIndex]));
        ++cursor;
    }
    publishedFrame.store(cursor, std::memory_order_relaxed);
    publishedPeak.store(maxPeak, std::memory_order_relaxed);
}
} // namespace fader
