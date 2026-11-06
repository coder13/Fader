// SPDX-License-Identifier: AGPL-3.0-or-later
#include "Arrangement.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <list>

namespace fader::detail {
namespace {
constexpr int tileWidth = 128, bleed = 12, imageWidth = tileWidth + 2 * bleed;
constexpr std::size_t maxSignaturePoints = 1024, maxSourcePoints = 32768;
using Position = juce::Point<float>;
struct Geometry {
    std::vector<Position> path, handles;
    juce::Colour colour;
    float thickness = 0, scale = 0;
    int height = 0;
    bool operator==(const Geometry &) const = default;
};
std::size_t imageBytes(const juce::Image &image) {
    // Native pixels include an extra scanline; its immutable CG image copies
    // those bytes. Reserve additional space for context/provider bookkeeping.
    return image.isNull() ? 0 : std::size_t(image.getWidth()) * (image.getHeight() + 1) * 8 + 4096;
}
std::size_t geometryBytes(const Geometry &key) {
    return (key.path.capacity() + key.handles.capacity()) * sizeof(Position);
}
void stroke(juce::Graphics &g, const juce::Path &path, juce::Colour colour, float width, bool macJoins) {
    g.setColour(colour);
    juce::PathStrokeType type(width);
    if (macJoins)
        type.setMiterLimit(10.f);
    g.strokePath(path, type);
}
Position handlePosition(const AutomationDrawing &drawing, const Point &point) {
    return {drawing.clipX + float(double(point.frame) / projectRate * drawing.pixelsPerSecond),
            drawing.bottom - point.value * drawing.height};
}
} // namespace
struct AutomationLayer::Impl {
    struct Tile {
        Geometry key;
        juce::Image image;
        std::size_t bytes = 0;
    };
    explicit Impl(bool enabled) : cacheEnabled(enabled) {}
    bool cacheEnabled;
    CacheStats stats;
    std::list<Tile> tiles;
    std::size_t bytes = 0;
    float scale = 0;
    juce::Image scratch, fullBand;
    std::vector<Position> source;
    Geometry candidate;

    std::size_t workingBytes() const {
        return imageBytes(scratch) + imageBytes(fullBand) + geometryBytes(candidate) +
               source.capacity() * sizeof(Position);
    }
    void evict() {
        bytes -= tiles.back().bytes;
        tiles.pop_back();
    }
    bool readPath(const juce::Path &path) {
        source.clear();
        bool started = false;
        for (juce::Path::Iterator i(path); i.next();) {
            if ((!started && i.elementType != juce::Path::Iterator::startNewSubPath) ||
                (started && i.elementType != juce::Path::Iterator::lineTo) || !std::isfinite(i.x1) ||
                !std::isfinite(i.y1) || (!source.empty() && i.x1 <= source.back().x) ||
                source.size() == maxSourcePoints)
                return false;
            source.emplace_back(i.x1, i.y1);
            started = true;
        }
        return true;
    }
    bool geometry(const AutomationDrawing &drawing, int left, juce::Rectangle<int> band) {
        candidate.path.clear();
        candidate.handles.clear();
        candidate.colour = drawing.colour;
        candidate.thickness = drawing.thickness;
        candidate.scale = scale;
        candidate.height = band.getHeight();
        const float x0 = float(left - bleed), x1 = float(left + tileWidth + bleed);
        if (!source.empty() && source.front().x <= x1 && source.back().x >= x0) {
            auto begin = std::lower_bound(source.begin(), source.end(), x0,
                                          [](Position p, float x) { return p.x < x; });
            if (begin != source.begin())
                --begin;
            auto end = std::upper_bound(begin, source.end(), x1, [](float x, Position p) { return x < p.x; });
            if (end != source.end())
                ++end;
            for (auto i = begin; i != end; ++i)
                candidate.path.push_back({i->x - x0, i->y - band.getY()});
        }
        bool dense = false;
        for (const auto &point : drawing.handles) {
            const auto p = handlePosition(drawing, point);
            if (p.x + 4 < x0 || p.x - 4 > x1)
                continue;
            if (candidate.handles.size() <= maxSignaturePoints)
                candidate.handles.push_back({p.x - x0, p.y - band.getY()});
            else
                dense = true;
        }
        return dense || candidate.path.size() > maxSignaturePoints ||
               candidate.handles.size() > maxSignaturePoints;
    }
    void raster(juce::Image &image, const AutomationDrawing &drawing, int left, juce::Rectangle<int> band,
                bool dense) {
        const int w = int(std::ceil(imageWidth * scale)), h = int(std::ceil(band.getHeight() * scale));
        if (image.getWidth() != w || image.getHeight() != h)
            image = juce::Image(juce::Image::ARGB, w, h, true, juce::NativeImageType{});
        // Native pixel writes invalidate its cached CG image. The explicit
        // software context avoids dispatching dense paths back to Quartz.
        juce::LowLevelGraphicsSoftwareRenderer context(image);
        context.setFill(juce::Colours::transparentBlack);
        context.fillRect(image.getBounds(), true);
        juce::Graphics g(context);
        g.addTransform(juce::AffineTransform::scale(scale));
        juce::Path curve;
        bool first = true;
        for (auto p : candidate.path) {
            if (first) {
                curve.startNewSubPath(p);
                first = false;
            } else {
                curve.lineTo(p);
            }
        }
        stroke(g, curve, drawing.colour, drawing.thickness, true);
        g.setColour(drawing.colour);
        juce::Path handles;
        if (!dense) {
            handles.preallocateSpace(int(candidate.handles.size()) * 32);
            for (auto p : candidate.handles)
                handles.addEllipse(p.x - 4, p.y - 4, 8, 8);
            g.fillPath(handles);
        } else {
            // The signature limit controls retention, never rendered density.
            handles.preallocateSpace(64 * 32);
            int count = 0;
            const float x0 = float(left - bleed), x1 = float(left + tileWidth + bleed);
            for (const auto &point : drawing.handles) {
                auto p = handlePosition(drawing, point);
                if (p.x + 4 < x0 || p.x - 4 > x1)
                    continue;
                p -= Position{x0, float(band.getY())};
                handles.addEllipse(p.x - 4, p.y - 4, 8, 8);
                if (++count == 64) {
                    g.fillPath(handles);
                    handles.clear();
                    count = 0;
                }
            }
            if (count)
                g.fillPath(handles);
        }
        ++stats.rasterizations;
    }
    void full(juce::Graphics &g, const AutomationDrawing &drawing, juce::Rectangle<int> band,
              float physicalScale) {
        const int w = std::max(1, int(std::ceil(band.getWidth() * physicalScale))),
                  h = std::max(1, int(std::ceil(band.getHeight() * physicalScale)));
        if (fullBand.getWidth() != w || fullBand.getHeight() != h)
            fullBand = juce::Image(juce::Image::ARGB, w, h, true, juce::SoftwareImageType{});
        fullBand.clear(fullBand.getBounds(), juce::Colours::transparentBlack);
        auto local = drawing.path;
        local.applyTransform(juce::AffineTransform::translation(-float(band.getX()), -float(band.getY())));
        {
            juce::Graphics raster(fullBand);
            raster.addTransform(juce::AffineTransform::scale(physicalScale));
            stroke(raster, local, drawing.colour, drawing.thickness, true);
            paintPointHandles(raster, drawing.handles, drawing.clipX - band.getX(), drawing.pixelsPerSecond,
                              drawing.bottom - band.getY(), drawing.height);
        }
        g.setColour(juce::Colours::white);
        g.drawImageTransformed(fullBand, juce::AffineTransform::scale(1.f / physicalScale)
                                             .translated(float(band.getX()), float(band.getY())));
    }
};
AutomationLayer::AutomationLayer(bool enabled) : impl(std::make_unique<Impl>(enabled)) {}
AutomationLayer::~AutomationLayer() = default;
AutomationLayer::CacheStats AutomationLayer::cacheStats() const noexcept {
    auto result = impl->stats;
    result.entries = impl->tiles.size();
    result.retainedBytes = impl->bytes + impl->workingBytes();
    return result;
}
void AutomationLayer::paint(juce::Graphics &g, const AutomationDrawing &drawing, juce::Rectangle<int> band) {
    if (band.isEmpty())
        return;
    const juce::Graphics::ScopedSaveState saved(g);
    struct TrackRetention {
        Impl &state;
        ~TrackRetention() {
            state.stats.peakRetainedBytes =
                std::max(state.stats.peakRetainedBytes, state.bytes + state.workingBytes());
        }
    } track{*impl};
    g.reduceClipRegion(band);
#if JUCE_MAC
    const auto scale = g.getInternalContext().getPhysicalPixelScaleFactor();
    if (std::isfinite(scale) && scale > 0 &&
        g.getInternalContext().getPreferredImageTypeForTemporaryImages()->getTypeID() ==
            juce::NativeImageType{}.getTypeID()) {
        if (scale != impl->scale) {
            impl->tiles.clear();
            impl->bytes = 0;
            impl->scratch = {};
            impl->fullBand = {};
            impl->scale = scale;
        }
        const bool tiled =
            (scale == 1 || scale == 2) && band.getHeight() <= 142 && std::isfinite(drawing.clipX) &&
            drawing.clipX > std::numeric_limits<int>::min() + 512.0 &&
            drawing.clipX < std::numeric_limits<int>::max() - 512.0 && drawing.thickness > 0 &&
            drawing.thickness <= 2.2f && band.getX() > std::numeric_limits<int>::min() + 512 &&
            band.getRight() < std::numeric_limits<int>::max() - 512 && impl->readPath(drawing.path);
        if (!tiled) {
            impl->tiles.clear();
            impl->bytes = 0;
            impl->scratch = {};
            impl->full(g, drawing, band, scale);
            return;
        }
        impl->fullBand = {};
        g.setColour(juce::Colours::white);
        const auto anchor = std::int64_t(std::floor(double(drawing.clipX)));
        const auto first = std::int64_t(std::floor(double(std::int64_t(band.getX()) - anchor) / tileWidth));
        for (auto x = anchor + first * tileWidth; x < band.getRight(); x += tileWidth) {
            const int left = int(x);
            const bool dense = impl->geometry(drawing, left, band);
            if (impl->candidate.path.empty() && impl->candidate.handles.empty())
                continue;
            const bool complete = !impl->source.empty() && impl->source.front().x <= left - bleed &&
                                  impl->source.back().x >= left + tileWidth + bleed;
            auto found = impl->tiles.end();
            if (complete && !dense && impl->cacheEnabled)
                found = std::find_if(impl->tiles.begin(), impl->tiles.end(),
                                     [&](const auto &tile) { return tile.key == impl->candidate; });
            juce::Image *image = nullptr;
            if (found != impl->tiles.end()) {
                ++impl->stats.hits;
                impl->tiles.splice(impl->tiles.begin(), impl->tiles, found);
                image = &impl->tiles.front().image;
            } else {
                if (!complete)
                    ++impl->stats.edgeRasters;
                if (dense)
                    ++impl->stats.denseRasters;
                const int w = int(std::ceil(imageWidth * scale)),
                          h = int(std::ceil(band.getHeight() * scale));
                const auto pixelBytes = std::size_t(w) * (h + 1) * 8 + 4096;
                const auto working = std::max(imageBytes(impl->scratch), pixelBytes) +
                                     geometryBytes(impl->candidate) +
                                     impl->source.capacity() * sizeof(Position);
                const auto estimate =
                    pixelBytes + 4096 +
                    (impl->candidate.path.size() + impl->candidate.handles.size()) * sizeof(Position);
                if (complete && !dense && impl->cacheEnabled && estimate + working <= maxRetainedBytes) {
                    Impl::Tile tile;
                    tile.key = impl->candidate;
                    const auto cost = pixelBytes + geometryBytes(tile.key) + 4096;
                    while (!impl->tiles.empty() && (impl->tiles.size() >= maxTiles ||
                                                    impl->bytes + cost + working > maxRetainedBytes))
                        impl->evict();
                    impl->raster(tile.image, drawing, left, band, false);
                    tile.bytes = imageBytes(tile.image) + geometryBytes(tile.key) + 4096;
                    impl->bytes += tile.bytes;
                    impl->tiles.push_front(std::move(tile));
                    image = &impl->tiles.front().image;
                } else {
                    impl->raster(impl->scratch, drawing, left, band, dense);
                    image = &impl->scratch;
                }
            }
            const juce::Graphics::ScopedSaveState clip(g);
            g.reduceClipRegion(juce::Rectangle<int>(left, band.getY(), tileWidth, band.getHeight()));
            g.drawImageTransformed(*image, juce::AffineTransform::scale(1.f / scale)
                                               .translated(float(left - bleed), float(band.getY())));
        }
        return;
    }
#endif
    stroke(g, drawing.path, drawing.colour, drawing.thickness, false);
    paintPointHandles(g, drawing.handles, drawing.clipX, drawing.pixelsPerSecond, drawing.bottom,
                      drawing.height);
}
} // namespace fader::detail
