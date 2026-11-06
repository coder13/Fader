# Short CPU diagnostic with full timeline density

Status: implementation candidate; local build, seven native suites and 19 Python checks pass.
Clean cross-platform CI and the first full-size concurrent diagnostic remain pending.
This supports investigation of the [failed painter gate](2026-11-05-playback-activity-hour.md), not a performance pass.

## Keep project size separate from run time

The ordinary short stress option builds a short project with fewer stored points.
It cannot represent the full hour's arrangement drawing load.
The new CPU-only `--timeline-seconds` option keeps a longer project while limiting the measured host interval.
Both clips retain their full length, source mapping and four stored points per second on each of six curves.
The timed host still runs the same input rates, delayed sources, background cadence, scrolling, alternating clip movement and 50-paint/s work for its requested interval.
Timing-history array lengths follow that interval, so a short run cannot qualify the original hour's memory or sustained timing targets.

An expanded timeline requires `--cpu-diagnostics`, must be at least as long as the timed interval and cannot exceed 3,600 seconds.
Invalid settings fail before media import.
The native parser rejects partial integers and duplicate options.
CPU instrumentation always forces `performanceQualificationEligible` and `hourTargetPassed` false.
The independent reader also rejects a short run that claims an hour pass, a reduced project/point declaration, or an expanded timeline without CPU diagnostics.
Existing default timing and project-size behavior remains unchanged.

For a five-minute diagnostic with the full 60-minute drawing scale:

```sh
python3 scripts/run-software-stress.py \
  --source-root /path/to/clean/candidate \
  --build-source /path/to/build-source \
  --probe /path/to/build-source/build/native/fader_probe_artefacts/Release/fader_probe \
  --build-info /path/to/build-source/build/native/fader-build-info.json \
  --fixtures /tmp/fader-stress-fixtures \
  --output /tmp/fader-full-timeline-cpu-evidence \
  --seconds 300 --timeline-seconds 3600 --block 128 --scheduler realtime --cpu-diagnostics
```

The equivalent probe option is `--timeline-seconds=3600`.
The expected original project has 172,800,000 frames and 86,406 stored points before capture compilation.
The report names the actual project frames, stored points and requested diagnostic timeline.
Keep the measured checkout/build source/binary unchanged until the whole driver and archive finish.

## CPU and scheduling context

Retain the existing per-callback and per-paint CPU readings with their wall brackets, painter context, bounded tails, omissions and maxima.
Add five host policy snapshots: painter before/during the activity, virtual callback thread before/after its loop, and painter after workers join.
These queries run outside the production Engine/Playback/device callback, not on every paint or callback.
Their wall brackets and query cost are recorded.
Owned Mach thread-port references are released after each query.
No query changes a priority or persistent system setting.

On Mac, record `pthread_get_qos_class_np`'s requested class/relative priority and its error status.
The SDK states that QoS overrides are not visible through this interface; the value is not continuous effective priority.
Also record the Mach time-constraint query status, returned default-policy flag, raw period/computation/constraint ticks, timebase and port-release status.
A returned default policy must not be described as a thread's active real-time constraint.
The sampled endpoints cannot prove a policy was held throughout an interval or identify a kernel wait or page fault.
Other platforms report these Mac-specific policy queries as unsupported.
Existing Windows CPU-accounting precision limits still apply.

## Checks completed and remaining

All targets build locally and seven native suites pass in 13.87 seconds.
The native case times two seconds of a four-second project and checks the declared count of 102 original stored points.
It reloads the capture and makes sure that both clip lengths remain unchanged.
It also retains every untouched second-clip point beyond the timed interval.
CPU coverage is complete and both qualification flags stay false.
Invalid native settings are checked against a missing fixture folder to prove rejection happens before import.
Direct CLI checks reject missing CPU mode, malformed integers and duplicate timeline options.
The native callback allocation checks remain enabled.

The 19 Python tests include wrapper guards, declared project density/duration, exclusion and signed pre-origin policy brackets with lifecycle ordering.
The independent reader accepts all three local smoke objects, including the expanded-timeline object.
All five Mac snapshots report successful pthread queries and thread-port release; error/default-policy fields remain visible.
These ordinary local smokes use the default thread mode and do not prove the requested real-time host policy.

CI retains independent checks of `fullTimelineDiagnosticSmoke` on Mac and Windows alongside both existing objects.
The completed results below record exact metadata, full logs, JUnit and artifact inspection.
The full-size short diagnostics are complete, while the actual painter repair remains open.
The original full-hour painter failure remains open; any repair still needs the full original uninstrumented hour.

## Completed full-size observations

Source `af66884f3096160b654595302ea07535378d9967` passed seven native suites on Mac and Windows, five Linux core suites, and 19 Python tests per runner.
[CI run 42126466639](https://github.com/coder13/Fader/actions/runs/42126466639) retained exact source metadata, JUnit, complete logs, and three independently read smoke objects on each native platform.
All builds used Release configuration and disabled UI fixtures.
Native builds used JUCE `d60b7a78030661574a615b586d9325ba7a7fa7dd`.
The downloaded executable, six fault files, original capture and rebased capture matched their recorded hashes.
The original runners checked physical source identity during capture rebasing.
Those runner-local sources are absent here, so artifact inspection does not reestablish that identity locally.
Windows CPU accounting retained its coarse precision and zero-delta limitations.

Two local diagnostics used the same clean source and executable SHA-256 `70e35d8c069f2c2d912dd62e333f93928842b78f2adc0f5bef14e2dea89ef266`.
Both used 128-frame virtual callbacks, full 3,600-second project geometry, 172,800,000 project frames and 86,406 original stored points.
The host was the Apple M3 Mac with 16 GiB RAM, macOS 26.6.2 and AppleClang 21.
The production offscreen painter still rendered every visible waveform column, curve and selected point handle.
The original media, exact executable, raw fault prefixes and both captures remain in local evidence folders.
The driver copied the executable before execution and the original capture before production rebasing.
Independent readers accepted timing arithmetic, bounded tail accounting and five policy-query brackets for both runs.
These CPU-instrumented diagnostics cannot qualify an hour, physical audio or native-window responsiveness.

| Observation | 300-second CPU diagnostic | 180-second sampled diagnostic |
| --- | ---: | ---: |
| Callbacks / captured controls | 112,500 / 8,999 | 67,500 / 5,399 |
| Completed paints | 15,000 | 9,000 |
| Paint starts more than 20 ms late | 1,113 | 120 |
| Paint wall p99.9 / maximum | 126.484 / 282.846 ms | 54.841 / 294.595 ms |
| Paint CPU p99.9 / maximum | 13.203 / 15.238 ms | 12.769 / 16.825 ms |
| Maximum paint start lateness | 1,278.681 ms | 445.417 ms |
| Virtual deadline misses | 0 | 9 |
| Source gaps | 0 | 0 |
| Retained paint tails / omitted | 256 / 1,181 | 218 / 0 |
| Peak resident bytes | 67,534,848 | 63,553,536 |
| Whole archive finished UTC | 23:12:34.850203 | 23:24:57.565656 |

The longest 300-second paint used 6.846 ms of enclosing thread CPU across 282.846 ms of wall time.
This discrepancy supports investigating waits and native drawing allocation.
It does not identify the cause of every pause.
The sampled run disturbed all threads, so its nine virtual misses do not establish an audio capacity regression.
Both runs preserved every captured control and passed software integrity checks.
The rebased captures preserved every non-path field, JSON value type and floating-point bit pattern.
The 300-second capture compared 91,725 floating-point values across two physical source identities.
The 180-second capture compared 89,605 floating-point values across two physical source identities.

[300-second aggregate](data/2026-11-05-full-timeline-cpu-300s.json) and [180-second aggregate](data/2026-11-05-full-timeline-stack-180s.json) retain the observations and archive hashes.
No raw media, private paths or raw process stacks appear in these aggregates.

## Sampled drawing paths and next repair

The read-only sampler attached to the exact retained synthetic process, after the driver reported 90 seconds.
It requested 55 seconds at 10 ms intervals, starting at 23:23:01.354039 UTC.
The command finished at 23:24:02.331552 UTC, including symbol processing.
No local build, second native host, hardware access or measured-source change overlapped either diagnostic.
Read-only GitHub artifact download and inspection overlapped the sampled diagnostic.

The main-thread tree contained 5,019 observations.
The frame-pacing branch contained 3,327 observations, mostly expected sleep calls.
The paint branch contained 1,528 observations, including 1,500 in the arrangement canvas.
Of those canvas observations, 589 reached individual handle ellipses and 575 reached curve path drawing.
Another 91 reached waveform vertical lines.
CoreGraphics raster allocation and deallocation reached `madvise` beneath both handle and curve paths.
These tree counts overlap by ancestry and are not CPU percentages or exact blocked durations.

Our inference is that repeated native raster operations contribute to the observed stalls.
The first proposed repair batches visible, opaque point handles into bounded compound paths.
Every supplied point keeps its position and four-pixel radius.
Overlapping handles form the same filled geometry, with small antialiasing differences at their edges.
Render comparisons must cover dense overlaps, fractional positions, viewport boundaries and each batch boundary.
The curve path remains a second measured target if handle batching does not resolve the pauses.
A full original uninstrumented hour still decides issue #40 after an actual repair.
