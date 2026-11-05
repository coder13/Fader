// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <juce_core/juce_core.h>

namespace fader {
// Compact encoding for already-admitted project/capture documents. Only the
// schema's initial/value control fields use binary32 round-trip formatting.
// Metadata doubles retain binary64 precision. The existing byte bound remains.
juce::String encodeManifest(const juce::var &);
} // namespace fader
