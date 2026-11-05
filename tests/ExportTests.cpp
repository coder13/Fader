// SPDX-License-Identifier: AGPL-3.0-or-later
#include "ReadFixture.h"
#include "WavExport.h"
#include <filesystem>
#include <limits>
#include <stdexcept>
namespace {
using namespace fader;
void check(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
template <class F> void rejects(F action) {
    try {
        action();
    } catch (const std::exception &) {
        return;
    }
    throw std::runtime_error("Failed export was reported successful");
}
Project fixture() {
    Project p;
    p.perClip = true;
    p.clips.resize(1);
    auto a = std::make_shared<AudioData>();
    for (auto &c : a->samples)
        c.assign(4097, 0.1f);
    p.clips[0].audio = a;
    p.clips[0].length = 4097;
    return p;
}
enum class Fault { none, initialHeader, body, shortWrite, finalHeader, finalSeek, flush, finish, replace };
class FaultDestination final : public ExportDestination {
    class Stream final : public juce::OutputStream {
      public:
        explicit Stream(FaultDestination &owner) : owner(owner) {}
        void flush() override {
            owner.inner->stream().flush();
            if (owner.fault == Fault::flush)
                owner.failed = true;
        }
        bool write(const void *data, std::size_t bytes) override {
            if (owner.fault == Fault::initialHeader && !payload) {
                owner.failed = true;
                return false;
            }
            if (bytes > 256) {
                payload = true;
                if (owner.fault == Fault::body) {
                    owner.failed = true;
                    return false;
                }
                if (owner.fault == Fault::shortWrite) {
                    owner.inner->stream().write(data, bytes / 2);
                    owner.failed = true;
                    return false;
                }
            }
            if (payload && owner.fault == Fault::finalHeader && getPosition() < 104) {
                owner.failed = true;
                return false;
            }
            return owner.inner->stream().write(data, bytes);
        }
        bool setPosition(juce::int64 at) override {
            if (payload && at == 0 && owner.fault == Fault::finalSeek) {
                owner.failed = true;
                return false;
            }
            return owner.inner->stream().setPosition(at);
        }
        juce::int64 getPosition() override {
            return owner.inner->stream().getPosition();
        }
        bool payload = false;

      private:
        FaultDestination &owner;
    };

  public:
    FaultDestination(const juce::File &file, Fault fault)
        : inner(makeFileExportDestination(file)), fault(fault), bridge(*this) {}
    juce::OutputStream &stream() override {
        return bridge;
    }
    bool healthy() const override {
        return !failed && inner->healthy();
    }
    bool finish(Frame frames) override {
        bridge.flush();
        return healthy() && fault != Fault::finish && inner->finish(frames);
    }
    bool commit() override {
        committed = true;
        return fault != Fault::replace && inner->commit();
    }
    std::unique_ptr<ExportDestination> inner;
    Fault fault;
    bool failed = false, committed = false;
    Stream bridge;
};
} // namespace
void testExportFailures() {
    using namespace fader;
    const auto dir = juce::File::getSpecialLocation(juce::File::tempDirectory)
                         .getNonexistentChildFile("fader-export", {}, false);
    dir.createDirectory();
    try {
        auto p = fixture();
        const auto originalId = p.id, clipId = p.clips[0].id;
        const auto curve = p.clips[0].automation[0].points;
        const auto file = dir.getChildFile("keep.wav");
        exportWav(p, file, true);
        const auto original = juce::SHA256(file);
        auto changed = p;
        changed.clips[0].automation[0].points = {{0, 0.2f}};
        for (auto fault : {Fault::initialHeader, Fault::body, Fault::shortWrite, Fault::finalHeader,
                           Fault::finalSeek, Fault::flush, Fault::finish, Fault::replace}) {
            {
                FaultDestination destination(file, fault);
                WavExportOptions options;
                options.floatingPoint = true;
                rejects([&] { renderWav(changed, destination, options); });
                if (fault != Fault::replace)
                    check(!destination.committed, "Failed output reached commit");
            }
            check(juce::SHA256(file) == original, "Storage failure replaced a valid destination");
            check(dir.findChildFiles(juce::File::findFiles, false, "*_temp*.wav").isEmpty(),
                  "Failed export leaked a temporary file");
        }
        WavExportOptions options;
        options.progress = [](double p) { return p < 0.5; };
        rejects([&] { exportWavFile(changed, file, options); });
        check(juce::SHA256(file) == original, "Cancelled export replaced destination");
        options = {};
        options.cancellation = std::make_shared<std::atomic<bool>>(true);
        rejects([&] { exportWavFile(p, file, options); });
        check(juce::SHA256(file) == original, "Pre-cancelled export replaced destination");
#if JUCE_MAC || JUCE_LINUX
        // POSIX permits unlinking an open output. Final reopen must fail with an
        // exception, not pass a null stream into JUCE's WAV reader constructor.
        options = {};
        options.progress = [&](double fraction) {
            if (fraction == 1) {
                for (const auto &temporary :
                     dir.findChildFiles(juce::File::findFiles, false, "keep_temp*.wav"))
                    check(temporary.deleteFile(), "Could not unlink synthetic temporary output");
            }
            return true;
        };
        rejects([&] { exportWavFile(changed, file, options); });
        check(juce::SHA256(file) == original, "Disappeared temporary output replaced destination");
#endif
        auto nonfinite = p;
        auto invalid = std::make_shared<AudioData>(*p.clips[0].audio);
        invalid->samples[0][0] = std::numeric_limits<float>::quiet_NaN();
        nonfinite.clips[0].audio = invalid;
        rejects([&] { exportWav(nonfinite, file, true); });
        check(juce::SHA256(file) == original, "Non-finite float audio replaced destination");
        const auto input = dir.getChildFile("input.wav");
        check(file.copyFileTo(input), "Cannot copy synthetic source");
        auto source = loadClip(input);
        Project streamed;
        streamed.perClip = true;
        streamed.clips = {source};
        // Physical aliases must not let an export overwrite a source.
        const auto alias = dir.getChildFile("input-alias.wav");
        std::filesystem::create_hard_link(input.getFullPathName().toStdString(),
                                          alias.getFullPathName().toStdString());
        rejects([&] { exportWav(streamed, alias); });
        check(juce::SHA256(input) == original, "Source alias protection failed");
        input.deleteFile();
        rejects([&] { exportWav(streamed, file); });
        check(juce::SHA256(file) == original, "Missing source replaced valid output");
        const auto blocked = dir.getChildFile("blocked.wav");
        blocked.createDirectory();
        file.copyFileTo(blocked.getChildFile("old.wav"));
        rejects([&] { exportWav(changed, blocked); });
        check(juce::SHA256(blocked.getChildFile("old.wav")) == original,
              "Failed filesystem replacement damaged existing data");
        rejects([&] { exportWav(p, dir.getChildFile("missing-parent/output.wav")); });
        options = {};
        options.floatingPoint = true;
        options.range = {123, 2057};
        options.blockFrames = 128;
        const auto result = exportWavFile(p, dir.getChildFile("range.wav"), options);
        check(result.frames == 1934, "Export range has wrong length");
        auto reader = std::unique_ptr<juce::AudioFormatReader>(juce::WavAudioFormat{}.createReaderFor(
            dir.getChildFile("range.wav").createInputStream().release(), true));
        check(reader && reader->lengthInSamples == 1934 && reader->numChannels == 2 &&
                  reader->usesFloatingPointData,
              "Range output header/format invalid");
        reader.reset(); // release the output before fixture cleanup on Windows
        check(p.id == originalId && p.clips[0].id == clipId && p.clips[0].automation[0].points == curve,
              "Offline export mutated active project");
        // A successful replacement publishes the new sound only at the end.
        options.range = {};
        exportWavFile(changed, file, options);
        check(juce::SHA256(file) != original, "Successful export failed to replace output");
        dir.deleteRecursively();
    } catch (...) {
        dir.deleteRecursively();
        throw;
    }
}
