# POC mixer contract

Tracking: [DJ-007](https://github.com/coder13/Fader/issues/7). This describes the existing engine version 2 behavior and its software fixtures. It is not a claim of production resampling quality, measured device latency or S4 routing support.

## Source time and resampling

The project clock is 48,000 frames/second. A clip's start and duration use project frames; its source offset uses original source frames. For project frame `t`, the source position is `sourceOffset + (t - start) × rate × sourceSampleRate / 48000`. The clip is active on `[start, start + length)`; exhausted source data produces silence. There is no implicit loop or effect tail.

Import keeps original PCM sample rate and frame count. Mono is duplicated at equal amplitude into left/right; stereo channels remain separate. Import duration is `floor(sourceFrames × 48000 / sourceSampleRate)`. A saved static rate changes source traversal; the caller owns the clip duration and must explicitly choose whether to trim or extend it when changing rate. Rate is pitch-coupled varispeed, not key lock. The current editor does not expose a rate-edit control.

The renderer linearly interpolates adjacent PCM frames and holds the last frame when the next index would exceed the source. This intentionally preserves the existing version 1/2 sound. It has no band-limited anti-alias stage and is not a final-quality resampler for large rate changes. Pitch-preserving stretch and its quality decision belong to [#19](https://github.com/coder13/Fader/issues/19); do not silently change a saved project's renderer when adopting it.

## Levels, filters and buses

- Each version 2 clip has independent volume, high-pass and low-pass automation. Volume is linear amplitude: 0 is mute, 1 is unity/0 dB, 0.5 is approximately −6.02 dB. Automation targets use the versioned 240-project-frame smoothing policy. A continuing ramp follows that policy; changing the target each sample is not a fixed 5 ms delay.
- Signal order is interpolated source → two cascaded high-pass stages → two cascaded low-pass stages → volume → master sum. Cutoff maps logarithmically from 20 to 20,000 Hz; endpoint blends bypass the relevant filter. Coefficients update every 16 clip-local frames. There is no resonance, EQ, pan law or limiter in this POC.
- Output slots 0/1 carry stereo master. Slots 2/3 carry the selected clips' **dry, pre-filter, pre-volume** signals, each at a fixed amplitude of 0.5. Selecting both cues sums them. The POC has no cue/master blend knob. Extra output slots are cleared. The engine's slots are a software bus contract; physical-device mapping is a separate hardware gate.
- Stereo-only processing produces only master; cue never folds into master. Export always requests stereo master, independent of cue selection. Cue state is monitoring state and is not serialized as a musical effect.
- The engine sums floating-point signals without clipping, limiting or automatic normalization. Its peak meter measures the largest absolute master sample in the latest block, not true peak or integrated loudness. Cue does not affect this meter. Users must leave headroom for overlaps.
- 24-bit PCM WAV export rejects a master peak above 1.0 and preserves any existing destination. Float WAV export preserves values outside ±1.0. Neither export path applies normalization, dither or a different mixing law; those delivery choices belong to [#24](https://github.com/coder13/Fader/issues/24).
- Legacy version 1 keeps its two global gains and equal-power crossfader (`cos`/`sin` from 0 to π/2), with the same dry cue contract. Version 2 ignores those legacy global curves. No automatic conversion is performed.

## Callback audit and limits

`Engine::process` and `processClips` only read the immutable project/decoded PCM and update preallocated DSP state, fixed queues and lock-free atomics. Construction, validation, source decoding, hashing, project copying, seek reconstruction, destruction and WAV writing run outside the live callback. The render loops contain no file/device/UI calls, allocation, blocking locks, sleeps or retry-until-success loops.

Work scales with the supplied block length and validated project bounds: at most 64 clips, three curves per clip with at most 200,000 points each, and binary-search evaluation of each curve. At most 4,096 queued commands are consumed per process call, even when the producer keeps writing. The input/capture capacities are 4,096/16,384; capture overflow is an explicit failure. These finite limits establish bounded work, not a deadline guarantee for every permitted project. The existing 20,000-call realtime fixture checks ordinary `new`/`delete` during processing; it is not a comprehensive allocation interposer or an OS scheduling test.

File sources now use the [bounded streaming contract](streaming-contract.md): per-clip PCM pages, capped waveform peaks, explicit live starvation and offline readiness. Import still scans the full source without retaining full PCM. [#38](https://github.com/coder13/Fader/issues/38) records its separate qualification; sustained-load qualification remains [#40](https://github.com/coder13/Fader/issues/40).

## Repeatable evidence

`tests/MixerTests.cpp` is part of the headless core suite. It checks separate stereo gains, dry cue with muted/filtered master, cue summation, unused-output clearing, master-only peak metering, unclipped floating-point summation, affine-source interpolation at 8/44.1/48/96 kHz and rates 0.5/1/2, source offsets, clip endpoints and legacy crossfader compatibility. Existing core tests cover filter rejection, capture/replay, seeking and block-size independence.

`tests/SourceRateTests.cpp` generates mono/stereo float WAV files at 44.1/48/96 kHz with a fractional project-frame duration. It checks decoded levels, mono duplication, source rate/count, duration rounding, actual stereo 48 kHz export, per-channel gain and byte-identical exports with cue on/off. It also checks atomic PCM overload rejection and float overload preservation. Only temporary synthetic files are used.

Run `cmake --build --preset native` and `ctest --preset native` for all fixtures, or use the `core` presets for the JUCE-independent checks. No music library or physical device is required. Exact tested revisions and CI results belong in the issue's completion evidence.
