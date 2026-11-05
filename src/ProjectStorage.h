// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include "Media.h"
namespace fader {
inline constexpr juce::int64 maxManifestBytes = 8 * 1024 * 1024;
juce::var projectDocument(const Project &, const juce::File &);
Project readProjectDocument(const juce::var &, const juce::File &, bool resolveMedia = true);
} // namespace fader
