# Audio engine and implementation stack

Status: research and proposed decisions; no implementation or hardware measurements yet.  
Researched: 2026-11-04.  
Scope: open-source software, macOS first, Windows next, Linux desirable. The first controller is a Native Instruments Traktor Kontrol S4 MK3. Two internal playback lanes are enough for the initial experiment; this does not imply full four-deck hardware support. The observed benchmark candidate is an M3 MacBook Air with 16 GB RAM running macOS 26.6.2; no audio benchmarks have been run.

## Recommendation

Use **C++20 for the audio engine and a small JUCE desktop shell for the first native POC**, subject to adopting a compatible open-source license. Keep the engine independent of GUI widgets and operating-system devices. The same engine must render a saved set to a device or to an offline audio file. Make web/hybrid delivery a separate measured decision; this recommendation is for the shortest native audio experiment, not a permanent restriction on the UI.

JUCE reduces the amount of platform plumbing for audio devices, MIDI, file formats, and GUI controls. It does not supply our arrangement model, editable performance recording, controller HID protocol, DJ synchronization, or guaranteed real-time behavior. Its current stable release page lists **9.0.5**, and that tag's license is **AGPLv3 or commercial JUCE 9**, not an assumed permissive license. Open source alone does not settle the application's exact license. [JUCE release list](https://github.com/juce-framework/JUCE/releases), [tag-pinned license](https://raw.githubusercontent.com/juce-framework/JUCE/9.0.5/LICENSE.md), [audio/MIDI modules](https://docs.juce.com/)

Rust is a credible alternative, particularly if the team strongly prefers it, but it is not automatically lower latency. CPAL provides cross-platform audio I/O; UI, MIDI, HID, decoding, and quality time stretching still require a collection of libraries and integration work. Choose one engine language for the POC. A C++/Rust split should solve a demonstrated problem before adding an FFI boundary, two build systems, and ownership rules.

Before finalizing the greenfield approach, run a bounded **Mixxx reuse spike**. Existing controller integration and DJ playback are valuable. A full fork is justified only if it can gain a timeline and headless deterministic rendering without a much larger refactor than a small new core. Do not treat Mixxx as an already separable engine SDK.

## What the framework choice changes

The assessments below are engineering judgments based on the cited capabilities, not comparative benchmark results.

| Candidate | Advantages for this project | Main cost or risk | Recommendation |
| --- | --- | --- | --- |
| C++20 + JUCE | One framework for native audio/MIDI, desktop GUI, basic file formats; direct use of C/C++ DSP libraries | C++ memory/concurrency discipline; AGPL/commercial license; timeline still custom; S4 HID still separate | Preferred native POC |
| C++20 + Qt + PortAudio + MIDI/HID libraries | Strong general desktop UI; independent audio layer; close to the ecosystem used by Mixxx | More packages and interfaces; Qt module/license choices; no ready-made DJ arrangement engine | Strong alternative if Qt timeline/product UI is decisive |
| Rust + CPAL + midir + native UI | Ownership and memory safety help model/control code; native device access exists | More integration choices; some DSP likely via C/C++; callback code can still allocate, block, or panic | Viable if Rust expertise outweighs integration cost |
| C++ or Rust core + web UI | Could share editor work with a browser version; native controller and audio layer retained | IPC timing, process lifecycle, packaging, rendering overhead; requires native scheduling boundary | Evaluate after audio/controller spike |
| Mixxx fork or selected reuse | Existing DJ engine, library, mappings, audio routing, and platform work | Coupled live-deck architecture; redesign for arrangement/automation; inherited licensing and maintenance surface | Time-box a feasibility spike before dismissing |
| C-only engine and custom shell | Excellent access to platform/DSP APIs; minimal language runtime | More manual ownership and application infrastructure with no clear project-specific win | Use C libraries, not a C-only application |

Verified library facts:

- JUCE exposes CoreAudio and Windows audio backends including WASAPI and optional ASIO, plus Linux options. A framework backend being available does not prove that a particular device, sample rate, or channel combination works. [JUCE device configuration](https://docs.juce.com/master/juce__audio__devices_8h.html)
- CPAL's upstream backend table includes macOS CoreAudio, Windows WASAPI/optional ASIO, and Linux ALSA plus optional alternatives. Its backend-specific toolchain and minimum OS requirements must be checked at the release we pin. CPAL is Apache-2.0. [CPAL repository](https://github.com/RustAudio/cpal), [backend matrix](https://raw.githubusercontent.com/RustAudio/cpal/master/README.md)
- midir supplies Rust MIDI I/O on CoreMIDI, Windows backends, and ALSA/JACK; it is MIT licensed. It is not a HID controller library. [midir](https://github.com/Boddlnagg/midir)
- PortAudio is MIT licensed. Qt's open-source terms differ by module: many modules use LGPLv3, while some use GPLv3; commercial licensing is another option. [PortAudio license](https://portaudio.com/license.html), [Qt module licensing](https://doc.qt.io/qt-6/licensing.html)

## Low latency is an architecture requirement

Audio has a deadline for every block. At 48 kHz, 64, 128, and 256 frames represent approximately 1.33, 2.67, and 5.33 ms of audio. These are **block durations, not total latency**. Device buffering, safety offsets, converters, DSP, controller transport, and scheduling add delay. Windows documentation also distinguishes input, output, and round-trip latency and describes driver-dependent small-buffer support. [Microsoft low-latency audio](https://learn.microsoft.com/en-us/windows-hardware/drivers/audio/low-latency-audio)

The callback must not allocate/free memory, read files, acquire contended locks, log to a console, or call unpredictable services. PortAudio explicitly documents these constraints; they apply to the design regardless of our chosen wrapper. Rust's lack of garbage collection does not make arbitrary Rust callback code safe: an allocation, blocking channel, mutex, final reference drop, or panic can still violate the deadline. [PortAudio callback rules](https://files.portaudio.com/docs/v19-doxydocs-dev/writing_a_callback.html)

Proposed division of work:

| Execution context | Owns | Transfers |
| --- | --- | --- |
| UI/application thread | Editable project, undo/redo, selection, device setup | Validated commands and compiled immutable playback plans |
| Controller I/O and scheduler | MIDI/HID parsing, timestamp conversion, mapping, outgoing LED messages | Bounded timestamped parameter/transport events |
| Media workers | File opening, decoding, source cache, waveform/beat analysis | Ready PCM/cache pages and metadata |
| Audio callback | Authoritative running transport, prepared DSP state, event application, mixer | Small telemetry and capture records through bounded queues |
| Recorder/save workers | Automation take assembly, audio recording, autosave | Durable files and UI progress |
| Offline renderer | Frozen project snapshot and same rendering core | WAV/other encoder stream, progress, diagnostics |

Design rules for the POC:

1. Preallocate deck buffers, event storage, DSP scratch, and queues before starting audio. Initialization and teardown happen off the callback.
2. Use bounded single-producer/single-consumer queues where ownership allows. Multiple input producers need explicit separate queues or a control aggregator; calling a queue “lock-free” does not prove bounded latency.
3. Publish prepared playback plans at a defined block boundary. Retire old plans on another thread; destroying a graph or releasing its final shared reference in the callback can free memory.
4. Give overflow and starvation visible behavior. Never silently drop recorded automation. An automation capture overflow invalidates the take or stops its recording with a clear error; an output underrun increments a diagnostic counter.
5. Reserve transport commands and releases from ordinary continuous-controller coalescing. Coalescing may be suitable for some UI meters; it must not erase short gestures in a recorded take.
6. Keep predictable DSP initially on the device callback. Decode and analysis workers are asynchronous and must never be awaited there. If a stretcher needs auxiliary real-time workers, benchmark that design explicitly rather than waiting on an ordinary thread pool.
7. Start with one audio device/clock for master and headphone channels. Separate physical devices introduce drift and resampling concerns; defer aggregate-device support.

On macOS, the framework's real-time audio thread is automatically associated with an audio workgroup. Custom auxiliary real-time threads need additional workgroup handling. This is a reason to keep the first engine single-render-thread, not to add parallel DSP immediately. [Apple audio workgroups](https://developer.apple.com/documentation/audiotoolbox/understanding-audio-workgroups?changes=_9)

## One timeline, several clocks

The arrangement should not run from UI refreshes or a generic timer. Use these distinct concepts:

- **Engine frame counter:** monotonically increasing frames rendered in the current device/render session.
- **Project transport position:** can stop, seek, or loop; maps the running engine to the arrangement.
- **Musical time:** beat/tick position interpreted through a versioned tempo map.
- **Source position:** frames in a particular original track, interpreted through its beat grid and warp/playback mapping.
- **Host/controller timestamp:** monotonic time used to place incoming physical events onto the engine timeline.

Proposed processing path: parse a device event, normalize it to a stable application parameter, map its timestamp to an engine frame, apply it at a sample offset within a render block, and capture the accepted event with its effective position. Late events are applied at the earliest available position and counted; they cannot alter samples already sent to the device. Store enough diagnostics to distinguish physical input delay from audio processing delay.

JUCE's callback context can expose a host timestamp but it can also be absent. MIDI timestamp units depend on context. The implementation must define and verify conversion rather than assuming all timestamps are interchangeable seconds. [JUCE callback context](https://docs.juce.com/master/structjuce_1_1AudioIODeviceCallbackContext.html), [JUCE MIDI timestamps](https://docs.juce.com/develop/classjuce_1_1MidiMessage.html)

For editable performance recording, store accepted **semantic events and parameter targets**, not only raw MIDI or HID bytes. Include stable clip/deck identity, parameter identifier, value/unit, sequence ordering, and timeline position. Preserve raw input optionally for diagnostics. A saved crossfader movement should replay without the original controller attached.

Smoothing and interpolation are part of the sound. Recording a target then replaying it through a different smoothing rule will change the result. Version the parameter curves and smoothing policy, retain the unsimplified take, and make any curve reduction non-destructive. Transport changes, cue jumps, loop edits, and rate changes need event semantics beyond a generic numeric automation curve.

Outgoing MIDI clock/control should be scheduled from this transport, but operating-system/USB output jitter is a separate measurement. MIDI output is not guaranteed sample-accurate merely because internal automation is. Controller feedback can usually be rate-limited off the audio thread.

## Decoding, streaming, and seeking

The earliest experiment may decode short test files into memory, with an explicit memory limit. A long DJ set must not require all songs to reside as decoded PCM. Decode workers should populate bounded read-ahead caches keyed by source identity, channel layout, sample rate, and source-frame range.

Normal playback needs forward read-ahead; cue jumps, loops, reverse, and scratching need different cache policies. A seek should prepare a new cursor/cache/DSP state off-thread where possible, then switch cleanly. Stale worker results need generation identifiers so an old seek cannot replace a newer one. Define whether an unavailable seek waits, stays at the old position, or fades to silence; never block the callback on disk.

Mixxx is useful implementation evidence: its player guide separates `EngineBuffer`, `CachingReader`, `ReadAheadManager`, and rate-scaling components. That supports this decomposition without establishing that those classes can be extracted unchanged. [Mixxx player architecture](https://github.com/mixxxdj/mixxx/wiki/Developer-Guide-Engine-Player)

Use WAV/AIFF test assets first. Add FLAC/MP3 import after accurate source-frame seeking, decoder priming, and gapless handling are tested. libsndfile supports common PCM/lossless formats and MP3 in builds with the appropriate dependencies; Symphonia is a Rust decoding/demuxing alternative under MPL-2.0. These are candidates, not a promise that every encoding variant works. [libsndfile formats](https://libsndfile.github.io/libsndfile/formats.html), [libsndfile build options](https://github.com/libsndfile/libsndfile), [Symphonia](https://github.com/Samadaeus/symphonia)

## Tempo, pitch, beat grids, and synchronization

Separate three operations:

1. **Sample-rate conversion:** reconcile a file's native sample rate with the engine rate.
2. **Varispeed:** change playback speed and pitch together, like changing turntable speed.
3. **Time stretch/key lock:** change speed while preserving pitch; optionally change pitch independently.

Implement varispeed before quality key lock so control capture and replay can be proved with fewer moving parts. Do not call a speed control “key lock” until a stretcher is integrated and tested. Ordinary resampling is not independent tempo/pitch control.

For time stretch, prototype both Rubber Band and Signalsmith Stretch behind a small adapter. Rubber Band supports dynamic independent tempo/pitch and offers GPL or commercial licensing. Its output availability can vary locally, so an output-driven/pull adapter and explicit padding/delay handling are required. Signalsmith Stretch is MIT licensed and documents separate input/output latency plus optional computation spreading that adds latency. Neither should be selected solely from API convenience or reputation. [Rubber Band overview](https://breakfastquay.com/rubberband/), [Rubber Band integration](https://breakfastquay.com/rubberband/integration.html), [Signalsmith Stretch](https://github.com/Signalsmith-Audio/signalsmith-stretch), [Signalsmith integration/latency](https://raw.githubusercontent.com/Signalsmith-Audio/signalsmith-stretch/main/README.md)

Our proposed adapter must report latency and startup requirements, accept scheduled rate/pitch changes, support explicit reset/seek, and expose bounded process sizes. The render plan can pre-roll prepared clips and compensate known delays. A spontaneous jog/scratch action cannot use future user input, so its response is a different problem from rendering a planned transition. Allow a deliberate varispeed/scratch path rather than forcing every interaction through a high-quality spectral stretcher.

Beat analysis should produce editable metadata, not irreversible changes to audio. Start with a manually entered BPM and first downbeat for constant-tempo material; later allow multiple beat anchors for variable-tempo recordings. A BPM estimate alone cannot establish beat phase, downbeats, or accurate long-term synchronization. Store detector/version/confidence separately from user corrections.

For sync, map source beats to project beats, deriving the playback rate from that mapping and the global tempo map. Decide what happens when the user nudges a synced deck and when sync re-engages. For the POC, use a fixed global tempo and manual grids; promote tempo ramps and piecewise source grids only after the time mapping and replay tests pass.

## Offline export and reproducibility

The renderer should consume a frozen project revision and render without opening an audio device or controller. Change only the sink and execution pace; retain the same transport, event ordering, source mapping, gain/pan/crossfade law, DSP, smoothing, and initial state. Default to a playback-equivalent export mode. A higher-quality alternate stretch algorithm may be offered later, explicitly labeled because it can sound different.

Proposed reproducibility contract:

- Same project/assets, pinned engine/DSP versions, parameters, and output format reproduce the same event timing.
- Simple no-stretch/no-random-DSP fixtures should match tightly between live-engine capture before the device and offline rendering.
- Compare across device callback sizes to expose accidental block-rate automation. If a DSP library is block-sensitive, use a canonical internal processing schedule and document its latency/cost.
- Exact bit identity across architectures, compilers, and arbitrary future DSP versions is not initially promised. Use numerical tolerances and listening tests where floating-point or DSP behavior differs.
- Keep source fingerprints, decoder/DSP identity, sample rates, latency compensation policy, and project schema version. Offer consolidated media so missing external files do not make a set unrenderable.
- Live external inputs and external hardware effects must be recorded as audio if the set is expected to reproduce them later. Automation cannot reconstruct an unrecorded microphone or external synth.

Export WAV first, using 32-bit float for test comparisons and 24-bit PCM as an initial delivery option. Define start/end selection, effect-tail policy, clipping behavior, normalization, and dither explicitly before adding format presets. MP3 encoding belongs on a worker/offline path; LAME is an LGPL encoder candidate. libsndfile's MP3 encoding depends on its build configuration. FFmpeg can broaden import/export later, but its effective license depends on enabled components. [LAME](https://lame.sourceforge.io/), [libsndfile MPEG support](https://github.com/libsndfile/libsndfile/blob/master/CHANGELOG.md), [FFmpeg licensing](https://ffmpeg.org/legal.html)

## Mixxx reuse spike

Verified: Mixxx targets macOS, Windows, and Linux. Its main license text specifies GPL version 2 **or later**, and notes separate licenses for bundled libraries and skins. Its engine guide describes a live callback-driven mixer, headphone routing, and deck processing. [Mixxx README](https://raw.githubusercontent.com/mixxxdj/mixxx/main/README.md), [Mixxx license](https://raw.githubusercontent.com/mixxxdj/mixxx/main/LICENSE), [Mixxx engine guide](https://github.com/mixxxdj/mixxx/wiki/Developer-Guide-Engine)

Proposed spike, with a written decision at its end:

1. Identify a stable tag and build it on the actual Mac. Inventory relevant source headers, controller mapping licenses, and dependency versions.
2. Trace one S4 input to a mixer parameter and identify where a semantic event could be captured with an engine timestamp.
3. Feed a scripted transition through the same control/render path and try a headless file sink.
4. Assess whether arrangement clips can have identities and lifetimes independent of the current live deck/library state.
5. Compare the amount of adaptation against the smallest greenfield JUCE experiment.

A fork becomes attractive if hardware support dominates the risk and the experiment can reuse it without losing deterministic replay. A greenfield engine becomes attractive if rendering depends deeply on live application globals or a timeline would fight the existing model. These are hypotheses to test, not findings from a completed code audit. Study and attribution are safe starting points; copied source, mappings, images, and skins each require their actual license review. Do not assume “both open source” makes every combination interchangeable.

## Licensing decisions to record

The project is intended to be open source. Choose its exact license before importing significant implementation dependencies.

| Component | Verified license route | Consequence for selection |
| --- | --- | --- |
| JUCE 9.0.5 | AGPLv3 or commercial | An AGPL-compatible project is the straightforward open-source route; a commercial JUCE license may be required if that route is not used |
| Qt | Module-dependent LGPLv3/GPLv3, commercial alternative | Pin modules and track notices/source/relink obligations appropriate to chosen terms |
| PortAudio | MIT | Keep copyright/license notices |
| CPAL / midir | Apache-2.0 / MIT | Permissive core I/O options; inspect optional dependencies too |
| Rubber Band | GPL or commercial | Exact release/library dependency review; open-source project license must fit |
| Signalsmith Stretch | MIT | Permissive candidate; still benchmark sound and latency |
| libsndfile / LAME | LGPL | Preserve terms and distribution obligations; review optional codecs |
| Mixxx | GPLv2-or-later at top level; per-component licenses | Audit reused code/assets/mappings individually |
| ASIO SDK | GPLv3 and proprietary routes | Windows packaging must choose an applicable route; not inherited automatically from the wrapper's license |

Sources: [JUCE 9.0.5 license](https://raw.githubusercontent.com/juce-framework/JUCE/9.0.5/LICENSE.md), [Qt](https://doc.qt.io/qt-6/licensing.html), [PortAudio](https://portaudio.com/license.html), [CPAL](https://github.com/RustAudio/cpal), [midir](https://github.com/Boddlnagg/midir), [Rubber Band](https://breakfastquay.com/rubberband/), [Signalsmith](https://github.com/Signalsmith-Audio/signalsmith-stretch), [libsndfile](https://libsndfile.github.io/libsndfile/), [LAME](https://lame.sourceforge.io/), [Mixxx](https://raw.githubusercontent.com/mixxxdj/mixxx/main/LICENSE), [Steinberg developer portal](https://www.steinberg.net/developers/), [Steinberg proprietary ASIO FAQ](https://forums.steinberg.net/t/asio-sdk-proprietary-licensing-faq/1044024)

Pin release tags/commit IDs and record their license files with the dependency manifest. Researching a current release does not mean floating the implementation on `master`. No project license is installed by this document.

## Proposed benchmark and correctness gates

These are initial acceptance targets, **not achieved results**. Record the Mac/chip, macOS, controller firmware, audio device/channel layout, driver, compiler/build type, buffer size, sample rate, DSP settings, and test material with every result.

| Gate | Proposed pass condition | What it proves |
| --- | --- | --- |
| Native output | Two stereo sources and mixer run at 48 kHz/128 frames; 256-frame fallback reported separately | Basic audio/device feasibility |
| Sustained audio | Zero detected underruns or missed callback deadlines during a 60-minute scripted run with UI interaction and background decoding | Stability for the tested configuration, not all machines |
| Deadline margin | p99.9 callback execution below 50% of block duration; maximum below 100% in the run | Processing headroom; log histograms off-thread |
| Allocation audit | No project-owned heap allocations/frees or blocking operations in the callback after preparation | One class of real-time hazards removed |
| Control latency | Target p95 under 10 ms from timestamped input receipt to scheduled audible output for gain/crossfader, excluding unknown physical-device delay; report full measured loopback/physical latency separately | Responsive mixer controls with precisely scoped measurement |
| Clock correctness | Synthetic events applied at their requested frame; late events and timestamp discontinuities counted | Internal scheduling accuracy |
| Automation replay | Scripted/live-captured gain and crossfader take replays after save/reload with matching transition positions and a defined numerical tolerance | Flagship feature survives persistence |
| Block independence | Fixtures rendered at several callback sizes and offline have matching timing; any residual differences characterized | No dependence on the UI/block refresh rate |
| Stretch evaluation | Test ±6%, ±10%, ±20% tempo, key changes, transients, vocals, bass, silence, short clips, seeks; measure latency and peak callback time | Quality/performance tradeoff understood |
| Audio routing | Master and cue channels independently verified on S4 audio device; cue does not leak into exported master | DJ monitoring correctness |
| Recovery | Device removal, reconnection, sample-rate change, and queue/cache failures produce controlled state and recoverable project data | Failure handling understood |

Do not fail the entire product concept because one conservative latency target misses on one machine. Record the measurements, locate the dominant delay, and change the supported configuration or target deliberately. Conversely, do not call the POC low latency based only on a configured 128-frame buffer.

## Decision gates and remaining unknowns

1. **Exact project license:** open source is confirmed; AGPL-compatible vs another policy is still a decision.
2. **Mac and S4 qualification:** the observed M3 MacBook Air/macOS 26.6.2 host, S4 firmware, audio routing, and stable buffer sizes require hardware testing. Controller HID and optional screens/motors need their own feature matrix.
3. **Greenfield vs reuse:** run the bounded Mixxx extraction/fork experiment before committing substantial architecture.
4. **UI delivery:** use the separate web/hybrid investigation to decide whether native JUCE remains the product shell or just the initial engine harness.
5. **DSP choice:** benchmark quality and latency, including abrupt input changes and offline parity; do not select a stretcher from documentation alone.
6. **Reproducibility scope:** start with supported internal DSP and local files; plugins and external effects require a wider state/capture strategy.

The strongest initial proof is one real controller transition that is captured as semantic automation, edited on a timeline, replayed with the controller disconnected, saved/reopened, and exported through the same engine. A large deck UI without that loop would leave the central product risk unresolved.
