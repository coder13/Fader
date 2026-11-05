// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once
#include <condition_variable>
#include <fader/Playback.h>
#include <functional>
#include <mutex>
#include <optional>
#include <thread>

namespace fader {
// Single control-thread client; never called by the audio callback. Owns at
// most one Playback (ready, borrowed or retiring) and one pending request.
// All Playback construction/destruction and decoder joins happen on worker.
class PlaybackPreparation {
  public:
    using Factory = std::function<std::unique_ptr<Playback>(const Project &, Frame, std::uint64_t,
                                                            std::shared_ptr<std::atomic<bool>>)>;
    struct Result {
        std::uint64_t generation = 0;
        Playback *playback = nullptr;
        std::string error;
    };
    explicit PlaybackPreparation(Factory = {});
    ~PlaybackPreparation(); // joins; call close() after detaching, before destroying the window
    std::uint64_t request(std::shared_ptr<const Project>, Frame, std::uint64_t segment);
    // A returned Playback is borrowed until cancel/close/another request. The
    // client MUST detach callbacks and stop using it before invalidating it.
    std::optional<Result> poll();
    void cancel(); // invalidates publication immediately, never joins a worker
    void close();  // same, and permanently stops accepting requests

  private:
    struct Request {
        std::shared_ptr<const Project> project;
        Frame start;
        std::uint64_t segment, generation;
        std::shared_ptr<std::atomic<bool>> cancellation;
    };
    void invalidate(); // mutex held
    void run();
    const Factory factory;
    std::mutex mutex;
    std::condition_variable wake;
    std::optional<Request> pending;
    std::optional<Result> completed;
    std::shared_ptr<std::atomic<bool>> cancellation;
    std::uint64_t generation = 0;
    bool closing = false;
    std::thread worker;
};
} // namespace fader
