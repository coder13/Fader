# Project storage version 3

Tracking: [DJ-011](https://github.com/coder13/Fader/issues/11). Implemented and qualified on the software fixtures below. This is the current file contract. The broader [set design](set-format-and-automation.md) also contains future features.

## Identity and ownership

Every project and clip has a 32-character lowercase hexadecimal ID, generated from 128 bits supplied by the platform random device on a control/worker thread. Copies used for undo, preparation and save preserve IDs. Moving, renaming or reordering a clip preserves its ID and local curves. Adding another instance of the same song creates a new clip ID. Duplicated IDs are rejected at persistence boundaries. Runtime event queues retain efficient clip indices within their immutable playback segment; those indices are compiled addresses, not portable identities. Current incomplete clip-take archives also retain clip ID and parameter name alongside the original index.

Assets live in a separate table. A clip references an asset ID, with an empty reference allowed only for a zero-length empty clip. Multiple clips can share one decoded asset while keeping independent placement, metadata, grids and automation. Missing/duplicate/unreferenced asset entries and dangling clip/parameter references are rejected before media access. The writer deduplicates identical encoded content and decoded sample domains; the first source location is its relative relinking hint. Rename/relocation does not change sound identity. Missing-media recovery remains separate work.

An asset records the encoded-file SHA-256, relative location, source sample rate, source frame count, original channel count, decoder name as provenance, and a decoded-PCM fingerprint. Its ID is `asset-` plus SHA-256 of the UTF-8 string `fileHash + newline + pcmHash + newline + sampleRate(17 decimal places) + newline + frameCount + newline + channelCount`. Changing a location or decoder label alone does not change that ID; changing the actual sample domain does.

## Decoded sample domain

The same encoded MP3 can have different sample origin/padding/length under different decoders. The [seek qualification](experiments/2026-11-04-cancellable-seeking.md) found this with JUCE and CoreAudio. Encoded-file SHA alone cannot preserve source-frame meaning.

During the existing full validation scan, form a bounded SHA-256 chain:

1. `digest = SHA256(UTF-8("fader-pcm-v1"))`, as 32 raw bytes.
2. Decode consecutive blocks of exactly 8,192 source frames, except the shorter final block. Duplicate mono to stereo as playback does.
3. Encode each frame left then right as IEEE binary32, least-significant byte first, preserving signed zero. Hash `previousDigest || blockPCM` into the next 32-byte digest.
4. Store the final digest as 64 lowercase hexadecimal characters. Sample rate, original channels and total frames are separate required identity fields.

This fingerprint is deliberately a versioned block chain, not a claim that `pcmHash` is ordinary SHA-256 of one contiguous PCM file. Extra retained scratch is 65,568 bytes plus the 32-byte digest; decoded tracks are not collected in memory for hashing. No new source-file pass is required, but import performs additional hashing work. Prior import timing figures predate this addition.

On reopen, the selected decoder must reproduce the saved encoded hash, PCM fingerprint, rate, original channel count and frame count. Decoder names are provenance: another reader is acceptable if the actual sample domain matches exactly. A mismatch fails explicitly before playback or resave. Automatic cross-decoder conversion is unavailable; decoder/platform qualification and a future explicit migration policy remain [#35](https://github.com/coder13/Fader/issues/35)/[#41](https://github.com/coder13/Fader/issues/41) work. Do not treat this guard as proof that every platform can reopen every saved source.

## Parameters, units and time

| Stored value | Meaning |
| --- | --- |
| `start`, `length` | Integer frames on the 48,000 Hz project timeline, encoded as decimal strings |
| `sourceOffset` | Integer frames at the asset's original sample rate, encoded as a decimal string |
| Clip curve `frame` | Integer project frames relative to that clip's start |
| Global curve `frame` | Absolute project frames |
| Clip `rate` | Static varispeed ratio, 0.25–4.0 |
| `volume` | Linear amplitude, normalized 0–1; UI dB conversion is presentation |
| `highPass`, `lowPass` | Normalized 0–1, logarithmic 20–20,000 Hz map; HP 0 / LP 1 bypass |
| `gainA`, `gainB`, `crossfader` | Original legacy renderer global controls; crossfader is −1…1 |
| `bpm`, `hasGrid`, `firstBeatSeconds` | Saved clip metadata/grid snapshot, independent of subsequent library analysis |

Curve objects carry explicit parameter names; array order is not addressing. Every parameter occurs exactly once in its scope. Each curve saves its initial value, `linear`/`step` interpolation and ordered points with explicit boolean `hold`. Values are finite binary32 controls; JSON numbers are converted to that declared representation. A held point supplies its target until the next point; the renderer applies the saved 240-frame smoothing policy. Global curves are also retained when the clip renderer is selected. The full future tempo map/warp model is not implemented here.

The source coordinate at project frame `f` is `sourceOffset + (f - start) * rate * sourceRate / 48000`, using the existing double-precision calculation. Playback reads `floor(coordinate)` and linearly interpolates to the next frame, clamped at the final source sample. A source's initial clip length is `floor(sourceFrames * 48000 / sourceRate)`. The active interval is `[start, start + length)`; exhaustion of source frames yields silence under the existing renderer. New persistence does not introduce rounding, trimming or tempo changes. Project positions remain bounded to 24 hours, source rates to 8–384 kHz, clips to 64 and rows to 4,096.

## Render policy and migration

Storage schema and audio renderer are separate. New saves write schema **3**, with renderer **1** for legacy global mixing or **2** for clip-owned volume/HP/LP. The manifest snapshots sample rate, smoothing frames, source interpolation version, mix law, filter/cutoff mapping, time basis and seek preroll. Unsupported policies or renderer versions fail; mutable library settings cannot silently redefine saved sound.

Existing schema 1 and 2 files still open through their original interpretation. Legacy v1 global point `hold` was never written/interpreted by the old format, and that reader behavior remains intact. Missing optional v2 hold/grid flags retain their original false default; present flags now require booleans. New IDs and the current decoded-domain snapshot are assigned during import because those fields did not exist in the old files. They become persistent on the first save to schema 3. Pre-existing cross-decoder ambiguity in an old file cannot be recovered after the fact.

Saving upgrades storage while keeping the selected renderer, clip placement and active automation. The UI explains that a new filename preserves the original; the file chooser confirms overwriting an existing file. Older Fader builds cannot read version 3. No automatic source conversion is performed. Saving a new filename is the supported way to retain a legacy file; previous-version backup/journaling is separate recovery work.

## Validation and storage limits

Read at most **8 MiB plus one byte** before parsing, even if a file grows after the initial size check. Require UTF-8, double-quoted JSON text, valid escapes, no NUL, one complete root object, at most 64 container levels and unique object keys (including equivalent Unicode-escaped keys). Then validate all types, stable IDs, counts, ranges, render policy and references before decoding sources. Parameter points remain capped at 200,000 per lane, subject to the aggregate file-size bound. The writer rejects impossible-sized point totals before constructing the JSON tree and checks the final compact UTF-8 byte count before replacement.

Writes use the same manifest admission rules as reads, reject unsupported storage versions and replace via a temporary file on the same filesystem. A rejected write leaves the existing destination intact. This is the existing atomic-replacement guarantee, not a crash-durability/fsync/backup claim. Unknown version 3 fields are rejected rather than silently discarded. Strings are bounded (4,096 UTF-8 bytes for displayed metadata, 8,192 for source hints, 128 for decoder provenance). Legacy recognized fields are strictly typed too.

Persisted sound contains no audio-device handles, controller addresses, temporary PCM-cache paths, playhead, selection, UI zoom, or headphone-cue routing. Cue remains runtime monitoring state and is excluded from master export. Incomplete capture archives contain their original project plus actions and failure metadata; they are not complete sets and cannot be opened as one.

The compact writer formats `initial` and `value` controls for an exact binary32 round trip.
Other numeric metadata retains binary64 precision, even when its value originated as a float.
Negative zero is explicit, and string/key escaping remains delegated to JUCE.
Whitespace removal and shorter control decimals do not change the schema or remove points.
The [dense-save regression](experiments/2026-11-05-dense-save-and-paint.md) checks the unchanged 8 MiB bound and numeric behavior.
Its full load qualification remains pending.

## Qualification

`ProjectModelTests` covers mono 44.1 kHz and VBR MP3 sources, separate clip IDs on shared assets, initial/held/linear/step curves, source offsets/rates, metadata/grid snapshots, move/reorder, parameter-array reorder, relocation, master render parity, strict malformed input, destination preservation, v1/v2 migration and decoded-domain mismatch. Local native/core suites pass 5/5, including existing callback allocation checks. The native media suite also passes with normal macOS codec access.

A separate real-process experiment uses the redistributable eight-second VBR fixture. JUCE reports 354,816 frames, fingerprint `5596226ec7d626ed31be60f1fb3e29579652438ff3d831f3ff49906dd18d1599`; CoreAudio reports 352,800 frames, fingerprint `6892edc56951b7cb105065a045742db5b3b56d4a86ba0e7f61bc815ae5dee491`. Both import/save their own domain. Each rejects the other's saved project and preserves an existing output file. These are observed decoder results, not universal golden fingerprints for every OS.

For API-only migration checks, `fader_probe --resave-project input.faderset output.faderset` uses the same reader/writer; it cannot bypass a saved domain mismatch. No physical audio device or controller is opened by the persistence tests. Exact tested source: `758ba355f5ecbf3644ec53af3e30d05e9150d13a`, JUCE `d60b7a78030661574a615b586d9325ba7a7fa7dd`. [Clean CI](https://github.com/coder13/Fader/actions/runs/41990487133) passes Linux 4/4 (GNU 13.3, CMake 3.31.6, x86_64) and macOS 5/5 (AppleClang 17.0.0.17000013, CMake 4.4.4, arm64 Darwin 24.6.0), including the app signature check. Downloaded metadata/JUnit/logs match those exact revisions, with fixtures OFF, zero failures/errors/skips, the project-identity/migration/domain success marker and callback allocation checks. Local Release tests use AppleClang 21, CMake 4.4.4, arm64/macOS 26.6.2. No Windows native execution is claimed. The preview remains unchanged while the Mac is locked.

An additional API-only schema 1 fixture exercises active gainA, gainB and crossfader curves (linear and step), nonzero source offsets and 0.75/1.25 rates. Resaving to schema 3 keeps renderer 1; pre/post 24-bit WAV outputs are byte-identical, SHA-256 `2eb8377ee9cb5ec34a2dccd3537f114ebbd76a28070fad6f37b8938e515cebda`. This complements the CI v1/v2 migration and float-master parity tests.
