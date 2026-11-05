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
Cross-platform CI, a synthetic image comparison and the full-size short CPU diagnostic remain pending for this candidate.
A short diagnostic cannot qualify the original hour, physical output or native-window responsiveness.
The next eligible performance run must retain the original full workload and thresholds.
