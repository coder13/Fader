# Risks and open questions

Status: Open research and implementation decisions. Updated 2026-11-04.

## Confirmed context

Open source, macOS and Windows, S4 MK3 first, Audacity-inspired clarity, editable prepared sets, automation/audio capture choices, file export, beat grids/tempo changes, and MIDI output are user requirements. Linux and web broaden the platform investigation. The observed Mac is an M3 MacBook Air with 16 GB on macOS 26.6.2. Firmware and hardware measurements are not known.

## Risk register

| Risk | Impact | Next evidence or mitigation | Gate |
| --- | --- | --- | --- |
| S4 HID access or initialization differs by firmware | Cannot control the actual target device | Exact report inventory, known parser fixtures, one LED round-trip | M0/M1 |
| Hardware monitoring differs from software mix | Recorded/exported sound differs from what user heard | Explicit monitor boundary; digital vs physical routing test | A03/A09 |
| Audio callback stalls | Audible glitches | Bounded work, preallocation, queue/caching policy, sustained load metrics | A04–A06 |
| Time-domain or input-timestamp errors | Transition replay drifts or reacts late | Explicit source/arrangement/render clocks; applied-event timestamps; boundary fixtures | A07–A10 |
| Capture misses events or initial state | A saved performance cannot be reconstructed | Record accepted values and discrete actions; completeness flag; preserve original takes | A08/A09/A12 |
| Tempo/stretch changes sound or alignment | Beat matching and export unreliable | Manual grids first, latency-aware adapter, listening and parity corpus | M3 |
| Forking Mixxx costs more than a small core | Maintenance burden and delayed flagship workflow | Bounded headless/capture-seam experiment before commitment | M0 |
| Framework/code reuse conflicts with chosen license | Distribution blocked or rework required | Exact project license and pinned dependency/file inventory | M0 |
| Web hardware/routing APIs unavailable | Browser version cannot satisfy S4 live workflow | Chromium qualification; native bridge or arrangement-only scope | M0/M6 |
| Long decode/export exhausts memory | Crash or failed long sets | Bounded caches and streaming encoder; long-set memory report | A15/M4 |
| Project media/DSP dependencies change | Old sets sound different or fail to open | Content hashes, grid snapshots, migrations, pinned render semantics | M2/M4 |
| Timeline editor becomes a general DAW project | POC loses focus | Gate features against the flagship workflow; two lanes and few parameters first | M2 |
| Windows driver behavior varies | Second required platform delayed | Version-specific current ASIO/WASAPI qualification | M5 |

## Decisions still needed

1. **Exact license:** open source is settled; AGPL/GPL/permissive policy is not. Resolve before importing implementation code. See [stack ADR](decisions/0001-platform-and-engine.md).
2. **New core or Mixxx fork:** select using the reuse experiment, not assumption.
3. **Native UI or web frontend:** choose after the minimal timeline prototype; keep audio independent either way.
4. **Browser product scope:** full controller application, preparation companion, or eventual standalone arrangement editor, based on capability tests.
5. **DSP candidate and sound quality:** compare Rubber Band and Signalsmith at relevant tempos and latency; no algorithm selected yet.
6. **Musical edit semantics:** validate beat-anchored versus absolute placements and automation movement rules with the user on a prototype.
7. **Distribution baseline:** set supported macOS/Windows versions and Apple Silicon/Intel policy after build and hardware evidence. Cross-platform does not imply every old OS.
8. **GitHub destination:** the relocated directory had no Git metadata or remote. Choose the repository/visibility before publishing the prepared issue backlog.

## Hardware details to collect during the first experiment

S4 firmware; direct USB versus hub; external power; current audio channel/rate options; headphone/master connections; NI background software contention; measured buffer stability and latency. These are experiment inputs, not reasons to delay the completed research.

## Roadmap boundaries

Future scratch/loop automation must retain source trajectories. External synths, microphones, and hardware FX need audio capture for reproducible export. Plugin hosting requires serialized plugin state and latency/migration policies. Streaming/DRM integration requires an independent access/export design and is not implied by local-file support. Track these as later epics without making them first-POC dependencies.
