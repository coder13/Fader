// SPDX-License-Identifier: AGPL-3.0-or-later
#include <fader/PlaybackPreparation.h>
#include <stdexcept>

namespace fader {
PlaybackPreparation::PlaybackPreparation(Factory make)
    : factory(make ? std::move(make)
                   : Factory{[](const Project &project, Frame start, std::uint64_t segment,
                                std::shared_ptr<std::atomic<bool>> stop) {
                         return std::make_unique<Playback>(project, start, segment,
                                                           Preparation{std::move(stop)});
                     }}),
      worker([this] { run(); }) {}
PlaybackPreparation::~PlaybackPreparation() {
    close();
    worker.join();
}
void PlaybackPreparation::invalidate() {
    ++generation;
    if (cancellation)
        cancellation->store(true);
    pending.reset();
    completed.reset(); // holds only a borrowed pointer; never destroys Playback
}
std::uint64_t PlaybackPreparation::request(std::shared_ptr<const Project> project, Frame start,
                                           std::uint64_t segment) {
    if (!project)
        throw std::invalid_argument("Playback request needs a project");
    const std::lock_guard lock(mutex);
    if (closing)
        throw std::logic_error("Playback preparation is closed");
    invalidate();
    cancellation = std::make_shared<std::atomic<bool>>(false);
    pending = Request{std::move(project), start, segment, generation, cancellation};
    wake.notify_one();
    return generation;
}
std::optional<PlaybackPreparation::Result> PlaybackPreparation::poll() {
    const std::lock_guard lock(mutex);
    auto result = std::move(completed);
    completed.reset();
    return result;
}
void PlaybackPreparation::cancel() {
    const std::lock_guard lock(mutex);
    invalidate();
    wake.notify_one();
}
void PlaybackPreparation::close() {
    const std::lock_guard lock(mutex);
    invalidate();
    closing = true;
    wake.notify_one();
}
void PlaybackPreparation::run() {
    std::unique_ptr<Playback> owned;
    std::uint64_t ownedGeneration = 0;
    std::unique_lock lock(mutex);
    for (;;) {
        wake.wait(lock, [&] { return closing || pending || (owned && ownedGeneration != generation); });
        if (owned) {
            // UI invalidation has ended its borrow. Never join while holding
            // the control mutex: Cancel and newer requests must stay responsive.
            lock.unlock();
            owned->cancelSourceReads();
            owned.reset();
            lock.lock();
        }
        if (closing)
            return;
        if (!pending)
            continue;
        auto request = std::move(*pending);
        pending.reset();
        lock.unlock();
        std::string error;
        try {
            if (!request.cancellation->load())
                owned = factory(*request.project, request.start, request.segment, request.cancellation);
            if (!owned && !request.cancellation->load())
                error = "Playback preparation returned no engine";
        } catch (const std::exception &e) {
            error = e.what();
        } catch (...) {
            error = "Playback preparation failed";
        }
        ownedGeneration = request.generation;
        request.project.reset(); // release the worker's snapshot off the control thread
        lock.lock();
        if (!closing && generation == ownedGeneration && !request.cancellation->load())
            completed = Result{ownedGeneration, owned.get(), std::move(error)};
        // Stale engines are retired on the next loop before the newest build.
    }
}
} // namespace fader
