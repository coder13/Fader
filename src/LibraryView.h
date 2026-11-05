// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "Library.h"
namespace fader {
class LibraryView final : public juce::Component,
                          private juce::TableListBoxModel,
                          private juce::ListBoxModel {
  public:
    LibraryView();
    void resized() override;
    void paint(juce::Graphics &) override;
    void setLibrary(LibraryResult, juce::File root);
    void setLoading();
    const TrackInfo *find(const juce::File &) const;
    std::function<void(const TrackInfo &)> addTrack;
    std::function<void()> refresh;
    std::function<void()> preferences;

  private:
    int getNumRows() override;
    void paintRowBackground(juce::Graphics &, int, int, int, bool) override;
    void paintCell(juce::Graphics &, int, int, int, int, bool) override;
    juce::Component *refreshComponentForCell(int, int, bool, juce::Component *) override;
    void cellDoubleClicked(int, int, const juce::MouseEvent &) override;
    void cellClicked(int, int, const juce::MouseEvent &) override;
    void sortOrderChanged(int, bool) override;
    juce::var getDragSourceDescription(const juce::SparseSet<int> &) override;
    void paintListBoxItem(int, juce::Graphics &, int, int, bool) override;
    void selectedRowsChanged(int) override;
    void rebuild();
    juce::String value(const TrackInfo &, int) const;
    struct Sources final : juce::ListBoxModel {
        explicit Sources(LibraryView &v) : view(v) {}
        LibraryView &view;
        int getNumRows() override {
            return view.sourceNames.size();
        }
        void paintListBoxItem(int r, juce::Graphics &g, int w, int h, bool s) override {
            view.paintListBoxItem(r, g, w, h, s);
        }
        void selectedRowsChanged(int r) override {
            view.selectedSource = r;
            view.rebuild();
        }
    } sourceModel;
    LibraryResult library;
    std::vector<std::size_t> filtered;
    juce::StringArray sourceNames;
    std::vector<juce::File> sourceFolders;
    int selectedSource = 0, sortColumn = 1;
    bool ascending = true;
    juce::File folder;
    juce::TextEditor search;
    juce::TextButton rescan{"Library ..."};
    juce::Label countLabel, detail;
    juce::TableListBox table;
    juce::ListBox sources;
};
class PreferencesPanel final : public juce::Component {
  public:
    PreferencesPanel(juce::AudioDeviceManager &, Preferences, std::function<void(Preferences)>);
    void paint(juce::Graphics &) override;
    void resized() override;

  private:
    void choose(bool);
    Preferences prefs;
    std::function<void(Preferences)> apply;
    juce::TextEditor music, nml;
    juce::TextButton chooseMusic{"Choose..."}, chooseNml{"Choose..."}, save{"Apply & close"},
        cancel{"Cancel"};
    juce::ToggleButton enabled{"Import Traktor metadata (read-only)"};
    juce::Label error;
    juce::AudioDeviceSelectorComponent audio;
    std::unique_ptr<juce::FileChooser> chooser;
};
} // namespace fader
