// SPDX-License-Identifier: AGPL-3.0-or-later
#include "ManifestJson.h"
#include "ProjectStorage.h"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <iterator>
#include <stdexcept>
#include <string_view>

namespace fader {
namespace {
class Writer {
  public:
    void value(const juce::var &v, bool binary32 = false, unsigned depth = 0) {
        if (depth > 64)
            throw std::runtime_error("Project JSON nesting exceeds its limit");
        if (v.isBool()) {
            append(bool(v) ? "true" : "false");
        } else if (v.isInt() || v.isInt64()) {
            number(static_cast<juce::int64>(v));
        } else if (v.isDouble()) {
            const auto d = double(v);
            if (!std::isfinite(d))
                throw std::runtime_error("Non-finite project number");
            if (d == 0 && std::signbit(d))
                append("-0.0");
            else if (binary32 && double(float(d)) == d)
                number(float(d));
            else
                number(d);
        } else if (v.isString()) {
            string(v.toString());
        } else if (const auto *array = v.getArray()) {
            append("[");
            bool first = true;
            for (const auto &item : *array) {
                if (!first)
                    append(",");
                first = false;
                value(item, false, depth + 1);
            }
            append("]");
        } else if (const auto *object = v.getDynamicObject()) {
            append("{");
            bool first = true;
            for (const auto &property : object->getProperties()) {
                if (!first)
                    append(",");
                first = false;
                const auto key = property.name.toString();
                string(key);
                append(":");
                value(property.value, key == "initial" || key == "value", depth + 1);
            }
            append("}");
        } else {
            throw std::runtime_error("Unsupported project JSON value");
        }
    }
    juce::String finish() const {
        return juce::String::fromUTF8(text.data(), static_cast<int>(text.size()));
    }

  private:
    template <class Number> void number(Number v) {
        char buffer[128];
        const auto result = std::to_chars(std::begin(buffer), std::end(buffer), v);
        if (result.ec != std::errc{})
            throw std::runtime_error("Cannot encode project number");
        append({buffer, static_cast<std::size_t>(result.ptr - buffer)});
    }
    void string(const juce::String &s) {
        // Reuse JUCE's string escaping, including control characters and UTF-8.
        // No string/key is inserted into the JSON as unescaped source text.
        const auto escaped = juce::JSON::toString(juce::var(s), true).toStdString();
        append(escaped);
    }
    void append(std::string_view part) {
        const auto limit = static_cast<std::size_t>(maxManifestBytes);
        if (part.size() > limit - text.size())
            throw std::runtime_error("Project exceeds the 8 MiB manifest limit");
        const auto size = text.size() + part.size();
        if (size > text.capacity())
            text.reserve(std::min(limit, std::max(size, std::max(std::size_t{4096}, text.capacity() * 2))));
        text.append(part);
    }
    std::string text;
};
} // namespace
juce::String encodeManifest(const juce::var &root) {
    Writer writer;
    writer.value(root);
    return writer.finish();
}
} // namespace fader
