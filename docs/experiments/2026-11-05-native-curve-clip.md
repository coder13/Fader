# Mac automation curves through a native outline clip

Candidate source [`172a73664d19ce708aa8e1405299216e71dadcb9`](https://github.com/coder13/Fader/tree/172a73664d19ce708aa8e1405299216e71dadcb9).
[Clean CI 42134061694](https://github.com/coder13/Fader/actions/runs/42134061694) is pending.
These implementation changes remain separate from main.

Issue [#40](https://github.com/coder13/Fader/issues/40) remains open after [handle batching](2026-11-05-handle-batching.md).
That candidate retains long wall pauses and increases median drawing CPU in the full-size short diagnostic.
The earlier stack profile identifies native curve stroking as another costly drawing path.
This candidate changes the production arrangement renderer while retaining the original workload and thresholds.

## Drawing change and visible effect

For native Mac graphics, fill a curve with a gradient whose endpoints have the same color and opacity.
The pinned JUCE CoreGraphics implementation routes this through `CGContextReplacePathWithStrokedPath`, an outline clip, and a gradient fill.
The pad spread fills before and after the gradient endpoints, so the result keeps a uniform color.
The implementation retains the native stroke width, cap and join settings.
Other platforms and the Mac software renderer retain the ordinary solid stroke.
A scoped graphics state restores the previous color, clip and transform after the helper returns.

Every original per-pixel curve vertex remains in the path.
Every visible handle, waveform column, stored automation point, clip position and duration remains in the workload.
The native outline clip changes antialias coverage at sharp tips and applies the requested opacity once.
It is not a pixel-identical replacement for the native solid stroke.
No audio, automation interpolation or project storage behavior changes.

A preliminary explicit JUCE outline with its default join limit missed native sharp tips.
Matching the native join limit and using Quartz's explicit outline still changed some tip coverage.
Those render differences ruled out treating old edge pixels as a mathematical definition of the curve.
The retained implementation uses the native stroke settings directly, through JUCE's supported graphics API.

## Geometry and state checks

Tests cover native and software images at one- and two-times scale, two stroke widths, two opacity values, and three dense curve shapes.
The shapes include a flat line, a sinusoid and repeated sharp corners at fractional coordinates.
Each path retains all 1,100 sampled vertices.
Every vertex must retain image coverage, and solid reference interiors must survive.
Every output pixel above 16/255 alpha must stay inside an independently computed segment and miter envelope.
That envelope includes 0.8 physical pixels for antialiasing and a join limit of ten.
The output cannot accumulate opacity above the requested value.
A separate mark makes sure that the helper restores the previous graphics color.

The handle tests use geometric circle interiors, preserve the independent one-pixel edge comparison, and retain all 133 points.
Existing viewport-edge, far-timeline rounding, clip movement, fixed duration and independent-curve edit tests remain enabled.
The media runner now catches failures from these early tests and prints the failed assertion.
Renderer identifiers aid Windows diagnosis if its native image backend fails before an assertion.

The final local build passes all seven native suites in 15.09 seconds, including the one- and two-times geometry matrix.
All 19 Python helper tests pass.
Fresh independent readers accept all three native smoke objects.
The synthetic screenshot below is complete.
Clean-source CI and the full-size short concurrent diagnostic remain pending.
The original uninstrumented hour remains the performance qualification gate.
CPU-instrumented or isolated drawing results cannot satisfy that gate.

## Isolated synthetic image

The exact retained probe SHA-256 is `62e181c704c335b340ae49685edf10c1ac70b6111933daf1856e59b6cac986e1`.
The synthetic offscreen snapshot completed on 2026-11-06 at 00:05:01.391049 UTC.
It preserves the 1,200-by-700 arrangement image, original project length and 14,401 points per clip parameter.
The image keeps the same clip positions, widths, waveform columns and selected handles.

The isolated 100-paint median was 11.730209 ms, p95 was 29.302709 ms and maximum was 49.029208 ms.
The preceding handle-batching case recorded a 5.217708 ms median and a 7.643958 ms maximum.
These sequential isolated observations indicate a cost increase and do not support adopting the native curve clip.
The same-source concurrent 300-second CPU diagnostic is still active, with its original full-size project and workload.
Keep that result even if it also fails.
Neither isolated images nor CPU instrumentation qualify the original hour.
