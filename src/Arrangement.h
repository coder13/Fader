// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "Media.h"
#include <span>

namespace fader {
namespace detail {
// Conservative slice for the four-pixel handle radius, including float-position
// rounding at the far end of the supported timeline. Curves stay intact.
std::span<const Point> visiblePointHandles(const Curve &, float clipX, double pixelsPerSecond,
                                           Frame clipLength, int left, int right) noexcept;
// Draw every supplied handle at its clip-local position. The caller supplies
// the selected curve's opaque colour. Overlapping circles use a filled union.
void paintPointHandles(juce::Graphics &, std::span<const Point>, float clipX, double pixelsPerSecond,
                       float bottom, float height);
struct AutomationDrawing {
    const juce::Path &path;
    juce::Colour colour;
    float thickness;
    std::span<const Point> handles{};
    float clipX = 0;
    double pixelsPerSecond = 10;
    float bottom = 0, height = 0;
};
// A viewport/row-sized software image bounds Mac automation raster work.
// The image contains current geometry, never the whole project history.
class AutomationLayer {
  public:
    void paint(juce::Graphics &, const AutomationDrawing &, juce::Rectangle<int> visibleBand);

  private:
    juce::Image pixels;
    float pixelScale = 0;
};
} // namespace detail
class Arrangement final : public juce::Component {
  public:
    Arrangement();
    Project *project = nullptr;
    int selected = -1, parameter = 0;
    Frame playhead = 0;
    bool editable = true;
    double pixelsPerSecond = 10;
    double snapSeconds = 0;
    std::function<bool()> beginEdit;
    std::function<void()> changed;
    std::function<void(int)> selectionChanged;
    std::function<void(int, int)> contextMenu; // Clip index and nearby point (-1 when absent).
    std::function<void(Frame)> seek;
    std::function<void(const juce::File &, Frame, int)> drop;
    void resized() override;
    void refresh();
    void zoom(double);
    void revealPlayhead();
    juce::Viewport viewport;

  private:
    class Canvas final : public juce::Component,
                         public juce::FileDragAndDropTarget,
                         public juce::DragAndDropTarget {
      public:
        explicit Canvas(Arrangement &a) : owner(a) {}
        void paint(juce::Graphics &) override;
        bool isInterestedInFileDrag(const juce::StringArray &) override {
            return true;
        }
        void filesDropped(const juce::StringArray &, int, int) override;
        bool isInterestedInDragSource(const SourceDetails &d) override {
            return d.description.toString().startsWith("fader-file:");
        }
        void itemDropped(const SourceDetails &) override;
        void mouseDown(const juce::MouseEvent &) override;
        void mouseDrag(const juce::MouseEvent &) override;
        void mouseUp(const juce::MouseEvent &) override;

      private:
        juce::Rectangle<float> bounds(const Clip &) const;
        Frame frame(float) const;
        int hit(juce::Point<float>) const;
        Arrangement &owner;
        detail::AutomationLayer automationLayer;
        int moving = -1, point = -1, pointClip = -1, pointParam = 0, originalRow = 0;
        Frame originalStart = 0;
        juce::Point<float> anchor;
        bool editing = false;
    } canvas;
};
} // namespace fader
