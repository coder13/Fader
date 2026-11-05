# Set format and automation

Status: Long-term design contract with a bounded implemented subset. Updated 2026-11-04.

The native prototype now writes [storage schema 3](project-format-v3.md): stable project/clip/asset IDs, named parameter curves, render/grid snapshots and decoded-source identity. It reads legacy schemas 1 and 2 while preserving their original renderer. Saving an old set upgrades its storage; use a new filename to retain the old file. The detailed future model below still includes unimplemented features. [ADR 0002](decisions/0002-clip-owned-automation.md) records clip-owned automation.

## What a set must preserve

A set is source media plus a versioned description of how to play and mix it. It contains assets, clip placement, source timing, tempo/grid information, parameter automation, discrete playback actions, initial state, and rendering settings. A raw audio recording is an optional reference or source asset. A raw controller log alone cannot reconstruct a portable performance.

Two representations serve different purposes: capture retains applied semantic events and take boundaries; the editable project stores clips and curves compiled from them. Playback and export consume the compiled project. For unedited captured controls, compile each accepted target as a step at its applied frame and replay the identical smoothing policy. Connecting those targets with linear ramps would change the original performance. Retaining the original take lets the user undo curve simplification and compare edits.

## Objects and identity

| Object | Required meaning |
| --- | --- |
| Project | Stable ID, schema version, engine/DSP version, sample-rate policy, initial state, tempo map, arrangement |
| Asset | Stable ID, content hash, relative location or relinking hint, original sample rate/channels/frame count, decoder identity |
| Beat grid | Asset reference, source-frame anchors with beat positions, provenance/version, user corrections |
| Clip | Stable ID, asset ID, lane/bus routing, arrangement start/end, source in/out or warp mapping, static playback-rate ratio, sync/key-lock mode |
| Automation lane | Parameter ID, owner/scope, explicit time basis, baseline value, ordered points and interpolation |
| Action | Stable target, action type and payload, effective time, ordering sequence; e.g. activate, stop, seek, loop |
| Take | Armed targets, capture interval, transport segments with engine/project anchors and sample rate, original events, completeness flag, selected/previous revision |
| Render settings | DSP configuration, mix law, sample rate, export range/tails, normalization policy, codec settings |

Stable IDs must survive rename, reorder, and file relocation. UI widgets, USB port numbers, and device-specific MIDI addresses are not parameter IDs. Controller mappings reference semantic controls such as `deck.a.gain` or `master.crossfader`; capture resolves them to the active clip/bus identity so later deck reassignment does not change an old take.

For the confirmed version 2/3 plugin requirement, reserve an extensible effect-instance model: stable instance/rack IDs, format/class ID, vendor/version, order/routing, bypass, channel layout, opaque state, external dependencies, and automation keyed by instance ID plus plugin parameter ID. Preserve unavailable plugin state without silently replacing its sound. Clip-owned effects move with the clip; bus effects stay attached to the bus. These fields are a future schema extension, not mandatory POC data. See [plugin effects](plugin-effects.md).

## Three time domains

1. **Source time:** integer frame positions at the original asset sample rate. These identify an exact source location.
2. **Arrangement time:** either absolute rational seconds or musical rational quarter-note positions, explicitly tagged. A tempo map converts between musical and absolute time.
3. **Render time:** integer frames at the current engine/output sample rate. Scheduling uses this domain, including an offset within the current block.

Use exact integer/rational representations on disk, with a documented rounding rule at the render boundary. In JSON/JavaScript bridges, encode potentially large integers as decimal strings so values beyond JavaScript's exact integer range are not corrupted. Do not serialize a floating-point wall-clock timestamp as the only timing authority. Captured engine frames carry a transport segment ID, sample rate, and anchor from engine frame to project position; split or stop takes at seek/pause/clock resets. This allows a retake at minute five to stay at minute five after reopening.

The POC uses absolute arrangement positions and a constant tempo, retaining exact capture frame/rate pairs. The format design reserves musical anchors; implementing all tempo-map types is not required for the first POC. A tempo edit must specify which objects follow beats and which remain at absolute time.

For a constant-tempo segment, elapsed seconds = quarter-note displacement × 60 / BPM. Later linear-in-time tempo ramps require integration and inversion; specify whether a ramp is linear in time or beats. They are different curves. Grid anchors likewise map source frames to source beats; project tempo then determines the playback-rate trajectory.

## Illustrative representation

This fragment illustrates a manually drawn linear envelope, not the step-target representation of an unedited capture. It is not a valid complete project or a finalized public format.

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

The current software prototype captures clip volume/high-pass/low-pass targets and persists static placement. Complete transport/source-action capture remains open in [#12](https://github.com/coder13/Fader/issues/12). The completed hardware POC must resolve controls to clip identity. The planned static varispeed ratio is set before capture and persisted; changing rate during capture is deferred until rate events are supported. Loading a different track during a performance requires a new clip/source identity in the recording. Unsupported actions must be shown as unsupported for capture, not silently presented as fully replayable.

Cue/headphone controls affect monitoring and stay outside the rendered master by default. Physical S4 Main/Booth/Headphone controls may affect hardware output outside the software mix. External microphone/line input requires an audio capture asset to be reproducible. See [controller findings](research/traktor-and-controller-integration.md).

## Playback versus manual control

| State | Behavior | Persistence |
| --- | --- | --- |
| Read | Automation supplies the parameter value | No edit |
| Override | User controls the parameter temporarily | No edit until explicitly committed; clear visual state |
| Record | Armed targets capture a new take in the selected interval | Creates an undoable edit |
| Resume automation | Return to the current curve with defined smoothing | Ends override |

The POC target uses explicit range recording with selected parameters. At punch-in, preserve the value that should apply immediately before the range. At punch-out, return to existing automation through a visible short return region included in the effective replacement interval. Apply the versioned volume/HP/LP smoothing policy, reconstruct baseline smoothing and filter state, and test continuity at the return boundary. Display the region before committing and test its boundary PCM. Retakes replace only armed targets inside this effective interval; preserve prior takes until committed, and make the operation one undo step. Refuse a successful take if capture overflow occurred. Full boundary qualification remains open in [#15](https://github.com/coder13/Fader/issues/15).

The outside-range guarantee concerns unchanged stored clips/automation. Do not generalize it to bit-identical audio for future stateful effects or stretch: their history can affect later sound. Those features need explicit tail/preroll behavior and an audition range before strict audio isolation can be promised.

Later touch mode follows contact/release where the device supplies it. Non-touch faders cannot reliably tell a hand release from a pause; a timeout is a policy, not actual touch sensing. Latch continues the last value until the chosen stop/punch boundary. Non-motorized faders need pickup/soft takeover so reconnecting or touching a mismatched control does not jump the mix.

## Editing and ownership

The [Audacity 4 UI direction](research/audacity-4-design-lessons.md) does not change capture semantics. Automation visibility is presentation state, separate from Read/Override/Record and armed targets. Workspace changes must not alter transport, recording, routing, undo history or project data.

Clip-owned automation moves with a clip. The v0.2 POC records volume/HP/LP on the clip, superseding the earlier two-fixed-bus POC design. Future trimming must preserve hidden curve data for reversible extension; it must not be triggered by ordinary movement. Proposed bus/master automation remains in the set timeline. Later operations can explicitly include bus automation when moving a transition; avoid silently attaching every parameter to whichever clip happened to play at that instant.

Deletion leaves a gap by default. Future ripple commands must declare their lane/set scope and apply one undoable transaction across affected clips, curves, tempo/markers and take boundaries; unsupported combinations remain unavailable with an explanation. Clip grouping is explicit and does not infer membership from overlap or automatically capture bus automation. These are later editing contracts, not new POC scope. Practice-loop bounds and export bounds remain distinct; using the loop for export requires an explicit choice.

Keep captured dense points until the take is safe. Editing a captured point preserves step-target semantics unless the user explicitly changes interpolation; the editor must show which mode is active. A worker can derive a simpler editable curve under a declared error bound in parameter units and audible output; preserve endpoints, steps, and sharp gestures. Never simplify on the audio callback. Undo restores the original capture.

## Project storage and recovery

Start with a readable versioned manifest and sidecar assets/cache directory. Save via a temporary file plus atomic replacement on the same filesystem, keep a previous known-good version, and journal take progress off-thread. Incomplete crash-recovered takes must be labelled incomplete. Validate size limits, references, ranges, finite numbers, and monotonic curve ordering before compiling.

Use content hashes to distinguish relocated media from a same-named but different track. Display missing media and offer relinking; do not substitute a different edit/remaster without a mismatch warning. Per-project grid snapshots protect old sets from changing global analysis results. Cache waveforms/analysis separately so cache deletion does not destroy edits.

Later provide a single-file collected-media bundle containing a manifest and selected assets, using safe relative paths and bounded extraction. A reference-only project is smaller but depends on source files. Saving a set and exporting audio are distinct operations in the UI.

## Reproducibility contract

Same project, same assets, pinned engine/DSP/decoder versions, and same render mode should reproduce the same scheduling and audio within documented numerical tolerances. Bit-identical output across CPU architectures, compiler versions, or different stretch algorithms is not promised.

The baseline gain/crossfade fixture compares live-core PCM capture to offline PCM before encoding, with equal preroll, latency alignment, and tail policy. Verify event frames exactly and float samples within `1e-6` maximum absolute difference for this simple deterministic fixture. More complex DSP needs its own measured tolerance. MP3 is lossy and has encoder delay; compare decoded, aligned audio using a separate criterion.

Unknown future schema versions must be rejected or opened read-only, never silently rewritten. Migrations make a backup and preserve fixtures. Freeze/bounce to audio is a future compatibility escape hatch for obsolete DSP, unavailable plugins, or external instruments.
