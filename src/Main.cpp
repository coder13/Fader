// SPDX-License-Identifier: AGPL-3.0-or-later
#include "Media.h"
#include <algorithm>
#include <cmath>
#include <functional>

using namespace fader;
namespace {
const juce::Colour ink{0xff243137}, paper{0xfff5f4ef}, teal{0xff167b78}, orange{0xffbf7144};
class Workspace;
class Timeline final : public juce::Component,
                       public juce::FileDragAndDropTarget,
                       public juce::DragAndDropTarget {
  public:
    explicit Timeline(Workspace &w) : owner(w) {}
    void paint(juce::Graphics &) override;
    bool isInterestedInFileDrag(const juce::StringArray &) override {
        return true;
    }
    void filesDropped(const juce::StringArray &, int, int) override;
    bool isInterestedInDragSource(const SourceDetails &details) override {
        return details.description.toString() == "Fader library file";
    }
    void itemDropped(const SourceDetails &) override;
    void mouseDown(const juce::MouseEvent &) override;
    void mouseDrag(const juce::MouseEvent &) override;
    void mouseUp(const juce::MouseEvent &) override;

  private:
    Workspace &owner;
    int draggingDeck = -1, pointIndex = -1;
    Frame dragStart = 0;
    float mouseStart = 0;
    double dragExtent = 30;
    int editingLane = 0;
    double extent() const;
    float x(Frame) const;
    Frame frame(float) const;
};

class Workspace final : public juce::Component,
                        public juce::DragAndDropContainer,
                        private juce::AudioIODeviceCallback,
                        private juce::Timer,
                        private juce::FileBrowserListener {
  public:
    Workspace()
        : directoryThread("Library scan"), directory(nullptr, directoryThread), library(directory),
          timeline(*this), workers(1) {
        setLookAndFeel(&look);
        look.setColour(juce::ResizableWindow::backgroundColourId, paper);
        look.setColour(juce::Label::textColourId, ink);
        look.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffe3e8e4));
        look.setColour(juce::TextButton::textColourOffId, ink);
        look.setColour(juce::Slider::thumbColourId, teal);
        look.setColour(juce::Slider::trackColourId, teal);
        look.setColour(juce::Slider::textBoxTextColourId, ink);
        look.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::white);
        look.setColour(juce::ToggleButton::textColourId, ink);
        look.setColour(juce::ToggleButton::tickColourId, teal);
        look.setColour(juce::ToggleButton::tickDisabledColourId, juce::Colour(0xff8a9790));
        look.setColour(juce::TextEditor::backgroundColourId, juce::Colours::white);
        look.setColour(juce::TextEditor::textColourId, ink);
        look.setColour(juce::DirectoryContentsDisplayComponent::textColourId, ink);
        const auto add = [this](juce::Component &c) { addAndMakeVisible(c); };
        for (auto *b : {&open, &save, &exportButton, &devices, &folder, &play, &record, &rewind, &undo})
            add(*b);
        for (auto *c : std::array<juce::Component *, 9>{&library, &timeline, &lane, &interpolation, &status,
                                                        &positionLabel, &punchLabel, &punchIn, &punchOut})
            add(*c);
        open.onClick = [this] { choose(false, false); };
        save.onClick = [this] { choose(true, false); };
        exportButton.onClick = [this] { choose(true, true); };
        devices.onClick = [this] { showDevices(); };
        folder.onClick = [this] { chooseFolder(); };
        play.onClick = [this] {
            if (engine)
                stop();
            else
                start(false);
        };
        record.onClick = [this] {
            if (recording)
                stop();
            else
                start(true);
        };
        rewind.onClick = [this] {
            stop();
            cursor = 0;
            timeline.repaint();
        };
        undo.onClick = [this] {
            if (busy)
                return;
            stop();
            if (!history.empty()) {
                project = history.back();
                history.pop_back();
                sync();
            }
        };
        lane.addItem("Deck A gain", 1);
        lane.addItem("Deck B gain", 2);
        lane.addItem("Crossfader", 3);
        lane.setSelectedId(1);
        lane.onChange = [this] {
            interpolation.setSelectedId(project.curves[static_cast<std::size_t>(selectedLane())].linear ? 2
                                                                                                        : 1,
                                        juce::dontSendNotification);
            timeline.repaint();
        };
        interpolation.addItem("Step targets", 1);
        interpolation.addItem("Linear envelope", 2);
        interpolation.setSelectedId(1);
        interpolation.onChange = [this] {
            if (busy)
                return;
            edit();
            project.curves[static_cast<std::size_t>(selectedLane())].linear =
                interpolation.getSelectedId() == 2;
            timeline.repaint();
        };
        punchLabel.setText("RETAKE RANGE (seconds; end 0 = set end)", juce::dontSendNotification);
        for (auto *e : {&punchIn, &punchOut}) {
            e->setInputRestrictions(9, "0123456789.");
            e->setText("0");
        }
        for (int i = 0; i < 3; ++i) {
            auto &s = sliders[static_cast<std::size_t>(i)];
            add(s);
            s.setRange(i == 2 ? -1.0 : 0.0, 1.0, 0.001);
            s.setSliderStyle(juce::Slider::LinearHorizontal);
            s.setTextBoxStyle(juce::Slider::TextBoxRight, false, 55, 22);
            s.onDragStart = [this] {
                if (!engine && !busy)
                    checkpoint();
            };
            s.onValueChange = [this, i] {
                if (busy)
                    return;
                auto value = static_cast<float>(sliders[static_cast<std::size_t>(i)].getValue());
                if (engine) {
                    if (!engine->command({engine->position(), static_cast<Param>(i), value})) {
                        captureLost = recording;
                        setStatus("Control queue is full; an active take will be discarded");
                    }
                } else
                    project.curves[static_cast<std::size_t>(i)].initial = value;
                timeline.repaint();
            };
        }
        for (int i = 0; i < 2; ++i) {
            const auto k = static_cast<std::size_t>(i);
            add(load[k]);
            add(cue[k]);
            add(startTime[k]);
            add(length[k]);
            add(rate[k]);
            load[k].setButtonText(i == 0 ? "Load A..." : "Load B...");
            load[k].onClick = [this, i] { chooseTrack(i); };
            cue[k].setButtonText("Headphone cue");
            cue[k].onClick = [this, k] {
                if (busy)
                    return;
                edit();
                project.clips[k].cue = cue[k].getToggleState();
            };
            for (auto *e : {&startTime[k], &length[k], &rate[k]})
                e->setInputRestrictions(10, "0123456789.");
            auto update = [this, k] {
                if (busy)
                    return;
                edit();
                auto candidate = project;
                auto &c = candidate.clips[k];
                c.start = static_cast<Frame>(startTime[k].getText().getDoubleValue() * projectRate);
                c.length = static_cast<Frame>(length[k].getText().getDoubleValue() * projectRate);
                c.rate = rate[k].getText().getDoubleValue();
                try {
                    candidate.validate();
                    project = std::move(candidate);
                    setStatus("Clip updated");
                } catch (const std::exception &e) {
                    setStatus(e.what());
                }
                sync();
            };
            startTime[k].onReturnKey = update;
            length[k].onReturnKey = update;
            rate[k].onReturnKey = update;
        }
        directoryThread.startThread();
        directory.setDirectory(juce::File::getSpecialLocation(juce::File::userMusicDirectory), true, true);
        library.addListener(this);
        add(libraryCaption);
        libraryCaption.setText("Double-click -> selected deck\nOr drag a file onto a waveform lane",
                               juce::dontSendNotification);
        library.setDragAndDropDescription("Fader library file");
        setSize(1240, 900);
        sync();
        startTimerHz(30);
        setStatus("Load two tracks to begin. S4 hardware qualification pending.");
    }
    ~Workspace() override {
        stopTimer();
        stop();
        delete audioDialog.getComponent();
        workers.removeAllJobs(true, -1);
        library.removeListener(this);
        directoryThread.stopThread(3000);
        setLookAndFeel(nullptr);
    }
    void paint(juce::Graphics &g) override {
        g.fillAll(paper);
        g.setColour(ink);
        g.setFont(30);
        g.drawText("Fader", 24, 12, 180, 40, juce::Justification::centredLeft);
        g.setFont(14);
        g.setColour(juce::Colour(0xff66736d));
        g.drawText("Prepared sets | native prototype", 170, 19, 400, 30, juce::Justification::centredLeft);
        g.setColour(juce::Colour(0xffd6dfd8));
        g.drawLine(256, 112, 256, float(getHeight() - 52));
        g.setFont(12);
        g.setColour(ink);
        const int sliderWidth = (getWidth() - 298) / 3;
        g.drawText("A gain", 280, getHeight() - 177, 75, 20, juce::Justification::centredLeft);
        g.drawText("B gain", 280 + sliderWidth, getHeight() - 177, 75, 20, juce::Justification::centredLeft);
        g.drawText("Crossfader", 280 + 2 * sliderWidth, getHeight() - 177, 100, 20,
                   juce::Justification::centredLeft);
    }
    void paintOverChildren(juce::Graphics &g) override {
        g.setFont(12);
        g.setColour(ink);
        for (int i = 0; i < 2; ++i) {
            const int y = 170 + i * 165;
            g.drawText("START (s)", getWidth() - 356, y, 100, 18, juce::Justification::centredLeft);
            g.drawText("LENGTH (s)", getWidth() - 244, y, 100, 18, juce::Justification::centredLeft);
            g.drawText("SPEED", getWidth() - 132, y, 90, 18, juce::Justification::centredLeft);
        }
    }
    void resized() override {
        int bx = 24;
        for (auto *b : {&open, &save, &exportButton, &devices}) {
            b->setBounds(bx, 66, 112, 30);
            bx += 122;
        }
        folder.setBounds(24, 116, 208, 28);
        library.setBounds(16, 154, 224, getHeight() - 266);
        libraryCaption.setBounds(24, getHeight() - 102, 214, 48);
        timeline.setBounds(274, 116, getWidth() - 298, getHeight() - 320);
        for (int i = 0; i < 2; ++i) {
            const auto k = static_cast<std::size_t>(i);
            int y = 136 + i * 165;
            load[k].setBounds(getWidth() - 356, y, 106, 28);
            cue[k].setBounds(getWidth() - 238, y, 200, 28);
            startTime[k].setBounds(getWidth() - 356, y + 54, 100, 24);
            length[k].setBounds(getWidth() - 244, y + 54, 100, 24);
            rate[k].setBounds(getWidth() - 132, y + 54, 90, 24);
        }
        lane.setBounds(280, getHeight() - 220, 205, 28);
        interpolation.setBounds(496, getHeight() - 220, 190, 28);
        const int sliderWidth = (getWidth() - 298) / 3;
        for (int i = 0; i < 3; ++i)
            sliders[static_cast<std::size_t>(i)].setBounds(274 + i * sliderWidth, getHeight() - 156,
                                                           sliderWidth - 12, 32);
        rewind.setBounds(280, getHeight() - 100, 54, 32);
        play.setBounds(344, getHeight() - 100, 82, 32);
        record.setBounds(436, getHeight() - 100, 112, 32);
        undo.setBounds(558, getHeight() - 100, 70, 32);
        punchLabel.setBounds(646, getHeight() - 108, 345, 22);
        punchIn.setBounds(650, getHeight() - 83, 100, 26);
        punchOut.setBounds(760, getHeight() - 83, 100, 26);
        positionLabel.setBounds(getWidth() - 290, 20, 262, 30);
        status.setBounds(24, getHeight() - 42, getWidth() - 48, 28);
    }
    int selectedLane() const {
        return std::max(0, lane.getSelectedId() - 1);
    }
    void checkpoint() {
        history.push_back(project);
        if (history.size() > 24)
            history.erase(history.begin());
    }
    void edit() {
        stop();
        checkpoint();
    }
    void sync() {
        for (std::size_t i = 0; i < 3; ++i)
            sliders[i].setValue(project.curves[i].initial, juce::dontSendNotification);
        for (std::size_t i = 0; i < 2; ++i) {
            startTime[i].setText(juce::String(double(project.clips[i].start) / projectRate, 3));
            length[i].setText(juce::String(double(project.clips[i].length) / projectRate, 3));
            rate[i].setText(juce::String(project.clips[i].rate, 3));
            cue[i].setToggleState(project.clips[i].cue, juce::dontSendNotification);
        }
        interpolation.setSelectedId(project.curves[static_cast<std::size_t>(selectedLane())].linear ? 2 : 1,
                                    juce::dontSendNotification);
        timeline.repaint();
    }
    void setStatus(const juce::String &s) {
        status.setText(s, juce::dontSendNotification);
    }
    void loadTrack(const juce::File &file, int deck) {
        if (busy)
            return;
        stop();
        auto base = project;
        projectJob(
            [base = std::move(base), file, deck]() mutable {
                auto c = loadClip(file);
                if (deck == 1)
                    c.start =
                        std::max(Frame{0}, base.clips[0].start + base.clips[0].length - 8 * projectRate);
                base.clips[static_cast<std::size_t>(deck)] = std::move(c);
                base.validate();
                return base;
            },
            "Track decoded. Drag its waveform to move it.");
    }
    void openProject(const juce::File &file) {
        if (busy)
            return;
        stop();
        projectJob([file] { return loadProject(file); }, "Set opened; source hashes verified.");
    }
    Project project;
    Frame cursor = 0;
    bool busy = false;

  private:
    void fileClicked(const juce::File &, const juce::MouseEvent &) override {}
    void fileDoubleClicked(const juce::File &file) override {
        if (file.existsAsFile())
            loadTrack(file, std::min(selectedLane(), 1));
    }
    void selectionChanged() override {}
    void browserRootChanged(const juce::File &) override {}
    void chooseFolder() {
        if (busy)
            return;
        chooser = std::make_unique<juce::FileChooser>("Choose music folder", directory.getDirectory());
        chooser->launchAsync(
            juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
            [safe = juce::Component::SafePointer<Workspace>(this)](const juce::FileChooser &fc) {
                if (safe && fc.getResult().isDirectory())
                    safe->directory.setDirectory(fc.getResult(), true, true);
            });
    }
    void chooseTrack(int deck) {
        if (busy)
            return;
        chooser = std::make_unique<juce::FileChooser>("Load audio into deck", directory.getDirectory(), "*");
        chooser->launchAsync(
            juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
            [safe = juce::Component::SafePointer<Workspace>(this), deck](const juce::FileChooser &fc) {
                if (safe && fc.getResult().existsAsFile())
                    safe->loadTrack(fc.getResult(), deck);
            });
    }
    void choose(bool writing, bool exporting) {
        if (busy)
            return;
        stop();
        chooser = std::make_unique<juce::FileChooser>(exporting ? "Export WAV" : "Fader set", juce::File{},
                                                      exporting ? "*.wav" : "*.faderset");
        auto flags =
            juce::FileBrowserComponent::canSelectFiles |
            (writing ? juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::warnAboutOverwriting
                     : juce::FileBrowserComponent::openMode);
        chooser->launchAsync(flags, [safe = juce::Component::SafePointer<Workspace>(this), writing,
                                     exporting](const juce::FileChooser &fc) {
            if (!safe || fc.getResult() == juce::File{})
                return;
            auto file = fc.getResult();
            if (!writing) {
                safe->projectJob([file] { return loadProject(file); }, "Set opened; source hashes verified.");
                return;
            }
            file = file.withFileExtension(exporting ? "wav" : "faderset");
            auto snapshot = safe->project;
            safe->busy = true;
            safe->setStatus(exporting ? "Rendering WAV..." : "Saving set...");
            safe->workers.addJob([safe, snapshot = std::move(snapshot), file, exporting] {
                juce::String result;
                try {
                    if (exporting)
                        exportWav(snapshot, file);
                    else
                        saveProject(snapshot, file);
                    result = exporting ? "WAV exported (24-bit / 48 kHz)."
                                       : "Set saved with source references and automation.";
                } catch (const std::exception &e) {
                    result = e.what();
                }
                juce::MessageManager::callAsync([safe, result] {
                    if (safe) {
                        safe->busy = false;
                        safe->setStatus(result);
                    }
                });
            });
        });
    }
    void projectJob(std::function<Project()> work, const juce::String &success) {
        busy = true;
        setStatus("Reading and decoding audio...");
        auto safe = juce::Component::SafePointer<Workspace>(this);
        workers.addJob([safe, work = std::move(work), success] {
            try {
                auto result = work();
                juce::MessageManager::callAsync([safe, result = std::move(result), success]() mutable {
                    if (safe) {
                        safe->history.clear();
                        safe->project = std::move(result);
                        safe->cursor = 0;
                        safe->busy = false;
                        safe->sync();
                        safe->setStatus(success);
                    }
                });
            } catch (const std::exception &e) {
                const juce::String error = e.what();
                juce::MessageManager::callAsync([safe, error] {
                    if (safe) {
                        safe->busy = false;
                        safe->setStatus(error);
                    }
                });
            }
        });
    }
    bool ensureAudio() {
        if (!audio.getCurrentAudioDevice()) {
            const auto error = audio.initialise(0, 4, nullptr, true);
            if (error.isNotEmpty()) {
                setStatus(error);
                return false;
            }
        }
        auto settings = audio.getAudioDeviceSetup();
        settings.sampleRate = projectRate;
        settings.bufferSize = 128;
        auto error = audio.setAudioDeviceSetup(settings, true);
        if (error.isNotEmpty()) {
            settings.bufferSize = 256;
            error = audio.setAudioDeviceSetup(settings, true);
        }
        if (error.isNotEmpty() || !audio.getCurrentAudioDevice() ||
            std::abs(audio.getCurrentAudioDevice()->getCurrentSampleRate() - projectRate) > 0.1) {
            setStatus("Select an output device supporting 48 kHz in Audio settings. " + error);
            return false;
        }
        return true;
    }
    void start(bool capture) {
        if (busy || project.end() == 0) {
            setStatus("Load a track first.");
            return;
        }
        stop();
        if (!ensureAudio())
            return;
        if (cursor >= project.end())
            cursor = 0;
        Frame in = cursor, out = project.end();
        if (capture) {
            for (const auto &c : project.curves)
                if (c.linear) {
                    setStatus("Recording requires Step targets. Linear envelopes remain editable manually.");
                    return;
                }
            in = std::max(Frame{0}, static_cast<Frame>(punchIn.getText().getDoubleValue() * projectRate));
            const auto requested = static_cast<Frame>(punchOut.getText().getDoubleValue() * projectRate);
            if (requested > 0)
                out = std::min(out, requested);
            if (in >= out) {
                setStatus("Retake start must be before its end.");
                return;
            }
            cursor = in;
        }
        try {
            engine = std::make_unique<Engine>(project, cursor);
        } catch (const std::exception &e) {
            setStatus(e.what());
            return;
        }
        recording = capture;
        take.clear();
        takeIn = in;
        takeOut = out;
        captureLost = false;
        if (capture)
            engine->arm(in, out);
        deviceInterrupted.store(false, std::memory_order_relaxed);
        audio.addAudioCallback(this);
        play.setButtonText("Stop");
        record.setButtonText(capture ? "Finish take" : "Record take");
        setStatus(capture ? "Recording A/B gain and crossfader. Stop to keep the take; Undo restores it."
                          : "Playing saved automation. Manual faders temporarily override it; stop/play "
                            "resumes the saved curves.");
    }
    void drain() {
        if (!engine)
            return;
        Event e;
        while (engine->captured(e)) {
            if (take.size() < 200000)
                take.push_back(e);
            else
                captureLost = true;
        }
    }
    void stop() {
        if (!engine)
            return;
        audio.removeAudioCallback(this);
        cursor = engine->position();
        drain();
        if (recording) {
            if (captureLost || engine->captureFailed())
                setStatus("Take discarded: capture interruption or overflow. Existing automation is intact.");
            else {
                try {
                    checkpoint();
                    commitTake(project, take, takeIn, std::min(cursor, takeOut));
                    setStatus("Take saved as editable Step targets. Undo restores the previous take.");
                } catch (const std::exception &e) {
                    setStatus(e.what());
                }
            }
        }
        if (!recording)
            setStatus(cursor >= project.end() ? "Playback finished." : "Playback stopped.");
        engine.reset();
        recording = false;
        play.setButtonText("Play");
        record.setButtonText("Record take");
        sync();
    }
    void showDevices() {
        if (busy)
            return;
        stop();
        if (audioDialog) {
            audioDialog->toFront(true);
            return;
        }
        audio.initialise(0, 4, nullptr, true);
        juce::DialogWindow::LaunchOptions o;
        o.content.setOwned(
            new juce::AudioDeviceSelectorComponent(audio, 0, 0, 2, 4, false, false, false, false));
        o.content->setSize(560, 480);
        o.dialogTitle = "Audio output - master 1/2, cue 3/4";
        o.dialogBackgroundColour = paper;
        o.escapeKeyTriggersCloseButton = true;
        o.useNativeTitleBar = true;
        o.resizable = false;
        audioDialog = o.launchAsync();
    }
    void audioDeviceIOCallbackWithContext(const float *const *, int, float *const *output, int channels,
                                          int samples, const juce::AudioIODeviceCallbackContext &) override {
        if (engine && rateOK.load(std::memory_order_relaxed))
            engine->process(output, channels, samples);
        else
            for (int c = 0; c < channels; ++c)
                if (output[c])
                    juce::FloatVectorOperations::clear(output[c], samples);
    }
    void audioDeviceAboutToStart(juce::AudioIODevice *device) override {
        rateOK.store(std::abs(device->getCurrentSampleRate() - projectRate) < 0.1, std::memory_order_relaxed);
    }
    void audioDeviceStopped() override {
        rateOK.store(false, std::memory_order_relaxed);
        deviceInterrupted.store(true, std::memory_order_relaxed);
    }
    void timerCallback() override {
        if (engine) {
            cursor = engine->position();
            drain();
            if (deviceInterrupted.load(std::memory_order_relaxed) && recording)
                captureLost = true;
            if (cursor >= project.end() || !rateOK.load() ||
                deviceInterrupted.load(std::memory_order_relaxed)) {
                stop();
            }
        }
        juce::String text = juce::String(double(cursor) / projectRate, 2) + " s";
        if (engine)
            text += "   peak " + juce::String(engine->peak(), 2) + (engine->peak() > 1 ? "  CLIPPING" : "");
        positionLabel.setText(text, juce::dontSendNotification);
        timeline.repaint();
    }

    juce::LookAndFeel_V4 look;
    juce::TextButton open{"Open set"}, save{"Save set"}, exportButton{"Export WAV"},
        devices{"Audio settings"}, folder{"Music folder..."}, play{"Play"}, record{"Record take"},
        rewind{"|<"}, undo{"Undo"};
    std::array<juce::TextButton, 2> load;
    std::array<juce::ToggleButton, 2> cue;
    std::array<juce::TextEditor, 2> startTime, length, rate;
    std::array<juce::Slider, 3> sliders;
    juce::ComboBox lane, interpolation;
    juce::Label status, positionLabel, punchLabel, libraryCaption;
    juce::TextEditor punchIn, punchOut;
    juce::TimeSliceThread directoryThread;
    juce::DirectoryContentsList directory;
    juce::FileTreeComponent library;
    Timeline timeline;
    juce::ThreadPool workers;
    juce::AudioDeviceManager audio;
    std::unique_ptr<Engine> engine;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::Component::SafePointer<juce::DialogWindow> audioDialog;
    std::vector<Project> history;
    std::vector<Event> take;
    Frame takeIn = 0, takeOut = 0;
    bool recording = false, captureLost = false;
    std::atomic<bool> rateOK{false};
    std::atomic<bool> deviceInterrupted{false};
    friend class Timeline;
};

double Timeline::extent() const {
    return std::max(30.0, double(owner.project.end()) / projectRate + 5.0);
}
float Timeline::x(Frame f) const {
    return 16.0f + float(double(f) / projectRate / extent()) * float(getWidth() - 32);
}
Frame Timeline::frame(float px) const {
    return std::max(Frame{0},
                    static_cast<Frame>((px - 16.0f) / float(getWidth() - 32) * extent() * projectRate));
}
void Timeline::paint(juce::Graphics &g) {
    g.setColour(ink);
    g.setFont(12);
    const float w = float(getWidth());
    for (int i = 0; i <= 6; ++i) {
        float px = 16 + (w - 32) * float(i) / 6;
        g.setColour(juce::Colour(0xffd5ddd7));
        g.drawVerticalLine(int(px), 22, float(getHeight()));
        g.setColour(ink);
        g.drawText(juce::String(extent() * i / 6, 1) + "s", int(px), 0, 74, 20, juce::Justification::left);
    }
    for (std::size_t deck = 0; deck < 2; ++deck) {
        const auto &c = owner.project.clips[deck];
        const float y = 30 + float(deck) * 165;
        g.setColour(juce::Colour(0xffe9ede7));
        g.fillRoundedRectangle(0, y, w, 150, 6);
        g.setColour(deck ? orange : teal);
        g.setFont(16);
        g.drawText(deck ? "B" : "A", 12, int(y) + 10, 30, 24, juce::Justification::left);
        g.setColour(ink);
        g.setFont(13);
        g.drawText(c.audio ? juce::String(c.name) : "Drop a song here", 48, int(y) + 10,
                   std::max(50, getWidth() - 440), 24, juce::Justification::left, true);
        if (c.audio) {
            const auto left = x(c.start), right = x(c.start + c.length);
            g.setColour((deck ? orange : teal).withAlpha(0.15f));
            g.fillRect(left, y + 86, right - left, 55.0f);
            g.setColour(deck ? orange : teal);
            const int pixels = std::max(1, int(right - left));
            for (int pixel = 0; pixel < pixels; ++pixel) {
                const double source = double(c.sourceOffset) + double(pixel) / pixels * double(c.length) *
                                                                   c.rate * c.audio->sampleRate / projectRate;
                const auto index = static_cast<std::size_t>(std::max(0.0, source / 512));
                if (index < c.audio->peaks.size()) {
                    float amp = std::min(1.0f, c.audio->peaks[index]) * 25;
                    g.drawVerticalLine(int(left) + pixel, y + 113 - amp, y + 113 + amp);
                }
            }
        }
    }
    const float top = 370, bottom = float(getHeight() - 14);
    if (bottom > top) {
        g.setColour(juce::Colours::white);
        g.fillRoundedRectangle(0, top - 15, w, bottom - top + 29, 6);
        const int selected = owner.selectedLane();
        const auto &curve = owner.project.curves[static_cast<std::size_t>(selected)];
        auto y = [&](float value) {
            float normalized = selected == 2 ? (value + 1) * 0.5f : value;
            return bottom - normalized * (bottom - top);
        };
        juce::Path path;
        path.startNewSubPath(16, y(curve.at(0)));
        for (int px = 17; px < getWidth() - 16; ++px)
            path.lineTo(float(px), y(curve.at(frame(float(px)))));
        g.setColour(teal);
        g.strokePath(path, juce::PathStrokeType(2));
        for (const auto &p : curve.points)
            g.fillEllipse(x(p.frame) - 4, y(p.value) - 4, 8, 8);
        g.setColour(ink);
        g.setFont(11);
        g.drawText("Click to add | drag a point | right-click to remove", 20, int(top) - 13, int(w) - 40, 18,
                   juce::Justification::right);
    }
    g.setColour(juce::Colour(0xffce5d49));
    g.drawLine(x(owner.cursor), 20, x(owner.cursor), float(getHeight()), 1.5f);
}
void Timeline::filesDropped(const juce::StringArray &files, int, int y) {
    if (!files.isEmpty())
        owner.loadTrack(juce::File(files[0]), y < 195 ? 0 : 1);
}
void Timeline::itemDropped(const SourceDetails &details) {
    const auto file = owner.library.getSelectedFile();
    if (file.existsAsFile())
        owner.loadTrack(file, details.localPosition.y < 195 ? 0 : 1);
}
void Timeline::mouseDown(const juce::MouseEvent &e) {
    if (owner.busy)
        return;
    owner.edit();
    draggingDeck = -1;
    pointIndex = -1;
    if (e.y < 28) {
        owner.cursor = std::min(frame(e.position.x), owner.project.end());
        repaint();
        return;
    }
    if (e.y >= 116 && e.y <= 171)
        draggingDeck = 0;
    else if (e.y >= 281 && e.y <= 336)
        draggingDeck = 1;
    if (draggingDeck >= 0) {
        dragStart = owner.project.clips[static_cast<std::size_t>(draggingDeck)].start;
        mouseStart = e.position.x;
        dragExtent = extent();
        return;
    }
    if (e.y < 370)
        return;
    const int lane = owner.selectedLane();
    editingLane = lane;
    auto &curve = owner.project.curves[static_cast<std::size_t>(lane)];
    const float top = 370, bottom = float(getHeight() - 14);
    if (bottom <= top)
        return;
    float normalized = std::clamp((bottom - e.position.y) / (bottom - top), 0.0f, 1.0f);
    float value = lane == 2 ? 2 * normalized - 1 : normalized;
    for (std::size_t i = 0; i < curve.points.size(); ++i)
        if (std::abs(x(curve.points[i].frame) - e.position.x) < 7) {
            pointIndex = int(i);
            break;
        }
    if (e.mods.isRightButtonDown()) {
        if (pointIndex >= 0)
            curve.points.erase(curve.points.begin() + pointIndex);
        pointIndex = -1;
        repaint();
        return;
    }
    if (pointIndex < 0) {
        if (curve.points.size() >= 200000)
            return;
        const Point point{std::min(frame(e.position.x), Frame{2 * 3600 * projectRate}), value};
        auto at = std::lower_bound(curve.points.begin(), curve.points.end(), point.frame,
                                   [](const Point &p, Frame f) { return p.frame < f; });
        pointIndex = int(at - curve.points.begin());
        if (at != curve.points.end() && at->frame == point.frame)
            at->value = value;
        else
            curve.points.insert(at, point);
    } else
        curve.points[static_cast<std::size_t>(pointIndex)].value = value;
    repaint();
}
void Timeline::mouseDrag(const juce::MouseEvent &e) {
    if (owner.busy)
        return;
    if (draggingDeck >= 0) {
        auto &c = owner.project.clips[static_cast<std::size_t>(draggingDeck)];
        const auto delta = static_cast<Frame>((e.position.x - mouseStart) / float(getWidth() - 32) *
                                              dragExtent * projectRate);
        c.start = std::clamp(dragStart + delta, Frame{0}, Frame{2 * 3600 * projectRate} - c.length);
        repaint();
        return;
    }
    if (pointIndex >= 0) {
        const int lane = editingLane;
        const float bottom = float(getHeight() - 14);
        const float normalized = std::clamp((bottom - e.position.y) / (bottom - 370), 0.0f, 1.0f);
        auto &points = owner.project.curves[static_cast<std::size_t>(lane)].points;
        const auto index = static_cast<std::size_t>(pointIndex);
        const auto low = index ? points[index - 1].frame + 1 : Frame{0};
        const auto high =
            index + 1 < points.size() ? points[index + 1].frame - 1 : Frame{2 * 3600 * projectRate};
        points[index] = {std::clamp(frame(e.position.x), low, high),
                         lane == 2 ? 2 * normalized - 1 : normalized};
        repaint();
    }
}
void Timeline::mouseUp(const juce::MouseEvent &) {
    auto &points = owner.project.curves[static_cast<std::size_t>(editingLane)].points;
    std::stable_sort(points.begin(), points.end(), [](auto a, auto b) { return a.frame < b.frame; });
    points.erase(std::unique(points.begin(), points.end(), [](auto a, auto b) { return a.frame == b.frame; }),
                 points.end());
    draggingDeck = pointIndex = -1;
    owner.sync();
}

class App final : public juce::JUCEApplication {
  public:
    const juce::String getApplicationName() override {
        return "Fader";
    }
    const juce::String getApplicationVersion() override {
        return "0.1.0";
    }
    void initialise(const juce::String &arguments) override {
        window = std::make_unique<Window>();
        const auto path = arguments.trim().unquoted();
        if (path.isNotEmpty()) {
            const auto file = juce::File::getCurrentWorkingDirectory().getChildFile(path);
            if (file.existsAsFile())
                static_cast<Workspace *>(window->getContentComponent())->openProject(file);
        }
    }
    void shutdown() override {
        window.reset();
    }
    void systemRequestedQuit() override {
        quit();
    }

  private:
    struct Window final : juce::DocumentWindow {
        Window() : DocumentWindow("Fader - prepared-set prototype", paper, allButtons) {
            setUsingNativeTitleBar(true);
            setContentOwned(new Workspace(), true);
            setResizable(true, false);
            setResizeLimits(1100, 800, 2200, 1500);
            centreWithSize(getWidth(), getHeight());
            setVisible(true);
        }
        void closeButtonPressed() override {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }
    };
    std::unique_ptr<Window> window;
};
} // namespace
START_JUCE_APPLICATION(App)
