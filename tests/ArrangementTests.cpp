// SPDX-License-Identifier: AGPL-3.0-or-later
#include "Arrangement.h"
#include "Theme.h"
#include <cmath>
#include <stdexcept>
using namespace fader;
namespace {
void require(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
juce::MouseEvent event(juce::Component *canvas, juce::Point<float> point, juce::Point<float> origin,
                       bool dragged = false, bool right = false) {
    const auto now = juce::Time::getCurrentTime();
    return {juce::Desktop::getInstance().getMainMouseSource(),
            point,
            juce::ModifierKeys(right ? juce::ModifierKeys::rightButtonModifier
                                     : juce::ModifierKeys::leftButtonModifier),
            1,
            0,
            0,
            0,
            0,
            canvas,
            canvas,
            now,
            origin,
            now,
            1,
            dragged};
}
} // namespace
void testArrangement() {
    // Compare the actual production helper with the original individual-circle
    // drawing. Every solid interior pixel must survive; neither renderer may
    // put colour beyond a one-pixel antialiasing fringe of the other. This
    // includes dense overlaps, fractional positions and both sides of each
    // 64-handle boundary, rather than an assertion about a batching counter.
    const juce::SoftwareImageType software;
    const juce::NativeImageType native;
    for (const juce::ImageType *imageType :
         {static_cast<const juce::ImageType *>(&software), static_cast<const juce::ImageType *>(&native)}) {
        for (double scale : {2.0, 10.0, 100.0})
            for (float clipX : {-6.25f, .125f, 37.75f}) {
                Curve curve;
                curve.points.clear();
                for (int i = 0; i < 133; ++i)
                    curve.points.push_back({Frame(i) * projectRate / 4, i % 3 == 0 ? .7f : .3f});
                const auto original = curve.points;
                juce::Image expected(juce::Image::ARGB, 1700, 100, true, *imageType);
                juce::Image actual(juce::Image::ARGB, 1700, 100, true, *imageType);
                {
                    juce::Graphics g(expected);
                    g.setColour(ui::accent);
                    for (const auto &point : curve.points) {
                        const float px = clipX + float(double(point.frame) / projectRate * scale);
                        g.fillEllipse(px - 4, 90 - point.value * 70 - 4, 8, 8);
                    }
                }
                {
                    juce::Graphics g(actual);
                    g.setColour(ui::accent);
                    detail::paintPointHandles(g, curve.points, clipX, scale, 90, 70);
                }
                require(curve.points == original, "painting changed stored automation");
                const juce::Image::BitmapData actualPixels(actual, juce::Image::BitmapData::readOnly);
                const juce::Image::BitmapData expectedPixels(expected, juce::Image::BitmapData::readOnly);
                for (int y = 1; y < 99; ++y)
                    for (int x = 1; x < 1699; ++x) {
                        const auto a = actualPixels.getPixelColour(x, y).getAlpha();
                        const auto e = expectedPixels.getPixelColour(x, y).getAlpha();
                        if (e == 255) {
                            require(a >= 250, "batch drawing lost a solid handle interior");
                            require(actualPixels.getPixelColour(x, y).getRed() >= 220,
                                    "batch drawing changed the handle colour");
                        }
                        if (a != 0 || e != 0) {
                            bool nearActual = false, nearExpected = false;
                            for (int dy = -1; dy <= 1; ++dy)
                                for (int dx = -1; dx <= 1; ++dx) {
                                    nearActual |= actualPixels.getPixelColour(x + dx, y + dy).getAlpha() != 0;
                                    nearExpected |=
                                        expectedPixels.getPixelColour(x + dx, y + dy).getAlpha() != 0;
                                }
                            require(a == 0 || nearExpected,
                                    "batch handle extended beyond its original geometry");
                            require(e == 0 || nearActual, "batch drawing omitted a visible handle");
                        }
                    }
            }
    }
    // Geometric reference: every circle that can touch the viewport must be in
    // the returned slice, including edges and far-timeline float rounding. The
    // slice size must depend on visible time, rather than total point count.
    Curve dense;
    dense.points.clear();
    for (int i = 0; i <= 100000; ++i)
        dense.points.push_back({Frame(i) * (projectRate / 10), .5f});
    for (double scale : {2.0, 10.0, 100.0})
        for (int start : {0, 20000, 76000}) {
            const auto x = float(80 + start * scale);
            const auto full = dense.points.back().frame;
            for (Frame length : {full, full / 2})
                for (double offset : {-100.0, 100.0, double(length) / projectRate * scale / 2,
                                      double(length) / projectRate * scale - 100}) {
                    const auto firstX = int(x + offset), lastX = firstX + 1700;
                    const auto subset = detail::visiblePointHandles(dense, x, scale, length, firstX, lastX);
                    const auto maximum = std::size_t(std::ceil((1700 + 12) / scale * 10)) + 2;
                    require(subset.size() <= maximum, "handle paint work grows with offscreen history");
                    for (const auto &point : dense.points) {
                        const auto px = x + float(double(point.frame) / projectRate * scale);
                        if (point.frame <= length && px + 4 >= firstX && px - 4 <= lastX)
                            require(!subset.empty() && &point >= subset.data() &&
                                        &point < subset.data() + subset.size(),
                                    "visible automation handle was culled at a viewport edge");
                    }
                }
        }
    Project project;
    project.perClip = true;
    project.clips[0].length = 12 * projectRate;
    auto &clip = project.clips[1];
    clip.length = 12 * projectRate;
    clip.start = 6 * projectRate;
    clip.row = 1;
    clip.automation[0].points = {{0, 0}, {6 * projectRate, .7f}};
    const auto original = clip;
    Arrangement arrangement;
    arrangement.project = &project;
    arrangement.setSize(1000, 600);
    arrangement.zoom(40);
    auto *canvas = arrangement.viewport.getViewedComponent();
    int edits = 0;
    arrangement.beginEdit = [&] {
        ++edits;
        return true;
    };
    const juce::Point<float> origin{380, 195}, destination{460, 337};
    canvas->mouseDown(event(canvas, origin, origin));
    require(arrangement.selected == 1, "clip header selection failed");
    canvas->mouseDrag(event(canvas, destination, origin, true));
    canvas->mouseUp(event(canvas, destination, origin, true));
    require(clip.start == 8 * projectRate && clip.row == 2 && clip.length == original.length,
            "UI drag resized clip or failed to move time/row");
    require(clip.automation[0].points == original.automation[0].points && edits == 1,
            "UI move changed clip automation or made multiple undo steps");
    arrangement.parameter = 1;
    const juce::Point<float> knot{560, 410};
    canvas->mouseDown(event(canvas, knot, knot));
    canvas->mouseUp(event(canvas, knot, knot));
    require(clip.automation[1].points.size() == 2 && clip.automation[1].points[1].frame == 4 * projectRate,
            "UI curve insertion did not use clip-local time");
    require(project.clips[0].automation[1].points.size() == 1 &&
                clip.automation[0].points == original.automation[0].points,
            "UI curve edit changed another clip or parameter");
    const juce::Point<float> movedKnot{600, 420};
    canvas->mouseDown(event(canvas, knot, knot));
    canvas->mouseDrag(event(canvas, movedKnot, knot, true));
    canvas->mouseUp(event(canvas, movedKnot, knot, true));
    require(clip.automation[1].points[1].frame == 5 * projectRate, "UI curve drag did not move the point");
    int contextClip = -1, contextPoint = -1;
    arrangement.contextMenu = [&](int c, int p) {
        contextClip = c;
        contextPoint = p;
    };
    const int editsBeforeMenu = edits;
    canvas->mouseDown(event(canvas, movedKnot, movedKnot, false, true));
    canvas->mouseUp(event(canvas, movedKnot, movedKnot, false, true));
    require(contextClip == 1 && contextPoint == 1 && clip.automation[1].points.size() == 2 &&
                edits == editsBeforeMenu,
            "Opening a context menu changed automation or targeted the wrong point");
    int seeks = 0;
    arrangement.seek = [&](Frame) { ++seeks; };
    const juce::Point<float> ruler{200, 10};
    canvas->mouseDown(event(canvas, ruler, ruler, false, true));
    canvas->mouseUp(event(canvas, ruler, ruler, false, true));
    require(contextClip == -1 && contextPoint == -1 && seeks == 0 && edits == editsBeforeMenu,
            "Opening the ruler context menu moved the playhead or edited the set");
    arrangement.viewport.setViewPosition(0, 0);
    arrangement.zoom(80);
    require(arrangement.viewport.getViewPositionX() == 0, "zoom moved the timeline origin out of view");
    clip.start = Frame{3} * 3600 * projectRate;
    arrangement.playhead = clip.start;
    arrangement.refresh();
    arrangement.revealPlayhead();
    const int longPosition = 80 + 3 * 3600 * 80;
    require(arrangement.viewport.getViewPositionX() <= longPosition &&
                arrangement.viewport.getViewPositionX() + arrangement.viewport.getViewWidth() >
                    longPosition &&
                clip.length == original.length,
            "multi-hour source could not be revealed without changing its duration");
}
