# First native prepared-set prototype

Date: 2026-11-04. Status: **software experiment passed the checks listed below; full POC qualification remains incomplete**. Hardware testing was deferred at the user's request. Implementation is on `codex/poc-foundation`, based on research commit `302c2d25b139423c068e61176905afd53fcaf721`; use the commit containing this report as the source identifier.

## Configuration and implementation

Apple M3 MacBook Air, 16 GB, macOS 26.6.2 (25G83), Apple Clang 21, C++20, Release build with CMake 4.4.4. JUCE 9.0.5 pinned at `d60b7a78030661574a615b586d9325ba7a7fa7dd`. Core tests also built with AddressSanitizer and UndefinedBehaviorSanitizer in Debug. The implementation uses AGPL-3.0-or-later for this experiment; the production stack/reuse and license preferences remain provisional.

The headless core has two source clips, sample-frame scheduling, fixed clip varispeed, equal-power crossfade, gain/crossfader curves, bounded SPSC command/capture queues, and 240-frame target smoothing at 48 kHz. UI/device adapters and decoding live outside the core. Core Audio playback and offline export call the same renderer. The app detaches its callback before replacing/reclaiming the render plan. Ordinary work, media I/O, JSON, hashes, and rendering output files occur outside the callback. Control processing is capped at 4,096 commands per callback even if a producer keeps filling the queue.

The UI offers two waveform lanes, folder browsing/Finder and internal file drag/drop, start/length/static speed edits, three automation lanes with step or linear interpolation, mouse-fader capture, range retakes, undo, project save/open, output-device selection, and stereo 24-bit WAV export. It does not implement the full POC scope or declare S4 support.

## Results

| Check | Observed result | Scope and limitation |
| --- | --- | --- |
| Release CTest | 3/3 suites passed: `core`, `realtime_core`, `media` | Synthetic deterministic fixtures; not long-run device qualification |
| Memory/undefined behavior instrumentation | Core tests passed ASan/UBSan | Not a thread-safety proof or OS/driver audit |
| Block independence | Exact float equality at 1, 64, 127, 128, 256, 512 and 1024 frames | The core fixture is small and excludes future plug-ins/time stretching |
| Seeking | Nonzero seek reconstructs smoother state and matches the uninterrupted suffix exactly | History reconstruction may delay UI preparation on long sets |
| Capture/replay | Applied-frame steps retain original timing and match replay exactly | Gain/crossfader only; no HID input or transport-action capture |
| Retakes | Nonzero-position take, punch-out with nearby following targets, and replay match; failed take commit is atomic | Stored points outside replacement remain intact; baseline PCM outside the interval is not guaranteed identical during smoothing |
| Queue pressure | Input queue remains bounded; capture overflow/late events are reported | Physical controller rate and pickup not tested |
| Core heap instrumentation | No ordinary `new`/`delete` during 20,000 render calls with two sources, automation and cue | Does not intercept every possible allocator or audit JUCE/OS callbacks |
| File behavior | Content-based decoding, JSON round-trip, wrong-content hash rejection, unknown schema rejection, cancelled export preservation, and clipped PCM export rejection passed | Relinking UI, crash recovery, long-media streaming remain open |
| Cue isolation | Enabling cue does not change master-render PCM | No physical four-channel routing test |
| User-library sample | 14/14 full decodes succeeded in native macOS context; all source hashes unchanged | Six MP3s plus one WAV per observed PCM/bit-depth/channel/rate group; not a random sample or full-library qualification |
| UI smoke | Opened synthetic set, recorded A gain/crossfader gestures, committed take, saved, exported successfully | A separate review app instance kept the existing user session intact |
| End-to-end file parity | Saved UI take contained 2 A-gain points and 43 crossfader points; reopened command-line export was byte-identical to GUI export | Synthetic 18-second, stereo, 48 kHz, 24-bit WAV; 864,000 frames |
| Device inventory | Core Audio output: built-in MacBook Air speakers; no matching S4 interfaces | Read-only HID inventory, not device binding/control |
| Native silent audio smoke | 752 callbacks over approximately two seconds; actual 48 kHz/128 frames; renderer p99 9.458 microseconds, max 34.375 microseconds | No acoustic output test or stress load; driver xrun count unavailable (`-1`) |

The separate offline microbenchmark reported p50 3.542, p95 11.375, p99.9 173, max 1080.71 microseconds for 20,000 128-frame calls during development activity. These diagnostic numbers are not A05 acceptance: the production UI workload, dense projects, cold caches, and long-duration behavior are not represented. They are not physical latency measurements.

The native device reported 1,552 output-latency frames (about 32.33 ms at 48 kHz). The 128-frame callback period is about 2.67 ms. Neither alone measures end-to-end control-to-sound latency, and no S4 latency can be inferred from built-in speakers.

Library WAV coverage: mono PCM16 at 44.1/88.2 kHz; stereo PCM16 at 44.1/48/96 kHz; stereo PCM24 at 44.1/48 kHz; stereo float32 at 44.1 kHz. Six MP3 selections were spaced across sorted candidate paths; track names and paths are not retained in this report. All selected files were read-only. An initial restricted-shell run failed two MP3s, as did an explicit built-in JUCE MP3 decoder trial. Restoring the normal format order and running with native macOS codec access decoded all 14. This exposes a backend/environment compatibility limitation; Windows decoding and the full library still need qualification. No error was converted to silent/truncated success.

## Reproduction

See [build and workflow instructions](../poc-quickstart.md). `tools/make_demo.py` generates redistributable synthetic WAV sources and a relative-path `.faderset`; no music or file lists are committed. The source tests provide deterministic event/capture/retake fixtures. Run:

```sh
ctest --test-dir build --output-on-failure
build/fader_realtime_tests
build/fader_probe_artefacts/Release/fader_probe --devices
build/fader_probe_artefacts/Release/fader_probe --audio-smoke
```

Run native codec/device probes in the normal desktop execution context. A restricted tool sandbox may not expose the same codecs or devices. Synthetic file/core tests also run without that access.

## Remaining gates and next increment

No whole A01–A16 matrix is marked complete. This experiment gives partial evidence for A06/A08/A09/A10/A11/A12/A13/A16. A01/A02/A03/A07/A14 require controller, routing, latency or MIDI work; A04/A05/A15 require sustained-load/deadline and bounded long-render qualification. The stricter finite punch-out return region, manual beat grid, raw-audio recording, transport capture and source-relink workflow remain unimplemented.

The next software work should prioritize a streamable media cache and stable decoder identity, sample-accurate finite return-region semantics, reliable seek preparation, project recovery/relinking, and timeline zoom/grid controls. Qualify S4 HID and master/cue when hardware is available. Keep the [adoption/reuse gate](../research/competitive-workflow-audit.md#proposed-adoption-and-reuse-gate) open: this first custom experiment supplies a comparison baseline, not evidence that reuse has been rejected.
