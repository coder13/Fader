# ADR 0002: Clip-owned automation and arrangement prototype

Status: Accepted for the user-requested v0.2 POC iteration, 2026-11-04. This does not settle the production engine/reuse choice in ADR 0001.

## Context

The user asked for independent volume and low/high cutoff automation on each song, visible over its waveform, plus DAW-style movement between rows and time positions without resizing. A single A/B crossfader and timeline scaling derived from total set duration do not meet that workflow.

The [UI evidence atlas](../research/ui-evidence-atlas.md) and [competitive audit](../research/competitive-workflow-audit.md) support an arrangement-first hierarchy, clear ownership, selected-parameter overlays and a searchable metadata table. They remain source-based research, not proof of our app's usability.

## Decision

- New sets use schema/engine version 2. Each clip owns three normalized envelopes: volume, high-pass cutoff (low cut), low-pass cutoff (high cut). Automation positions are local 48 kHz project frames, independent of the clip's arrangement start.
- Dragging a header changes only start and row. Duration, source position and envelopes remain unchanged. Pixels per second is controlled explicitly by zoom. Canvas space expands within declared POC bounds.
- Drawn envelopes interpolate linearly. Captured controls retain held targets at the exact applied frame; the live and offline engine apply the same 240-frame finite smoothing. Retakes alter only the armed clip's touched parameters and preserve later stored data.
- HP/LP cutoff maps logarithmically from 20 to 20,000 Hz. Endpoints bypass their respective filter with a small continuous blend. Two one-pole stages provide a simple 12 dB/octave proof; no resonance/analog-model claim.
- Version 1 sets keep the original renderer and global crossfade semantics. The editor opens them for playback/export only. Do not silently approximate the old mix into new volume envelopes.
- The browser reads Traktor metadata, labels its provenance and keeps source files read-only. Preferences select a music root and collection. Unknown BPM/key are not fabricated.

## Consequences and next decision points

The design no longer assumes one arrangement row equals a physical deck. S4 mappings must resolve to a clip identity before recording. Index-based clip identity is sufficient for immutable POC playback snapshots; durable IDs are still needed for collaborative editing, takes, plugin references and future schema evolution.

The prototype is bounded to 64 clips, 4,096 row positions, two hours, 256 MiB per unique source and 1 GiB decoded source PCM per set. Long-set streaming, cancellable seeks, project recovery and complete keyboard editing remain follow-up work. Filter history reconstruction is exact but can be slow. Baseline audio immediately outside a retake can change during smoothing, as documented in the earlier capture experiment.

Validation and known gaps: [arrangement experiment](../experiments/2026-11-04-clip-arrangement.md). User workflow: [quickstart](../poc-quickstart.md).
