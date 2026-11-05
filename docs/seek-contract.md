# Seek preparation: qualification contract

Tracking: [DJ-039](https://github.com/coder13/Fader/issues/39). Status: implementation/qualification in progress. Targets below are set before measurement; they are not achieved results.

## Targets and measurement scope

For the current two-clip volume/HP/LP renderer on the development Apple Silicon Mac, target **warm p95 ≤150 ms** and **cold p95 ≤500 ms**, from accepted request to a ready playback engine. Report p50, p95, maximum and misses. Cold means a new engine/source reader, not an artificially purged OS disk cache. Fixtures include the two long overlapping WAVs from DJ-038 and CBR/VBR MP3 arrangements with distant/nonzero source positions. Import/validation/cache creation is measured separately and never hidden inside a warm seek result.

Compare nonzero-start master/cue PCM with uninterrupted sequential rendering, with an absolute tolerance of **1e-5 for the normalized synthetic fixtures**. Compare prepared volume/HP/LP values at the same tolerance. Explicitly test continuous ramps, held targets, bypass boundaries, delayed clip starts, source offsets/rates and changes immediately around the destination. Report errors rather than weakening the tolerance after measurement.

## Selected approach to test

Use at most **two seconds of project-frame preroll** for the existing finite volume smoother and simple stable one-pole HP/LP stages. Start those controls from their envelope values at the preroll origin and process the real source through the same engine until the destination. Retain exact-from-start preparation as the reference option. Processing from project zero, including ordinary full export, keeps its existing behavior. This bounded-preroll policy does not apply automatically to future stretchers, resonant effects, delay/reverb or third-party plugins; they need checkpoint/state/tail contracts of their own.

The pinned MP3 decoder's sequential-replay fallback is correct but is not a fast random-seek mechanism. Test a session-owned local PCM cache populated during the existing validation scan, so seeking preserves that decoder's exact sample domain without re-decoding every preceding frame. Bound aggregate decoded disk storage to **8 GiB**, keep the existing bounded memory pages, and retain original-file identity checks. If the cache cannot be admitted or written, preserve readable-source support with an explicit slower preparation path; report that mode and any missed target. Cache paths are internal and never serialized into the set or written into the music library.

## Cancellation, ownership and UI

Carry one cancellation flag through preparation, decode readiness and worker reads. Observe it between bounded render/decode chunks. A superseding request cancels the active preparation and replaces one pending request; old completions may not start playback or mutate the project. Scope each request to a generation and immutable project snapshot.

No callback waits, allocations or ownership reclamation are introduced. Keep playback destruction and source-worker joins off interactive UI actions. Bound pending requests, completed results and retired engines; do not queue an unlimited job for each mouse movement. Show Preparing and a Cancel action, allow a new seek to replace the old destination, and preserve the latest project on source changes/close. OS reads already in flight can outlive cancellation, so storage lifetime must remain valid until those reads return.

Native UI inspection and delayed-worker lifetime tests are required in addition to timing and PCM comparisons. None of these measurements qualifies physical input/output latency.
