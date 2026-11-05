// SPDX-License-Identifier: AGPL-3.0-or-later
#include "FileSource.h"
#include <stdexcept>

namespace fader {
namespace {
struct Description {
    juce::File file;
    SourceFrame frames;
    double sampleRate;
    unsigned channels;
    juce::int64 bytes, modified;
    bool unchanged() const {
        return file.existsAsFile() && file.getSize() == bytes &&
               file.getLastModificationTime().toMilliseconds() == modified;
    }
};
class Decoder final : public SourceDecoder {
  public:
    explicit Decoder(Description description) : description(std::move(description)) {
        open();
        // JUCE 9.0.5's MP3 random seek can fail at VBR tails even when a full
        // sequential decode succeeds. Preserve its exact sequential sample
        // domain by replaying skipped frames on this worker. Fast compressed
        // seeking needs a separately qualified checkpoint/decoder strategy.
        if (reader->getFormatName() == "MP3 file")
            discard = std::make_unique<juce::AudioBuffer<float>>(2, 8192);
    }
    bool read(SourceFrame first, int frames, float *const *stereo,
              const std::atomic<bool> &cancelled) override {
        if (cancelled.load() || !description.unchanged() || first < 0 || frames < 0 ||
            first > description.frames - frames)
            return false;
        if (discard && position != first) {
            if (first < position)
                open();
            while (position < first) {
                if (cancelled.load())
                    return false;
                const int count = static_cast<int>(std::min(SourceFrame{8192}, first - position));
                if (!reader->read(discard->getArrayOfWritePointers(), 2, position, count))
                    return false;
                position += count;
            }
        }
        const bool ok = reader->read(stereo, 2, first, frames);
        position = first + frames;
        if (description.channels == 1)
            std::copy_n(stereo[0], frames, stereo[1]);
        return ok && !cancelled.load() && description.unchanged();
    }

  private:
    void open() {
        if (!this->description.unchanged())
            throw std::runtime_error("Source file changed or is unavailable");
        juce::AudioFormatManager formats;
        formats.registerBasicFormats();
        reader.reset(formats.createReaderFor(this->description.file.createInputStream()));
        if (!reader || reader->lengthInSamples != this->description.frames ||
            reader->sampleRate != this->description.sampleRate ||
            reader->numChannels != this->description.channels)
            throw std::runtime_error("Source decoder metadata changed");
        position = 0;
    }
    Description description;
    std::unique_ptr<juce::AudioFormatReader> reader;
    std::unique_ptr<juce::AudioBuffer<float>> discard;
    SourceFrame position = 0;
};
class FileSource final : public StreamSource {
  public:
    explicit FileSource(Description description) : description(std::move(description)) {}
    SourceFrame frames() const noexcept override {
        return description.frames;
    }
    std::unique_ptr<SourceDecoder> open() const override {
        return std::make_unique<Decoder>(description);
    }

  private:
    const Description description;
};
} // namespace
std::shared_ptr<const StreamSource> makeFileSource(const juce::File &file, SourceFrame frames,
                                                   double sampleRate, unsigned channels, juce::int64 bytes,
                                                   juce::int64 modifiedMillis) {
    return std::make_shared<FileSource>(
        Description{file, frames, sampleRate, channels, bytes, modifiedMillis});
}
} // namespace fader
