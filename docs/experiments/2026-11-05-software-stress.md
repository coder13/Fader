# Software stress qualification

The short software runs pass source and capture integrity.
They do not complete [#40](https://github.com/coder13/Fader/issues/40) or the hour gate.
The first hour attempt used source `6e25222c8e615e0e1b406d3075032758b1e6e4b6`.
It stopped after about five timed minutes when macOS CI exposed a harness portability error.
That partial attempt does not count toward the hour gate.
A corrected full run remains pending.
No audio or MIDI device opens during these runs.

## Reproducible workload

The [contract](../stress-contract.md) declares the workload, deadlines, memory budgets and measurement limits before the sustained run.
The [fixture script](../../scripts/make-stress-fixtures.py) creates continuous percussion and sustained tones in two stereo WAVs.
It also creates a 1,000-file synthetic library.
The [driver](../../scripts/run-software-stress.py) compares every tracked build-source file with a clean committed checkout before execution.
It records the binary hash, source revision, fixture hashes, OS and build metadata.
It archives the captured project through the production loader and writer.

The virtual host processes four output channels at 48 kHz with 128-frame blocks.
It delays each production decoder read by 1 ms.
It paints the production arrangement into a 1,200 by 700 pixel image, targets 50 paints per second, and scrolls the viewport.
Separate threads scan the library, decode a short fixture, cancel seek preparation, submit controls and drain capture.
Clip 1 evaluates dense stored volume and filter curves while live inputs override clip 0.
This exercises software rendering, without native input events or display presentation.

The default thread and JUCE real-time thread use the same binary and workload.
Both follow absolute clock boundaries and spin for the final 100 microseconds before each callback.
The real-time request declares computation as one tenth of the block period and its maximum as one half.
The contract retains the original callback and virtual deadline targets.

## Short-run observations

These are ten-second diagnostic runs, not an extrapolation to one hour.
Each run processed 3,750 callbacks and captured all 699 planned controls.
Both runs recorded zero source gaps, source errors, correlation errors and lost metrics.
Both saved and reopened the captured project.
The [aggregate JSON](data/2026-11-05-software-stress-short.json) preserves distributions, counters, scheduling examples and fault results.

| Measurement | Default thread | JUCE real-time thread |
| --- | ---: | ---: |
| Virtual deadline misses | 29 | 0 |
| Callback p99.9 | 530.375 µs | 69.625 µs |
| Callback maximum | 1,217.125 µs | 86.000 µs |
| Receipt to predicted software output, p95 | 5.689 ms | 5.247 ms |
| PCM page storage | 1 MiB | 1 MiB |
| Process peak RSS, including capture commit/save | 34.484 MiB | 34.609 MiB |

The production callback duration and the host's late wake are separate measurements.
The default host missed virtual deadlines despite its callback execution staying within the block budget.
The real-time policy removed that failure in this short run.
This result supports an hour trial with that policy, not a claim of sustained reliability.
The predicted output calculation assumes one virtual block of buffering.
It does not measure device, controller, converter or acoustic delay.

## Fault retention and callback discipline

The fault runs retain 4,096 captured events after input queue overflow and 16,384 after capture queue overflow.
They label the saved automation as an incomplete prefix and reopen its stored curve.
A simulated device restart leaves its old segment invalid and retains its applied event.
Blocked decode cancellation allows a new preparation request to complete.
A deliberately blocked active source reports starvation independently of capture integrity.
A missing-media load rejects the source and preserves the saved project bytes.
Telemetry overflow reports its own loss without invalidating musical capture.

All seven local CTest suites pass, including the new two-second stress smoke.
The callback allocation test also runs with control telemetry enabled and reports no ordinary or aligned allocation/reclamation in the checked callbacks.
A standalone ThreadSanitizer run passes the scheduling, control telemetry, pressure, concurrent producer and interruption tests.
That run compiles production `Engine.cpp`, `Playback.cpp` and `Streaming.cpp` with `PlaybackTests.cpp`.
It does not instrument the entire JUCE app or the offscreen painter.
Clean Linux CI passed all five suites at source `6e25222` in [run 42001694441](https://github.com/coder13/Fader/actions/runs/42001694441).
The macOS build failed because its older libc++ lacks `std::jthread`.
The harness now uses `std::thread` with explicit stop-and-join ownership for both normal and exceptional cleanup.
The correction keeps the workload and scheduling targets unchanged.
Its clean CI result remains pending.

## Configuration and remaining evidence

The local build uses AppleClang 21.0.0.21000334, CMake 4.4.4, C++20 and Release configuration on arm64 Darwin 25.6.0.
JUCE is pinned to `d60b7a78030661574a615b586d9325ba7a7fa7dd`.
The local build enables synthetic UI fixtures, while default CI disables them.
The driver records that the local build folder is a source archive and separately proves its byte equality with the committed source.
No local build or sanitizer process runs during the sustained measurement.
The sustained command wraps the driver in `caffeinate -i` to prevent idle system sleep for its process lifetime.
It does not disable the lock screen.

The hour result, 256-frame fallback, native seek/export observations and physical device evidence remain incomplete.
The preview remains preserved because the UI tool reports the Mac as locked.
Keep #40 open until its stated evidence exists.
