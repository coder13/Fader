// SPDX-License-Identifier: AGPL-3.0-or-later
#include "LibraryView.h"
#include <cmath>
namespace fader {
namespace {
const juce::Colour ink{0xff27343c}, muted{0xff75838c}, accent{0xff087f8c};
}
LibraryView::LibraryView()
    : sourceModel(*this), table("Music library", this), sources("Sources", &sourceModel) {
    for (auto *c : std::array<juce::Component *, 6>{&search, &rescan, &countLabel, &detail, &table, &sources})
        addAndMakeVisible(c);
    search.setTextToShowWhenEmpty("Search title, artist, BPM, key...", muted);
    search.setName("Search music library");
    search.onTextChange = [this] { rebuild(); };
    rescan.onClick = [this] {
        if (refresh)
            refresh();
    };
    auto &h = table.getHeader();
    h.addColumn("Title", 1, 300, 140, 900);
    h.addColumn("Artist", 2, 190, 90, 500);
    h.addColumn("BPM", 3, 64, 50, 120);
    h.addColumn("Key", 4, 54, 45, 120);
    h.addColumn("Time", 5, 58, 50, 100);
    h.addColumn("Genre", 6, 110, 50, 250);
    h.addColumn("Metadata", 7, 90, 65, 150);
    h.setSortColumnId(1, true);
    table.setRowHeight(27);
    table.setHeaderHeight(28);
    table.setOutlineThickness(0);
    table.setMultipleSelectionEnabled(false);
    table.setColour(juce::ListBox::backgroundColourId, juce::Colours::white);
    sources.setRowHeight(29);
    sources.setColour(juce::ListBox::backgroundColourId, juce::Colour(0xfff0f3f5));
    sourceNames.add("All music");
    sourceNames.add("Traktor metadata");
    sourceNames.add("Missing files");
    countLabel.setColour(juce::Label::textColourId, muted);
    detail.setColour(juce::Label::textColourId, muted);
}
void LibraryView::paint(juce::Graphics &g) {
    g.fillAll(juce::Colours::white);
    g.setColour(ink);
    g.setFont(14);
    g.drawText("MUSIC LIBRARY", 12, 3, 160, 30, juce::Justification::left);
    g.setColour(juce::Colour(0xffdce3e8));
    g.drawHorizontalLine(0, 0, float(getWidth()));
}
void LibraryView::resized() {
    search.setBounds(190, 5, std::max(180, getWidth() / 3), 28);
    rescan.setBounds(getWidth() - 96, 5, 84, 28);
    countLabel.setBounds(200 + getWidth() / 3, 5, std::max(0, getWidth() - 310 - getWidth() / 3), 28);
    sources.setBounds(0, 40, 180, getHeight() - 64);
    table.setBounds(190, 40, getWidth() - 190, getHeight() - 64);
    detail.setBounds(10, getHeight() - 24, getWidth() - 20, 24);
    table.getHeader().setColumnWidth(1, std::max(180, getWidth() - 790));
}
void LibraryView::setLoading() {
    countLabel.setText("Reading music and Traktor metadata...", juce::dontSendNotification);
}
void LibraryView::setLibrary(LibraryResult result, juce::File root) {
    library = std::move(result);
    folder = root;
    selectedSource = 0;
    sourceNames.clear();
    sourceFolders.clear();
    sourceNames.add("All music");
    sourceNames.add("Traktor metadata");
    sourceNames.add("Missing files");
    for (const auto &f : root.findChildFiles(juce::File::findDirectories, false)) {
        sourceNames.add(f.getFileName());
        sourceFolders.push_back(f);
    }
    sources.updateContent();
    sources.selectRow(0);
    rebuild();
    detail.setText(library.warning.isNotEmpty() ? library.warning
                                                : "Drag a track into the arrangement | Double-click adds at "
                                                  "the playhead | Source files stay unchanged",
                   juce::dontSendNotification);
}
const TrackInfo *LibraryView::find(const juce::File &f) const {
    for (const auto &t : library.tracks)
        if (t.file == f)
            return &t;
    return nullptr;
}
void LibraryView::rebuild() {
    const auto query = search.getText().trim();
    filtered.clear();
    for (std::size_t i = 0; i < library.tracks.size(); ++i) {
        const auto &t = library.tracks[i];
        if (selectedSource == 1 && t.source != "Traktor")
            continue;
        if (selectedSource == 2 && !t.missing)
            continue;
        if (selectedSource >= 3 && static_cast<std::size_t>(selectedSource - 3) < sourceFolders.size() &&
            !t.file.isAChildOf(sourceFolders[static_cast<std::size_t>(selectedSource - 3)]))
            continue;
        const auto text = t.title + " " + t.artist + " " + t.key + " " + t.genre + " " +
                          (t.bpm ? juce::String(t.bpm, 2) : juce::String{});
        if (query.isNotEmpty() && !text.containsIgnoreCase(query))
            continue;
        filtered.push_back(i);
    }
    std::stable_sort(filtered.begin(), filtered.end(), [this](auto a, auto b) {
        const auto &x = library.tracks[a];
        const auto &y = library.tracks[b];
        int comparison = 0;
        if (sortColumn == 3)
            comparison = x.bpm < y.bpm ? -1 : x.bpm > y.bpm ? 1 : 0;
        else if (sortColumn == 5)
            comparison = x.duration < y.duration ? -1 : x.duration > y.duration ? 1 : 0;
        else
            comparison = value(x, sortColumn).compareNatural(value(y, sortColumn));
        return ascending ? comparison < 0 : comparison > 0;
    });
    table.updateContent();
    table.repaint();
    countLabel.setText(juce::String(static_cast<int>(filtered.size())) + " tracks | " +
                           juce::String(library.matched) + " from Traktor",
                       juce::dontSendNotification);
}
int LibraryView::getNumRows() {
    return static_cast<int>(filtered.size());
}
juce::String LibraryView::value(const TrackInfo &t, int column) const {
    switch (column) {
    case 1:
        return (t.missing ? "Missing - " : "") + t.title;
    case 2:
        return t.artist.isEmpty() ? "--" : t.artist;
    case 3:
        return t.bpm > 0 ? juce::String(t.bpm, 2) : "--";
    case 4:
        return t.key.isEmpty() ? "--" : t.key;
    case 5:
        return t.duration > 0 ? juce::String(int(t.duration) / 60) + ":" +
                                    juce::String(int(t.duration) % 60).paddedLeft('0', 2)
                              : "--";
    case 6:
        return t.genre.isEmpty() ? "--" : t.genre;
    case 7:
        return t.source;
    default:
        return {};
    }
}
void LibraryView::paintRowBackground(juce::Graphics &g, int row, int w, int h, bool selected) {
    g.fillAll(selected ? juce::Colour(0xffd9eef0)
                       : (row % 2 ? juce::Colour(0xfff7f9fa) : juce::Colours::white));
    if (selected) {
        g.setColour(accent);
        g.fillRect(0, 0, 3, h);
    }
    g.setColour(juce::Colour(0xffedf0f2));
    g.drawHorizontalLine(h - 1, 0, float(w));
}
void LibraryView::paintCell(juce::Graphics &, int, int, int, int, bool) {}
juce::Component *LibraryView::refreshComponentForCell(int row, int column, bool, juce::Component *existing) {
    auto *label = dynamic_cast<juce::Label *>(existing);
    if (!label) {
        delete existing;
        label = new juce::Label();
        label->setInterceptsMouseClicks(false, false);
        label->setBorderSize({0, 8, 0, 6});
        label->setFont(juce::FontOptions(12));
    }
    if (row < 0 || row >= getNumRows()) {
        label->setText({}, juce::dontSendNotification);
        return label;
    }
    const auto &t = library.tracks[filtered[static_cast<std::size_t>(row)]];
    label->setColour(juce::Label::textColourId,
                     t.missing ? juce::Colour(0xffaa4c37) : (column == 4 ? accent : ink));
    label->setName(table.getHeader().getColumnName(column));
    label->setText(value(t, column), juce::dontSendNotification);
    return label;
}
void LibraryView::cellDoubleClicked(int row, int, const juce::MouseEvent &) {
    if (row >= 0 && row < getNumRows() && addTrack)
        addTrack(library.tracks[filtered[static_cast<std::size_t>(row)]]);
}
void LibraryView::sortOrderChanged(int column, bool direction) {
    sortColumn = column;
    ascending = direction;
    rebuild();
}
juce::var LibraryView::getDragSourceDescription(const juce::SparseSet<int> &rows) {
    if (rows.size() && rows[0] >= 0 && rows[0] < getNumRows())
        return "fader-file:" +
               library.tracks[filtered[static_cast<std::size_t>(rows[0])]].file.getFullPathName();
    return {};
}
void LibraryView::paintListBoxItem(int row, juce::Graphics &g, int w, int h, bool selected) {
    if (selected)
        g.fillAll(juce::Colour(0xffdce9ed));
    g.setColour(selected ? accent : ink);
    g.setFont(12);
    g.drawText(sourceNames[row], 12, 0, w - 20, h, juce::Justification::centredLeft, true);
}
void LibraryView::selectedRowsChanged(int row) {
    if (row >= 0 && row < getNumRows()) {
        const auto &t = library.tracks[filtered[static_cast<std::size_t>(row)]];
        detail.setText(t.file.getFullPathName() + " | " + t.source +
                           (t.hasGrid ? " | Beat grid imported" : "") +
                           (t.cueCount ? " | " + juce::String(t.cueCount) + " cue markers in Traktor" : ""),
                       juce::dontSendNotification);
    }
}

PreferencesPanel::PreferencesPanel(juce::AudioDeviceManager &manager, Preferences current,
                                   std::function<void(Preferences)> callback)
    : prefs(std::move(current)), apply(std::move(callback)),
      audio(manager, 0, 0, 2, 4, false, false, false, false) {
    for (auto *c : std::array<juce::Component *, 9>{&music, &nml, &chooseMusic, &chooseNml, &enabled, &save,
                                                    &cancel, &error, &audio})
        addAndMakeVisible(c);
    music.setReadOnly(true);
    nml.setReadOnly(true);
    music.setText(prefs.musicFolder.getFullPathName());
    nml.setText(prefs.collection.getFullPathName());
    enabled.setToggleState(prefs.importTraktor, juce::dontSendNotification);
    chooseMusic.onClick = [this] { choose(false); };
    chooseNml.onClick = [this] { choose(true); };
    save.onClick = [this] {
        prefs.importTraktor = enabled.getToggleState();
        if (!prefs.musicFolder.isDirectory()) {
            error.setText("Choose an existing music folder.", juce::dontSendNotification);
            return;
        }
        if (prefs.importTraktor && !prefs.collection.existsAsFile()) {
            error.setText("Choose collection.nml or disable Traktor import.", juce::dontSendNotification);
            return;
        }
        try {
            prefs.save();
            if (apply)
                apply(prefs);
            if (auto *d = findParentComponentOfClass<juce::DialogWindow>())
                d->exitModalState(1);
        } catch (const std::exception &e) {
            error.setText(e.what(), juce::dontSendNotification);
        }
    };
    cancel.onClick = [this] {
        if (auto *d = findParentComponentOfClass<juce::DialogWindow>())
            d->exitModalState(0);
    };
    setSize(720, 620);
}
void PreferencesPanel::paint(juce::Graphics &g) {
    g.fillAll(juce::Colour(0xfff5f7f9));
    g.setColour(ink);
    g.setFont(20);
    g.drawText("Library & audio", 24, 15, 500, 32, juce::Justification::left);
    g.setFont(13);
    g.drawText("Music folder", 24, 58, 500, 22, juce::Justification::left);
    g.drawText("Traktor collection", 24, 126, 500, 22, juce::Justification::left);
    g.setColour(muted);
    g.drawText(
        "Imports title, artist, BPM, Open Key and the first beat-grid marker. Original files stay unchanged.",
        24, 216, 670, 38, juce::Justification::left, true);
    g.setColour(ink);
    g.setFont(16);
    g.drawText("Audio output", 24, 264, 500, 25, juce::Justification::left);
    g.setFont(12);
    g.setColour(muted);
    g.drawText("Master: channels 1/2 | Headphones: 3/4 | Audio changes apply immediately", 24, 290, 650, 22,
               juce::Justification::left);
}
void PreferencesPanel::resized() {
    music.setBounds(24, 84, 565, 28);
    chooseMusic.setBounds(600, 84, 96, 28);
    nml.setBounds(24, 151, 565, 28);
    chooseNml.setBounds(600, 151, 96, 28);
    enabled.setBounds(24, 188, 650, 26);
    audio.setBounds(18, 321, 684, 220);
    error.setBounds(24, 543, 672, 28);
    cancel.setBounds(457, 580, 100, 28);
    save.setBounds(568, 580, 128, 28);
}
void PreferencesPanel::choose(bool collection) {
    chooser = std::make_unique<juce::FileChooser>(
        collection ? "Choose Traktor collection.nml" : "Choose music folder",
        collection ? prefs.collection : prefs.musicFolder, collection ? "*.nml" : "*");
    chooser->launchAsync(juce::FileBrowserComponent::openMode |
                             (collection ? juce::FileBrowserComponent::canSelectFiles
                                         : juce::FileBrowserComponent::canSelectDirectories),
                         [safe = juce::Component::SafePointer<PreferencesPanel>(this),
                          collection](const juce::FileChooser &fc) {
                             if (!safe || fc.getResult() == juce::File{})
                                 return;
                             if (collection) {
                                 safe->prefs.collection = fc.getResult();
                                 safe->nml.setText(fc.getResult().getFullPathName());
                             } else {
                                 safe->prefs.musicFolder = fc.getResult();
                                 safe->music.setText(fc.getResult().getFullPathName());
                             }
                         });
}
} // namespace fader
