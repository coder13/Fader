# Bounded streaming qualification

Issue: [DJ-038](https://github.com/coder13/Fader/issues/38). Status: local software qualification in progress; clean CI and native visual verification are recorded before closure. No physical audio device or controller is opened by these fixtures.

## Configuration

Local Apple Silicon arm64, Darwin 25.6.0/macOS 26.6.2; AppleClang 21.0.0.21000334, CMake 4.4.4, Release/C++20. JUCE 9.0.5 is pinned at `d60b7a78030661574a615b586d9325ba7a7fa7dd`. Use the [native/core presets](../build-and-ci.md). Source SHA and clean-CI results will be attached to the tested commit before closure.

## Reproduction and fixtures

```sh
cmake --preset native
cmake --build --preset native
ctest --preset native
python3 scripts/make-streaming-fixtures.py /tmp/fader-stream-fixtures
build/native/fader_probe_artefacts/Release/fader_probe --streaming-soak \
  /tmp/fader-stream-fixtures/long-A.wav /tmp/fader-stream-fixtures/long-B.wav
build/native/fader_probe_artefacts/Release/fader_probe --qualify-stream /path/to/synthetic.mp3
```

The script makes two sparse 2 h 15 m stereo PCM16/48 kHz WAVs. Each is 1,555,200,044 logical bytes, with a 100 ms generated tone/pulse burst every minute and zeros between. They are real RIFF files, not fake length metadata, but do not represent worst-case decoding or continuous music density. The probe places B at 30 minutes and processes all **475,200,000 project frames (2 h 45 m)**, with 1 h 45 m overlap. It uses the real import, streaming cache and mixer; readiness waits occur outside processing. No audio output or giant export file is created.

Optional `--mp3-encoder /path/to/ffmpeg` adds 180-second synthetic rhythm WAVs at 44.1/48 kHz and four MP3s: libmp3lame 320 kbit/s CBR and quality-2 VBR at each rate. The local generator used FFmpeg 7.1 from isolated [imageio-ffmpeg 0.6.0](https://pypi.org/project/imageio-ffmpeg/0.6.0/); this is a fixture tool, not a Fader dependency. The checked-in eight-second [synthetic VBR fixture](../../tests/fixtures/README.md) and a notification asset from the pinned JUCE checkout run in native CI without an encoder.

## Measurements

The declared two-source probe budget is **128 MiB peak RSS**, **1 MiB PCM**, and **at most 1 MiB peaks**. The final direct-probe run exited successfully and reported:

| Measurement | Result |
| --- | ---: |
| Peak resident memory, including import (`getrusage`) | 12,402,688 bytes (11.83 MiB) |
| PCM cache | 1,048,576 bytes |
| Waveform peaks | 1,012,504 bytes |
| Source frames decoded | 777,600,000 |
| Missing clip-frames / failed sources | 0 / 0 |
| Sum of absolute output samples | 62,025.5139542 |
| Full import/validation time | 64.253 s |
| Full accelerated rendering time | 15.360 s |

The earlier successful probe had equivalent frames/cache/checksum and 12,042,240 peak RSS bytes. Its external `/usr/bin/time -l` wrapper failed a sandboxed sysctl after completion; the final run above uses the probe's own memory metric and verified exit status.

| Fixture | Sequential frames | Sequential max error | Seek max error |
| --- | ---: | ---: | ---: |
| 44.1 kHz WAV | 7,938,000 | 0 | 0 |
| 44.1 kHz CBR MP3 | 7,939,584 | 0 | 0 |
| 44.1 kHz VBR MP3 | 7,939,584 | 0 | 0 |
| 48 kHz WAV | 8,640,000 | 0 | 0 |
| 48 kHz CBR MP3 | 8,641,152 | 0 | 0 |
| 48 kHz VBR MP3 | 8,641,152 | 0 | 0 |

Each file is compared completely against an independent sequential decoder. Fourteen 128-frame windows cover forward/backward requests, cold opens, page boundaries, distant positions and the tail. The waveform uses the same source length/rate; WAV duration is exactly 180 s, MP3 durations are 180.0359167/180.024 s under the pinned decoder's delay/padding convention. These results do not establish cross-decoder gapless equivalence.

The first implementation exposed JUCE VBR random-seek failures at the final page (44.1 kHz fixture: source frame 7,938,048, count 1,536). Full sequential import succeeded. The [MP3 worker policy](../streaming-contract.md#decoder-correctness-and-limits) fixes the measured failure by sequential discard/replay. It uses bounded memory but deliberately leaves fast compressed seeking unresolved.

## Failure, lifetime and render checks

- Two logical eight-hour sources exercise page boundaries, terminal samples and distant/backward requests at a fixed 1 MiB PCM size.
- A blocked decoder stays blocked while 10,000 nonblocking sample requests return. Offline preparation times out explicitly. Actual engine processing crosses an unavailable page and verifies the 240-frame fade, cue relation and recovery blend.
- A delayed read completing after a seek is discarded. Cancelled/replaced banks cannot serve stale PCM; a new generation reads its own samples; decoder destruction follows worker shutdown. Injected read/non-finite failures are explicit.
- Real file mutation and deletion fail preparation. Offline source failure preserves an existing destination. Existing project/hash, atomic export/cancel, automation/filter, source-rate and master/cue fixtures still pass.
- A 44.1 kHz source at rate 1.25, nonzero offset/start and seek, with HP/LP automation, matches in-memory rendering exactly at 64/127/1,024-frame blocks.
- The C++ allocation fixture adds 20,000 prepared streaming callbacks and cancelled-source processing under ordinary/aligned new/delete interception. Preparation and worker allocations stay outside the guarded render thread.
- A standalone Apple ThreadSanitizer build of `Engine.cpp`, `Streaming.cpp` and `StreamingTests.cpp` passes without a race report, including actual decode workers and delayed seek/cancellation. This is targeted coverage, not a whole-application sanitizer claim.

## Scope and next work

This proves bounded PCM storage, tested ownership/failure behavior and source-frame parity for these fixtures. Accelerated rendering is not a wall-clock callback deadline, multi-hour device-playback or acoustic test. Import still scans/hashes full sources; seek reconstruction and MP3 replay can be slow; control-side destruction may wait for an OS read. Maximum 64-clip load, all-library/codecs, removable/network storage and Windows parity remain open. #39 addresses responsive seeking; #13/#40 qualify broader parity and sustained load.
