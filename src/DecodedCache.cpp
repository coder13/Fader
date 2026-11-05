// SPDX-License-Identifier: AGPL-3.0-or-later
#include "DecodedCache.h"
#if JUCE_WINDOWS
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#else
#include <fcntl.h>
#include <unistd.h>
#endif

namespace fader {
namespace {
std::atomic<std::uint64_t> reserved{0};
std::FILE *temporaryHandle() {
#if JUCE_WINDOWS
    // CRT tmpfile uses the drive root. Use the user's temp directory and an
    // OS-owned delete-on-close handle so a crash cannot orphan decoded PCM.
    const auto name = juce::File::getSpecialLocation(juce::File::tempDirectory)
                          .getChildFile("fader-" + juce::Uuid().toString() + ".pcm");
    const auto handle =
        CreateFileW(name.getFullPathName().toWideCharPointer(), GENERIC_READ | GENERIC_WRITE,
                    FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, CREATE_NEW,
                    FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
    if (handle == INVALID_HANDLE_VALUE)
        return nullptr;
    const int descriptor = _open_osfhandle(reinterpret_cast<intptr_t>(handle), _O_BINARY);
    if (descriptor < 0) {
        CloseHandle(handle);
        return nullptr;
    }
    auto *file = _fdopen(descriptor, "w+b");
    if (!file)
        _close(descriptor);
#else
    // Use the platform's user temp directory, then unlink the private file.
    // Closing the last descriptor releases it even after a crash.
    auto name = juce::File::getSpecialLocation(juce::File::tempDirectory)
                    .getChildFile("fader-pcm-XXXXXX")
                    .getFullPathName()
                    .toStdString();
    const int descriptor = ::mkstemp(name.data());
    if (descriptor < 0)
        return nullptr;
    const bool unlinked = ::unlink(name.c_str()) == 0;
    if (!unlinked || ::fcntl(descriptor, F_SETFD, FD_CLOEXEC) == -1) {
        ::close(descriptor);
        if (!unlinked)
            ::unlink(name.c_str());
        return nullptr;
    }
    auto *file = ::fdopen(descriptor, "w+b");
    if (!file)
        ::close(descriptor);
#endif
    return file;
}
bool position(std::FILE *file, std::uint64_t bytes) {
#if JUCE_WINDOWS
    return _fseeki64(file, static_cast<__int64>(bytes), SEEK_SET) == 0;
#else
    return ::fseeko(file, static_cast<off_t>(bytes), SEEK_SET) == 0;
#endif
}
} // namespace
DecodedCache::DecodedCache(std::uint64_t bytes) : reservation(bytes), file(temporaryHandle()) {}
DecodedCache::~DecodedCache() {
    if (file)
        std::fclose(file); // release storage before admitting another reservation
    reserved.fetch_sub(reservation);
}
std::uint64_t DecodedCache::reservedBytes() noexcept {
    return reserved.load();
}
std::shared_ptr<DecodedCache> DecodedCache::create(SourceFrame frames) {
    if (frames <= 0 || static_cast<std::uint64_t>(frames) > budgetBytes / (2 * sizeof(float)))
        return {};
    const auto bytes = static_cast<std::uint64_t>(frames) * 2 * sizeof(float);
    auto used = reserved.load();
    do {
        if (used > budgetBytes - bytes)
            return {};
    } while (!reserved.compare_exchange_weak(used, used + bytes));
    std::unique_ptr<DecodedCache> result;
    try {
        result.reset(new DecodedCache(bytes));
    } catch (...) {
        reserved.fetch_sub(bytes);
        return {};
    }
    if (!result->file)
        return {};
    return std::shared_ptr<DecodedCache>(std::move(result));
}
bool DecodedCache::append(const juce::AudioBuffer<float> &buffer, int frames) {
    if (!file || ready || frames < 0 || frames > 8192 || frames > buffer.getNumSamples() ||
        buffer.getNumChannels() < 2)
        return false;
    const auto bytes = static_cast<std::uint64_t>(frames) * 2 * sizeof(float);
    if (written > reservation || bytes > reservation - written)
        return false;
    for (int frame = 0; frame < frames; ++frame)
        for (int channel = 0; channel < 2; ++channel)
            interleaved[static_cast<std::size_t>(frame * 2 + channel)] = buffer.getSample(channel, frame);
    if (std::fwrite(interleaved.data(), 1, static_cast<std::size_t>(bytes), file) != bytes)
        return false;
    written += bytes;
    return true;
}
bool DecodedCache::finish() {
    if (!file || ready || written != reservation)
        return false;
    ready = std::fflush(file) == 0;
    return ready;
}
bool DecodedCache::read(SourceFrame first, int frames, float *const *stereo,
                        const std::atomic<bool> &cancelled) const {
    if (!ready || first < 0 || static_cast<std::uint64_t>(first) > reservation / (2 * sizeof(float)) ||
        frames < 0 || frames > 8192 || cancelled.load())
        return false;
    const auto offset = static_cast<std::uint64_t>(first) * 2 * sizeof(float);
    const auto bytes = static_cast<std::uint64_t>(frames) * 2 * sizeof(float);
    if (offset > reservation || bytes > reservation - offset)
        return false;
    const std::lock_guard lock(filePosition);
    if (cancelled.load() || !position(file, offset) ||
        std::fread(interleaved.data(), 1, static_cast<std::size_t>(bytes), file) != bytes)
        return false;
    for (int frame = 0; frame < frames; ++frame)
        for (int channel = 0; channel < 2; ++channel)
            stereo[channel][frame] = interleaved[static_cast<std::size_t>(frame * 2 + channel)];
    return !cancelled.load();
}
} // namespace fader
