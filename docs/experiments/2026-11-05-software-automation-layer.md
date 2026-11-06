# Viewport-sized software automation layer

Candidate source [`e38e390ca3b673ba057a4e84d988679bc842256e`](https://github.com/coder13/Fader/tree/e38e390ca3b673ba057a4e84d988679bc842256e).
The implementation remains an experimental branch.

Issue [#40](https://github.com/coder13/Fader/issues/40) retains its original workload and performance thresholds.
[Handle batching](2026-11-05-handle-batching.md) alone retains long pauses.
The [native curve clip](2026-11-05-native-curve-clip.md) increases drawing CPU and remains an unsuccessful experiment.
This candidate changes the production arrangement component to draw its current automation into a small software image.
The benchmark still paints that production component into its original native target.

## Drawing and memory

On native Mac graphics, the component draws each visible curve and selected handle into a reusable ARGB software image.
It then composites that image once through the native graphics context.
The grid, waveform columns and text retain their existing drawing path.
Every original per-pixel curve vertex and every visible point handle remains in the work.
The image clears and redraws on every call, so it never substitutes stale geometry for current edits.
Other platforms and the Mac software context retain direct curve and handle drawing.

One arrangement canvas owns one buffer, rather than one buffer for every clip or the entire set.
Its width follows the viewport and its height follows the visible automation band.
A 12-pixel margin covers the maximum 2.2-pixel stroke's native miter extension and the four-pixel handles.
Selected and unselected rows use the same margin.
The capacity grows for partially visible rows and resets with the backing scale, avoiding repeated height changes between clips.
All image allocation and raster work runs on the UI thread.
No callback, playback scheduler, decoder or automation-capture path changes.

The buffer uses the graphics context's physical pixel scale, including Retina scale.
Curve coordinates become local to the visible band before stroking, which keeps calculations precise far down a set.
The software stroke uses a join limit of ten, matching the [Cocoa graphics-state default](https://developer.apple.com/library/archive/documentation/Cocoa/Conceptual/CocoaDrawingGuide/GraphicsContexts/GraphicsContexts.html).
The composite applies opacity once and restores the original graphics state.
Software and native antialiasing differ, so this is not a pixel-identical replacement.

The pinned JUCE software image has no native image extension.
Its Mac composite uses `CoreGraphicsPixelData::createCGImage`, which copies the bytes with `CFDataCreate` before creating the provider.
That independent copy keeps a prior composite safe when the buffer draws another clip.
The copy and native allocation still have a cost, so timing and peak memory require measurement.
A reused image does not imply an allocation-free native composite.

## Checks and remaining evidence

The geometry matrix covers native/software targets, one- and two-times scale, two widths, two opacity values and three dense curve shapes.
Every one of the 1,100 curve vertices must retain coverage.
An independent segment/miter envelope bounds colored pixels, and solid interiors plus requested opacity remain checked.
The handle checks retain all 133 points and compare independent circle interiors and edge coverage.
Existing viewport edges, far-timeline rounding, fixed clip duration and independent curve edits remain checked.

Additional tests paint two differently colored clips through one buffer before reading either result.
They grow the visible band, reuse a shorter band, and make sure that earlier pixels remain intact.
They also make sure that a 0.65 curve stays at 0.65 over transparency despite the caller's 0.2 opacity.
A separate mark tests clip and opacity restoration.
Near and 108,000-pixel-shifted curves plus handles must produce identical images at both backing scales.
A handle away from the curve must retain its opaque center.

The final local build passes all seven native suites in 15.74 seconds.
All 19 Python helper tests pass, and fresh readers accept all three local smoke objects.
Clean-source CI, the synthetic image and two full-size short diagnostics are complete.
The original uninstrumented hour remains required after a measured repair.
No hardware or physical latency claim follows from these image checks.

## Completed isolated and concurrent observations

The exact executed probe SHA-256 is `9e8822a3da8edf90e35565f0d5d5501345338834ea42fc392c66305e1a87d352`.
The isolated 100-paint snapshot completed on 2026-11-06 at 00:27:31.552493 UTC.
Its median was 2.634375 ms, p95 was 11.915500 ms and maximum was 54.985250 ms.
The synthetic offscreen image was inspected and shared in the chat.
It is not a native-window or concurrent performance result.

The [300-second aggregate](data/2026-11-05-software-layer-cpu-300s.json) retains the original 3,600-second project, 172,800,000 frames and 86,406 stored points.
Its whole archive finished on 2026-11-06 at 00:42:50.924207 UTC.
All 173 tracked files matched the committed source, build stage and starting manifest after completion.
The run retained all 112,500 callbacks, 8,999 controls and 15,000 paints.
Source gaps and virtual callback deadline misses were zero.
Software integrity passed, while 125 paint starts were more than 20 ms late.

| Observation | Handle batching | Native curve clip | Software layer |
| --- | ---: | ---: | ---: |
| Median drawing CPU | 6.432209 ms | 9.463083 ms | 4.151833 ms |
| Drawing CPU p99.9 | 12.252375 ms | 18.352292 ms | 9.385084 ms |
| Maximum drawing CPU | 16.179416 ms | 20.933834 ms | 10.757708 ms |
| Paint wall p99.9 | 61.475208 ms | 93.924541 ms | 50.965250 ms |
| Maximum paint wall | 445.055625 ms | 300.403833 ms | 210.086708 ms |
| Late paint starts | 263 | 562 | 125 |
| Maximum start lateness | 575.608500 ms | 522.691750 ms | 256.434917 ms |

These sequential desktop runs do not isolate every source of timing variation.
The software-layer observation shows lower median CPU, but it still fails drawing cadence.
The longest wall paint used 5.508292 ms of enclosing thread CPU across 210.086708 ms of elapsed time.
The trace retains all 202 tails without omissions and keeps global maxima.
Peak probe RSS was 69,599,232 bytes, which is not a full-hour memory result.
Import took 123.492824 seconds and remained outside the timed phase.
No competing local build, sampler, sanitizer or playback host overlapped this run.
Read-only CI inspection and private report preparation overlapped it.

The exact executable, six raw fault files, original capture and rebased capture match their recorded hashes.
The archive preserves every non-path field, JSON type and all 91,702 float64 values across two physical source identities.
The independent reader accepts six host rows, 205 paint rows and five policy snapshots without API errors.
CPU instrumentation keeps both performance eligibility and hour qualification false.
The original workload, point density, memory budget and thresholds remain unchanged.

## Sampled paths and next renderer step

The [180-second sampled aggregate](data/2026-11-05-software-layer-stack-180s.json) uses the same clean source, executable and full project geometry.
The exact process arguments and PID were checked before attaching the read-only sampler at the 90-second progress marker.
It requested 55 seconds at 10 ms intervals, starting at 00:52:59.624738 UTC.
The command finished at 00:53:55.771831 UTC.
The whole driver archive finished at 00:55:32.741326 UTC on 2026-11-06.

The main-thread tree contains 5,003 observations, including 3,740 in expected frame-pacing sleep.
One canvas call site contains 567 observations in the automation layer.
Its main child sites contain 266 observations in curve drawing, 222 in handle drawing, and 62 in native composition.
Software edge-table construction, sorting, allocation and cleanup dominate the first two sites.
The native copy creation accounts for 20 observations under its composition subtree.
Other canvas sites contain 129 observations in waveform columns and 84 in clip-border rectangles.
Memory cleanup reaches `madvise` beneath software rasterization and native border drawing.
These overlapping tree counts are not CPU percentages, exact blocked durations or proof of every pause's cause.

The sampled run retains all 67,500 callbacks, 5,399 controls and 9,000 paints, without source gaps.
It reports 377 late paint starts and 18 virtual deadline misses under sampler interference.
Sampling disturbs all threads, so these counts do not establish an audio capacity regression or a failure rate.
Its trace keeps 256 of 463 paint tails and reports 207 omissions.
Independent checks accept 47 host rows and 259 paint rows, including repeated maxima.
All 173 source files, retained hashes, archive fields/types and 89,597 float64 values match after completion.

The next proposed change caches bounded automation tiles whose current geometry matches the cached image.
Each paint must still build every original curve vertex and retain every visible handle.
Every tile must be checked against current path geometry, handle positions, color, stroke width and backing scale before reuse.
Partial viewport edges require current rendering and cannot reuse a stale interior image.
Tile count and bytes must remain bounded, including native image copies and cache metadata.
Tests must cover edits without vector reallocation, clip movement, panning, parameter changes, endpoints, cache eviction and Retina scale.
This cache is not implemented or qualified yet.

The existing JUCE activity requests `NSActivityUserInitiatedAllowingIdleSystemSleep`.
Apple also documents an [activity option for highest timer and I/O precision](https://developer.apple.com/documentation/foundation/processinfo/activityoptions/latencycritical).
That separate policy remains untested here.
The sampler and policy queries do not change process power flags or thread priorities.
No persistent system setting changed.
A system query during sampling reported 27 percent free memory, not continuous memory-pressure evidence or a causal explanation.

## Completed clean CI

[CI 42136827921](https://github.com/coder13/Fader/actions/runs/42136827921) passes seven native suites on Mac and Windows, five Linux core suites, and 19 helpers per runner.
The [checked aggregate](data/2026-11-05-software-layer-ci.json) retains exact source metadata, dependency pin, Release configuration, fixtures OFF and reader results.
Full logs and downloaded JUnit match those counts without failures or skipped suites.
Fresh readers match all three smoke outputs and the default short-driver output on both native platforms.
Retained executable, fault and capture hashes match.
Downloaded captures preserve every non-path field, type and float64 bit pattern.
Runner-local source identity remains recorded and is not independently reestablished from absent media here.
Windows CPU-accounting precision limits remain explicit in the artifacts.
Functional CI does not satisfy the failed drawing target or replace the required original uninstrumented hour.
