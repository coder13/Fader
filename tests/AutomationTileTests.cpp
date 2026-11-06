// SPDX-License-Identifier: AGPL-3.0-or-later
#include "Arrangement.h"
#include "Theme.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
using namespace fader;
namespace {
void require(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
bool equalImages(const juce::Image &a, const juce::Image &b) {
    if (a.getBounds() != b.getBounds())
        return false;
    const juce::Image::BitmapData x(a, juce::Image::BitmapData::readOnly),
        y(b, juce::Image::BitmapData::readOnly);
    for (int row = 0; row < a.getHeight(); ++row)
        if (std::memcmp(x.getLinePointer(row), y.getLinePointer(row), std::size_t(a.getWidth()) * 4) != 0)
            return false;
    return true;
}
Curve fixture(int parameter = 0) {
    Curve result;
    result.points.clear();
    for (int i = 0; i <= 1600; ++i)
        result.points.push_back(
            {Frame(i) * projectRate / 4, .5f + .25f * std::sin(float(i) * .2f + parameter)});
    return result;
}
juce::Image frame(detail::AutomationLayer &layer, const Curve &curve, const Curve &handles, int viewX,
                  float clipX, float scale, juce::Colour colour = ui::accent, bool selected = true,
                  float thickness = 2.2f) {
    constexpr int width = 768, height = 100;
    constexpr double pixelsPerSecond = 10;
    const auto length = 400 * projectRate;
    juce::Path path;
    const int begin = std::max(int(clipX), viewX), end = std::min(int(clipX + 4000), viewX + width);
    for (int x = begin; x < end; ++x) {
        const auto local = Frame((x - clipX) / pixelsPerSecond * projectRate);
        const float y = 85 - curve.at(local) * 60;
        if (x == begin)
            path.startNewSubPath(float(x), y);
        else
            path.lineTo(float(x), y);
    }
    const auto points =
        selected ? detail::visiblePointHandles(handles, clipX, pixelsPerSecond, length, viewX, viewX + width)
                 : std::span<const Point>{};
    juce::Image image(juce::Image::ARGB, int(width * scale), int(height * scale), true,
                      juce::NativeImageType{});
    juce::Graphics g(image);
    g.addTransform(juce::AffineTransform::translation(-float(viewX), 0).scaled(scale));
    layer.paint(g, {path, colour, thickness, points, clipX, pixelsPerSecond, 85, 60},
                {viewX, 0, width, height});
    return image;
}
void compareFrame(detail::AutomationLayer &cached, const Curve &curve, const Curve &handles, int view,
                  float clip, float scale, juce::Colour colour = ui::accent, bool selected = true,
                  float thickness = 2.2f) {
    detail::AutomationLayer fresh(false);
    auto actual = frame(cached, curve, handles, view, clip, scale, colour, selected, thickness);
    auto expected = frame(fresh, curve, handles, view, clip, scale, colour, selected, thickness);
    require(equalImages(actual, expected), "warm automation tiles differ from current uncached geometry");
    const auto stats = cached.cacheStats();
    require(stats.entries <= detail::AutomationLayer::maxTiles &&
                stats.retainedBytes <= stats.peakRetainedBytes &&
                stats.peakRetainedBytes <= detail::AutomationLayer::maxRetainedBytes,
            "automation tile retention exceeds its entry/byte bound");
}
} // namespace
void testAutomationTiles() {
    detail::AutomationLayer cached;
    auto curve = fixture(), high = fixture(1), low = fixture(2);
    for (float scale : {1.f, 2.f, 1.f}) {
        compareFrame(cached, curve, curve, 400, 80.25f, scale);
        const auto before = cached.cacheStats();
        compareFrame(cached, curve, curve, 400, 80.25f, scale);
#if JUCE_MAC
        require(cached.cacheStats().hits > before.hits,
                "unchanged interior automation tiles were not reused");
#endif
        for (int view : {0, 3, 117, 127, 128, 130, 400, 403, 1100, 2200, 3900, 4070}) {
            compareFrame(cached, curve, curve, view, 80.25f, scale);
            compareFrame(cached, high, high, view + 3, 90.25f, scale, ui::blue);
            compareFrame(cached, low, low, view + 6, 80.75f, scale, ui::mint.withAlpha(.65f), false, 1.4f);
        }
        // Current content changes in the existing vector allocation. A pointer
        // or point-count key cannot detect this edit.
        const auto *storage = curve.points.data();
        const auto old = frame(cached, curve, curve, 400, 80.25f, scale);
        curve.points[200].value = curve.points[200].value < .25f ? .9f : .05f;
        require(curve.points.data() == storage, "fixture edit unexpectedly moved point storage");
        compareFrame(cached, curve, curve, 400, 80.25f, scale);
        require(!equalImages(old, frame(cached, curve, curve, 400, 80.25f, scale)),
                "curve edit did not reach the current tile image");
        // Change only handles while keeping the curve's sampled path fixed.
        auto knots = curve;
        knots.points[210].value = .95f;
        compareFrame(cached, curve, knots, 400, 80.25f, scale);
        knots.points[210].frame += projectRate / 16;
        compareFrame(cached, curve, knots, 400, 80.25f, scale);
        knots.points.erase(knots.points.begin() + 210);
        compareFrame(cached, curve, knots, 400, 80.25f, scale);
        knots.points.insert(knots.points.begin() + 210, {Frame(210) * projectRate / 4, .2f});
        compareFrame(cached, curve, knots, 400, 80.25f, scale, ui::mint, true, 1.4f);
        // A circle at the start straddles the negative tile and tile zero.
        const auto start = frame(cached, curve, curve, 0, 10.25f, scale);
        require(start.getPixelAt(int(7 * scale), int(55 * scale)).getAlpha() >= 250,
                "tile zero clipped the left half of a selected start handle");
    }
    // More unique content than the cache can retain, then revisit older data.
    for (int i = 0; i < 80; ++i)
        compareFrame(cached, curve, curve, 400, 80.25f, 1,
                     juce::Colour::fromRGB(juce::uint8(i + 1), juce::uint8(200 - i), juce::uint8(i * 3)));
    compareFrame(cached, high, high, 400, 80.25f, 1, ui::blue);
    // The retention cap must never become a cap on displayed handles. The final
    // knot is above the fixed line and follows more than 1,024 earlier knots.
    Curve dense, flat;
    dense.points.clear();
    dense.points.push_back({0, .5f});
    for (int i = 0; i < 2048; ++i)
        dense.points.push_back({Frame(23) * projectRate + Frame(i) * (projectRate / 1000), .5f});
    dense.points.back().value = 0;
    const auto oldDense = cached.cacheStats().denseRasters;
    compareFrame(cached, flat, dense, 200, 80.25f, 1);
    const auto denseImage = frame(cached, flat, dense, 200, 80.25f, 1);
    const auto x = int(80.25 + double(dense.points.back().frame) / projectRate * 10 - 200);
    require(denseImage.getPixelAt(x, 85).getAlpha() >= 250,
            "dense tile fallback omitted a handle beyond its signature capacity");
#if JUCE_MAC
    require(cached.cacheStats().denseRasters > oldDense, "dense geometry did not use the uncached fallback");
#endif
}
