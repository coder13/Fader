// SPDX-License-Identifier: AGPL-3.0-or-later
#include "WavExport.h"
#include <algorithm>
#include <filesystem>
#include <stdexcept>
namespace fader {
namespace {
class FileDestination final : public ExportDestination {
  public:
    explicit FileDestination(const juce::File &file) : temporary(file) {
        // Unbuffered writes make short writes observable, including final headers.
        output = std::make_unique<juce::FileOutputStream>(temporary.getFile(), 0);
        if (!output->openedOk())
            throw std::runtime_error("Could not create export file");
    }
    juce::OutputStream &stream() override {
        return *output;
    }
    bool healthy() const override {
        return valid && (!output || output->getStatus().wasOk());
    }
    bool finish(Frame frames) override {
        output->flush();
        valid = output->getStatus().wasOk();
        output.reset();
        if (!valid)
            return false;
        auto reader = std::unique_ptr<juce::AudioFormatReader>(
            juce::WavAudioFormat{}.createReaderFor(temporary.getFile().createInputStream().release(), true));
        valid = reader && reader->lengthInSamples == frames && reader->sampleRate == projectRate &&
                reader->numChannels == 2;
        return valid;
    }
    bool commit() override {
        return !output && valid && temporary.overwriteTargetFileWithTemporary();
    }

  private:
    juce::TemporaryFile temporary;
    std::unique_ptr<juce::FileOutputStream> output;
    bool valid = true;
};
struct IoState {
    bool failed = false;
};
class CheckedStream final : public juce::OutputStream {
  public:
    CheckedStream(ExportDestination &destination, IoState &state) : destination(destination), state(state) {}
    void flush() override {
        destination.stream().flush();
        state.failed |= !destination.healthy();
    }
    bool write(const void *data, std::size_t bytes) override {
        if (state.failed)
            return false;
        const bool ok = destination.stream().write(data, bytes);
        state.failed |= !ok || !destination.healthy();
        return !state.failed;
    }
    bool setPosition(juce::int64 at) override {
        const bool ok = !state.failed && destination.stream().setPosition(at);
        state.failed |= !ok || !destination.healthy();
        return !state.failed;
    }
    juce::int64 getPosition() override {
        return destination.stream().getPosition();
    }

  private:
    ExportDestination &destination;
    IoState &state;
};
void validProject(const Project &project, const juce::File *file = nullptr) {
    project.validate();
    if (project.end() <= 0)
        throw std::runtime_error("The set is empty");
    for (const auto &clip : project.clips) {
        if (clip.length > 0 && !clip.audio)
            throw std::runtime_error("Missing source audio");
        if (file && !clip.path.empty()) {
            std::error_code error;
            const auto same = std::filesystem::equivalent(
                std::filesystem::u8path(clip.path),
                std::filesystem::u8path(file->getFullPathName().toStdString()), error);
            if (juce::File(clip.path) == *file || (!error && same))
                throw std::runtime_error("Export cannot replace a source audio file");
        }
    }
}
} // namespace
std::unique_ptr<ExportDestination> makeFileExportDestination(const juce::File &file) {
    return std::make_unique<FileDestination>(file);
}
WavExportResult renderWav(const Project &project, ExportDestination &destination, WavExportOptions options) {
    validProject(project);
    if (options.blockFrames < 1 || options.blockFrames > OfflineRender::maxBlockFrames)
        throw std::runtime_error("Invalid export block size");
    auto cancelled = [&] { return options.cancellation && options.cancellation->load(); };
    if (cancelled() || (options.progress && !options.progress(0)))
        throw std::runtime_error("Export cancelled");
    OfflineRender renderer(project, options.range, options.cancellation);
    IoState io;
    std::unique_ptr<juce::OutputStream> checked = std::make_unique<CheckedStream>(destination, io);
    auto format = juce::AudioFormatWriterOptions{}
                      .withSampleRate(projectRate)
                      .withNumChannels(2)
                      .withBitsPerSample(options.floatingPoint ? 32 : 24)
                      .withSampleFormat(options.floatingPoint
                                            ? juce::AudioFormatWriterOptions::SampleFormat::floatingPoint
                                            : juce::AudioFormatWriterOptions::SampleFormat::integral);
    auto writer = juce::WavAudioFormat{}.createWriterFor(checked, format);
    if (!writer || io.failed)
        throw std::runtime_error("Could not write WAV header");
    juce::AudioBuffer<float> buffer(2, options.blockFrames);
    WavExportResult result;
    while (renderer.completed() < renderer.frames()) {
        const auto count = renderer.read(buffer.getArrayOfWritePointers(), options.blockFrames);
        result.maximumPeak = std::max(result.maximumPeak, renderer.peak());
        if (!options.floatingPoint && renderer.peak() > 1)
            throw std::runtime_error("Mix exceeds 0 dBFS. Lower the deck gains before exporting PCM WAV.");
        if (!writer->writeFromFloatArrays(buffer.getArrayOfReadPointers(), 2, count) || io.failed ||
            !destination.healthy())
            throw std::runtime_error("WAV export failed (check available disk space)");
        if (cancelled() ||
            (options.progress && !options.progress(double(renderer.completed()) / renderer.frames())))
            throw std::runtime_error("Export cancelled");
    }
    result.frames = renderer.frames();
    result.sources = renderer.streamStats();
    if (!writer->flush() || io.failed)
        throw std::runtime_error("Could not finalize WAV header");
    writer.reset(); // final header rewrite must also pass the checked stream
    if (io.failed || !destination.healthy() || !destination.finish(result.frames))
        throw std::runtime_error("Could not flush or validate WAV export");
    if (cancelled())
        throw std::runtime_error("Export cancelled");
    if (!destination.commit())
        throw std::runtime_error("Could not replace WAV export destination");
    return result;
}
WavExportResult exportWavFile(const Project &project, const juce::File &file, WavExportOptions options) {
    validProject(project, &file);
    auto destination = makeFileExportDestination(file);
    return renderWav(project, *destination, std::move(options));
}
} // namespace fader
