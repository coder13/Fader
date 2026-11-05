# Plugin effects for version 2 and version 3

Status: User-confirmed future requirement; implementation design proposed. Updated 2026-11-04. Plugin hosting is outside the POC. Version 2/3 are product-release targets, not the existing M2/M3 milestone IDs.

## Intended workflow

Scan installed plugins into an effects browser. Drag an effect onto a song/clip, lane/deck, or master effects rack; open its editor, reorder the chain, and bypass an effect. Record exposed parameter changes from the plugin editor or mapped controller, or draw them on automation lanes. Save the chain, settings, and automation with the set. Playback and audio export process the same saved chain.

Start with **VST3 audio effects on macOS and Windows**, qualified for supported OS/CPU/plugin builds. JUCE already supplies plugin-format management and instance loading, making this a credible extension of the proposed C++ host. It still requires our own rack, persistence, automation, delay compensation, lifecycle, and compatibility work. [JUCE hosting API](https://docs.juce.com/master/classjuce_1_1AudioPluginFormatManager.html).

Legacy VST2, virtual instruments, and other formats are not implied by the initial requirement. Audio Units on macOS are a possible later extension; no commitment is needed for the POC.

## Proposed delivery

| Release target | Scope |
| --- | --- |
| Version 2 | VST3 stereo effects; plugin browser and editors; ordered song/lane/master racks; parameter automation; state save/restore; basic latency compensation; export and missing-plugin handling; isolated scanning and a tested plugin matrix |
| Version 3 | Broader compatibility, runtime process isolation if viable, freeze/bounce portability, richer routing/sidechains/sends, and optional additional formats |

Version 2 must validate export with supported effects; version 3 is not a deferral of basic recording or project correctness. No promise that every plugin or every version works identically. Runtime crash containment is a separate capability from scanning in a helper process.

## Contracts to preserve in the architecture

- **Identity and ownership:** each rack and plugin instance has a stable project ID. Persist plugin format/class ID, vendor/version, channel layout, order, bypass state, opaque processor/editor state, and relevant external-asset references. A clip-owned rack moves with the song; a lane/master rack remains attached to that bus. Preserve effect tails after a clip ends according to an explicit tail/fade/cut policy.
- **Automation:** address parameters by plugin instance ID plus stable plugin parameter ID, not UI name or list position. Capture exposed parameter gestures and their accepted timing; not every control in a plugin GUI is necessarily automatable. Do not apply the built-in fader smoothing policy blindly to plugin parameters. Restore initial state before replay and test plugin/wrapper behavior at different block sizes. VST3 separates processor state, controller state, and parameter automation. [VST3 API and persistence](https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical%2BDocumentation/API%2BDocumentation/Index.html).
- **Latency and routing:** account for reported plugin delay across parallel signal paths, including bypass and dry/wet paths. Compensating delay keeps paths aligned; it cannot remove the latency a lookahead effect adds to a new controller gesture. Display chain latency and offer a deliberate low-latency bypass/freeze workflow. Dynamic latency changes need a controlled plan update. VST3 provides latency-change notifications and host queries. [Processing contract](https://steinbergmedia.github.io/vst3_dev_portal/pages/FAQ/Processing.html).
- **Export and seeking:** instantiate an independent chain from the saved state; supply tempo/transport context, preroll, and tail handling. Validate offline versus real-time behavior per plugin; some processors depend on randomness or render mode. Offer a real-time render path where required. Infinite tails need a bounded user-visible export policy. A serialized preset is not necessarily a snapshot of every internal DSP buffer.
- **Failure and portability:** scan outside the main application process, time out and quarantine failed scans, and keep state even when an effect is unavailable. Show a missing/incompatible plugin placeholder; require an explicit bypass or frozen render before exporting a changed mix. Native plugin binaries and authorizations remain installed dependencies, not automatically bundled project media. Matching Mac/Windows builds and compatible state are required for cross-platform reopening; a Windows binary does not run natively on the Mac.

State capture/restore, loading, editor creation, and graph rebuilding occur outside the real-time callback under a defined lifecycle. Third-party processing can still miss deadlines or crash; an in-process host cannot contain all such failures. Evaluate runtime isolation against its buffering/latency cost before promising reliable live use with arbitrary effects.

## Web boundary

A desktop web UI over our native engine can expose the same VST3 racks; plugin editors may open as native windows. The browser-only host will treat installed native VST3 effects as unavailable. It can use a native companion or previously frozen audio; a Wasm-specific effect is a separate implementation target, not automatic compatibility with an installed plugin. Preserve unknown plugin state when a browser edits other parts of a set. See [web platform design](research/web-platform.md).

## Acceptance evidence

Use redistributable test effects covering gain, delay/reverb tails, reported and changing latency, bypass, and parameter automation. Add a representative third-party compatibility matrix. Verify reorder/save/reopen, two instances of the same effect, clip versus bus ownership, tempo-aware processing, nonzero-position playback, and export. Inject missing-plugin, scan failure, and incompatible-build cases; retain the original state. For deterministic test effects, compare timing and PCM under an explicit tolerance; do not extend that guarantee to arbitrary third-party DSP.

## Dependency note

The current upstream VST3 SDK uses the MIT license; pin and review the exact SDK version when adopting it. This does not change JUCE's own licensing or individual plugin licenses. [VST3 SDK license](https://raw.githubusercontent.com/steinbergmedia/vst3sdk/master/LICENSE.txt).
