# Software stress qualification

The first full software hour fails its deadline, storage and process-memory targets.
The [repeated hour after repair](2026-11-05-software-stress-followup.md) passes save and memory, but still fails timing and painter-start targets.
It retains every applied control and reports zero source starvation.
[#40](https://github.com/coder13/Fader/issues/40) remains open for remediation and qualification.
The first hour attempt used source `6e25222c8e615e0e1b406d3075032758b1e6e4b6`.
It stopped after about five timed minutes when macOS CI exposed a harness portability error.
That partial attempt does not count toward the hour gate.
The completed hour run uses source `566b0ca981f86d33c873a9b63a5b0919068ae7c2`.
Its failed result is retained below without changing the declared targets.
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

## Full-hour result

The [complete aggregate](data/2026-11-05-software-stress-hour-failed.json) records the exact build, fixture hashes and all reported counters.
The host processes 1,350,000 callbacks over 3,600.022 seconds at 48 kHz/128 frames.
It captures and correlates all 107,999 planned inputs with zero queue, metric or retained-event loss.
Both sources remain active, with zero starvation, failed sources, non-finite samples or quiet blocks.
The two PCM page caches total 1 MiB, and 42,202 decoder reads receive the declared 1 ms delay.

| Measurement | Target | Observed |
| --- | --- | --- |
| Virtual deadline misses | 0 | 12 |
| Callback p99.9 | Below 1.333 ms | 0.027458 ms |
| Callback maximum | Below 2.667 ms | 3.173500 ms |
| Host wake lateness maximum | Report separately | 15.124583 ms |
| Receipt to predicted software output p95 | Below 10 ms | 5.136792 ms |
| Process peak before capture commit | Report separately | 85.797 MiB |
| Process peak including commit/save | At most 128 MiB | 150.469 MiB |
| Editable captured-set save | Within the 8 MiB manifest bound | Rejected as oversized |
| Offscreen arrangement paints | Target 50 per second | 133,593 total, about 37.1 per second |

The callback wall-time maximum includes possible descheduling.
The current aggregate does not preserve enough per-outlier correlation to attribute the timing tails to DSP computation or the host scheduler.
The next diagnostic must retain callback/frame/wake relationships and report the limits of its clock model.
These virtual deadline misses are not measured driver xruns or acoustic dropouts.

The painter misses its declared cadence on this dense project.
Its selected-handle loop visits every stored point, including points outside the viewport.
That is a concrete UI workload to repair without deleting automation or reducing the declared point density.
The run completes 360 library scans, 360 analysis imports and 120 seek cancellations.

The final capture commit reaches the current manifest-size rejection, and no captured project is published.
All applied targets are present in the measured in-memory take, but this does not satisfy durable editable-set delivery.
The process peak rises during commit/save, beyond its pre-commit peak and declared budget.
The next storage trial must retain all curves and targets, preserve numeric meaning and existing destination protection, and measure the full process peak again.
Do not turn this result into a pass by lowering event rates, sparsifying curves or increasing the declared limits.

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
Corrected source `566b0ca981f86d33c873a9b63a5b0919068ae7c2` passes clean [run 42003392676](https://github.com/coder13/Fader/actions/runs/42003392676).
Linux passes five suites and macOS passes seven suites, including the stress smoke.
Downloaded artifacts match the exact source, pinned JUCE revision, disabled UI fixtures and zero failed or skipped JUnit cases.
The macOS job also passes app signature verification.
A ten-second trial of this corrected build retains all 699 controls with zero source gaps and virtual deadline misses.
Its callback p99.9 is 68.292 µs and its maximum is 94.750 µs.

## Configuration and remaining evidence

The local build uses AppleClang 21.0.0.21000334, CMake 4.4.4, C++20 and Release configuration on arm64 Darwin 25.6.0.
JUCE is pinned to `d60b7a78030661574a615b586d9325ba7a7fa7dd`.
The local build enables synthetic UI fixtures, while default CI disables them.
The driver records that the local build folder is a source archive and separately proves its byte equality with the committed source.
No local build or sanitizer process runs during the sustained measurement.
The sustained command wraps the driver in `caffeinate -i` to prevent idle system sleep for its process lifetime.
It does not disable the lock screen.

The 128-frame hour failed the targets above.
A corrected hour, separately reported 256-frame fallback, native seek/export observations and physical device evidence remain required.
The preview remains preserved because the UI tool reports the Mac as locked.
Keep #40 open until its stated evidence exists.
