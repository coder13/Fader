# Frame-correlated software timing

Candidate `0fc6ddeafb02306e607329ac098acc692afaf878` adds bounded diagnostic rows to the
[storage and viewport repair](2026-11-05-dense-save-and-paint.md).
It keeps the original workload, block deadline, input rates, automation density and memory/storage limits.
The [repeated hour](2026-11-05-software-stress-followup.md) now provides complete frame-correlated sustained evidence.
Clean candidate [CI](https://github.com/coder13/Fader/actions/runs/42022680469) passes on all three hosts.
It passes save and memory but still fails timing and painter-start targets; the diagnostics are not an hour pass.

## Measurement changes

The collector indexes callback and host history by project frame before sorting distributions.
Missing, duplicate or invalid records cannot shift the association of later callbacks.
For the first 256 qualifying blocks, it records the target, wake, process return, sink completion and deadline.
Rows also contain the internal production callback duration from that same frame.
A wake or process call at least half its interval, or any missed deadline, qualifies a block.
Global maximum wake, process call and missed deadline remain available after the row bound.
The report exposes omitted rows and trace completeness.

These are wall durations from the existing steady clock, including any time spent descheduled.
They distinguish a late wake from time inside the process call and time in the checksum sink.
They do not measure thread CPU time or establish a DSP or kernel cause for a slow call.
The host still uses absolute steady-clock boundaries with the final 100 microseconds spent spinning.
The JUCE real-time policy request is unchanged.

The three exact callback-history arrays are allocated and touched before the clock origin.
They replace the old append-only histories rather than adding another full callback history.
Painter update/execution and start-spacing histories are also allocated before timing.
The full-process peak still includes those arrays and final capture commit/save.
No history allocation or row retention occurs on the virtual callback thread.

The hour flag now checks all scheduled 50 Hz paints and zero starts more than one paint interval late.
The original contract already required that workload; the earlier flag omitted this check.
The report includes exact painter-duration and start-spacing distributions.
These measure offscreen component work, not native display presentation.

The driver also refuses to archive a previous fixture-side captured project after the current save fails.
It requires the current probe to report a successful persistence operation.

## Local checks

All seven native suites pass in Release on arm64 Darwin 25.6 with AppleClang 21, CMake 4.4.4 and pinned JUCE.
Local UI fixtures are ON; default CI keeps them OFF.
All 70 tracked compiled-source, test and CMake files match the committed candidate byte for byte.

The smoke tests inject reversed drain order, missing and duplicate callback records, invalid frames/segments,
invalid host timestamps and tail overflow.
They require correct frame pairing, visible loss counters and global maxima after the bounded row list fills.
A separate driver integration check uses a stub probe to cover failed save with an old file, successful new save,
and reported success with a missing file.
Only the successful new save is archived.
That driver check does not exercise or qualify the audio engine.

The [12-second diagnostic](data/2026-11-05-timing-trace-short.json) processes 4,500 callbacks with short media.
It has zero virtual deadline misses and complete callback/host histories.
All 600 scheduled paints occur, with no start more than 20 ms late.
Update-and-paint p99 is 7.875 ms; maximum is 15.265 ms.
The slowest callback is 2.436417 ms at project frame 495,488.
Its paired host wake is only 83 ns late and its full process call is 2.436833 ms.
Thus the trace locates the long wall duration inside that call, without establishing CPU cost.
The short run saves its captured project and stays within the memory budget.
Its short media, short curves and duration do not qualify the dense one-hour workload.

## Clean CI functional checks

Downloaded artifacts match exact source `0fc6ddeafb02306e607329ac098acc692afaf878`, the pinned JUCE revision and UI fixtures OFF.
JUnit records five Linux suites and seven suites on each native host, with no failures, errors or skips.
The [verified aggregate](data/2026-11-05-timing-trace-ci.json) records the toolchains and results.
The integration checkout has no source, test or CMake changes relative to this tested code.

Both native logs report the injected timing-model tests as passed.
Each native smoke retains all 750 callback and host samples and all 247 controls, with zero source gaps or invalid/duplicate timing records.
All 259 serialized rows on each host pass frame/boundary and timestamp-arithmetic checks.
That count includes the maximum rows, which may repeat frames from the first 256 outliers.

The shared runners also demonstrate explicit trace truncation and performance failures:

| Two-second ordinary-priority smoke | macOS | Windows |
| --- | ---: | ---: |
| Virtual deadline misses | 510 | 3 |
| Scheduled paints completed | 100 / 100 | 100 / 100 |
| Paint starts more than one interval late | 53 | 0 |
| Qualifying outlier blocks | 627 | 316 |
| Outlier rows omitted after the bound | 371 | 60 |
| Trace complete | No | No |
| Predicted-latency target passed | No | Yes |

The smoke acceptance checks functional integrity, not sustained performance on shared CI machines.
The bounded row list does not hide omission or discard the global maxima.
Neither smoke qualifies the hour targets, and successful CI must not be described as doing so.
The dedicated real-time run retains its original timing, memory, storage and painting limits.

## Remaining work

The exact candidate's full declared hour is now reported, including its failed targets.
Finish the separate 256-frame fallback and use the observed boundary correlation to choose the next timing diagnostic.
Keep native seek/export observations and physical-device evidence separate.
The controller remains excluded from current work, and no audio or MIDI device opens in these tests.
