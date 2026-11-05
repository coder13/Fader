// SPDX-License-Identifier: AGPL-3.0-or-later
#include "DecodedCache.h"
#include <future>
#include <limits>
#include <stdexcept>

namespace {
void check(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}
} // namespace
void testDecodedCache() {
    using namespace fader;
    const auto initial = DecodedCache::reservedBytes();
    check(initial == 0, "Earlier media operations leaked a disk reservation");
    {
        // Reserve, but never write, the full budget. No multi-GiB test artifact.
        auto full = DecodedCache::create(DecodedCache::budgetBytes / 8);
        check(full && !DecodedCache::create(1), "Disk budget admits more than 8 GiB");
    }
    check(DecodedCache::reservedBytes() == initial, "Disk reservation was not released");
    auto cache = DecodedCache::create(16384);
    check(cache != nullptr, "Cannot create session PCM cache");
    juce::AudioBuffer<float> block(2, 8192);
    for (int chunk = 0; chunk < 2; ++chunk) {
        for (int frame = 0; frame < 8192; ++frame) {
            block.setSample(0, frame, float(chunk * 8192 + frame) / 32768);
            block.setSample(1, frame, -float(chunk * 8192 + frame) / 32768);
        }
        check(cache->append(block, 8192), "Cannot fill cache");
    }
    check(cache->finish(), "Cannot commit cache");
    const auto read = [cache](int offset) {
        std::atomic<bool> cancelled{false};
        std::array<float, 128> left{}, right{};
        float *output[]{left.data(), right.data()};
        for (int i = 0; i < 1000; ++i) {
            const auto at = (offset + i * 7919) % (16384 - 128);
            check(cache->read(at, 128, output, cancelled), "Concurrent cache read failed");
            for (int j = 0; j < 128; ++j)
                check(left[j] == float(at + j) / 32768 && right[j] == -left[j],
                      "Two decoder readers raced on a shared cache position");
        }
        check(!cache->read(std::numeric_limits<SourceFrame>::max(), 128, output, cancelled),
              "Huge source offset wrapped into valid cached PCM");
        cancelled.store(true);
        check(!cache->read(0, 128, output, cancelled), "Cancelled cache read succeeded");
    };
    auto first = std::async(std::launch::async, read, 0);
    auto second = std::async(std::launch::async, read, 137);
    cache.reset(); // readers own the storage until both complete
    first.get();
    second.get();
    // read lambda still owns its shared cache here; scope exit releases it.
}
