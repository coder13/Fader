// SPDX-License-Identifier: AGPL-3.0-or-later
#include "Arrangement.h"
#include "AudioPlayback.h"
#include "AudioSetup.h"
#include "LibraryView.h"
#include "Theme.h"
#include <cmath>
#include <deque>
#include <fader/PlaybackPreparation.h>
#include <set>

using namespace fader;
namespace {
const auto ink = ui::text, paper = ui::background, accent = ui::accent, muted = ui::muted;
class Workspace final : public juce::Component, public juce::DragAndDropContainer, private juce::Timer {
  public:
    explicit Workspace(PlaybackPreparation &preparation, int fixtureMode = 0)
        : workers(2), libraryWorker(1), preparation(preparation), fixtureMode(fixtureMode) {
        setLookAndFeel(&look);
        tooltips.setLookAndFeel(&look);
        for (auto *c : std::array<juce::Component *, 18>{
                 &arrangement, &library, &save, &undo, &rewind, &play, &record, &resume, &zoom, &snap, &title,
                 &mode, &position, &status, &selectedTitle, &selectedMeta, &setMenuButton, &clipMenuButton})
            addAndMakeVisible(c);
        for (auto *c :
             std::array<juce::Component *, 5>{&newButton, &open, &exportButton, &prefsButton, &remove})
            addChildComponent(c);
        setMenuButton.onClick = [this] { showSetMenu(&setMenuButton); };
        clipMenuButton.onClick = [this] { showClipMenu(&clipMenuButton); };
        setMenuButton.setTooltip("New, open, export and preferences");
        clipMenuButton.setTooltip("Clip actions and retake options; also available on right-click");
        save.setTooltip("Save set (Cmd/Ctrl+S)");
        undo.setTooltip("Undo (Cmd/Ctrl+Z)");
        play.setTooltip("Play / stop (Space)");
        rewind.setTooltip("Return to the beginning");
        setWantsKeyboardFocus(true);
        for (auto *c : std::array<juce::Component *, 4>{&clipStart, &punchIn, &punchOut, &cue})
            addAndMakeVisible(c);
        for (int i = 0; i < 3; ++i) {
            addAndMakeVisible(parameterButtons[static_cast<std::size_t>(i)]);
            addAndMakeVisible(controls[static_cast<std::size_t>(i)]);
            auto &button = parameterButtons[static_cast<std::size_t>(i)];
            button.setButtonText(i == 0 ? "Volume" : i == 1 ? "Low cut (HP)" : "High cut (LP)");
            button.setClickingTogglesState(true);
            button.setRadioGroupId(1);
            button.onClick = [this, i] {
                arrangement.parameter = i;
                arrangement.refresh();
            };
            auto &control = controls[static_cast<std::size_t>(i)];
            control.setRange(0, 1, 0.001);
            control.setSliderStyle(juce::Slider::LinearHorizontal);
            control.setTextBoxStyle(juce::Slider::TextBoxRight, false, 80, 24);
            control.textFromValueFunction = [i](double v) {
                if (i == 0)
                    return v <= 0 ? juce::String("Mute") : juce::String(20 * std::log10(v), 1) + " dB";
                if ((i == 1 && v <= 0) || (i == 2 && v >= 1))
                    return juce::String("Off");
                const auto hz = cutoffHz(float(v));
                return hz >= 1000 ? juce::String(hz / 1000, 2) + " kHz" : juce::String(hz, 0) + " Hz";
            };
            control.setTextBoxIsEditable(false);
            control.setName(i == 0   ? "Clip volume"
                            : i == 1 ? "Clip high-pass cutoff"
                                     : "Clip low-pass cutoff");
            control.setValue(i == 2 ? 1 : 0, juce::dontSendNotification);
            control.updateText();
            control.onDragStart = [this] {
                if (!engine && !busy && editableSelection()) {
                    checkpoint();
                    sliderGesture = true;
                }
            };
            control.onDragEnd = [this] { sliderGesture = false; };
            control.onValueChange = [this, i] {
                changeControl(i, float(controls[static_cast<std::size_t>(i)].getValue()));
            };
        }
        parameterButtons[0].setToggleState(true, juce::dontSendNotification);
        newButton.onClick = [this] {
            confirmDiscard([this] {
                stop();
                planSnapshot.reset();
                project = Project{};
                project.perClip = true;
                project.clips.clear();
                history.clear();
                arrangement.selected = -1;
                cursor = 0;
                dirty = false;
                currentFile = juce::File{};
                sync();
                setStatus("Drag songs from the library to start a set.");
            });
        };
        open.onClick = [this] { confirmDiscard([this] { chooseOpen(); }); };
        save.onClick = [this] { chooseSave(false); };
        exportButton.onClick = [this] { chooseSave(true); };
        prefsButton.onClick = [this] { showPreferences(); };
        undo.onClick = [this] {
            if (busy || history.empty())
                return;
            stop();
            planSnapshot.reset();
            project = history.back();
            history.pop_back();
            dirty = true;
            sync();
        };
        rewind.onClick = [this] {
            stop();
            cursor = 0;
            sync();
            arrangement.revealPlayhead();
        };
        play.onClick = [this] {
            if (engine || preparing)
                stop();
            else
                start(false);
        };
        record.onClick = [this] {
            if (recording || preparing)
                stop();
            else
                start(true);
        };
        resume.onClick = [this] {
            if (engine) {
                stop();
                start(false);
            }
        };
        remove.onClick = [this] {
            if (!editableSelection() || busy)
                return;
            stop();
            checkpoint();
            project.clips.erase(project.clips.begin() + arrangement.selected);
            arrangement.selected = -1;
            dirty = true;
            sync();
        };
        clipStart.setInputRestrictions(10, "0123456789.");
        clipStart.onReturnKey = [this] {
            if (!editableSelection() || busy)
                return;
            stop();
            checkpoint();
            auto &c = project.clips[static_cast<std::size_t>(arrangement.selected)];
            c.start = std::clamp(static_cast<Frame>(clipStart.getText().getDoubleValue() * projectRate),
                                 Frame{0}, maxProjectFrames - c.length);
            dirty = true;
            sync();
        };
        for (auto *e : {&punchIn, &punchOut}) {
            e->setInputRestrictions(10, "0123456789.");
            e->setText("0");
            e->onTextChange = [this] { repaint(); };
            e->setTooltip("Absolute seconds; 0 means current position / clip end");
        }
        cue.setButtonText("Headphone cue");
        cue.onClick = [this] {
            if (!editableSelection() || busy)
                return;
            const auto enabled = cue.getToggleState();
            stop();
            planSnapshot.reset();
            project.clips[static_cast<std::size_t>(arrangement.selected)].cue = enabled;
            syncControls();
            setStatus("Cue uses output channels 3/4 on the selected audio device.");
        };
        zoom.setRange(2, 100, 0.5);
        zoom.setValue(10, juce::dontSendNotification);
        zoom.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        zoom.onValueChange = [this] { arrangement.zoom(zoom.getValue()); };
        snap.addItem("Snap off", 1);
        snap.addItem("Snap 1 second", 2);
        snap.addItem("Snap 1 bar (120 BPM)", 3);
        snap.setSelectedId(1);
        snap.onChange = [this] {
            arrangement.snapSeconds = snap.getSelectedId() == 2 ? 1 : snap.getSelectedId() == 3 ? 2 : 0;
        };
        arrangement.project = &project;
        arrangement.beginEdit = [this] {
            if (busy || !project.perClip)
                return false;
            stop();
            checkpoint();
            return true;
        };
        arrangement.changed = [this] {
            cancelPreparation();
            planSnapshot.reset();
            dirty = true;
            syncControls();
            updateTitle();
        };
        arrangement.selectionChanged = [this](int selected) {
            if (recording && selected != recordingClip) {
                arrangement.selected = recordingClip;
                setStatus("Finish this take before selecting a different clip.");
            }
            syncControls();
        };
        arrangement.contextMenu = [this](int clip, int point) {
            if (clip < 0)
                showSetMenu(nullptr);
            else
                showClipMenu(nullptr, point);
        };
        library.preferences = [this] {
            if (prefsButton.isEnabled())
                showPreferences();
        };
        arrangement.seek = [this](Frame f) {
            if (busy)
                return;
            const bool continuePlayback = (engine || preparing) && !recording && !preparingCapture;
            stop();
            cursor = std::min(f, project.end());
            sync();
            if (continuePlayback && cursor < project.end())
                start(false);
        };
        arrangement.drop = [this](const juce::File &f, Frame at, int row) { addFile(f, at, row); };
        library.addTrack = [this](const TrackInfo &t) {
            int row = 0;
            for (const auto &c : project.clips)
                row = std::max(row, c.row + 1);
            addFile(t.file, cursor, row);
        };
        library.refresh = [this] { scan(); };
        project.perClip = true;
        project.clips.clear();
        setSize(1400, 940);
        sync();
        startTimerHz(30);
        library.setLoading();
        prefsButton.setEnabled(false);
        auto safe = juce::Component::SafePointer<Workspace>(this);
        if (fixtureMode) {
            library.setLibrary({}, {});
            library.refresh = {};
            setStatus("Synthetic UI fixture; no music-library scan or audio device.");
            return;
        }
        libraryWorker.addJob([safe] {
            try {
                auto settings = Preferences::load();
                settings.save();
                juce::MessageManager::callAsync([safe, settings] {
                    if (!safe)
                        return;
                    safe->preferences = settings;
                    safe->prefsButton.setEnabled(true);
                    safe->scan();
                });
            } catch (const std::exception &e) {
                const juce::String error = e.what();
                juce::MessageManager::callAsync([safe, error] {
                    if (!safe)
                        return;
                    safe->prefsButton.setEnabled(true);
                    safe->setStatus(error);
                });
            }
        });
        setStatus(
            "Drag songs to arrange. Move clip headers; draw envelopes on waveforms. Right-click for more.");
    }
    ~Workspace() override {
        stopTimer();
        stop();
        delete preferencesWindow.getComponent();
        workers.removeAllJobs(true, -1);
        libraryWorker.removeAllJobs(true, -1);
        setLookAndFeel(nullptr);
    }
    void requestQuit() {
        confirmDiscard([] { juce::JUCEApplication::getInstance()->quit(); });
    }
    void openFile(const juce::File &f, int fixtureCase = 0) {
        if (busy)
            return;
        stop();
        busy = true;
        setStatus("Opening set and checking its media...");
        auto safe = juce::Component::SafePointer<Workspace>(this);
        workers.addJob([safe, f, fixtureCase] {
            try {
                auto p = loadProject(f);
                juce::MessageManager::callAsync([safe, p = std::move(p), f, fixtureCase]() mutable {
                    if (!safe)
                        return;
                    safe->planSnapshot.reset();
                    safe->project = std::move(p);
                    safe->currentFile = f;
                    safe->history.clear();
                    safe->cursor = 0;
                    safe->arrangement.selected = safe->project.clips.empty() ? -1 : 0;
                    safe->dirty = false;
                    safe->busy = false;
                    safe->sync();
                    safe->setStatus(safe->project.perClip
                                        ? "Set opened. Clip automation and source identities restored."
                                        : "Version 1 set: original playback and export preserved. Choose New "
                                          "set to use clip automation.");
#if FADER_ENABLE_UI_FIXTURES
                    if (fixtureCase != 0)
                        safe->start(false);
#else
                    juce::ignoreUnused(fixtureCase);
#endif
                });
            } catch (const std::exception &e) {
                postError(safe, e.what());
            }
        });
    }
#if FADER_ENABLE_UI_FIXTURES
    // Developer fixture: exercise the real stop/recovery UI without opening hardware.
    // Only compiled with the explicit opt-in CMake flag; requires synthetic source media.
    void simulateSourceGap() {
        if (project.clips.empty() || project.end() < 1024)
            return;
        engine->deviceStarted(projectRate);
        std::array<float, 512> left{}, right{};
        float *out[]{left.data(), right.data()};
        engine->process(out, 2, 128, monotonicTime());
        engine->cancelSourceReads(); // deterministic missing PCM; no physical device involved
        engine->process(out, 2, 512, monotonicTime());
        play.setButtonText("Stop");
        sync();
    }
    void simulateInterruptedCapture() {
        if (project.clips.empty() || project.end() < 512)
            return;
        engine->arm(0, project.end(), 7, 0);
        engine->deviceStarted(projectRate);
        const auto now = monotonicTime();
        engine->submit(InputSource::ui, {engine->segment(), now, Param::volume, 0.2f, 0});
        std::array<float, 128> left{}, right{};
        float *out[]{left.data(), right.data()};
        engine->process(out, 2, 128, now);
        audioPlayback->audioDeviceStopped();
        takeIn = 0;
        takeOut = project.end();
        recording = true;
        stop(); // fault is inspected here before any timer runs
    }
#endif
    void paint(juce::Graphics &g) override {
        g.fillAll(paper);
        g.setColour(ui::panel);
        g.fillRect(0, 0, getWidth(), 54);
        g.setColour(ink);
        g.setFont(22);
        g.drawText("Fader", 20, 12, 85, 28, juce::Justification::left);
        g.setColour(ui::border);
        g.drawHorizontalLine(53, 0, float(getWidth()));
        const int x = getWidth() - 264;
        g.setColour(ui::panel);
        g.fillRect(getWidth() - 280, 108, 260, getHeight() - 410);
        g.setColour(muted);
        g.setFont(11);
        g.drawText("SELECTED CLIP", x, 120, 160, 20, juce::Justification::left);
        g.setFont(12);
        for (int i = 0; i < 3; ++i)
            g.drawText(i == 0   ? "VOLUME"
                       : i == 1 ? "LOW CUT / HIGH-PASS"
                                : "HIGH CUT / LOW-PASS",
                       x, 214 + i * 70, 228, 20, juce::Justification::left);
        g.drawText("START / SECONDS", x, 424, 228, 18, juce::Justification::left);
        if (retakeVisible)
            g.drawText("RETAKE / FROM - TO", x, 516, 228, 18, juce::Justification::left);
        else if (hasRetakeRange()) {
            g.setColour(accent);
            g.drawText("Retake range active - Clip menu", x, 516, 228, 20, juce::Justification::left);
        }
        g.drawText("Zoom", getWidth() - 460, 66, 42, 24, juce::Justification::left);
    }
    void resized() override {
        title.setBounds(110, 13, std::max(130, getWidth() - 760), 28);
        position.setBounds(getWidth() - 620, 13, 180, 28);
        mode.setBounds(getWidth() - 430, 13, 110, 28);
        undo.setBounds(getWidth() - 286, 13, 66, 28);
        save.setBounds(getWidth() - 212, 13, 66, 28);
        setMenuButton.setBounds(getWidth() - 134, 13, 114, 28);
        rewind.setBounds(20, 64, 36, 30);
        play.setBounds(64, 64, 70, 30);
        record.setBounds(142, 64, 112, 30);
        for (int i = 0; i < 3; ++i)
            parameterButtons[static_cast<std::size_t>(i)].setBounds(278 + i * 100, 64, 96, 30);
        snap.setBounds(594, 64, 170, 30);
        zoom.setBounds(getWidth() - 412, 64, 112, 30);
        arrangement.setBounds(20, 108, getWidth() - 320, getHeight() - 410);
        library.setBounds(20, getHeight() - 284, getWidth() - 40, 246);
        status.setBounds(20, getHeight() - 30, getWidth() - 40, 24);
        const int x = getWidth() - 264;
        clipMenuButton.setBounds(getWidth() - 100, 117, 64, 25);
        selectedTitle.setBounds(x, 148, 228, 32);
        selectedMeta.setBounds(x, 180, 228, 26);
        for (int i = 0; i < 3; ++i)
            controls[static_cast<std::size_t>(i)].setBounds(x, 237 + i * 70, 228, 28);
        clipStart.setBounds(x, 448, 228, 26);
        cue.setBounds(x, 480, 228, 28);
        punchIn.setBounds(x, 539, 106, 26);
        punchOut.setBounds(x + 116, 539, 112, 26);
        punchIn.setVisible(retakeVisible);
        punchOut.setVisible(retakeVisible);
        resume.setBounds(x, 580, 228, 28);
    }
    bool keyPressed(const juce::KeyPress &key) override {
        const auto mods = key.getModifiers();
        if (mods.isCommandDown()) {
            switch (key.getKeyCode()) {
            case 'N':
                newButton.onClick();
                return true;
            case 'O':
                open.onClick();
                return true;
            case 'S':
                save.onClick();
                return true;
            case 'Z':
                undo.onClick();
                return true;
            case ',':
                if (prefsButton.isEnabled())
                    showPreferences();
                return true;
            case 'E':
                if (mods.isShiftDown()) {
                    exportButton.onClick();
                    return true;
                }
                break;
            }
        }
        auto *focused = juce::Component::getCurrentlyFocusedComponent();
        const bool typing = focused && (dynamic_cast<juce::TextEditor *>(focused) ||
                                        focused->findParentComponentOfClass<juce::TextEditor>());
        if (!typing && key.getKeyCode() == juce::KeyPress::spaceKey) {
            play.onClick();
            return true;
        }
        if (!typing && key.getKeyCode() == juce::KeyPress::F10Key && mods.isShiftDown()) {
            showClipMenu(&clipMenuButton);
            return true;
        }
        return false;
    }
    void mouseDown(const juce::MouseEvent &e) override {
        if (e.mods.isPopupMenu())
            showSetMenu(nullptr);
    }

  private:
    bool hasRetakeRange() const {
        return punchIn.getText().getDoubleValue() > 0 || punchOut.getText().getDoubleValue() > 0;
    }
    void showSetMenu(juce::Component *anchor) {
        juce::PopupMenu menu;
        menu.setLookAndFeel(&look);
        menu.addSectionHeader("Set");
        menu.addItem(1, "New set", !busy);
        menu.addItem(2, "Open set...", !busy);
        menu.addItem(3, "Save set...", !busy);
        menu.addItem(4, "Export WAV...", !busy && project.end() > 0);
        menu.addSeparator();
        menu.addItem(5, "Preferences...", !busy && prefsButton.isEnabled());
        menu.addSeparator();
        menu.addItem(6, "Save incomplete take...", !busy && incompleteTake != nullptr);
        menu.addItem(7, "Discard incomplete take...", !busy && incompleteTake != nullptr);
        auto options =
            juce::PopupMenu::Options().withTargetComponent(anchor ? anchor : this).withMinimumWidth(210);
        if (!anchor)
            options = options.withMousePosition();
        menu.showMenuAsync(options, [safe = juce::Component::SafePointer<Workspace>(this)](int action) {
            if (!safe || safe->busy)
                return;
            switch (action) {
            case 1:
                safe->newButton.onClick();
                break;
            case 2:
                safe->open.onClick();
                break;
            case 3:
                safe->save.onClick();
                break;
            case 4:
                safe->exportButton.onClick();
                break;
            case 5:
                if (safe->prefsButton.isEnabled())
                    safe->showPreferences();
                break;
            case 6:
                safe->chooseIncompleteSave();
                break;
            case 7:
                safe->discardIncomplete();
                break;
            }
        });
    }
    void showClipMenu(juce::Component *anchor, int point = -1) {
        const int clip = arrangement.selected, parameter = arrangement.parameter;
        const bool enabled = editableSelection() && !busy && !recording;
        juce::PopupMenu menu;
        menu.setLookAndFeel(&look);
        menu.addSectionHeader("Selected clip");
        for (int p = 0; p < 3; ++p)
            menu.addItem(10 + p,
                         p == 0   ? "Show volume"
                         : p == 1 ? "Show low cut (HP)"
                                  : "Show high cut (LP)",
                         enabled, p == parameter);
        if (point >= 0) {
            menu.addSeparator();
            menu.addItem(20, "Delete automation point", enabled);
        }
        menu.addSeparator();
        menu.addItem(30, "Show retake range", enabled, retakeVisible);
        menu.addItem(31, "Clear retake range",
                     enabled && (punchIn.getText() != "0" || punchOut.getText() != "0"));
        menu.addSeparator();
        menu.addItem(40, "Remove clip", enabled);
        auto options =
            juce::PopupMenu::Options().withTargetComponent(anchor ? anchor : this).withMinimumWidth(220);
        if (!anchor)
            options = options.withMousePosition();
        menu.showMenuAsync(options, [safe = juce::Component::SafePointer<Workspace>(this), clip, parameter,
                                     point](int action) {
            if (!safe || action == 0 || safe->busy || safe->recording || safe->arrangement.selected != clip ||
                !safe->editableSelection())
                return;
            if (action >= 10 && action <= 12) {
                const int p = action - 10;
                safe->arrangement.parameter = p;
                safe->parameterButtons[static_cast<std::size_t>(p)].setToggleState(
                    true, juce::dontSendNotification);
                safe->arrangement.refresh();
            } else if (action == 20) {
                safe->stop();
                auto &points = safe->project.clips[static_cast<std::size_t>(clip)]
                                   .automation[static_cast<std::size_t>(parameter)]
                                   .points;
                if (point >= 0 && static_cast<std::size_t>(point) < points.size()) {
                    safe->checkpoint();
                    points.erase(points.begin() + point);
                    safe->dirty = true;
                    safe->sync();
                }
            } else if (action == 30) {
                safe->retakeVisible = !safe->retakeVisible;
                safe->resized();
                safe->repaint();
            } else if (action == 31) {
                safe->punchIn.setText("0");
                safe->punchOut.setText("0");
            } else if (action == 40)
                safe->remove.onClick();
        });
    }
    bool editableSelection() const {
        return project.perClip && arrangement.selected >= 0 &&
               static_cast<std::size_t>(arrangement.selected) < project.clips.size();
    }
    void setStatus(const juce::String &s) {
        status.setText(s, juce::dontSendNotification);
    }
    static void postError(juce::Component::SafePointer<Workspace> safe, juce::String error) {
        juce::MessageManager::callAsync([safe, error] {
            if (safe) {
                safe->busy = false;
                safe->sync();
                safe->setStatus(error);
            }
        });
    }
    void checkpoint() {
        cancelPreparation();
        planSnapshot.reset();
        history.push_back(project);
        undo.setEnabled(!busy);
        if (history.size() > 24)
            history.erase(history.begin());
    }
    void updateTitle() {
        title.setText(
            (currentFile == juce::File{} ? "Untitled set" : currentFile.getFileNameWithoutExtension()) +
                (dirty ? " *" : ""),
            juce::dontSendNotification);
    }
    void sync() {
        arrangement.editable = !busy && project.perClip;
        arrangement.playhead = cursor;
        if (arrangement.selected >= static_cast<int>(project.clips.size()))
            arrangement.selected = -1;
        arrangement.refresh();
        syncControls();
        updateTitle();
    }
    void syncControls() {
        const bool enabled = editableSelection() && !busy;
        for (auto &c : controls)
            c.setEnabled(enabled);
        remove.setEnabled(enabled);
        clipStart.setEnabled(enabled);
        cue.setEnabled(enabled);
        record.setEnabled(enabled);
        clipMenuButton.setEnabled(enabled);
        undo.setEnabled(!busy && !history.empty());
        save.setEnabled(!busy);
        arrangement.editable = !busy && project.perClip;
        if (arrangement.selected < 0 ||
            static_cast<std::size_t>(arrangement.selected) >= project.clips.size()) {
            selectedTitle.setText("Select a clip", juce::dontSendNotification);
            selectedMeta.setText("Its volume and filters live here.", juce::dontSendNotification);
            return;
        }
        const auto &c = project.clips[static_cast<std::size_t>(arrangement.selected)];
        selectedTitle.setText(juce::String(c.name), juce::dontSendNotification);
        selectedMeta.setText((c.bpm > 0 ? juce::String(c.bpm, 2) + " BPM" : "BPM unknown") + " | " +
                                 (c.key.empty() ? juce::String("Key unknown") : juce::String(c.key)),
                             juce::dontSendNotification);
        if (!sliderGesture)
            for (std::size_t i = 0; i < 3; ++i)
                controls[i].setValue(c.automation[i].at(std::clamp(cursor - c.start, Frame{0}, c.length)),
                                     juce::dontSendNotification);
        if (!clipStart.hasKeyboardFocus(true))
            clipStart.setText(juce::String(double(c.start) / projectRate, 3));
        cue.setToggleState(c.cue, juce::dontSendNotification);
    }
    void changeControl(int parameter, float value) {
        if (busy || !editableSelection())
            return;
        const auto selected = static_cast<unsigned>(arrangement.selected);
        if (engine) {
            if (!engine->submit(InputSource::ui, {engine->segment(), monotonicTime(),
                                                  static_cast<Param>(parameter), value, selected})) {
                captureLost = recording;
                setStatus(
                    recording
                        ? "Control input was lost. Applied actions will be retained as an incomplete take."
                        : "Control input was lost. Stop and restart playback before continuing.");
            } else if (!recording) {
                overriding = true;
                setStatus("Temporary override. Resume automation returns to the saved envelopes.");
            }
            return;
        }
        if (!sliderGesture)
            checkpoint();
        cancelPreparation();
        planSnapshot.reset();
        auto &c = project.clips[selected];
        auto &curve = c.automation[static_cast<std::size_t>(parameter)];
        const auto frame = cursor >= c.start && cursor < c.start + c.length ? cursor - c.start : Frame{0};
        auto at = std::lower_bound(curve.points.begin(), curve.points.end(), frame,
                                   [](auto p, Frame f) { return p.frame < f; });
        if (at != curve.points.end() && at->frame == frame) {
            at->value = value;
            at->hold = false;
        } else
            curve.points.insert(at, {frame, value});
        dirty = true;
        updateTitle();
        arrangement.refresh();
    }
    void scan() {
        if (scanning) {
            rescanPending = true;
            return;
        }
        scanning = true;
        library.setLoading();
        auto settings = preferences;
        auto safe = juce::Component::SafePointer<Workspace>(this);
        libraryWorker.addJob([safe, settings] {
            try {
                auto result = scanLibrary(settings);
                juce::MessageManager::callAsync([safe, result = std::move(result), settings]() mutable {
                    if (safe) {
                        safe->library.setLibrary(std::move(result), settings.musicFolder);
                        safe->scanning = false;
                        if (safe->rescanPending) {
                            safe->rescanPending = false;
                            safe->scan();
                        }
                    }
                });
            } catch (const std::exception &e) {
                juce::String error = e.what();
                juce::MessageManager::callAsync([safe, error] {
                    if (safe) {
                        safe->scanning = false;
                        safe->setStatus(error);
                        if (safe->rescanPending) {
                            safe->rescanPending = false;
                            safe->scan();
                        }
                    }
                });
            }
        });
    }
    void addFile(const juce::File &file, Frame start, int row) {
        if (busy) {
            if (importing && queuedImports.size() < 64) {
                queuedImports.push_back({file, start, row});
                setStatus("Loading tracks... " + juce::String(queuedImports.size()) + " queued");
                return;
            }
            setStatus("Please wait for the current import to finish.");
            return;
        }
        if (!project.perClip) {
            setStatus("Choose New set to create a clip-based arrangement.");
            return;
        }
        if (project.clips.size() >= 64) {
            queuedImports.clear();
            setStatus("This prototype supports 64 clips per set.");
            return;
        }
        stop();
        busy = true;
        importing = true;
        sync();
        setStatus("Loading track...");
        auto base = project;
        TrackInfo info;
        if (const auto *found = library.find(file))
            info = *found;
        auto safe = juce::Component::SafePointer<Workspace>(this);
        workers.addJob([safe, base = std::move(base), file, start, row, info]() mutable {
            try {
                Clip c;
                bool reused = false;
                for (const auto &existing : base.clips)
                    if (existing.path == file.getFullPathName().toStdString()) {
                        c = existing;
                        c.automation = Clip{}.automation;
                        c.cue = false;
                        reused = true;
                        break;
                    }
                if (!reused)
                    c = loadClip(file);
                c.start = std::clamp(start, Frame{0}, maxProjectFrames - c.length);
                c.row = std::clamp(row, 0, 4095);
                if (info.title.isNotEmpty())
                    c.name = info.title.toStdString();
                c.artist = info.artist.toStdString();
                c.key = info.key.toStdString();
                c.bpm = info.bpm;
                c.hasGrid = info.hasGrid;
                c.firstBeatSeconds = info.firstBeat;
                base.clips.push_back(std::move(c));
                std::set<const AudioData *> seen;
                std::size_t bytes = 0;
                for (const auto &clip : base.clips)
                    if (clip.audio && seen.insert(clip.audio.get()).second)
                        bytes += clip.audio->peaks.size() * sizeof(float);
                if (bytes > 32ULL * 1024 * 1024)
                    throw std::runtime_error("This prototype has a 32 MiB waveform-cache budget per set.");
                base.validate();
                juce::MessageManager::callAsync([safe, base = std::move(base)]() mutable {
                    if (!safe)
                        return;
                    safe->history.clear();
                    safe->planSnapshot.reset();
                    safe->project = std::move(base);
                    safe->arrangement.selected = static_cast<int>(safe->project.clips.size() - 1);
                    safe->busy = false;
                    safe->dirty = true;
                    safe->sync();
                    safe->setStatus(
                        "Track added. Drag its header to move it; draw an envelope over the waveform.");
                    safe->continueImports();
                });
            } catch (const std::exception &e) {
                juce::String error = e.what();
                juce::MessageManager::callAsync([safe, error] {
                    if (!safe)
                        return;
                    safe->busy = false;
                    safe->sync();
                    safe->setStatus(error);
                    safe->continueImports();
                });
            }
        });
    }
    void continueImports() {
        importing = false;
        if (queuedImports.empty())
            return;
        const auto next = queuedImports.front();
        queuedImports.pop_front();
        addFile(next.file, next.start, next.row);
    }
    void confirmDiscard(std::function<void()> action) {
        if (busy) {
            setStatus("Wait for the current operation to finish.");
            return;
        }
        stop(); // collect a running take and its failure state before deciding whether work is unsaved
        if (!dirty && !incompleteTake) {
            action();
            return;
        }
        auto safe = juce::Component::SafePointer<Workspace>(this);
        juce::AlertWindow::showAsync(
            juce::MessageBoxOptions{}
                .withIconType(juce::MessageBoxIconType::QuestionIcon)
                .withAssociatedComponent(this)
                .withTitle("Unsaved work")
                .withMessage(
                    "Save unsaved set changes and any incomplete take from the Set menu before continuing.")
                .withButton("Discard changes", 1)
                .withButton("Cancel", 0),
            [safe, action = std::move(action)](int result) {
                if (safe && result == 1) {
                    safe->incompleteTake.reset();
                    action();
                }
            });
    }
    void chooseOpen() {
        if (busy)
            return;
        stop();
        chooser = std::make_unique<juce::FileChooser>("Open Fader set", currentFile, "*.faderset");
        chooser->launchAsync(
            juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [safe = juce::Component::SafePointer<Workspace>(this)](const juce::FileChooser &c) {
                if (safe && c.getResult().existsAsFile())
                    safe->openFile(c.getResult());
            });
    }
    void chooseSave(bool exporting) {
        if (busy)
            return;
        stop();
        chooser = std::make_unique<juce::FileChooser>(exporting ? "Export WAV" : "Save Fader set",
                                                      exporting ? juce::File{} : currentFile,
                                                      exporting ? "*.wav" : "*.faderset");
        chooser->launchAsync(
            juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles |
                juce::FileBrowserComponent::warnAboutOverwriting,
            [safe = juce::Component::SafePointer<Workspace>(this),
             exporting](const juce::FileChooser &choice) {
                if (!safe || choice.getResult() == juce::File{})
                    return;
                auto file = choice.getResult().withFileExtension(exporting ? "wav" : "faderset");
                auto snapshot = safe->project;
                safe->busy = true;
                safe->sync();
                safe->setStatus(exporting ? "Rendering WAV..." : "Saving set...");
                safe->workers.addJob([safe, snapshot = std::move(snapshot), file, exporting] {
                    try {
                        if (exporting)
                            exportWav(snapshot, file);
                        else
                            saveProject(snapshot, file);
                        juce::MessageManager::callAsync([safe, file, exporting] {
                            if (!safe)
                                return;
                            safe->busy = false;
                            if (!exporting) {
                                safe->dirty = false;
                                safe->currentFile = file;
                            }
                            safe->sync();
                            safe->setStatus(exporting
                                                ? "WAV exported: stereo, 24-bit, 48 kHz."
                                                : "Set saved with clip automation and source references.");
                        });
                    } catch (const std::exception &e) {
                        postError(safe, e.what());
                    }
                });
            });
    }
    void chooseIncompleteSave() {
        if (busy || !incompleteTake)
            return;
        stop();
        chooser = std::make_unique<juce::FileChooser>("Save incomplete take archive", juce::File{},
                                                      "*.fadercapture");
        chooser->launchAsync(
            juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles |
                juce::FileBrowserComponent::warnAboutOverwriting,
            [safe = juce::Component::SafePointer<Workspace>(this)](const juce::FileChooser &choice) {
                if (!safe || !safe->incompleteTake || choice.getResult() == juce::File{})
                    return;
                const auto file = choice.getResult().withFileExtension("fadercapture");
                auto snapshot = safe->incompleteTake;
                safe->busy = true;
                safe->sync();
                safe->setStatus("Saving incomplete actions and the original set...");
                safe->workers.addJob([safe, snapshot, file] {
                    try {
                        saveIncompleteTake(*snapshot, file);
                        juce::MessageManager::callAsync([safe, snapshot] {
                            if (!safe)
                                return;
                            if (safe->incompleteTake == snapshot)
                                safe->incompleteTake.reset();
                            safe->busy = false;
                            safe->sync();
                            safe->setStatus("Incomplete take archived. This prototype cannot reopen "
                                            "capture archives yet. Your set envelopes are unchanged.");
                        });
                    } catch (const std::exception &e) {
                        postError(safe, e.what());
                    }
                });
            });
    }
    void discardIncomplete() {
        if (busy || !incompleteTake)
            return;
        auto snapshot = incompleteTake;
        juce::AlertWindow::showAsync(
            juce::MessageBoxOptions{}
                .withIconType(juce::MessageBoxIconType::QuestionIcon)
                .withAssociatedComponent(this)
                .withTitle("Discard incomplete take?")
                .withMessage("This removes its retained actions. Save the incomplete take first if you "
                             "want to keep them. Your existing set envelopes will stay unchanged.")
                .withButton("Discard take", 1)
                .withButton("Cancel", 0),
            [safe = juce::Component::SafePointer<Workspace>(this), snapshot](int result) {
                if (safe && result == 1 && safe->incompleteTake == snapshot) {
                    safe->incompleteTake.reset();
                    safe->setStatus("Incomplete take discarded. Existing envelopes are intact.");
                }
            });
    }
    bool ensureAudio() {
        const auto error = configureAudioOutput(audio, audioConfigured);
        if (error.isNotEmpty()) {
            setStatus(error);
            return false;
        }
        audioConfigured = true;
        return true;
    }
    void start(bool capture) {
        if (busy || project.end() == 0)
            return;
        if (capture && !editableSelection())
            return;
        stop();
        if (capture && incompleteTake) {
            setStatus("Save or discard the incomplete take from the Set menu before recording again.");
            return;
        }
        if (!fixtureMode && !ensureAudio())
            return;
        if (cursor >= project.end())
            cursor = 0;
        Frame in = cursor, out = project.end();
        int selected = arrangement.selected;
        if (capture) {
            const auto &c = project.clips[static_cast<std::size_t>(selected)];
            in = std::max(cursor, c.start);
            if (in >= c.start + c.length)
                in = c.start;
            const auto requestedIn = static_cast<Frame>(punchIn.getText().getDoubleValue() * projectRate);
            if (requestedIn > 0)
                in = std::max(c.start, requestedIn);
            out = c.start + c.length;
            const auto requestedOut = static_cast<Frame>(punchOut.getText().getDoubleValue() * projectRate);
            if (requestedOut > 0)
                out = std::min(out, requestedOut);
            if (in >= out) {
                setStatus("Retake start must be before its end and inside the selected clip.");
                return;
            }
            cursor = in;
        }
        if (!planSnapshot)
            planSnapshot = std::make_shared<const Project>(project);
        preparing = true;
        preparingCapture = capture;
        preparingClip = selected;
        takeIn = in;
        takeOut = out;
        requestGeneration = preparation.request(planSnapshot, cursor, ++nextSegment);
        play.setButtonText("Cancel");
        sync();
        bool sequential = false;
        for (const auto &clip : project.clips)
            sequential |= clip.audio && clip.audio->stream && clip.audio->stream->sequentialSeek();
        setStatus(sequential
                      ? "Preparing playback. An uncached MP3 needs a slower seek; Cancel is available."
                      : "Preparing playback... Cancel, or click the ruler to choose another position.");
    }
    void cancelPreparation() {
        if (!preparing)
            return;
        preparing = false;
        preparingCapture = false;
        preparation.cancel();
        play.setButtonText("Play");
        setStatus("Playback preparation cancelled.");
    }
    void acceptPreparation() {
        auto result = preparation.poll();
        if (!result || !preparing || result->generation != requestGeneration)
            return;
        preparing = false;
        if (!result->playback) {
            preparingCapture = false;
            play.setButtonText("Play");
            setStatus(juce::String(result->error));
            sync();
            return;
        }
        engine = result->playback;
        take.clear();
        captureLost = false;
        recording = preparingCapture;
        preparingCapture = false;
        recordingClip = preparingClip;
        overriding = false;
        sourceGapSeen = false;
        if (recording)
            engine->arm(takeIn, takeOut, 7, recordingClip);
        audioPlayback = std::make_unique<AudioPlayback>(*engine);
        if (!fixtureMode)
            audioPlayback->attach(audio);
        play.setButtonText("Stop");
        record.setButtonText(recording ? "Finish take" : "Record clip");
        sync();
        setStatus(
            recording
                ? "Recording the selected clip's volume and filters. Finish take keeps the editable gestures."
                : "Playing saved automation. Fader moves are temporary until you record them.");
#if FADER_ENABLE_UI_FIXTURES
        if (fixtureMode == 1)
            simulateInterruptedCapture();
        else if (fixtureMode == 2)
            simulateSourceGap();
        else if (fixtureMode == 3) {
            engine->deviceStarted(projectRate);
            setStatus("Synthetic seek ready at the latest position; no audio device is open.");
        }
#endif
    }
    void drain() {
        if (!engine)
            return;
        Event e;
        for (unsigned budget = 16384; budget > 0 && engine->captured(e); --budget) {
            if (take.size() < 200000)
                take.push_back(e);
            else
                engine->markIncomplete(captureStorageFull);
        }
    }
    void stop() {
        cancelPreparation();
        if (!engine)
            return;
        audioPlayback->detach();
        audioPlayback.reset();
        cursor = engine->position();
        const bool hadMediaGaps = engine->starvationFrames() > 0 || engine->streamStats().failedSources > 0;
        drain();
        if (recording) {
            if (captureLost && !engine->captureFailed())
                engine->markIncomplete(inputOverflow);
            if (engine->captureFailed()) {
                incompleteTake = std::make_shared<IncompleteTake>(
                    IncompleteTake{engine->plan(), std::move(take), takeIn, takeOut, engine->stats()});
                setStatus("Take incomplete. Applied actions are retained; save them from the Set menu. "
                          "Existing envelopes are intact.");
            } else {
                try {
                    checkpoint();
                    commitTake(project, take, takeIn, std::min(cursor, takeOut));
                    dirty = true;
                    setStatus(hadMediaGaps
                                  ? "Actions saved, but playback had audio gaps. Replay from the sources may "
                                    "sound different. Undo restores previous envelopes."
                                  : "Take saved on the clip. Undo restores its previous envelopes.");
                } catch (const std::exception &e) {
                    setStatus(e.what());
                }
            }
        } else
            setStatus(engine->clockFailed() ? "Playback stopped because the audio device clock changed. "
                                              "Press Play to start a new playback segment."
                      : hadMediaGaps        ? "Playback stopped after source gaps. Check the source files "
                                              "and storage before playing again."
                                            : "Playback stopped.");
        engine = nullptr;
        preparation.cancel(); // worker owns retirement; the callback is already detached
        recording = false;
        overriding = false;
        play.setButtonText("Play");
        record.setButtonText("Record clip");
        sync();
    }
    void showPreferences() {
        if (busy)
            return;
        stop();
        if (preferencesWindow) {
            preferencesWindow->toFront(true);
            return;
        }
        const auto error = configureAudioOutput(audio, audioConfigured);
        audioConfigured = error.isEmpty();
        if (error.isNotEmpty())
            setStatus(error);
        auto safe = juce::Component::SafePointer<Workspace>(this);
        juce::DialogWindow::LaunchOptions options;
        options.content.setOwned(new PreferencesPanel(audio, preferences, [safe](Preferences p) {
            if (safe) {
                safe->preferences = std::move(p);
                safe->scan();
                safe->setStatus("Preferences saved.");
            }
        }));
        options.content->setLookAndFeel(&look);
        options.dialogTitle = "Fader Preferences";
        options.dialogBackgroundColour = paper;
        options.useNativeTitleBar = true;
        options.escapeKeyTriggersCloseButton = true;
        options.resizable = false;
        preferencesWindow = options.launchAsync();
    }
    void timerCallback() override {
        acceptPreparation();
        if (engine) {
            cursor = engine->position();
            if (!sourceGapSeen &&
                (engine->starvationFrames() > 0 || engine->streamStats().failedSources > 0)) {
                sourceGapSeen = true;
                setStatus("Audio source gap: missing audio fades out and back in. Check source files and "
                          "storage. Automation capture is reported separately.");
            }
            drain();
            CallbackMetric metric;
            for (unsigned budget = 1024; budget > 0 && engine->metric(metric); --budget)
                lastCallback = metric;
            if (cursor >= project.end() || engine->clockFailed() || (recording && engine->captureFailed()))
                stop();
            else if (!recording && !overriding && !sliderGesture)
                syncControls();
        }
        arrangement.playhead = cursor;
        arrangement.repaint();
        arrangement.refresh();
        position.setText(juce::String(int(cursor / projectRate) / 60) + ":" +
                             juce::String(double(cursor) / projectRate - 60 * (cursor / projectRate / 60), 2)
                                 .paddedLeft('0', 5) +
                             (engine && engine->peak() > 1 ? "  CLIPPING" : ""),
                         juce::dontSendNotification);
        mode.setText((busy || preparing) ? "Preparing"
                     : recording         ? (sourceGapSeen ? "REC GAP" : "RECORD")
                     : overriding        ? (sourceGapSeen ? "OVR GAP" : "OVERRIDE")
                     : engine            ? (sourceGapSeen ? "READ GAP" : "READ")
                     : incompleteTake    ? "INCOMPLETE"
                                         : "EDIT",
                     juce::dontSendNotification);
        mode.setColour(juce::Label::textColourId,
                       recording || incompleteTake || (engine && sourceGapSeen) ? ui::danger : accent);
        record.setColour(juce::TextButton::textColourOffId, recording ? ui::danger : ui::text);
        resume.setVisible(engine && overriding);
        if (!recording)
            record.setButtonText(hasRetakeRange() ? "Record range" : "Record clip");
        resume.setEnabled(engine && overriding);
    }
    ui::Theme look;
    juce::TooltipWindow tooltips{this, 650};
    Project project;
    Preferences preferences;
    Arrangement arrangement;
    LibraryView library;
    juce::TextButton newButton{"New set"}, open{"Open"}, save{"Save"}, exportButton{"Export"},
        prefsButton{"Preferences"}, undo{"Undo"}, rewind{"|<"}, play{"Play"}, record{"Record clip"},
        resume{"Resume automation"}, remove{"Remove selected clip"};
    juce::TextButton setMenuButton{"Set ..."}, clipMenuButton{"Clip ..."};
    bool retakeVisible = false;
    std::array<juce::TextButton, 3> parameterButtons;
    std::array<juce::Slider, 3> controls;
    juce::Slider zoom;
    juce::ComboBox snap;
    juce::Label title, mode, position, status, selectedTitle, selectedMeta;
    juce::TextEditor clipStart, punchIn, punchOut;
    juce::ToggleButton cue;
    juce::ThreadPool workers, libraryWorker;
    juce::AudioDeviceManager audio;
    PlaybackPreparation &preparation;
    const int fixtureMode;
    Playback *engine = nullptr; // borrowed from preparation until stop()
    std::shared_ptr<const Project> planSnapshot;
    std::uint64_t requestGeneration = 0;
    int preparingClip = -1;
    bool preparing = false, preparingCapture = false;
    std::unique_ptr<AudioPlayback> audioPlayback;
    std::shared_ptr<const IncompleteTake> incompleteTake;
    std::uint64_t nextSegment = 0;
    CallbackMetric lastCallback;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::Component::SafePointer<juce::DialogWindow> preferencesWindow;
    juce::File currentFile;
    std::vector<Project> history;
    std::vector<Event> take;
    Frame cursor = 0, takeIn = 0, takeOut = 0;
    struct PendingImport {
        juce::File file;
        Frame start;
        int row;
    };
    std::deque<PendingImport> queuedImports;
    int recordingClip = -1;
    bool busy = false, dirty = false, recording = false, overriding = false, captureLost = false,
         sourceGapSeen = false, sliderGesture = false, scanning = false, rescanPending = false,
         importing = false, audioConfigured = false;
};
class App final : public juce::JUCEApplication {
  public:
    const juce::String getApplicationName() override {
        return "Fader";
    }
    const juce::String getApplicationVersion() override {
        return "0.2.0";
    }
    void initialise(const juce::String &args) override {
        int fixture = 0;
        auto path = args.trim();
#if FADER_ENABLE_UI_FIXTURES
        fixture = path.startsWith("--incomplete-take-fixture ") ? 1
                  : path.startsWith("--source-gap-fixture ")    ? 2
                  : path.startsWith("--seek-fixture ")          ? 3
                                                                : 0;
        if (fixture)
            path = path.fromFirstOccurrenceOf(" ", false, false).trim();
#endif
        PlaybackPreparation::Factory factory;
#if FADER_ENABLE_UI_FIXTURES
        if (fixture == 3)
            factory = [](const Project &p, Frame at, std::uint64_t segment,
                         std::shared_ptr<std::atomic<bool>> cancelled) {
                // Deterministic delayed readiness for inspecting Cancel/supersede.
                for (unsigned i = 0; i < 5000 && !cancelled->load(); ++i)
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                return std::make_unique<Playback>(p, at, segment, Preparation{cancelled});
            };
#endif
        preparation = std::make_unique<PlaybackPreparation>(std::move(factory));
        window = std::make_unique<Window>(*preparation, fixture);
        path = path.unquoted();
        if (path.isNotEmpty()) {
            auto file = juce::File::getCurrentWorkingDirectory().getChildFile(path);
            if (file.existsAsFile())
                static_cast<Workspace *>(window->getContentComponent())->openFile(file, fixture);
        }
    }
    void shutdown() override {
        window.reset();
        preparation.reset(); // decoder joins after the window and its callbacks are gone
    }
    void systemRequestedQuit() override {
        if (window)
            static_cast<Workspace *>(window->getContentComponent())->requestQuit();
        else
            quit();
    }

  private:
    struct Window final : juce::DocumentWindow {
        explicit Window(PlaybackPreparation &preparation, int fixture)
            : DocumentWindow("Fader - arrangement prototype", paper, allButtons) {
            setUsingNativeTitleBar(true);
            setContentOwned(new Workspace(preparation, fixture), true);
            setResizable(true, false);
            setResizeLimits(1250, 920, 2400, 1700);
            centreWithSize(getWidth(), getHeight());
            setVisible(true);
        }
        void closeButtonPressed() override {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }
    };
    std::unique_ptr<PlaybackPreparation> preparation;
    std::unique_ptr<Window> window;
};
} // namespace
START_JUCE_APPLICATION(App)
