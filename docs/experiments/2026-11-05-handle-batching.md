# Bounded drawing of automation point handles

Issue [#40](https://github.com/coder13/Fader/issues/40) still fails the original hour's painter-start target.
The [full-size diagnostic](2026-11-05-full-timeline-painter-diagnostic.md) identifies CoreGraphics work beneath individual handle ellipses and automation curve paths.
This candidate changes the production arrangement painter, which also serves the software workload.
It fills paths containing at most 64 visible point handles instead of one native ellipse call per handle.
Each point keeps its four-pixel radius, clip-local position and selected curve color.
The viewport slice and every stored curve remain unchanged.

Each batch uses nonzero winding, so overlapping circles remain filled rather than canceling into holes.
Path storage reserves 2,048 float coordinates, approximately 8 KiB, and retains that storage between batches within a paint.
The implementation does not cache the project history or omit dense visible handles.
Opaque overlaps form a filled union, which changes antialiasing at overlapping edges.
The curve path and waveform rendering remain unchanged.

The new image test compares production handle drawing against the former individual-ellipse loop on both native and software images.
It covers 133 handles, dense overlaps, fractional clip positions, three zoom scales and both 64-handle boundaries.
Every originally solid pixel retains at least 250/255 alpha, and both images stay within a one-pixel edge fringe of each other.
The test also makes sure that painting does not change the stored points.
Existing viewport-edge, far-timeline rounding, clip movement, duration and independent-curve edit tests remain enabled.

All local native targets build, and seven suites pass in 15.73 seconds.
All 19 Python helper tests pass.
Fresh independent observation checks accept the three native smoke objects.
The completed sections below record cross-platform CI, a synthetic image and the full-size short CPU diagnostic.
This candidate still fails drawing cadence and Windows image checks.
A short diagnostic cannot qualify the original hour, physical output or native-window responsiveness.
The next eligible performance run must retain the original full workload and thresholds.

## Completed full-size short diagnostic

Clean source `99b1fb4b972cd1c2c2c4e25d77d5e7d13224a3a0` completed the same 300-second CPU diagnostic with the original 3,600-second project.
The exact retained probe SHA-256 is `3fee3a939b651af4ae6598a9db318fae52b006d0c266725d731e0aacf6c5a4c5`.
All 169 tracked source files matched the build stage before execution and again after the whole archive completed.
The [aggregate](data/2026-11-05-handle-batching-cpu-300s.json) retains every result, including failed flags.
The driver finished on 2026-11-05 at 23:45:13.197433 UTC.
No build, second host, sanitizer or sampler overlapped the run.
Read-only CI status and completed-job log inspection overlapped it.

The run retained 112,500 callbacks, 8,999 controls and 15,000 paints, with zero source gaps or virtual deadline misses.
All software integrity checks passed.
There were 263 late paint starts, versus 1,113 in the preceding full-size diagnostic.
Paint wall p99.9 was 61.475208 ms, maximum 445.055625 ms, and maximum start lateness was 575.608500 ms.
The longest wall paint used only 11.317167 ms of enclosing thread CPU.
The trace retained 256 of 425 tails and reported 169 omissions, with global maxima retained.

Median wall time increased from 5.968708 to 6.577000 ms.
Median paint CPU increased from 5.625583 to 6.432209 ms, while maximum paint CPU reached 16.179416 ms.
These sequential desktop observations do not establish a causal improvement or stable failure rate.
Batching alone does not repair the painter gate.
The CPU flag also makes this run ineligible for hour qualification.
Peak probe RSS was 65,748,992 bytes, below the original budget but not a full-hour memory result.

The independent reader accepted six retained host rows and 259 paint rows, including repeated maxima.
The retained executable, six raw fault files, original compiled capture and rebased capture matched their recorded hashes.
The production archive preserved every non-path field, value type and floating-point bit pattern across two physical source identities.
Every original asset and capture remains local, with no raw user media or private paths in the public aggregate.

The 100-paint isolated snapshot recorded a 7.643958 ms maximum, versus 12.866334 ms for the preceding same-painter baseline.
This isolated case does not include concurrent playback or 50 Hz pacing and cannot replace the failing concurrent result.
The synthetic offscreen 1,200-by-700 image was inspected and shared in the chat.
It keeps the same clip sizes, waveforms, point centers and radii.
The next repair targets native curve stroking, the other hotspot already visible in the preserved stack profile.

## Cross-platform check

[CI 42130489032](https://github.com/coder13/Fader/actions/runs/42130489032) passes Linux core and helpers, and all seven Mac native suites plus 19 helpers.
Downloaded metadata and JUnit match exact source `99b1fb4`, Release configuration, the pinned dependency and fixtures OFF.
Windows passes six native suites but fails `media` with exit `0xc0000409`.
Its complete log prints no failed assertion.
The arrangement, audio setup and virtual playback tests ran before the existing exception handler, which prevented useful failure reporting.
The next candidate moves those calls inside that handler and adds renderer context.
This result does not qualify the candidate on Windows.

The original handle test treated every fully opaque reference pixel as a solid circle interior.
Repeated overlapping antialias edges can accumulate opaque reference pixels outside any individual circle's solid interior.
The revised test defines solid interiors geometrically: pixel centers within three pixels of a four-pixel circle center.
Every such pixel must retain at least 250/255 alpha and the selected color.
The independent edge coverage comparison remains within one pixel, and every stored point remains unchanged.
The missing Windows assertion prevents attributing its failure to this test-oracle problem before the next run.
