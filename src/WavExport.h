// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "Media.h"
#include <fader/OfflineRender.h>
namespace fader {
struct WavExportOptions {
    bool floatingPoint = false;
    RenderRange range;
    int blockFrames = OfflineRender::maxBlockFrames;
    std::shared_ptr<std::atomic<bool>> cancellation;
    std::function<bool(double)> progress;
};
struct WavExportResult {
    Frame frames{};
    StreamStats sources;
    float maximumPeak{};
};
// Worker-only transaction. The encoder borrows stream() until its destruction;
// finish closes/checks storage, commit publishes only an entirely successful file.
// Injection permits deterministic storage faults in the actual export path.
class ExportDestination {
  public:
    virtual ~ExportDestination() = default;
    virtual juce::OutputStream &stream() = 0;
    virtual bool healthy() const = 0;
    virtual bool finish(Frame expectedFrames) = 0;
    virtual bool commit() = 0;
};
std::unique_ptr<ExportDestination> makeFileExportDestination(const juce::File &);
WavExportResult renderWav(const Project &, ExportDestination &, WavExportOptions = {});
WavExportResult exportWavFile(const Project &, const juce::File &, WavExportOptions = {});
} // namespace fader
