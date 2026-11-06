# Current-geometry automation tiles

Issue [#40](https://github.com/coder13/Fader/issues/40) retains its original workload and acceptance targets.
This experimental candidate extends the [software automation layer](2026-11-05-software-automation-layer.md).
The completed parent profile places most automation work in software rasterization, rather than native image copying alone.
The candidate reuses small rendered regions when their complete current geometry remains equal.
No sustained performance result exists for this candidate yet.

## Production drawing and validity

The production arrangement canvas owns the cache.
On native Mac graphics at one- or two-times backing scale, each tile covers 128 logical pixels with 12 pixels of bleed.
The tile height follows the visible automation band, with a maximum of 142 logical pixels.
The actual production band is at most 93 pixels tall.
Other scales, unsupported paths and larger bands use the prior current-frame layer.
Other platforms and Mac software graphics retain direct curve and handle drawing.

The caller still constructs every original visible per-pixel curve vertex and supplies every visible handle.
The cache does not simplify a curve or reduce its stored or displayed density.
Each key contains the current local path vertices, handle centers, color, stroke width, backing scale and band height.
The cache compares those values directly.
A pointer, point count or unchecked hash cannot establish validity.
Edits in an existing vector allocation therefore change the current image immediately.

A tile includes adjacent path vertices and every handle that can touch its padded area.
The 12-pixel margin covers the native miter limit and four-pixel handle radius.
Tile centers partition the output without overlapping opacity.
Negative tile indices retain the left half of handles at clip starts.
Local coordinates preserve precision far along the timeline.

Only tiles with complete path coverage enter the cache.
Partial viewport edges redraw through a reusable scratch image on every paint.
This prevents changing edge signatures from evicting otherwise reusable interior tiles.
Moving or editing a clip changes its geometry before lookup.
Scale changes discard old images and signatures.

The tile image uses native pixel storage with an explicit software graphics context.
Software writes invalidate its cached native image.
An unchanged tile reuses its immutable native image and avoids a new raster or copy.
A normal tile draws the union of its relevant handle circles.
This changes antialiasing at former 64-circle batch boundaries, while retaining every center and radius.
The output is not a pixel-identical replacement for the parent renderer.

## Bounds and measurements

The cache retains at most 64 entries under a 32 MiB accounting budget.
Accounting includes retained pixel storage, its immutable native copy, vector capacities and a bookkeeping allowance.
It also includes the current scratch image, path workspace and candidate signature.
These values do not measure every native allocation or the process resident set.
The original whole-process 128 MiB target remains unchanged and still requires measurement.

A cached signature can retain at most 1,024 vertices or handles.
A tile with more relevant geometry renders uncached and still draws every supplied handle.
The input path reader accepts at most 32,768 monotone vertices before choosing the full current-frame fallback.
Neither limit authorizes dropping displayed geometry.
All allocation, lookup and raster work runs on the UI thread.
No callback, source decoder, automation capture or scheduler changes.

The stress report records hits, rasterizations, partial edges, dense fallbacks, retained entries and accounted byte peaks.
The independent reader checks integer types, original limits, accounting bounds and raster counters.
Old reports without cache fields remain readable.
This diagnostic metadata does not qualify performance.

## Functional evidence and next gate

Dedicated image checks compare a warm cache with fresh rendering after scrolling, clip movement, fractional starts and parameter changes.
They cover scale transitions, selection, color, width, in-place edits, handle-only edits, insertion, deletion and eviction.
A 2,048-handle fixture requires a visible handle beyond the signature limit to survive its uncached fallback.
Independent existing tests retain curve geometry, all sampled vertices, solid circle interiors, opacity and state restoration at both backing scales.

The final local Release build passes all seven native suites in 15.65 seconds.
All 20 Python helpers pass in 0.723 seconds.
Fresh independent readers accept all three smoke objects, including the new byte accounting.
The short smoke projects have no complete interior tiles, so their zero hit counts do not test reuse.
The dedicated long-curve fixtures require reuse and immediate invalidation explicitly.
Clean Mac, Windows and Linux CI remains pending.
The branch is [`codex/issue-40-automation-tiles`](https://github.com/coder13/Fader/tree/codex/issue-40-automation-tiles).
A synthetic offscreen image remains separate from a native preview observation.
The next performance diagnostic uses the full 3,600-second project and unchanged stored automation density.
The original uninstrumented hour remains required after a measured repair.
The candidate remains experimental until the required evidence passes.
No hardware opens or physical latency claims form part of this work.
