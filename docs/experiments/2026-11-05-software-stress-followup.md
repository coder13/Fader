# Repeated software hour after storage and viewport repair

The repeated 128-frame hour saves the full compiled take and passes the original process-memory limit.
It still fails the callback maximum, zero-deadline-miss and painter-start targets.
[#40](https://github.com/coder13/Fader/issues/40) remains open; this is not a successful hour qualification.
The [first failed hour](2026-11-05-software-stress.md) remains published.

## Exact run and integrity

The [complete aggregate and final metadata](data/2026-11-05-software-stress-followup-128.json) identify clean source
`e2e1a47b1c80f7d67c0acc6ece6e69f9b5cd7e83` and compiled code `0fc6ddeafb02306e607329ac098acc692afaf878`.
The driver byte-compares every tracked file with the build source before execution and records the binary hash.
The two original continuous-source hashes were independently rechecked before launch.
The local build is Release, AppleClang 21, CMake 4.4.4, pinned JUCE and UI fixtures ON on arm64 Darwin 25.6.
The compiled candidate also passes the [clean cross-platform functional checks](2026-11-05-timing-trace.md).

The virtual host processes 1,350,000 callbacks over 3,600.001660 seconds at 48 kHz/128 frames.
All 107,999 planned inputs are attempted, accepted, captured and correlated.
Input, callback-metric, control-metric, host-metric and retained-event loss counters are zero.
Source starvation, source failure, non-finite output and quiet blocks are zero.
The PCM caches total 1 MiB, and 42,202 production reads receive the declared additional 1 ms delay.
The background work completes 360 library scans, 360 analysis imports and 120 seek cancellations.
No audio or MIDI device opens.

## Results against the unchanged limits

| Measurement | Target | Repeated 128-frame hour |
| --- | --- | ---: |
| Virtual deadline misses | 0 | 3 |
| Callback p99.9 | Below 1.333 ms | 0.080334 ms |
| Callback maximum | Below 2.667 ms | 2.809791 ms |
| Host wake lateness maximum | Report separately | 1.532875 ms |
| Receipt to predicted software output p95 | Below 10 ms | 5.154750 ms |
| Peak RSS before capture commit | Report separately | 89.109 MiB |
| Peak RSS including capture commit/save | At most 128 MiB | 102.828 MiB |
| Captured manifest | Within 8 MiB | 7,438,959 bytes |
| Compiled points reopened after archive | Preserve every point and identity | 150,042 |
| Offscreen paints completed | 180,000 | 180,000 |
| Paint starts more than 20 ms late | 0 under the stated follow-up gate | 161 |

The save takes about 0.305 seconds after the timed interval.
The production loader and writer archive the result successfully.
An independent comparison requires every field, numeric value, curve and identity to match after rebasing only asset paths.
The original and archived paths resolve to the same two synthetic sources.
The archived manifest is 7,439,013 bytes because those relative paths differ in length.
Hashes of both files are in the aggregate.
The deterministic storage regression has a different applied-frame schedule and compiles 149,169 points;
the actual hour retains all 150,042 compiled points without substituting that smaller regression result.

This run improves on the earlier rejected save and 150.469 MiB peak without raising either limit or removing points.
There is one sustained observation for each build, not a controlled estimate of failure frequency.
The change from twelve to three virtual misses does not establish their cause or a reliability rate.

## Correlated deadline tails

The collector retains all 39 qualifying tail blocks with zero omitted rows.
All 42 serialized rows, including the three maximum rows that can duplicate tail frames, pass frame and timestamp-arithmetic checks.
All three missed blocks have an almost punctual wake and a long wall duration inside the production callback:

| Project frame | Wake lateness | Internal callback | Completion after deadline |
| ---: | ---: | ---: | ---: |
| 54,890,368 | 0 ns | 2.809791 ms | 143.583 µs |
| 127,287,168 | 41 ns | 2.676791 ms | 10.749 µs |
| 150,507,392 | 83 ns | 2.735500 ms | 69.708 µs |

Each miss begins at frame `4096 * n - 128`.
Of the 39 tail blocks, 34 have that same remainder of 3,968 modulo 4,096.
That is a concrete boundary correlation to investigate, not proof of a decoder, memory, DSP or scheduler cause.
The stream cache uses 8,192-frame logical pages; 4,096 frames of one float32 channel occupy 16 KiB.
Future diagnostics should distinguish thread CPU time from wall time and examine these boundaries before changing the sleep backend or DSP.

Other rows also separate process-call wall time from the internal timer.
For example, frame 51,464,704 has a 1.929 ms process call but only a 12.375 µs internal callback measurement.
Thus a long call can occur outside the internally timed region as well.
Neither wall timer identifies why execution paused.
These virtual misses remain distinct from physical driver xruns or acoustic dropouts.

## Painter cadence

All 180,000 requested offscreen paints complete, averaging 50 per second over the timed interval.
However, 161 starts are more than one 20 ms paint interval late, so the stated follow-up painter gate fails.
Update-and-paint p99 is 12.821 ms, p99.9 is 27.738 ms, and maximum is 94.280 ms.
Those durations include scrolling, parameter selection, the editor-copy movement and production painting.
They also include any descheduling of that ordinary-priority thread.
The result does not establish native presentation cadence or isolate a particular drawing operation.
Retain the late-start count and duration tails rather than calling the average a complete pass.

## Activity and retained evidence

The run uses `caffeinate -i` to prevent idle system sleep for the driver lifetime.
There is no local compilation, sanitizer run, physical-device test or native preview interaction during the timed host.
Operator activity consists of source/document review, small Git operations, CI checks, artifact downloads and metadata/log/hash verification.
The aggregate records those activities; other user and system activity is not controlled.
This is not a claim of an isolated machine.

The original fixture-side fault files are copied byte for byte before a later preflight can replace them.
Their hashes and original relative-path context are retained locally with the raw run.
The preflight already reopened and checked its saved prefixes.
The completed captured set and raw logs also remain local; no user library or private media is published.

## Next qualification

Report the separate 256-frame fallback with the same source, workload and limits.
It cannot substitute for the failed 128-frame result.
Use the observed boundary correlation and CPU-versus-wall diagnostics to select the next timing repair.
Measure painter tails with enough context to distinguish computation from scheduling delay.
Keep native seek/export observations and physical-device evidence separate and unqualified until actually performed.
