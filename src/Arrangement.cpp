// SPDX-License-Identifier: AGPL-3.0-or-later
#include "Arrangement.h"
#include "Theme.h"
#include <algorithm>
#include <cmath>
namespace fader {
namespace {
constexpr int rowHeight = 142, left = 80, top = 34;
const auto ink = ui::text, muted = ui::muted, line = ui::grid;
juce::Colour curveColour(int p) {
    return ui::envelope(p);
}
} // namespace
std::span<const Point> detail::visiblePointHandles(const Curve &curve, float clipX, double scale,
                                                   Frame length, int firstX, int lastX) noexcept {
    if (curve.points.empty() || !std::isfinite(clipX) || !std::isfinite(scale) || scale <= 0 || length < 0 ||
        firstX > lastX)
        return {};
    const auto bound = double(std::min(length, maxProjectFrames));
    // Two extra pixels cover intermediate binary32 position rounding. The
    // painter's actual radius remains four pixels.
    const auto first =
        Frame(std::clamp(std::floor((double(firstX) - 6 - clipX) / scale * projectRate), 0.0, bound));
    const auto last =
        Frame(std::clamp(std::ceil((double(lastX) + 6 - clipX) / scale * projectRate), 0.0, bound));
    const auto begin = std::lower_bound(curve.points.begin(), curve.points.end(), first,
                                        [](const Point &p, Frame f) { return p.frame < f; });
    const auto end = std::upper_bound(begin, curve.points.end(), last,
                                      [](Frame f, const Point &p) { return f < p.frame; });
    return {begin, end};
}
void detail::paintPointHandles(juce::Graphics &g, std::span<const Point> points, float clipX, double scale,
                               float bottom, float height) {
    // Selected handles share one opaque colour. Bound path storage and native
    // raster calls independently of the project's total automation history.
    constexpr int batchSize = 64;
    juce::Path handles;
    handles.setUsingNonZeroWinding(true);
    handles.preallocateSpace(batchSize * 32);
    int count = 0;
    for (const auto &point : points) {
        const float px = clipX + float(double(point.frame) / projectRate * scale);
        handles.addEllipse(px - 4, bottom - point.value * height - 4, 8, 8);
        if (++count == batchSize) {
            g.fillPath(handles);
            handles.clear();
            count = 0;
        }
    }
    if (count != 0)
        g.fillPath(handles);
}
Arrangement::Arrangement() : canvas(*this) {
    addAndMakeVisible(viewport);
    viewport.setViewedComponent(&canvas, false);
    viewport.setScrollBarsShown(true, true);
    viewport.setScrollOnDragMode(juce::Viewport::ScrollOnDragMode::never);
}
void Arrangement::resized() {
    viewport.setBounds(getLocalBounds());
    refresh();
}
void Arrangement::refresh() {
    int rows = 6;
    double seconds = 300;
    if (project) {
        seconds = std::max(seconds, double(project->end()) / projectRate + 180);
        for (const auto &c : project->clips)
            rows = std::max(rows, c.row + 5);
    }
    canvas.setSize(std::max(viewport.getWidth() - 14,
                            left +
                                static_cast<int>(std::min(double(maxProjectFrames) / projectRate, seconds) *
                                                 pixelsPerSecond) +
                                60),
                   std::max(viewport.getHeight() - 14, top + rows * rowHeight));
    canvas.repaint();
}
void Arrangement::zoom(double value) {
    const auto leftTime = std::max(0.0, double(viewport.getViewPositionX()) / pixelsPerSecond);
    pixelsPerSecond = std::clamp(value, 2.0, 100.0);
    refresh();
    viewport.setViewPosition(std::max(0, int(leftTime * pixelsPerSecond)), viewport.getViewPositionY());
}
void Arrangement::revealPlayhead() {
    const auto px = left + int(double(playhead) / projectRate * pixelsPerSecond);
    if (px < viewport.getViewPositionX() || px > viewport.getViewPositionX() + viewport.getViewWidth() - 30)
        viewport.setViewPosition(std::max(0, px - 60), viewport.getViewPositionY());
}
juce::Rectangle<float> Arrangement::Canvas::bounds(const Clip &c) const {
    return {float(left + double(c.start) / projectRate * owner.pixelsPerSecond),
            float(top + c.row * rowHeight + 8), float(double(c.length) / projectRate * owner.pixelsPerSecond),
            float(rowHeight - 16)};
}
Frame Arrangement::Canvas::frame(float x) const {
    return std::clamp(static_cast<Frame>((x - left) / owner.pixelsPerSecond * projectRate), Frame{0},
                      Frame{maxProjectFrames});
}
int Arrangement::Canvas::hit(juce::Point<float> position) const {
    if (!owner.project)
        return -1;
    for (int i = int(owner.project->clips.size()) - 1; i >= 0; --i)
        if (bounds(owner.project->clips[static_cast<std::size_t>(i)]).contains(position))
            return i;
    return -1;
}
void Arrangement::Canvas::paint(juce::Graphics &g) {
    g.fillAll(ui::background);
    const auto visible = owner.viewport.getViewArea();
    const int firstRow = std::max(0, (visible.getY() - top) / rowHeight),
              lastRow = (visible.getBottom() - top) / rowHeight;
    for (int row = firstRow; row <= lastRow; ++row) {
        const int y = top + row * rowHeight;
        g.setColour(row % 2 ? ui::panel : ui::background);
        g.fillRect(0, y, getWidth(), rowHeight);
        g.setColour(line);
        g.drawHorizontalLine(y, 0, float(getWidth()));
        g.setColour(muted);
        g.setFont(12);
        g.drawText("TRACK " + juce::String(row + 1), 8, y + 13, 62, 20, juce::Justification::left);
    }
    const double tick = owner.pixelsPerSecond >= 30 ? 1 : owner.pixelsPerSecond >= 8 ? 5 : 15;
    const double from =
        std::max(0.0, std::floor((visible.getX() - left) / owner.pixelsPerSecond / tick) * tick);
    for (double sec = from; left + sec * owner.pixelsPerSecond < visible.getRight(); sec += tick) {
        const int px = left + int(sec * owner.pixelsPerSecond);
        g.setColour(line);
        g.drawVerticalLine(px, std::max(top, visible.getY()), float(visible.getBottom()));
        g.setColour(muted);
        g.setFont(11);
        g.drawText(juce::String(int(sec) / 60) + ":" + juce::String(int(sec) % 60).paddedLeft('0', 2), px + 4,
                   visible.getY() + 4, 60, 22, juce::Justification::left);
    }
    if (!owner.project)
        return;
    g.saveState();
    g.reduceClipRegion(visible.withTrimmedTop(top));
    for (std::size_t i = 0; i < owner.project->clips.size(); ++i) {
        const auto &c = owner.project->clips[i];
        const auto b = bounds(c);
        if (!b.intersects(visible.toFloat()) || b.getWidth() < 1)
            continue;
        const bool selected = static_cast<int>(i) == owner.selected;
        const auto colour = ui::clip(static_cast<int>(i));
        g.setColour(ui::panel.interpolatedWith(colour, 0.10f));
        g.fillRect(b);
        g.setColour(colour.withAlpha(0.14f));
        g.fillRect(b.withHeight(35));
        g.setColour(selected ? curveColour(owner.parameter) : colour.withAlpha(0.5f));
        g.drawRect(b.reduced(0.7f), selected ? 1.5f : 1.0f);
        g.setColour(ink);
        g.setFont(13);
        g.drawText(juce::String(c.name), b.getSmallestIntegerContainer().withTrimmedLeft(10).withHeight(21),
                   juce::Justification::centredLeft, true);
        g.setFont(10);
        g.setColour(muted);
        g.drawText(juce::String(c.artist) + (c.artist.empty() ? "" : "  |  ") +
                       (c.bpm > 0 ? juce::String(c.bpm, 1) : juce::String("--")) + " BPM  " +
                       juce::String(c.key),
                   int(b.getX() + 10), int(b.getY() + 20), std::max(0, int(b.getWidth() - 20)), 14,
                   juce::Justification::left, true);
        const float y0 = b.getY() + 45, y1 = b.getBottom() - 12, mid = (y0 + y1) * 0.5f;
        const int begin = std::max(int(b.getX()), visible.getX()),
                  end = std::min(int(b.getRight()), visible.getRight());
        if (c.audio) {
            g.setColour(colour.withAlpha(0.62f));
            for (int px = begin; px < end; ++px) {
                const auto source = double(c.sourceOffset) +
                                    (px - b.getX()) / owner.pixelsPerSecond * c.rate * c.audio->sampleRate;
                const auto nextSource = source + c.rate * c.audio->sampleRate / owner.pixelsPerSecond;
                const auto a = static_cast<std::size_t>(std::max(0.0, source / double(c.audio->peakStride))),
                           z = std::min(c.audio->peaks.size(),
                                        static_cast<std::size_t>(
                                            std::max(0.0, nextSource / double(c.audio->peakStride)) + 1));
                float peak = 0;
                for (auto n = a; n < z; ++n)
                    peak = std::max(peak, c.audio->peaks[n]);
                g.drawVerticalLine(px, mid - peak * (y1 - y0) * 0.45f, mid + peak * (y1 - y0) * 0.45f);
            }
        }
        if (c.audio && c.hasGrid && c.bpm >= 20 && owner.pixelsPerSecond * 60 / c.bpm > 8) {
            const auto beat = 60.0 / c.bpm / c.rate;
            const auto first =
                c.firstBeatSeconds / c.rate - double(c.sourceOffset) / c.audio->sampleRate / c.rate;
            const auto localFrom = std::max(0.0, (begin - b.getX()) / owner.pixelsPerSecond);
            g.setColour(colour.withAlpha(0.14f));
            for (double time = first + std::max(0.0, std::ceil((localFrom - first) / beat)) * beat;
                 time < double(c.length) / projectRate; time += beat) {
                const float px = b.getX() + float(time * owner.pixelsPerSecond);
                if (px > end)
                    break;
                g.drawVerticalLine(int(px), y0, y1);
            }
        }
        if (owner.project->perClip) {
            const auto &curve = c.automation[static_cast<std::size_t>(owner.parameter)];
            juce::Path path;
            for (int px = begin; px < end; ++px) {
                const auto local = static_cast<Frame>((px - b.getX()) / owner.pixelsPerSecond * projectRate);
                const float y = y1 - curve.at(local) * (y1 - y0);
                if (px == begin)
                    path.startNewSubPath(float(px), y);
                else
                    path.lineTo(float(px), y);
            }
            g.setColour(curveColour(owner.parameter).withAlpha(selected ? 1.0f : 0.65f));
            g.strokePath(path, juce::PathStrokeType(selected ? 2.2f : 1.4f));
            if (selected)
                detail::paintPointHandles(g,
                                          detail::visiblePointHandles(curve, b.getX(), owner.pixelsPerSecond,
                                                                      c.length, visible.getX(),
                                                                      visible.getRight()),
                                          b.getX(), owner.pixelsPerSecond, y1, y1 - y0);
        }
    }
    g.restoreState();
    g.setColour(ui::raised);
    g.fillRect(visible.withHeight(top));
    g.setColour(muted);
    g.setFont(11);
    for (double sec = from; left + sec * owner.pixelsPerSecond < visible.getRight(); sec += tick) {
        const int px = left + int(sec * owner.pixelsPerSecond);
        g.drawText(juce::String(int(sec) / 60) + ":" + juce::String(int(sec) % 60).paddedLeft('0', 2), px + 4,
                   visible.getY() + 4, 60, 22, juce::Justification::left);
    }
    const auto px = float(left + double(owner.playhead) / projectRate * owner.pixelsPerSecond);
    g.setColour(ui::text);
    g.drawLine(px, float(visible.getY()), px, float(visible.getBottom()), 1.5f);
    if (owner.project->clips.empty()) {
        g.setColour(muted);
        g.setFont(19);
        g.drawText("Drag a song from the library into the arrangement", left + 45, 100, 650, 32,
                   juce::Justification::left);
        g.setFont(13);
        g.drawText("Move clips by their header. Draw volume or filters directly over the waveform.",
                   left + 45, 138, 650, 25, juce::Justification::left);
    }
}
void Arrangement::Canvas::filesDropped(const juce::StringArray &files, int x, int y) {
    if (!owner.editable || !owner.drop)
        return;
    int row = std::clamp((y - top) / rowHeight, 0, 4095);
    for (const auto &file : files)
        owner.drop(juce::File(file), frame(float(x)), row++);
}
void Arrangement::Canvas::itemDropped(const SourceDetails &d) {
    juce::StringArray files(d.description.toString().substring(11));
    filesDropped(files, d.localPosition.x, d.localPosition.y);
}
void Arrangement::Canvas::mouseDown(const juce::MouseEvent &e) {
    moving = point = pointClip = -1;
    editing = false;
    if (!owner.project)
        return;
    if (e.position.y < owner.viewport.getViewPositionY() + top) {
        if (e.mods.isPopupMenu()) {
            if (owner.contextMenu)
                owner.contextMenu(-1, -1);
        } else if (owner.seek)
            owner.seek(frame(e.position.x));
        return;
    }
    const int clip = hit(e.position);
    if (clip < 0) {
        if (e.mods.isPopupMenu() && owner.contextMenu) {
            owner.contextMenu(-1, -1);
            return;
        }
        if (owner.seek)
            owner.seek(frame(e.position.x));
        return;
    }
    owner.selected = clip;
    if (owner.selectionChanged)
        owner.selectionChanged(clip);
    if (owner.selected != clip)
        return;
    repaint();
    if (!owner.editable || !owner.project->perClip)
        return;
    const auto b = bounds(owner.project->clips[static_cast<std::size_t>(clip)]);
    if (e.mods.isPopupMenu()) {
        int nearby = -1;
        const auto &points = owner.project->clips[static_cast<std::size_t>(clip)]
                                 .automation[static_cast<std::size_t>(owner.parameter)]
                                 .points;
        for (std::size_t i = 0; i < points.size(); ++i) {
            const float x = b.getX() + float(double(points[i].frame) / projectRate * owner.pixelsPerSecond);
            const float y = b.getBottom() - 12 - points[i].value * (b.getHeight() - 57);
            if (std::abs(x - e.position.x) < 8 && std::abs(y - e.position.y) < 9) {
                nearby = static_cast<int>(i);
                break;
            }
        }
        if (owner.contextMenu)
            owner.contextMenu(clip, nearby);
        return;
    }
    if (e.position.y < b.getY() + 36) {
        moving = clip;
        originalStart = owner.project->clips[static_cast<std::size_t>(clip)].start;
        originalRow = owner.project->clips[static_cast<std::size_t>(clip)].row;
        anchor = e.position;
        return;
    }
    if (owner.beginEdit && !owner.beginEdit())
        return;
    editing = true;
    pointClip = clip;
    pointParam = owner.parameter;
    auto &c = owner.project->clips[static_cast<std::size_t>(clip)];
    auto &curve = c.automation[static_cast<std::size_t>(pointParam)];
    const float y0 = b.getY() + 45, y1 = b.getBottom() - 12;
    const float value = std::clamp((y1 - e.position.y) / (y1 - y0), 0.0f, 1.0f);
    const Frame local = std::clamp(frame(e.position.x) - c.start, Frame{0}, c.length);
    for (std::size_t i = 0; i < curve.points.size(); ++i) {
        const float x = b.getX() + float(double(curve.points[i].frame) / projectRate * owner.pixelsPerSecond);
        if (std::abs(x - e.position.x) < 8 &&
            std::abs(y1 - curve.points[i].value * (y1 - y0) - e.position.y) < 9) {
            point = static_cast<int>(i);
            break;
        }
    }
    if (point < 0) {
        auto at = std::lower_bound(curve.points.begin(), curve.points.end(), local,
                                   [](const Point &p, Frame f) { return p.frame < f; });
        point = static_cast<int>(at - curve.points.begin());
        if (at != curve.points.end() && at->frame == local) {
            at->value = value;
            at->hold = false;
        } else
            curve.points.insert(at, {local, value});
    } else {
        curve.points[static_cast<std::size_t>(point)].value = value;
        curve.points[static_cast<std::size_t>(point)].hold = false;
    }
    if (owner.changed)
        owner.changed();
    repaint();
}
void Arrangement::Canvas::mouseDrag(const juce::MouseEvent &e) {
    if (!owner.editable || !owner.project)
        return;
    if (moving >= 0) {
        if (!editing) {
            if (owner.beginEdit && !owner.beginEdit())
                return;
            editing = true;
        }
        auto &c = owner.project->clips[static_cast<std::size_t>(moving)];
        auto start = originalStart +
                     static_cast<Frame>((e.position.x - anchor.x) / owner.pixelsPerSecond * projectRate);
        if (owner.snapSeconds > 0)
            start = static_cast<Frame>(std::round(double(start) / projectRate / owner.snapSeconds) *
                                       owner.snapSeconds * projectRate);
        c.start = std::clamp(start, Frame{0}, Frame{maxProjectFrames} - c.length);
        c.row = std::clamp(originalRow + int(std::round((e.position.y - anchor.y) / rowHeight)), 0, 4095);
        owner.refresh();
        auto position = owner.viewport.getLocalPoint(this, e.position.toInt());
        owner.viewport.autoScroll(position.x, position.y, 36, 18);
        if (owner.changed)
            owner.changed();
    } else if (point >= 0 && pointClip >= 0) {
        auto &c = owner.project->clips[static_cast<std::size_t>(pointClip)];
        auto &points = c.automation[static_cast<std::size_t>(pointParam)].points;
        const auto index = static_cast<std::size_t>(point);
        const auto b = bounds(c);
        const auto low = index ? points[index - 1].frame + 1 : Frame{0},
                   high =
                       index + 1 < points.size() ? std::min(c.length, points[index + 1].frame - 1) : c.length;
        if (high >= low)
            points[index] = {
                std::clamp(frame(e.position.x) - c.start, low, high),
                std::clamp((b.getBottom() - 12 - e.position.y) / (b.getHeight() - 57), 0.0f, 1.0f)};
        if (owner.changed)
            owner.changed();
        repaint();
    }
}
void Arrangement::Canvas::mouseUp(const juce::MouseEvent &) {
    const bool wasEditing = editing;
    moving = point = pointClip = -1;
    editing = false;
    if (wasEditing && owner.changed)
        owner.changed();
    owner.refresh();
}
} // namespace fader
