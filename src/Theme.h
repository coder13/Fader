// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <juce_gui_basics/juce_gui_basics.h>

namespace fader::ui {
// Semantic roles shared by the arrangement, browser, inspector and dialogs.
inline const juce::Colour background{0xff15181b}, panel{0xff1d2125}, raised{0xff282d32}, field{0xff191d21},
    hover{0xff333a41}, border{0xff3b434b}, grid{0xff2c3339}, text{0xffe6eaed}, muted{0xffa2acb6},
    disabled{0xff727c86}, accent{0xffe6af62}, selection{0xff37332d}, danger{0xffff8b80}, blue{0xff6bbdd8},
    mint{0xff8bcbb2};
inline juce::Colour envelope(int parameter) {
    return parameter == 0 ? accent : parameter == 1 ? blue : mint;
}
inline juce::Colour clip(int index) {
    return index % 2 ? juce::Colour(0xffcba078) : blue;
}

class Theme final : public juce::LookAndFeel_V4 {
  public:
    Theme() {
        setColour(juce::ResizableWindow::backgroundColourId, background);
        setColour(juce::Label::textColourId, text);
        setColour(juce::TextButton::buttonColourId, raised);
        setColour(juce::TextButton::buttonOnColourId, selection);
        setColour(juce::TextButton::textColourOffId, text);
        setColour(juce::TextButton::textColourOnId, accent);
        setColour(juce::Slider::thumbColourId, accent);
        setColour(juce::Slider::trackColourId, accent);
        setColour(juce::Slider::textBoxTextColourId, text);
        setColour(juce::Slider::textBoxBackgroundColourId, field);
        setColour(juce::Slider::textBoxOutlineColourId, border);
        setColour(juce::TextEditor::backgroundColourId, field);
        setColour(juce::TextEditor::textColourId, text);
        setColour(juce::TextEditor::highlightColourId, selection.brighter(.15f));
        setColour(juce::TextEditor::highlightedTextColourId, text);
        setColour(juce::TextEditor::outlineColourId, border);
        setColour(juce::TextEditor::focusedOutlineColourId, accent);
        setColour(juce::CaretComponent::caretColourId, accent);
        setColour(juce::ToggleButton::textColourId, text);
        setColour(juce::ToggleButton::tickColourId, accent);
        setColour(juce::ToggleButton::tickDisabledColourId, disabled);
        setColour(juce::ComboBox::backgroundColourId, field);
        setColour(juce::ComboBox::textColourId, text);
        setColour(juce::ComboBox::arrowColourId, muted);
        setColour(juce::PopupMenu::backgroundColourId, raised);
        setColour(juce::PopupMenu::textColourId, text);
        setColour(juce::PopupMenu::highlightedBackgroundColourId, hover);
        setColour(juce::PopupMenu::highlightedTextColourId, accent);
        setColour(juce::PopupMenu::headerTextColourId, muted);
        setColour(juce::TableHeaderComponent::backgroundColourId, raised);
        setColour(juce::TableHeaderComponent::textColourId, muted);
        setColour(juce::TableHeaderComponent::outlineColourId, border);
        setColour(juce::ListBox::backgroundColourId, panel);
        setColour(juce::ListBox::textColourId, text);
        setColour(juce::ScrollBar::thumbColourId, disabled);
        setColour(juce::AlertWindow::backgroundColourId, panel);
        setColour(juce::AlertWindow::textColourId, text);
        setColour(juce::AlertWindow::outlineColourId, border);
        setColour(juce::TooltipWindow::backgroundColourId, raised);
        setColour(juce::TooltipWindow::textColourId, text);
        setColour(juce::TooltipWindow::outlineColourId, border);
    }
    juce::Font getTextButtonFont(juce::TextButton &, int) override {
        return juce::FontOptions(13);
    }
    void drawButtonBackground(juce::Graphics &g, juce::Button &b, const juce::Colour &, bool over,
                              bool down) override {
        auto area = b.getLocalBounds().toFloat();
        const bool active = b.getToggleState();
        g.setColour((active         ? selection
                     : over || down ? hover
                                    : raised)
                        .withAlpha(b.isEnabled() ? 1.f : .45f));
        g.fillRect(area);
        if (active) {
            g.setColour(accent);
            g.fillRect(area.removeFromBottom(2));
        }
        if (b.hasKeyboardFocus(true)) {
            g.setColour(accent);
            g.drawRect(b.getLocalBounds(), 1);
        }
    }
    void drawToggleButton(juce::Graphics &g, juce::ToggleButton &b, bool over, bool) override {
        const auto box = juce::Rectangle<float>(1, (b.getHeight() - 14) * .5f, 14, 14);
        g.setColour(over ? hover : field);
        g.fillRect(box);
        g.setColour(b.isEnabled() ? (b.getToggleState() ? accent : muted) : disabled);
        g.drawRect(box, 1);
        if (b.getToggleState()) {
            juce::Path check;
            check.startNewSubPath(box.getX() + 3, box.getY() + 7);
            check.lineTo(box.getX() + 6, box.getY() + 10);
            check.lineTo(box.getX() + 11, box.getY() + 4);
            g.strokePath(check, juce::PathStrokeType(1.5f));
        }
        if (b.hasKeyboardFocus(true)) {
            g.setColour(accent);
            g.drawRect(b.getLocalBounds(), 1);
        }
        g.setColour(b.isEnabled() ? text : disabled);
        g.setFont(13);
        g.drawText(b.getButtonText(), b.getLocalBounds().withTrimmedLeft(24),
                   juce::Justification::centredLeft);
    }
    void drawComboBox(juce::Graphics &g, int w, int h, bool, int, int, int, int, juce::ComboBox &b) override {
        g.fillAll(field);
        g.setColour(b.hasKeyboardFocus(true) ? accent : border);
        g.drawRect(0, 0, w, h);
        g.setColour(b.isEnabled() ? muted : disabled);
        juce::Path arrow;
        arrow.startNewSubPath(float(w - 16), float(h / 2 - 2));
        arrow.lineTo(float(w - 11), float(h / 2 + 3));
        arrow.lineTo(float(w - 6), float(h / 2 - 2));
        g.strokePath(arrow, juce::PathStrokeType(1.3f));
    }
    void fillTextEditorBackground(juce::Graphics &g, int, int, juce::TextEditor &) override {
        g.fillAll(field);
    }
    void drawTextEditorOutline(juce::Graphics &g, int w, int h, juce::TextEditor &e) override {
        g.setColour(e.hasKeyboardFocus(true) ? accent : border);
        g.drawRect(0, 0, w, h);
    }
    void drawLinearSlider(juce::Graphics &g, int x, int y, int w, int h, float pos, float lo, float hi,
                          juce::Slider::SliderStyle style, juce::Slider &s) override {
        if (style != juce::Slider::LinearHorizontal) {
            LookAndFeel_V4::drawLinearSlider(g, x, y, w, h, pos, lo, hi, style, s);
            return;
        }
        const float mid = float(y) + float(h) * .5f;
        g.setColour(border);
        g.fillRect(float(x), mid - 1, float(w), 2.f);
        g.setColour(s.isEnabled() ? accent : disabled);
        g.fillRect(float(x), mid - 1, std::max(0.f, pos - float(x)), 2.f);
        g.fillRect(pos - 2, mid - 7, 4.f, 14.f);
        if (s.hasKeyboardFocus(true)) {
            g.setColour(accent);
            g.drawRect(s.getLocalBounds(), 1);
        }
    }
    void drawScrollbar(juce::Graphics &g, juce::ScrollBar &, int x, int y, int w, int h, bool vertical,
                       int start, int size, bool over, bool down) override {
        g.setColour(over || down ? muted : border);
        if (vertical)
            g.fillRect(x + w / 2 - 3, start, 6, size);
        else
            g.fillRect(start, y + h / 2 - 3, size, 6);
    }
    void drawTableHeaderBackground(juce::Graphics &g, juce::TableHeaderComponent &h) override {
        g.fillAll(raised);
        g.setColour(border);
        g.drawHorizontalLine(h.getHeight() - 1, 0, float(h.getWidth()));
    }
    void drawTableHeaderColumn(juce::Graphics &g, juce::TableHeaderComponent &, const juce::String &name, int,
                               int w, int h, bool over, bool down, int flags) override {
        if (over || down)
            g.fillAll(hover);
        auto area = juce::Rectangle<int>(w, h).reduced(5, 0);
        if (flags &
            (juce::TableHeaderComponent::sortedForwards | juce::TableHeaderComponent::sortedBackwards)) {
            const auto arrow = area.removeFromRight(14).toFloat();
            const float x = arrow.getCentreX(), y = arrow.getCentreY();
            const float direction = flags & juce::TableHeaderComponent::sortedForwards ? -1.f : 1.f;
            juce::Path shape;
            shape.addTriangle(x - 4, y - direction * 2, x, y + direction * 3, x + 4, y - direction * 2);
            g.setColour(accent);
            g.fillPath(shape);
        }
        g.setColour(muted);
        g.setFont(juce::FontOptions(13, juce::Font::bold));
        g.drawFittedText(name, area, juce::Justification::centredLeft, 1);
    }
    void drawPopupMenuBackground(juce::Graphics &g, int w, int h) override {
        g.fillAll(raised);
        g.setColour(border);
        g.drawRect(0, 0, w, h);
    }
};
} // namespace fader::ui
