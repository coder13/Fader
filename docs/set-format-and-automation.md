# Set format and automation

Status: Proposed design contract, not an implemented schema. Updated 2026-11-04.

## What a set must preserve

A set is source media plus a versioned description of how to play and mix it. It contains assets, clip placement, source timing, tempo/grid information, parameter automation, discrete playback actions, initial state, and rendering settings. A raw audio recording is an optional reference or source asset. A raw controller log alone cannot reconstruct a portable performance.

Two representations serve different purposes: capture retains applied semantic events and take boundaries; the editable project stores clips and curves compiled from them. Playback and export consume the compiled project. Retaining the original take lets the user undo curve simplification and compare edits.

## Objects and identity

| Object | Required meaning |
| --- | --- |
| Project | Stable ID, schema version, engine/DSP version, sample-rate policy, initial state, tempo map, arrangement |
| Asset | Stable ID, content hash, relative location or relinking hint, original sample rate/channels/frame count, decoder identity |
| Beat grid | Asset reference, source-frame anchors with beat positions, provenance/version, user corrections |
| Clip | Stable ID, asset ID, lane/bus routing, arrangement start/end, source in/out or warp mapping, sync/key-lock mode |
| Automation lane | Parameter ID, owner/scope, explicit time basis, baseline value, ordered points and interpolation |
| Action | Stable target, action type and payload, effective time, ordering sequence; e.g. activate, stop, seek, loop |
| Take | Armed targets, capture interval, original events, completeness flag, selected/previous revision |
| Render settings | DSP configuration, mix law, sample rate, export range/tails, normalization policy, codec settings |

Stable IDs must survive rename, reorder, and file relocation. UI widgets, USB port numbers, and device-specific MIDI addresses are not parameter IDs. Controller mappings reference semantic controls such as `deck.a.gain` or `master.crossfader`; capture resolves them to the active clip/bus identity so later deck reassignment does not change an old take.

## Three time domains

1. **Source time:** integer frame positions at the original asset sample rate. These identify an exact source location.
2. **Arrangement time:** either absolute rational seconds or musical rational quarter-note positions, explicitly tagged. A tempo map converts between musical and absolute time.
3. **Render time:** integer frames at the current engine/output sample rate. Scheduling uses this domain, including an offset within the current block.

Use exact integer/rational representations on disk, with a documented rounding rule at the render boundary. In JSON/JavaScript bridges, encode potentially large integers as decimal strings so values beyond JavaScript's exact integer range are not corrupted. Do not serialize a floating-point wall-clock timestamp as the only timing authority.

The POC uses absolute arrangement positions and a constant tempo, retaining exact capture frame/rate pairs. The format design reserves musical anchors; implementing all tempo-map types is not required for the first POC. A tempo edit must specify which objects follow beats and which remain at absolute time.

For a constant-tempo segment, elapsed seconds = quarter-note displacement × 60 / BPM. Later linear-in-time tempo ramps require integration and inversion; specify whether a ramp is linear in time or beats. They are different curves. Grid anchors likewise map source frames to source beats; project tempo then determines the playback-rate trajectory.

## Illustrative representation

This fragment explains semantics; it is not a valid complete project or a finalized public format.

```json
{
  "schemaVersion": 1,
  "render": { "sampleRate": 48000, "engineVersion": "poc-1" },
  "automation": [{
    "target": { "kind": "bus", "id": "bus-a", "parameter": "gainDb" },
    "timeBasis": "absolute-seconds",
    "initialValue": 0,
    "interpolation": "linear-db",
    "points": [
      { "time": { "numerator": "24", "denominator": "1" }, "value": 0 },
      { "time": { "numerator": "32", "denominator": "1" }, "value": -24 }
    ]
  }]
}
```

The parameter registry must define units, range, mute representation, interpolation, smoothing, and version compatibility. Zero linear amplitude and negative-infinity dB need an explicit representable state; do not store invalid JSON infinity/NaN. Do not interpolate enumeration/boolean values as continuous curves.

## Capturing what actually happened

Capture initial deck/clip state and all resolved software actions that affect the master. Record the effective frame after input mapping, pickup, clamping, quantization, and scheduling. Preserve the target values before a versioned smoothing algorithm; replay applies that same algorithm. Capturing arbitrary UI polling snapshots can miss fast gestures and change their timing.

At equal timestamps, use a stable sequence number and documented ordering. Resolve a load/start or seek/gain interaction consistently. Record resolved launch positions and playback-rate changes rather than relying on a future beat analysis or sync heuristic to make the same decision again.

POC supported actions are clip activation/stop/source placement plus gain and crossfader targets. Loading a different track during a performance requires a new clip/source identity in the recording. Unsupported actions must be shown as unsupported for capture, not silently presented as fully replayable.

Cue/headphone controls affect monitoring and stay outside the rendered master by default. Physical S4 Main/Booth/Headphone controls may affect hardware output outside the software mix. External microphone/line input requires an audio capture asset to be reproducible. See [controller findings](research/traktor-and-controller-integration.md).

## Playback versus manual control

| State | Behavior | Persistence |
| --- | --- | --- |
| Read | Automation supplies the parameter value | No edit |
| Override | User controls the parameter temporarily | No edit until explicitly committed; clear visual state |
| Record | Armed targets capture a new take in the selected interval | Creates an undoable edit |
| Resume automation | Return to the current curve with defined smoothing | Ends override |

The first POC uses explicit range recording with selected parameters. At punch-in, preserve the value that should apply immediately before the range. At punch-out, restore the following automation with a defined boundary/return policy and continuity test. Retakes replace only armed targets inside the interval; preserve prior takes until committed, and make the operation one undo step. Refuse a successful take if capture overflow occurred.

Later touch mode follows contact/release where the device supplies it. Non-touch faders cannot reliably tell a hand release from a pause; a timeout is a policy, not actual touch sensing. Latch continues the last value until the chosen stop/punch boundary. Non-motorized faders need pickup/soft takeover so reconnecting or touching a mismatched control does not jump the mix.

## Editing and ownership

Clip-owned automation moves with a clip and trims non-destructively with it. Bus/master automation remains in the set timeline. The POC records mixer gain on two fixed buses, with that ownership visible. Later operations can explicitly include bus automation when moving a transition; avoid silently attaching every parameter to whichever clip happened to play at that instant.

Keep captured dense points until the take is safe. A worker can derive a simpler editable curve under a declared error bound in parameter units and audible output; preserve endpoints, steps, and sharp gestures. Never simplify on the audio callback. Undo restores the original capture.

## Project storage and recovery

Start with a readable versioned manifest and sidecar assets/cache directory. Save via a temporary file plus atomic replacement on the same filesystem, keep a previous known-good version, and journal take progress off-thread. Incomplete crash-recovered takes must be labelled incomplete. Validate size limits, references, ranges, finite numbers, and monotonic curve ordering before compiling.

Use content hashes to distinguish relocated media from a same-named but different track. Display missing media and offer relinking; do not substitute a different edit/remaster without a mismatch warning. Per-project grid snapshots protect old sets from changing global analysis results. Cache waveforms/analysis separately so cache deletion does not destroy edits.

Later provide a single-file collected-media bundle containing a manifest and selected assets, using safe relative paths and bounded extraction. A reference-only project is smaller but depends on source files. Saving a set and exporting audio are distinct operations in the UI.

## Reproducibility contract

Same project, same assets, pinned engine/DSP/decoder versions, and same render mode should reproduce the same scheduling and audio within documented numerical tolerances. Bit-identical output across CPU architectures, compiler versions, or different stretch algorithms is not promised.

The baseline gain/crossfade fixture compares live-core PCM capture to offline PCM before encoding, with equal preroll, latency alignment, and tail policy. Verify event frames exactly and float samples within `1e-6` maximum absolute difference for this simple deterministic fixture. More complex DSP needs its own measured tolerance. MP3 is lossy and has encoder delay; compare decoded, aligned audio using a separate criterion.

Unknown future schema versions must be rejected or opened read-only, never silently rewritten. Migrations make a backup and preserve fixtures. Freeze/bounce to audio is a future compatibility escape hatch for obsolete DSP, unavailable plugins, or external instruments.
