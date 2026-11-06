// SPDX-License-Identifier: AGPL-3.0-or-later
#include "Arrangement.h"
#include "Theme.h"
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <vector>
using namespace fader;
namespace {
void require(bool value, const std::string &message) {
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
    // drawing. Analytically solid circle interiors must survive; neither renderer may
    // put colour beyond a one-pixel antialiasing fringe of the other. This
    // excludes accumulated opacity at overlapping edges from the interior oracle.
    // It includes dense overlaps, fractional positions and both sides of each
    // 64-handle boundary, rather than an assertion about a batching counter.
    const juce::SoftwareImageType software;
    const juce::NativeImageType native;
    for (const juce::ImageType *imageType :
         {static_cast<const juce::ImageType *>(&software), static_cast<const juce::ImageType *>(&native)}) {
        std::cout << "Handle image tests: renderer=" << imageType->getTypeID() << std::endl;
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
                std::vector<bool> solid(1700 * 100);
                for (const auto &point : curve.points) {
                    const float cx = clipX + float(double(point.frame) / projectRate * scale),
                                cy = 90 - point.value * 70;
                    for (int y = int(std::floor(cy - 3)); y <= int(std::ceil(cy + 3)); ++y)
                        for (int x = int(std::floor(cx - 3)); x <= int(std::ceil(cx + 3)); ++x)
                            if (juce::isPositiveAndBelow(x, 1700) && juce::isPositiveAndBelow(y, 100) &&
                                std::hypot(x + .5f - cx, y + .5f - cy) <= 3.f)
                                solid[std::size_t(y * 1700 + x)] = true;
                }
                for (int y = 1; y < 99; ++y)
                    for (int x = 1; x < 1699; ++x) {
                        const auto a = actualPixels.getPixelColour(x, y).getAlpha();
                        const auto e = expectedPixels.getPixelColour(x, y).getAlpha();
                        if (solid[std::size_t(y * 1700 + x)]) {
                            if (a < 250)
                                throw std::runtime_error(
                                    "batch drawing lost a solid handle interior: type=" +
                                    std::to_string(imageType->getTypeID()) +
                                    ", scale=" + std::to_string(scale) + ", clipX=" + std::to_string(clipX) +
                                    ", x/y=" + std::to_string(x) + "/" + std::to_string(y) +
                                    ", alpha actual/reference=" + std::to_string(a) + "/" +
                                    std::to_string(e));
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
    // A native solid stroke and its outline clip rasterize sharp miter tips
    // differently. Use independent line-distance/miter geometry, rather than
    // making either rasterizer's edge pixels the definition of correctness.
    // Every sampled vertex must retain coverage, solid interiors must survive,
    // and the clip must apply the requested opacity once.
    for (const juce::ImageType *imageType :
         {static_cast<const juce::ImageType *>(&software), static_cast<const juce::ImageType *>(&native)})
        for (float deviceScale : {1.f, 2.f})
            for (float width : {1.4f, 2.2f})
                for (float opacity : {.65f, 1.f})
                    for (int shape = 0; shape < 3; ++shape) {
                        std::vector<juce::Point<float>> vertices;
                        juce::Path path;
                        for (int x = 0; x < 1100; ++x) {
                            const float y =
                                50.25f + (shape == 0   ? 0.f
                                          : shape == 1 ? 25.f * std::sin(float(x) * .04f)
                                                       : float(x % 10 < 5 ? x % 5 : 5 - x % 5) * 4.f);
                            vertices.emplace_back(float(x) + .125f, y);
                            if (x == 0)
                                path.startNewSubPath(vertices.back());
                            else
                                path.lineTo(vertices.back());
                        }
                        const auto original = path;
                        const int imageWidth = int(1100 * deviceScale), imageHeight = int(110 * deviceScale);
                        juce::Image expected(juce::Image::ARGB, imageWidth, imageHeight, true, *imageType);
                        juce::Image actual(juce::Image::ARGB, imageWidth, imageHeight, true, *imageType);
                        {
                            juce::Graphics g(expected);
                            g.addTransform(juce::AffineTransform::scale(deviceScale));
                            g.setColour(ui::accent.withAlpha(opacity));
                            g.strokePath(path, juce::PathStrokeType(width));
                        }
                        {
                            juce::Graphics g(actual);
                            g.addTransform(juce::AffineTransform::scale(deviceScale));
                            g.setColour(ui::mint);
                            detail::paintAutomationCurve(g, path, ui::accent.withAlpha(opacity), width);
                            // State restoration is part of the production contract.
                            g.fillRect(1096, 1, 2, 2);
                        }
                        require(path == original, "curve drawing changed its sampled vertices");
                        require(actual.getPixelAt(int(1096 * deviceScale), int(deviceScale)) == ui::mint,
                                "curve drawing leaked its colour state");
                        std::vector<bool> allowed(std::size_t(imageWidth * imageHeight));
                        const float half = width * .5f, fringe = .8f / deviceScale;
                        auto mark = [&](juce::Rectangle<float> area, auto contains) {
                            const auto bounds = area.transformedBy(juce::AffineTransform::scale(deviceScale))
                                                    .getSmallestIntegerContainer()
                                                    .getIntersection({0, 0, imageWidth, imageHeight});
                            for (int y = bounds.getY(); y < bounds.getBottom(); ++y)
                                for (int x = bounds.getX(); x < bounds.getRight(); ++x)
                                    if (contains(juce::Point<float>{(x + .5f) / deviceScale,
                                                                    (y + .5f) / deviceScale}))
                                        allowed[std::size_t(y * imageWidth + x)] = true;
                        };
                        for (std::size_t i = 1; i < vertices.size(); ++i) {
                            const auto a = vertices[i - 1], b = vertices[i], d = b - a;
                            const float radius = half + fringe;
                            mark(juce::Rectangle<float>::leftTopRightBottom(
                                     std::min(a.x, b.x) - radius, std::min(a.y, b.y) - radius,
                                     std::max(a.x, b.x) + radius, std::max(a.y, b.y) + radius),
                                 [&](juce::Point<float> p) {
                                     const auto v = p - a;
                                     const auto t = std::clamp(
                                         (v.x * d.x + v.y * d.y) / (d.x * d.x + d.y * d.y), 0.f, 1.f);
                                     return p.getDistanceFrom(a + d * t) <= radius;
                                 });
                            if (i + 1 < vertices.size()) {
                                const auto next = vertices[i + 1] - b;
                                const float dot =
                                    std::clamp((d.x * next.x + d.y * next.y) /
                                                   (d.getDistanceFromOrigin() * next.getDistanceFromOrigin()),
                                               -1.f, 1.f);
                                const float factor = std::sqrt(2.f / std::max(.000001f, 1.f + dot));
                                // Native Quartz's limit is ten; all tested renderers
                                // stay within that bound, including beveled joins.
                                const float join = half * (factor <= 10.f ? factor : 1.f) + fringe;
                                mark({b.x - join, b.y - join, join * 2, join * 2},
                                     [&](juce::Point<float> p) { return p.getDistanceFrom(b) <= join; });
                            }
                        }
                        const juce::Image::BitmapData a(actual, juce::Image::BitmapData::readOnly),
                            e(expected, juce::Image::BitmapData::readOnly);
                        for (int y = 2; y < imageHeight - 2; ++y)
                            for (int x = 2; x < imageWidth - 2; ++x) {
                                if (y <= 3 * deviceScale && x >= 1095 * deviceScale)
                                    continue;
                                const auto aa = a.getPixelColour(x, y).getAlpha(),
                                           ea = e.getPixelColour(x, y).getAlpha();
                                bool interior = ea >= 250;
                                for (auto d : {juce::Point<int>{-1, 0}, {1, 0}, {0, -1}, {0, 1}})
                                    interior &= e.getPixelColour(x + d.x, y + d.y).getAlpha() >= 250;
                                require(!interior || aa >= 245, "native curve clip lost a solid interior");
                                require(aa <= int(std::lround(opacity * 255)),
                                        "curve clip accumulated opacity");
                                require(aa <= 16 || allowed[std::size_t(y * imageWidth + x)],
                                        "curve clip exceeded its geometric stroke/miter envelope");
                            }
                        for (auto p : vertices) {
                            const int cx = int(std::floor(p.x * deviceScale)),
                                      cy = int(std::floor(p.y * deviceScale));
                            int maximumAlpha = 0;
                            for (int dy = -1; dy <= 1; ++dy)
                                for (int dx = -1; dx <= 1; ++dx)
                                    if (juce::isPositiveAndBelow(cx + dx, actual.getWidth()) &&
                                        juce::isPositiveAndBelow(cy + dy, actual.getHeight()))
                                        maximumAlpha = std::max(
                                            maximumAlpha, int(a.getPixelColour(cx + dx, cy + dy).getAlpha()));
                            require(maximumAlpha >= int(opacity * 100), "curve clip lost a sampled vertex");
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
