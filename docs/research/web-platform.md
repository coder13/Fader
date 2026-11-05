# Web platform research

Status: research and proposed experiments; no platform benchmark or controller test has been run. Reviewed 2026-11-04. Hardware target: Native Instruments Traktor Kontrol S4 MK3. Project direction: open source.

## Recommendation

Keep a browser version in the architecture, particularly for arranging tracks, editing automation, and rendering prepared sets. For the first hardware POC, prefer a native audio/controller engine. A web UI can run above that engine if its timeline editing advantages justify the bridge and packaging work. A browser-only POC should be a bounded alternative experiment, with S4 MK3 input, output feedback, and separate master/headphone routing as explicit gates.

This is an engineering recommendation, not a claim that browsers cannot make professional audio. AudioWorklet provides an audio processing thread, and Wasm enables reuse of C/C++ DSP. The decisive uncertainty is the complete hardware and operating environment, rather than basic mixing arithmetic. [AudioWorklet documentation](https://developer.mozilla.org/en-US/docs/Web/API/AudioWorklet), [Emscripten Wasm audio worklets](https://emscripten.org/docs/api_reference/wasm_audio_worklets.html).

Do not make the file format depend on DOM components, a Web Audio graph, or any UI framework. A headless render core and versioned project model preserve the option to build native, browser, and hybrid hosts.

## Platform comparison

The assessments below are project recommendations, not measured performance rankings.

| Option | Strength for this project | Constraint to resolve | Suggested role |
| --- | --- | --- | --- |
| Browser/PWA, Web Audio + Wasm | Easy access; strong timeline/editor tooling; local media need not leave the computer | Browser hardware APIs, output routing, memory, file permissions, lifecycle | Prepared-set editor and bounded Chromium hardware experiment |
| Electron UI + native engine | Consistent bundled Chromium UI; native files/audio/controller access through an explicit bridge | Larger runtime; native module/process packaging; IPC ownership and update maintenance | Credible desktop option if web UI is preferred |
| Tauri UI + native engine | Web frontend with Rust/native integration; uses OS webview | Different browser engines by OS; C++ engine requires FFI or process integration | Credible desktop option, especially if Rust is selected |
| Native UI + native engine | Fewest host boundaries between hardware and audio; mature audio libraries | Timeline UI still requires substantial custom work | Lowest architectural risk for first controller/audio POC |
| Browser UI + installed local service | Browser editor with native hardware access | Still requires installation, secure pairing, version compatibility and lifecycle management | Defer unless a browser-hosted editor becomes a product requirement |

Electron exposes device APIs with application-managed permission handling. Tauri's standard webview uses WKWebView on macOS, WebView2 on Windows, and WebKitGTK on Linux. A wrapper does not itself make browser audio run through a native engine; that engine and bridge must be built. [Electron device access](https://www.electronjs.org/docs/latest/tutorial/devices), [Tauri architecture](https://github.com/tauri-apps/tauri/blob/dev/README.md).

## Browser audio: what is viable

Use one AudioContext for the interactive audio graph and an AudioWorklet for time-critical custom processing. Keep waveform generation, file decoding, analysis, and UI work off that processing path. Compile suitable DSP kernels to Wasm; initialize modules and allocate buffers before playback. Emscripten supports C/C++ worklet processors and also describes a non-shared-memory integration route. Wasm does not remove every possible browser/OS stall, nor does it make allocations inside our own DSP safe. [Emscripten worklet integration](https://emscripten.org/docs/api_reference/wasm_audio_worklets.html).

The commonly documented render quantum is 128 frames, about 2.67 ms at 48 kHz. That is a processing block duration, **not** controller-to-speaker latency. Code must read actual buffer lengths because block sizing is evolving. `latencyHint` is a request the browser may ignore; `baseLatency` covers only part of the output path. Hardware measurements are required. [Worklet processing blocks](https://developer.mozilla.org/en-US/docs/Web/API/AudioWorkletProcessor/process), [baseLatency](https://developer.mozilla.org/en-US/docs/Web/API/AudioContext/baseLatency).

Web Audio supports scheduled parameter automation on its audio clock. Use scheduled events or a sample-indexed queue consumed by the worklet; do not make UI animation frames or JavaScript timers the musical clock. Convert captured input timestamps into the engine timeline once, preserving the original capture time as diagnostic metadata. `getOutputTimestamp()` provides a relationship between audio context and performance time useful for clock mapping. [AudioParam](https://developer.mozilla.org/en-US/docs/Web/API/AudioParam), [getOutputTimestamp](https://developer.mozilla.org/en-US/docs/Web/API/AudioContext/getOutputTimestamp).

Playback rate alone is not our key-lock algorithm: `AudioBufferSourceNode.playbackRate` performs resampling. Pitch-preserving tempo changes require an explicit stretch implementation, with its latency and quality measured in the same two-deck workload. [Playback rate documentation](https://developer.mozilla.org/en-US/docs/Web/API/AudioBufferSourceNode/playbackRate).

For the proposed browser implementation, use a small JS/TypeScript adapter around the same DSP and project semantics as native. Avoid duplicating the final mixer/automation rules in separate JavaScript and native implementations; cross-host parity should be a testable design property.

## Shared memory and deployment

AudioWorklet + Wasm does not inherently require pthreads. If worker decoding or rendering communicates through SharedArrayBuffer, plan the synchronization and deployment requirements explicitly. Emscripten pthreads require correctly configured cross-origin opener/embedder policies in the deployed page. A single-threaded fallback is a separate build/configuration decision. [Emscripten pthread requirements](https://emscripten.org/docs/porting/pthreads.html).

A worker-to-worklet ring buffer can smooth production variability but introduces buffering. It cannot make slow DSP meet its deadline by itself. The audio consumer must never wait for a worker, lock, network request, or UI message. Keep underflow counters and bounded queues. Chrome's documented design patterns illustrate this tradeoff. [AudioWorklet design patterns](https://developer.chrome.com/blog/audio-worklet-design-pattern).

Proposed deployment rules: serve the application and Wasm over HTTPS; feature-detect the required APIs; verify cross-origin isolation; self-host dependencies where practical; show an explicit reduced-capability state if shared memory or a hardware API is unavailable. Do not attempt to bypass browser permission or device protections.

## S4 MK3 and other hardware

The S4 MK3 is a HID integration target, not a generic firmware MIDI-controller assumption; see the [controller investigation](traktor-and-controller-integration.md). MIDI output to other equipment remains a separate feature.

| Capability | Browser evidence | Consequence for this project |
| --- | --- | --- |
| MIDI input/output | Web MIDI supports receiving messages and queued output timestamps, subject to browser availability and permission | Useful for compatible MIDI controllers and external MIDI output; does not turn S4 HID reports into MIDI |
| HID reports | Chromium WebHID exposes reports, subject to protected usages and device blocklists | Enumerate the actual S4 interfaces and validate reports; API presence alone is insufficient |
| USB vendor transfers | WebUSB exposes selected device interfaces, with permissions and protected-interface restrictions | Advanced screen/vendor features require a separate feasibility test |
| Safari hardware APIs | WebKit explicitly lists Web MIDI, WebHID and WebUSB among features it has not implemented | Do not promise Safari S4 control without native integration or an independently qualified extension |

Sources: [W3C Web MIDI](https://www.w3.org/TR/webmidi/), [Chromium WebHID](https://developer.chrome.com/docs/capabilities/hid), [Chromium WebUSB](https://developer.chrome.com/docs/capabilities/usb), [WebKit feature policy](https://webkit.org/tracking-prevention/). WebUSB's protected interface enforcement is also documented in a [Chromium implementation issue](https://issues.chromium.org/issues/524520965); it is not a universal raw USB escape hatch.

For Chromium qualification, first enumerate the exact device's HID report descriptors and relevant interfaces through supported permission flows. Check that play/cue buttons, channel faders, crossfader and jog input are readable, and that one known LED output report works. Avoid treating successful enumeration as full compatibility. Screens and jog motors are separate later work with their own protocols and safety bounds.

For Tauri on macOS, native CoreMIDI/HID access must feed the engine directly; using its webview's `navigator.requestMIDIAccess` would inherit the WebKit limitation. For either wrapper, controller events should reach the engine independently of UI event-loop health. [Tauri webview versions](https://tauri.app/reference/webview-versions/).

For MIDI clock output, schedule ahead against the engine clock rather than emitting from animation callbacks. Verify drift, jitter, reconnect behavior and cancellation of stale queued messages. Timestamped Web MIDI output is a capability, not evidence that a particular browser/device meets our timing target. [Web MIDI output semantics](https://www.w3.org/TR/webmidi/).

## Master and headphone routing

Chrome supports selecting a permitted output device with `AudioContext.setSinkId()`. Web Audio also exposes the destination's maximum channel count. Therefore a browser can potentially route a four-channel graph to one multichannel interface, but the actual controller, OS configuration, channel ordering and browser implementation must be tested. Selecting a device does not establish that master channels 1/2 and cue channels 3/4 are independently available. [Chrome output selection](https://developer.chrome.com/blog/audiocontext-setsinkid), [Destination channel count](https://developer.mozilla.org/en-US/docs/Web/API/AudioDestinationNode/maxChannelCount).

Prefer a single interface and audio clock for master plus cue. Two independently selected devices/contexts are an additional synchronization problem; do not assume their clocks remain aligned. This is an architectural inference from having independent output paths. Browser Web Audio exposes a higher-level output abstraction rather than an ASIO driver-selection/buffer-control UI. Native device control therefore remains the stronger starting point when exact routing and buffer settings are POC requirements. [Web Audio API draft](https://www.w3.org/TR/webaudio-1.1/).

The first browser routing test should play distinguishable quiet test signals on all four requested channels and verify them at the physical outputs. Record sample rate, channel count, context latency reports and loopback measurement. A failure here can still leave the browser useful as an arranger with stereo preview.

## Local library, project files, and export

File inputs and drag/drop can support a basic local-media workflow. Chromium's File System Access API adds user-authorized directory browsing and writing, but permission and browser differences require a fallback such as selecting files and downloading project/export files. Keep media identifiers and hashes in the project; file handles alone are not a portable project format. [File System Access](https://developer.chrome.com/docs/capabilities/web-apis/file-system-access).

Use browser storage for recoverable caches and autosave, not as the sole copy of the user's set. Quotas and eviction vary by browser, and best-effort storage can be removed. Offer explicit project export and media relinking. [Storage quotas and eviction](https://developer.mozilla.org/en-US/docs/Web/API/Storage_API/Storage_quotas_and_eviction_criteria).

`decodeAudioData()` accepts complete encoded files and returns resampled PCM, so loading an entire library this way is not a scalable streaming plan. AudioBuffer uses 32-bit PCM. Calculated memory for one hour of 48 kHz stereo float PCM is `3600 × 48000 × 2 × 4 = 1,382,400,000 bytes`, about 1.29 GiB, before source buffers, caches and encoder copies. Use bounded decode/cache windows and worker processing for longer sets. [decodeAudioData](https://developer.mozilla.org/en-US/docs/Web/API/BaseAudioContext/decodeAudioData), [AudioBuffer](https://developer.mozilla.org/en-US/docs/Web/API/AudioBuffer).

`OfflineAudioContext` can render faster than playback, but returns an AudioBuffer. It is suitable for a short proof, while long exports need a bounded-memory renderer or a carefully stateful chunking design. Independent render chunks must preserve stretch, resampling, filter and effect history; restarting each chunk is likely to produce boundary differences. The latter is an engineering inference and test requirement. [OfflineAudioContext](https://developer.mozilla.org/en-US/docs/Web/API/OfflineAudioContext).

WAV export can start with a PCM writer. MP3/FLAC/other formats need a qualified encoder and container path, either Wasm or browser-native where supported. WebCodecs exposes low-level codecs, not a universal complete file-export feature; test each requested configuration rather than assuming every browser encodes MP3. Keep codec licensing and build settings in the dependency inventory. [WebCodecs](https://developer.mozilla.org/en-US/docs/Web/API/WebCodecs_API), [AudioEncoder capability checking](https://developer.mozilla.org/en-US/docs/Web/API/AudioEncoder/isConfigSupported_static).

## Lifecycle and reliability

Initialize/resume browser audio from an explicit user action and display suspended/interrupted states. Browser autoplay rules affect AudioContext startup. [Chrome autoplay policy](https://developer.chrome.com/blog/autoplay/).

Do not claim that all background tabs necessarily stop audio: Chrome treats playing audio as a reason to preserve a hidden page, except under extreme resource constraints. Still test hidden tabs, minimizing, long UI stalls, memory pressure, device removal and sleep/wake. Persist automation incrementally; recovery must not depend on receiving a final unload callback. [Chrome page lifecycle](https://developer.chrome.com/docs/web-platform/page-lifecycle-api).

Electron has a `backgroundThrottling` option affecting timers/animation. Disabling it is not a replacement for moving authoritative transport and audio out of the UI process. A hybrid engine should continue correctly when the renderer is busy, and recover its view from engine state after a UI reload. [Electron BrowserWindow](https://www.electronjs.org/docs/latest/api/browser-window).

## Proposed experiments and acceptance evidence

These are future tests, not completed results. Browser versions, OS version, Mac model, device firmware, power mode and project revision belong in every report.

| ID | Experiment | Evidence required to make a decision |
| --- | --- | --- |
| WEB-01 | Capability page in current macOS Chrome, Edge and Safari | AudioWorklet, hardware APIs, file APIs, cross-origin isolation, encoder and output-device capabilities; explicit unsupported states |
| WEB-02 | S4 MK3 HID/control qualification in Chromium | Input reports for essential controls; one LED round-trip; permission-denial and unplug recovery; interface/protocol notes |
| WEB-03 | S4 master/cue routing | Four distinct physical channels verified, no master/cue leakage, actual latency measured; compare native baseline |
| WEB-04 | Two-deck browser audio stress | Both tracks with stretch, EQ and gain automation while editing/zooming; 30-minute run with dropout/underflow observations and CPU/memory report |
| WEB-05 | Capture/edit/replay equivalence | Record a 30-second fader transition, edit it, save/reopen and render; event boundaries agree within one engine sample in an offline fixture; live capture latency measured separately |
| WEB-06 | Long-set import/export | A 60-minute set with bounded working memory, progress/cancel and valid WAV; no clicks at processing boundaries; missing-media recovery |
| WEB-07 | Hybrid host spike, only if web UI is selected | Same project and engine fixtures; UI stalls do not interrupt native audio; controller capture bypasses UI; macOS bridge/package works |

Decision gates: browser-only hardware support proceeds only if WEB-02 and WEB-03 pass on the exact S4 MK3, and WEB-04 meets the project's native comparison thresholds. Failure keeps browser arrangement/preview viable. A hybrid frontend is selected based on editor productivity and measured bridge overhead, not merely because its UI is written in web technologies.

## Open questions

- Exact Mac model, macOS version, firmware and output route remain to be recorded during hardware qualification.
- Whether Chromium exposes every S4 interface needed for the first mapping is unmeasured.
- Browser master/cue channel mapping and actual controller-to-audio latency are unmeasured.
- The chosen DSP libraries need Wasm build, license and quality checks before shared-code portability is claimed.
- Whether one web frontend is worth maintaining across browser and desktop hosts should follow an editor prototype, not precede it.
