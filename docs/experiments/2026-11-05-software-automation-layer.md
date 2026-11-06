# Viewport-sized software automation layer

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
The software stroke uses the native Mac join limit of ten.
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
Clean-source CI, the synthetic image and the full-size short diagnostic remain pending.
The original uninstrumented hour remains required after a measured repair.
No hardware or physical latency claim follows from these image checks.
