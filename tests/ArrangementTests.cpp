// SPDX-License-Identifier: AGPL-3.0-or-later
#include "Arrangement.h"
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
