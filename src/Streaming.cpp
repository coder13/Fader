// SPDX-License-Identifier: AGPL-3.0-or-later
#include <algorithm>
#include <cmath>
#include <fader/Streaming.h>
#include <limits>
#include <stdexcept>
#include <thread>

namespace fader {
namespace {
static_assert(std::atomic<SourceFrame>::is_always_lock_free &&
              std::atomic<std::uint64_t>::is_always_lock_free && std::atomic<unsigned>::is_always_lock_free &&
              std::atomic<bool>::is_always_lock_free);
std::atomic<std::uint64_t> nextGeneration{1};
enum SlotState : unsigned { empty, ready, reading, writing };
struct Page {
    std::array<std::array<float, StreamBank::pageFrames>, 2> pcm{};
    SourceFrame number = -1; // protected by the state transition
    std::atomic<unsigned> state{empty};
};
struct Cache {
    explicit Cache(std::shared_ptr<const StreamSource> source)
        : source(std::move(source)), length(this->source->frames()) {
        if (length <= 0 || length > std::numeric_limits<SourceFrame>::max() -
                                        StreamBank::pageFrames * StreamBank::pagesPerSource)
            throw std::runtime_error("Invalid streaming source length");
    }
    std::shared_ptr<const StreamSource> source;
    const SourceFrame length;
    std::array<Page, StreamBank::pagesPerSource> pages;
    std::atomic<SourceFrame> requested{-1};
    std::atomic<unsigned> failure{0};
    std::unique_ptr<SourceDecoder> decoder; // belongs exclusively to the assigned worker
    Page *pinned = nullptr;                 // belongs exclusively to the render/preparation consumer
    SourceFrame pinnedNumber = -1;
    SourceFrame lastRequest = -1; // same single consumer as request()/sample()/prepare()

    Page *acquire(SourceFrame number) noexcept {
        auto &page = pages[static_cast<std::size_t>(number % StreamBank::pagesPerSource)];
        unsigned expected = ready;
        if (!page.state.compare_exchange_strong(expected, reading, std::memory_order_acquire))
            return nullptr;
        if (page.number == number)
            return &page;
        page.state.store(ready, std::memory_order_release);
        return nullptr;
    }
    void release() noexcept {
        if (pinned)
            pinned->state.store(ready, std::memory_order_release);
        pinned = nullptr;
        pinnedNumber = -1;
    }
};
} // namespace

struct StreamBank::Impl {
    explicit Impl(std::vector<std::shared_ptr<const StreamSource>> sources,
                  std::shared_ptr<std::atomic<bool>> cancellation)
        : cancelled(cancellation ? std::move(cancellation) : std::make_shared<std::atomic<bool>>(false)),
          generation(nextGeneration.fetch_add(1)) {
        if (sources.size() > maxSources)
            throw std::runtime_error("Streaming supports at most 64 clip readers");
        for (auto &source : sources) {
            if (cancelled->load())
                throw std::runtime_error("Playback preparation cancelled");
            if (source) {
                caches.push_back(std::make_unique<Cache>(std::move(source)));
                pcmBytes += pcmBytesPerSource;
            } else {
                caches.push_back(nullptr);
            }
        }
        try {
            if (pcmBytes > 0)
                for (unsigned i = 0; i < workers.size(); ++i)
                    workers[i] = std::thread([this, i] { run(i); });
        } catch (...) {
            cancelled->store(true);
            for (auto &worker : workers)
                if (worker.joinable())
                    worker.join();
            throw;
        }
    }
    ~Impl() {
        cancelled->store(true, std::memory_order_release);
        for (auto &worker : workers)
            if (worker.joinable())
                worker.join();
    }
    bool fill(Cache &cache) noexcept {
        const auto first = cache.requested.load(std::memory_order_acquire);
        if (first < 0 || cache.failure.load(std::memory_order_relaxed))
            return false;
        if (!cache.decoder) {
            try {
                cache.decoder = cache.source->open();
            } catch (...) {
            }
            if (!cache.decoder) {
                cache.failure.store(static_cast<unsigned>(SourceError::open));
                return false;
            }
        }
        for (SourceFrame number = first; number < first + pagesPerSource; ++number) {
            if (number * pageFrames >= cache.length || cancelled->load(std::memory_order_acquire))
                break;
            auto &page = cache.pages[static_cast<std::size_t>(number % pagesPerSource)];
            auto state = page.state.load(std::memory_order_acquire);
            if (state == reading || state == writing || (state == ready && page.number == number))
                continue; // one worker is the only writer of page.number
            if (!page.state.compare_exchange_strong(state, writing, std::memory_order_acquire))
                continue;
            const auto count =
                static_cast<int>(std::min(SourceFrame(pageFrames), cache.length - number * pageFrames));
            float *destinations[]{page.pcm[0].data(), page.pcm[1].data()};
            auto error = SourceError::none;
            try {
                if (!cache.decoder->read(number * pageFrames, count, destinations, *cancelled))
                    error = SourceError::read;
                if (error == SourceError::none)
                    for (const auto &channel : page.pcm)
                        for (int frame = 0; frame < count; ++frame)
                            if (!std::isfinite(channel[static_cast<std::size_t>(frame)]))
                                error = SourceError::nonFinite;
            } catch (...) {
                error = SourceError::read;
            }
            const auto current = cache.requested.load(std::memory_order_acquire);
            if (cancelled->load(std::memory_order_acquire) || number < current ||
                number >= current + pagesPerSource) {
                discarded.fetch_add(1, std::memory_order_relaxed);
                page.state.store(empty, std::memory_order_release);
            } else if (error != SourceError::none) {
                cache.failure.store(static_cast<unsigned>(error), std::memory_order_release);
                page.state.store(empty, std::memory_order_release);
            } else {
                page.number = number;
                decoded.fetch_add(static_cast<std::uint64_t>(count), std::memory_order_relaxed);
                page.state.store(ready, std::memory_order_release);
            }
            return true; // one read per source per sweep, rather than monopolising a worker
        }
        return false;
    }
    void run(unsigned worker) noexcept {
        while (!cancelled->load(std::memory_order_acquire)) {
            bool worked = false;
            for (std::size_t i = worker; i < caches.size(); i += workers.size()) {
                if (cancelled->load(std::memory_order_acquire))
                    break;
                if (caches[i])
                    worked = fill(*caches[i]) || worked;
            }
            if (!worked)
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
    }
    std::vector<std::unique_ptr<Cache>> caches;
    std::array<std::thread, 2> workers;
    const std::shared_ptr<std::atomic<bool>> cancelled;
    std::atomic<std::uint64_t> decoded{0}, discarded{0};
    const std::uint64_t generation;
    std::uint64_t pcmBytes = 0;
};

StreamBank::StreamBank(std::vector<std::shared_ptr<const StreamSource>> sources,
                       std::shared_ptr<std::atomic<bool>> cancellation)
    : impl(std::make_unique<Impl>(std::move(sources), std::move(cancellation))) {}
StreamBank::~StreamBank() = default;
void StreamBank::request(unsigned clip, SourceFrame first) noexcept {
    if (clip < impl->caches.size() && impl->caches[clip]) {
        auto &cache = *impl->caches[clip];
        if (first >= 0 && first < cache.length && first / pageFrames != cache.lastRequest) {
            cache.lastRequest = first / pageFrames;
            cache.requested.store(cache.lastRequest, std::memory_order_release);
        }
    }
}
bool StreamBank::sample(unsigned clip, SourceFrame index, std::array<float, 2> &current,
                        std::array<float, 2> &next) noexcept {
    if (impl->cancelled->load(std::memory_order_relaxed) || clip >= impl->caches.size() ||
        !impl->caches[clip])
        return false;
    auto &cache = *impl->caches[clip];
    if (index < 0 || index >= cache.length || cache.failure.load(std::memory_order_relaxed))
        return false;
    const auto number = index / pageFrames;
    request(clip, index);
    if (cache.pinnedNumber != number) {
        cache.release();
        cache.pinned = cache.acquire(number);
        if (!cache.pinned)
            return false;
        cache.pinnedNumber = number;
    }
    const auto offset = static_cast<std::size_t>(index % pageFrames);
    auto *following = cache.pinned;
    if (offset == pageFrames - 1 && index + 1 < cache.length) {
        following = cache.acquire(number + 1);
        if (!following)
            return false;
    }
    const auto nextOffset = index + 1 == cache.length ? offset : (offset + 1) % pageFrames;
    for (unsigned channel = 0; channel < 2; ++channel) {
        current[channel] = cache.pinned->pcm[channel][offset];
        next[channel] = following->pcm[channel][nextOffset];
    }
    if (following != cache.pinned)
        following->state.store(ready, std::memory_order_release);
    return true;
}
void StreamBank::release() noexcept {
    for (const auto &cache : impl->caches)
        if (cache)
            cache->release();
}
SourceError StreamBank::error(unsigned clip) const noexcept {
    return clip < impl->caches.size() && impl->caches[clip]
               ? static_cast<SourceError>(impl->caches[clip]->failure.load(std::memory_order_relaxed))
               : SourceError::none;
}
StreamStats StreamBank::stats() const noexcept {
    unsigned failed = 0;
    for (unsigned i = 0; i < impl->caches.size(); ++i)
        failed += error(i) != SourceError::none;
    return {impl->generation, impl->decoded.load(),   impl->discarded.load(), impl->pcmBytes,
            failed,           impl->cancelled->load()};
}
void StreamBank::prepare(unsigned clip, SourceFrame first, SourceFrame last,
                         std::chrono::milliseconds timeout) {
    if (clip >= impl->caches.size() || !impl->caches[clip])
        return;
    auto &cache = *impl->caches[clip];
    if (first < 0 || last < first || last >= cache.length ||
        last / pageFrames - first / pageFrames >= pagesPerSource)
        throw std::runtime_error("Prepare audio in smaller blocks within the source range");
    cache.release();
    request(clip, first);
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    for (;;) {
        if (impl->cancelled->load())
            throw std::runtime_error("Audio preparation cancelled");
        if (error(clip) != SourceError::none)
            throw std::runtime_error("Audio source could not be read");
        bool complete = true;
        for (auto number = first / pageFrames; number <= last / pageFrames; ++number) {
            if (auto *page = cache.acquire(number))
                page->state.store(ready, std::memory_order_release);
            else
                complete = false;
        }
        if (complete)
            return;
        if (std::chrono::steady_clock::now() >= deadline)
            throw std::runtime_error("Audio preparation timed out");
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}
void StreamBank::cancel() noexcept {
    impl->cancelled->store(true, std::memory_order_release);
}
} // namespace fader
