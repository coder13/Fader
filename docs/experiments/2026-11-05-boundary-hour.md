# Completed full-scale CPU diagnostic

The diagnostic completes the declared hour and preserves capture, media, save and memory integrity.
It still records three virtual deadline misses and 80 late painter starts.
CPU observations make expensive computation a poor first explanation for the longest wall intervals.
They do not identify a scheduler, memory-fault or power-policy cause.
This instrumented run cannot qualify performance.

The [full aggregate](data/2026-11-05-boundary-hour.json) includes final metadata, faults, retained rows, independent checks and operator activity.
The [uninstrumented 128/256 results](2026-11-05-software-stress-followup.md) remain failed and separate.
The [diagnostic design](2026-11-05-boundary-diagnostics.md) defines clock brackets, capacity and precision limits.

## Exact workload and completion

The clean measured source is `a00dca80d3659f310ff6aea45b731ac5aa60c21b`.
Compiled diagnostic code is `806f4252b7c442e077a0ea9463b360b3814d97ad`.
The driver compares every tracked source file with the build source before execution.
The source-tree hash is `7e0dd1ea1de06131da5010097dc5c8c1e2b24acd5a2e076877c6f8cf59aee016`.
The executable hash is `c15fad316fed3c5fd7c3329e2bc13d6d45f6ea01bb935047f27f7d9a28f60925`.

The build uses Release C++20, AppleClang 21, arm64 macOS 26.6.2, CMake 4.4.4 and pinned JUCE `d60b7a7`.
UI fixtures are ON locally.
All seven local native suites pass before this run.
Exact diagnostic code also passes the checked macOS/Windows native and Linux core CI recorded in the design report.

The original 3,600-second timeline, stored curves and baseline history scale remain unchanged.
CPU histories and native queries add the declared observer overhead.
The virtual host runs at 48 kHz/128 frames with the same real-time request, delayed reads and background jobs.
No audio or MIDI device opens.
The painter renders the production component offscreen, without native presentation.

The timed probe finishes at `2026-11-05T15:33:37.838158Z`.
The complete driver and production archive finish at `2026-11-05T15:34:20.630946Z`, with exit code 3.
The observed interval is 3,600.002482375 seconds.
`performanceQualificationEligible` and `hourTargetPassed` remain false by design.
The timing and painter counters also fail their original targets independently.

## Integrity, persistence and memory

All 1,350,000 callback and host observations survive with zero invalid or duplicate metrics.
All 107,999 planned controls are attempted, accepted, captured and correlated.
Input, capture-retention, callback/control/host metric losses and failure flags are zero.
Source starvation, source failures, non-finite output and quiet blocks are zero.
The 1 MiB PCM cache serves 42,202 reads with the declared 1 ms delay.
Library and analysis jobs each run 360 times, with 120 seek cancellations.

The saved manifest contains 150,032 compiled points in 7,438,454 bytes, below the unchanged 8 MiB limit.
Save takes 0.360349375 seconds.
The production archive reopens the set and writes 7,438,508 bytes after relative-path rebasing.
An independent recursive comparison removes only asset paths and matches every remaining JSON type and value.
It also matches all 150,040 parsed floating-point values bit for bit.
Both rebased assets resolve to the same original synthetic sources.

The process peak is 111,722,496 bytes, or 106.546875 MiB, below the unchanged 128 MiB budget.
The peak before capture commit is 106,463,232 bytes, or 101.53125 MiB.
Exact original set bytes, the exact measured executable and six raw fault-prefix files are preserved locally with hashes and path context.
Archive preservation does not independently prove capture/replay parity.

## Callback observations

Callback wall p99.9 is 78.750 microseconds, but the maximum is 4.769375 ms, above the 2.666667 ms interval.
The enclosing callback CPU distribution has p99.9 of 79.750 microseconds and a maximum of 356.291 microseconds.
All 1,350,000 CPU observations are valid and ordered, with zero clock-read errors.
The native API reports 42 ns nominal resolution, which does not guarantee accounting precision.
CPU readings include query overhead and enclose the process call rather than measuring DSP alone.

| Project frame | Wake lateness | Internal wall | Enclosing CPU | Deadline lateness |
| --- | --- | --- | --- | --- |
| 15,470,464 | 42 ns | 4.769375 ms | 158.208 microseconds | 2.104166 ms |
| 36,040,576 | 42 ns | 3.317875 ms | 112.042 microseconds | 0.652625 ms |
| 106,704,768 | 41 ns | 2.828459 ms | 126.459 microseconds | 0.163458 ms |

Every missed callback again begins at `4096*n - 128`.
Thirteen of the seventeen retained host tails share this remainder.
No host tail is omitted.
The full boundary population contains 42,187 blocks, compared with 1,307,813 other blocks.
Their maximum internal wall durations are 4.769375 ms and 2.060709 ms respectively.
This project-frame correlation does not establish a native page boundary or a paging fault.

The missed rows have query brackets below 0.6 microseconds each.
Their enclosing wall windows retain the long pauses, while their CPU deltas remain much smaller.
This makes high measured thread CPU use an unsupported explanation for these particular misses.
It does not identify why the thread stops making progress.
Do not turn wall-minus-CPU into a precise blocked-time estimate.

Predicted receipt-to-software-output p95 is 5.153500 ms under the declared one-block virtual buffer assumption.
This is not physical or acoustic latency.
Producer wake-to-receipt jitter remains separately reported.

## Painter observations

All 180,000 paints complete, but 80 starts are more than 20 ms late.
Update-and-paint wall p99.9 is 25.665542 ms and maximum is 72.842417 ms.
All 180,000 enclosing painter CPU observations survive with zero read errors.
Their p99.9 is 8.639500 ms and maximum is 13.517500 ms.
These clocks include query cost and do not measure native display presentation.

The largest wall interval occurs at paint 106,381, viewport X 31,143, high-pass parameter, with the editor clip displaced by one second.
Its enclosing CPU delta is 5.918791 ms across a 72.842417 ms wall interval.
The largest CPU observation occurs at paint 144,000 after the viewport returns to zero.
It records 13.517500 ms of CPU across a 16.485042 ms wall interval.
The maximum start lateness is 53.909916 ms at paint 66,015.

The bounded painter trace retains 256 of 479 slow/late iterations and reports 223 omitted rows.
Global execution, wake and CPU maxima remain available after truncation.
This is not a complete painter-tail history.
The distributions retain all actual paint samples, independently of that row bound.

## Separate logical cache boundaries from frame correlation

A further read-only check separates the 4,096-frame pattern from the cache's 8,192-frame logical pages.
The [derived counts and missed rows](data/2026-11-05-boundary-moduli.json) name and hash the published input aggregates.
All host tails are present in these three inputs; none is omitted.
The calculation uses `(projectFrame + blockFrames) % 8192` for each retained tail.

| Completed run | Tails ending at logical page boundary | Tails ending halfway through a page | Other retained tails |
| --- | --- | --- | --- |
| Uninstrumented 128 frames | 11 | 23 | 5 |
| Uninstrumented 256 frames | 1 | 2 | 25 |
| CPU diagnostic, 128 frames | 3 | 10 | 4 |

All three CPU diagnostic missed blocks end halfway through a logical source page.
The uninstrumented 128-frame misses split into two halfway endings and one page-boundary ending.
The 256-frame misses remain the separately reported startup scheduling observation.
These tail counts are not per-population failure rates; the retained-tail selection is duration/wake based.

The fixture's two sources start at project zero with source offset zero, rate one and 48 kHz source rate.
The painter moves its separate editor copy, not the playback snapshot.
Under these anchors, a rendered project frame has the same integer source index.
In [StreamBank::sample](../../src/Streaming.cpp), the explicit following-page acquisition occurs at source index remainder 8,191.
A block ending at remainder 4,096 does not execute that branch.
That branch therefore cannot be a necessary explanation for all three CPU diagnostic misses.
Ordinary page pinning still occurs at callback entry, and this observation does not rule out other memory or scheduling effects.

No native PCM addresses, per-thread fault counts or kernel scheduling trace were captured.
The 4,096-frame pattern alone cannot identify a native memory page boundary.
Use the scoped activity experiment and, if needed, a trace that distinguishes execution, scheduling and faults before changing cache layout.
This arithmetic check adds no playback run and changes no performance limit.

## Independent checks and next experiment

The independent reader at [exact source `c9a68f5`](https://github.com/coder13/Fader/blob/c9a68f549c1f6637fac9f46f7bf1593a4b0c9acd/scripts/verify-stress-observations.py) checks 21 serialized host rows and 259 painter rows.
Global maxima can repeat retained tails.
All 280 serialized CPU observations pass delta, bracket and context arithmetic checks.
None exceeds its enclosing wall window.
The largest query bracket among retained rows is 29.750 microseconds, not a bound for unseen queries.
Passing these checks does not qualify performance, precision or unseen rows.

The next candidate will test a scoped user-initiated process activity during playback before changing DSP or drawing algorithms.
Apple documents activities as hints for lengthy user-requested work, including recording and export.
Activities must end when the work finishes, and user preferences can override the request. [Apple activity guidance](https://developer.apple.com/library/archive/documentation/Performance/Conceptual/power_efficiency_guidelines_osx/PrioritizeWorkAtTheAppLevel.html), [ProcessInfo](https://developer.apple.com/documentation/foundation/processinfo)
The pinned JUCE implementation provides such a scoped activity on macOS and a no-op elsewhere.
This is an evidence-directed policy experiment, not proof that App Nap caused the observed pauses.

Keep the callback workload, origin, painter cadence, density, storage limits and memory budget unchanged.
Qualify an actual candidate without CPU instrumentation, preserving this failed diagnostic and the earlier failed hours.
A quiet candidate run alone cannot identify a kernel or memory cause or establish a long-term failure rate.
Native presentation and physical-device gates remain separate and open.
