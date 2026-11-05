// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <filesystem>
#include <juce_core/juce_core.h>

namespace fader {
// Stored paths and JUCE strings use UTF-8. std::filesystem's narrow path
// constructor uses a platform encoding on Windows, so convert explicitly.
inline std::filesystem::path filesystemPath(const std::string &utf8) {
    return std::filesystem::u8path(utf8);
}
inline std::filesystem::path filesystemPath(const juce::File &file) {
    return filesystemPath(file.getFullPathName().toStdString());
}
inline juce::File fileFromPath(const std::filesystem::path &path) {
    const auto utf8 = path.u8string();
    return juce::File(
        juce::String::fromUTF8(reinterpret_cast<const char *>(utf8.data()), static_cast<int>(utf8.size())));
}
inline juce::String relativeMediaPath(const juce::File &file, const juce::File &base) {
    return file.getRelativePathFrom(base).replaceCharacter(juce::File::getSeparatorChar(), '/');
}
} // namespace fader
