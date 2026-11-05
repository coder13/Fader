# Offline render qualification

Issue: [DJ-013](https://github.com/coder13/Fader/issues/13). Status: local software checks and the real 60-minute export pass; clean Linux/macOS CI passes. The new native Cancel export/progress controls await visual inspection because the Mac is locked. Existing preview/user work is preserved. No physical audio device or controller is opened by these tests.

## Configuration and scope

Final implementation and measured source `6ba1da5d82892385d957ff656ffee455f10f95cc`, JUCE 9.0.5 at `d60b7a78030661574a615b586d9325ba7a7fa7dd`; local AppleClang 21.0.0.21000334, CMake 4.4.4, Release/C++20, arm64 Darwin 25.6.0/macOS 26.6.2. The final full export was repeated after the missing-temporary validation guard; all staged implementation/test files match the final source, and its WAV checksum matches the first measurement. Ordinary local tests pass 6/6 in both the sandbox and normal macOS codec environment.

The [offline contract](../offline-render-contract.md) declares master PCM tolerance **1e-6**, 64/128/256/512-frame comparison blocks, and a **128 MiB** peak-RSS budget for the two-source 60-minute probe before measurement. This is accelerated file rendering, not one hour of wall-clock playback or an acoustic/deadline qualification.

## Live/offline equivalence

A new headless core suite drives the real `Playback` scheduler with synthetic host timestamps and captures applied semantic events. It compiles those events into a separate project with `commitTake`, then compares an independent `OfflineRender` host. Resident and streamed stereo sources cover 44.1/96 kHz, 1.3/0.75 playback rates, nonzero source offsets, a delayed second clip, volume/HP/LP, curve/hold boundaries and events around block boundaries. Applied frame/parameter/clip/value must match the submitted baseline. Same-frame targets retain their application order.

Across all four block sizes, full master PCM and exact-history range slices have **maximum absolute error 0**, inside 1e-6. The range fixture renders `[19001,30009)` with exactly 11,008 output frames. The live host also emits nonzero cue, while the offline host produces only stereo master. The original project remains intact. These tests qualify the current renderer, not future stretchers or third-party effect modes.

A gated decoder proves preparation waits for missing data rather than rendering silence. Cooperative cancellation releases a blocked preparation; failed decoding rejects the render. The host rejects invalid ranges/buffer pointers and blocks larger than 1,024 frames. Existing callback allocation checks continue to pass. A targeted Apple ThreadSanitizer build of Engine, Streaming, Playback, OfflineRender and OfflineTests passes with zero PCM error and no race report. This is not a whole-app sanitizer result.

## Real 60-minute export

Inputs are the two real sparse 2 h 15 m PCM16/48 kHz stereo WAVs generated for [DJ-038](2026-11-04-bounded-streaming.md). Each source has a 100 ms synthetic tone/pulse burst every minute and silence between bursts. This is not worst-case continuous music density. Clip A spans the entire hour; B overlaps from minute 15 to the end. Both have saved volume/HP/LP curves and runtime cue enabled. Every decoder read in the export copy is delayed by **1 ms**. Import and verification use ordinary source readers.

The probe writes a real 32-bit float stereo 48 kHz WAV, finalizes/closes/reopens it, then compares **every** output sample against a separately prepared `Playback` instance. Only bounded input/output blocks are retained. The independent comparison also verifies nonzero master/cue checksums and an unchanged original project manifest.

| Measurement | Observed |
| --- | ---: |
| Project/output frames | 172,800,000 |
| Duration | 3,600 s |
| WAV bytes | 1,382,400,104 |
| Peak RSS, import/export/verification included | 12,288,000 bytes (~11.72 MiB) |
| PCM page cache | 1,048,576 bytes |
| Delayed decoder reads | 36,929 |
| Maximum absolute master PCM error | 0 |
| Failed source readers | 0 |
| Master absolute sample sum | 23,739.2952373758 |
| Independent cue absolute sample sum | 48,238.5147857666 |
| Original project manifest | unchanged |
| Import + encoded/decoded identity scan | 92.104 s |
| Export, including delayed reads/publication | 32.912 s |
| Full independent verification | 9.792 s |

The 128 MiB RSS and 1 MiB PCM budgets pass. RSS is this process's `getrusage` high-water mark; OS filesystem cache and the separately running GUI are outside that process measurement. Timing is local elapsed time and not a cross-platform or live-latency promise. No starvation fades appear in the output: every sample matches the independent master baseline despite delayed reads.

[Raw synthetic evidence](data/2026-11-05-offline-export.json) contains no user songs or private source paths. The local WAV artifact is deliberately not committed. Its SHA-256 is `dc4a6d668d671e42304cec9f3bad7fbf75a0d45566500d4813506cd0d74d0948`.

```sh
cmake --preset native
cmake --build --preset native
ctest --preset native
python3 scripts/make-streaming-fixtures.py /tmp/fader-export-inputs
build/native/fader_probe_artefacts/Release/fader_probe --export-soak \
  /tmp/fader-export-inputs/long-A.wav /tmp/fader-export-inputs/long-B.wav \
  /tmp/fader-hour.wav
```

Allow space for the substantial output file. FFmpeg, a UI, controller and audio-output device are unnecessary for this WAV fixture.

## Storage failure and publication

`exportWav` now routes the GUI/API through the same independent offline host and checked output transaction. The pinned JUCE writer's `flush()` rewrites headers but does not itself flush/check the underlying file. Its destructor rewrites the header again. The new stream latches ignored write/seek errors, uses unbuffered native writes to observe short writes, validates both final header passes, flushes/closes storage and checks the finished WAV metadata before publication.

Faults are injected into the actual encoder/destination path: initial header, data write, partial data write, final header, final seek, storage flush, final validation and refused replacement. All reject success, preserve a prior valid WAV and clean the temporary file. The test also covers cancellation before work and partway through output, non-finite float audio, an unreadable source, a temporary output unlinked while still open (POSIX), an unavailable parent directory, a real filesystem replacement blocked by a directory, and source-protection through a hard-link alias. A successful replacement publishes changed audio only after all checks pass. These simulations qualify error propagation and transaction behavior, not a physically exhausted volume or power-loss durability.

## Remaining verification

[Clean CI](https://github.com/coder13/Fader/actions/runs/41996740351) passes Linux 5/5 (GNU 13.3/CMake 3.31.6/x86_64) and macOS 6/6 (AppleClang 17.0.0.17000013/CMake 4.4.4/arm64 Darwin 24.6.0), including app signature verification. Downloaded metadata/JUnit/logs match source `6ba1da5d82892385d957ff656ffee455f10f95cc` and the JUCE pin, with fixtures OFF, zero failures/errors/skips, zero offline PCM error, callback allocation checks and native export-failure retention success markers. The GUI now reuses the transport button for **Cancel export** and reports progress in the mode label; its worker shares the same cancellation flag with source preparation. [Native click/progress inspection is now complete](2026-11-05-native-seek-export.md): the full-hour synthetic set shows progress and Cancel export, cancellation returns to editing, and an independently compared native Save copy preserves all 150,032 points and source identities. The new destination and temporary output are absent after cancellation. The native overwrite sheet is manually cancelled before this new-destination test; existing-target retention remains backend evidence. Synthetic native screenshots are shared in the development chat. A fresh recovery copy is preserved before updating the same preview, and the user set is restored afterward.

Current tail/range, clipping, render-version, RF64 scope and cancellation/publication boundary are defined in the [contract](../offline-render-contract.md). Windows native execution, sustained callback load and physical-device behavior remain separate evidence. Graceful shutdown can still wait for outstanding storage I/O; the ordinary Cancel action requests worker cleanup without joining from the click handler.
