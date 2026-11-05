# Roadmap and draft GitHub issues

Status: Proposed backlog, not published issues. Updated 2026-11-04. IDs such as `DJ-001` are local planning IDs, not GitHub issue numbers. The new project directory has no configured GitHub remote. Research is complete for this planning pass; implementation has not begun.

The order prioritizes hardware feasibility and the editable-performance loop. Web research starts early; a production browser host depends on measured capability. Milestones have exit criteria instead of speculative release dates.

## Milestones

| Milestone | Outcome | Exit gate |
| --- | --- | --- |
| M0 Architecture decisions | Choose license, build/reuse route, initial host strategy; record S4/browser feasibility | ADR accepted with evidence and unresolved risks; no assumption that MIDI controls S4 MK3 |
| M1 Native Mac hardware harness | Two sources, S4 essential controls/LEDs, master/cue, timestamped event path, MIDI output proof | POC A01–A03/A06/A08/A14 demonstrated in a small harness |
| M2 Editable-set POC | Arrange, perform, edit, retake, save/reopen, export WAV | Full [POC acceptance matrix](poc-plan.md), including long-run/recovery evidence |
| M3 Musical mixing alpha | Beat/grid editing, key lock, tempo changes, EQ/filter, loops and cue actions | DJ-oriented transitions retain correct timing and editable replay |
| M4 Reliable Mac alpha | Long libraries/sets, formats, portable projects, recovery, packaging | User can finish and reopen a realistic set; Mac release checklist and evidence |
| M5 Windows and device expansion | Qualified Windows build, general MIDI mappings, another controller | Repeat critical workflow and hardware tests on Windows and second device |
| M6 Web delivery decision and implementation | Browser editor and/or hybrid frontend according to M0 results | Shared project fixtures pass; supported capability matrix and long-export behavior |
| M7 Advanced performance and Linux | Four decks, deeper S4 features, Linux qualification, later integrations | Per-feature/per-device release evidence; no implied blanket compatibility |

M3/M4 order can be adjusted after M2. Windows build smoke tests can start early without promising Windows device support. Full external MIDI clock follows a working tempo map. M7 is a collection of later epics, not a single release that must wait for every possible feature.

## Publishing convention

When the GitHub repository is selected, publish one issue per draft below, using the title, scope, dependencies, and acceptance text. Suggested labels: `type:spike`, `type:feature`, `type:quality`, and areas `audio`, `controller`, `automation`, `editor`, `storage`, `platform`, `research`. Create milestones matching the table, then replace local dependencies with actual issue links. Link the design documents from issue bodies. Do not create duplicate issues if publication is retried.

Keep M0–M2 actionable at task level. Later drafts are epics to split after the POC reveals the real implementation seams. “Done” requires evidence, document updates, and explicit limitations; screenshots alone cannot close an audio/controller issue.

## M0 decision issues

### DJ-001 Choose project license and pin initial dependencies

Type: spike. Dependencies: none. Scope: compare intended JUCE, HIDAPI, DSP and optional Mixxx reuse licenses; propose exact project license and build-version inventory.

Acceptance: license choice recorded by maintainer; intended dependency versions and relevant file licenses documented; notices/distribution requirements identified; no blanket assumption that open source means interchangeable licenses. Accept/update ADR 0001 after other M0 results. [Research](research/audio-engine-and-stack.md).

### DJ-002 Qualify S4 MK3 interfaces on the target Mac

Type: spike. Dependencies: none for inventory; DJ-001 before substantial implementation reuse. Scope: exact firmware/descriptors, HID reports, Core Audio channel/rate enumeration, physical routing, LED proof.

Acceptance: report with actual configuration, input fixtures and channel map; required controls and monitoring-only controls listed; master/cue tested separately; no screen/motor writes in this spike. Failed access produces an identified next experiment. [Controller plan](research/traktor-and-controller-integration.md).

### DJ-003 Evaluate Mixxx fork versus new core

Type: spike. Dependencies: DJ-001. Scope: build a pinned Mixxx version; trace one S4 input to engine state; attempt semantic capture and a headless scripted render seam.

Acceptance: written reuse/fork/new-core decision with concrete modules, coupling, license obligations, and a minimal code-path sketch. Set a bounded effort before starting; stop with findings rather than attempting an unbounded refactor. [Mixxx investigation](research/audio-engine-and-stack.md).

### DJ-004 Test browser S4 access and master/cue routing

Type: spike. Dependencies: DJ-002 supplies native baseline. Scope: WEB-01–WEB-03 on current macOS Chromium, plus Safari capability inventory; small AudioWorklet graph.

Acceptance: actual HID input/LED results and four-channel physical routing report; permission failure and unplug behavior recorded; clear decision between browser live support, native bridge, and arrangement-only browser scope. API availability is insufficient to pass. [Web experiments](research/web-platform.md).

### DJ-005 Compare minimal native and web timeline interactions

Type: spike. Dependencies: none for a static/interaction prototype; DJ-001 before implementation dependencies. Scope: two waveform clips, zoom/scroll/trim, one automation lane, keyboard/numeric editing, drag/drop.

Acceptance: run the product workflow tasks with project owner; record mode confusion and editor implementation/bridge cost; select initial UI host or deliberately keep thin native harness until M2. Do not duplicate production audio engines. [UX plan](product-and-ux.md).

## M1 native harness issues

### DJ-006 Create native/headless build and audio host

Type: feature. Dependencies: DJ-001, DJ-003, accepted initial stack decision. Scope: reproducible build, headless engine target, minimal native host, audio-device selection and metrics.

Acceptance: clean Mac build; silence/test signal output at negotiated settings; offline target has no UI/device dependency; callback metrics collected off-thread; pinned dependencies recorded. Windows compile-only CI can be added if available, without hardware certification.

### DJ-007 Implement bounded two-source PCM playback and mixing

Type: feature. Dependencies: DJ-006. Scope: WAV decoding worker/cache, source cursors, gain/crossfader, documented mix law, master/cue buses and underrun behavior.

Acceptance: two sources play with independent cue routing; bounded buffers; source-rate handling defined; starvation is visible and callback never waits for a file; baseline fixtures show expected gains and crossfade curve. A03/A06.

### DJ-008 Implement engine-clock event scheduling and telemetry

Type: feature. Dependencies: DJ-006. Scope: monotonic engine frame counter, project transport mapping, input clock conversion, bounded queues, ordering, lateness and overflow metrics.

Acceptance: sample-offset/block-boundary fixtures pass; same-frame ordering documented; late events counted; transport/release events protected from expendable feedback pressure; plan reclamation and capture queue failure are bounded. A06/A08.

### DJ-009 Map essential S4 controls and feedback

Type: feature. Dependencies: DJ-002, DJ-007, DJ-008. Scope: HID reader/parser, two deck roles, play/cue, channel faders/crossfader, pickup, LED feedback, reconnect state.

Acceptance: report fixtures normalize correctly; controller capture bypasses UI event loop; feedback follows engine state; reconnect does not jump gain or restart playback; exact supported/unsupported controls documented. A01/A02.

### DJ-010 Prove independent MIDI output

Type: feature. Dependencies: DJ-008. Scope: separate MIDI endpoint enumeration and timestamped note/CC scheduler, virtual-loopback test, offline suppression.

Acceptance: note/CC sequence ordered at the virtual endpoint with timing distribution recorded; cancel pending events on stop/disconnect; offline rendering emits no physical MIDI; explicitly distinguish S4 HID feedback from MIDI output. A14.

## M2 flagship POC issues

### DJ-011 Implement versioned project model and initial state

Type: feature. Dependencies: DJ-008. Scope: stable asset/clip/bus/parameter IDs, absolute-time POC representation, semantic actions and curves, initial state, validation and schema version.

Acceptance: project round-trip preserves IDs/times/values; invalid/non-finite/dangling references rejected; runtime device handles absent from saved data; time conversion fixtures cover sample-rate changes. [Set format](set-format-and-automation.md).

### DJ-012 Capture applied actions and preserve original takes

Type: feature. Dependencies: DJ-008, DJ-009, DJ-011. Scope: capture accepted frame-stamped starts/stops, source identity, gains/crossfader; completeness detection and incremental take persistence.

Acceptance: scripted and physical gestures replay without controller; initial state retained; original points preserved; injected queue overflow marks incomplete capture visibly; monitor-only controls stay outside master automation. A08/A09/A12.

### DJ-013 Render projects offline and compare with playback

Type: feature. Dependencies: DJ-007, DJ-008, DJ-011. Scope: same render core in independent offline instance, floating-point reference WAV, delivery WAV path, range and tail policy.

Acceptance: baseline PCM/event comparisons meet A09/A10; export needs no controller/audio device; cue excluded; bounded output buffering, cancellation and disk-error reporting; save/export do not mutate the active set.

### DJ-014 Build the two-lane arrangement editor

Type: feature. Dependencies: DJ-005, DJ-011, DJ-013. Scope: chosen-folder browser, file drag/drop, background waveform display, placement/trim, playhead, snap and exact positions, undo.

Acceptance: two tracks imported and arranged through the UI; source audio unchanged; long UI tasks cannot block engine; selected clip/transport state clear; the user can complete the first two workflow tasks. A13.

### DJ-015 Edit recorded automation and replace a range

Type: feature. Dependencies: DJ-012, DJ-014. Scope: point/numeric editing, Read/Override/Record display, Resume automation, selected-parameter punch-in/out, take keep/discard, undo.

Acceptance: one recorded transition is edited and partly reperformed; unarmed lanes and material outside selection stay unchanged; punch boundaries obey documented return rule; one undo restores previous take; mismatched physical fader uses pickup. A09/A13.

### DJ-016 Save, reopen, relink, and recover the POC set

Type: feature. Dependencies: DJ-011, DJ-012. Scope: manifest + sidecars, content identities, atomic save, previous-version backup, recovery journal, missing-media UI.

Acceptance: A11/A12; reopen without S4; relocated matching assets relink; wrong-content asset detected; interrupted save retains previous good project; unknown schema does not get silently rewritten.

### DJ-017 Qualify and document the complete Mac POC

Type: quality. Dependencies: DJ-009–DJ-016. Scope: complete workflow, sustained load, failure injection, callback/control latency, 60-minute export.

Acceptance: every A01–A15 result recorded with configuration/method; baseline tolerance and supported buffer stated; no claim of hardware latency from buffer size alone; failed gates remain open; video/screenshots supplement audio/event evidence. Update ADR and next milestone priorities.

## M3 musical mixing epics

### DJ-018 Add beat analysis and editable grids

Type: feature epic. Dependencies: DJ-017. Scope: BPM/beat candidates, manual first beat/BPM correction, multiple source anchors, confidence/version provenance, per-project grid snapshots.

Acceptance: fixed and variable-tempo fixtures; phase/downbeat and BPM distinguished; user correction survives reanalysis/reopen; detector failures visibly editable; sync drift quantified over long tracks.

### DJ-019 Select and integrate pitch-preserving time stretch

Type: spike then feature. Dependencies: DJ-017. Scope: compare Rubber Band/Signalsmith, adapter with latency/startup/tail/reset/rate-change behavior, representative listening corpus.

Acceptance: measured quality/latency/CPU decision; both decks meet selected callback budget; key-lock is distinct from varispeed; seeks/preroll and offline parity characterized; licensing/version recorded.

### DJ-020 Add project tempo maps and musical anchors

Type: feature epic. Dependencies: DJ-018, DJ-019. Scope: step changes then defined tempo ramps, beat/time transforms, beat-anchored clips/automation, sync/nudge/resume behavior.

Acceptance: conversions round-trip with bounded error; edits move beat-anchored objects as documented; absolute objects remain fixed; tempo changes survive capture/replay/export; no click or unexpected phase jump at segment boundaries.

### DJ-021 Capture EQ, filters, loops, cues, and jog trajectories

Type: feature epic. Dependencies: DJ-017, plus DJ-019/DJ-020 for time-dependent actions. Scope: three-band EQ/filter first, then discrete cues/loops, relative jog/varispeed/scratch representations; touch/latch only with defined semantics.

Acceptance: each new parameter/action has versioned units/interpolation, capture and seek reconstruction fixtures; source trajectory survives loops/reverse; latency and audible continuity measured; unsupported modes cannot masquerade as reproducible capture.

### DJ-022 Schedule external MIDI clock and transport

Type: feature. Dependencies: DJ-010, DJ-020. Scope: clock at 24 pulses per quarter note, Start/Continue/Stop, transport changes, queue cancellation.

Acceptance: jitter/drift distributions at fixed tempo and ramps; seek/loop/restart policy documented; virtual and physical tests distinguished; offline export sends no hardware clock; external-instrument audio requires capture for export.

## M4 reliable Mac alpha epics

### DJ-023 Add long-library caching and analysis jobs

Type: feature epic. Dependencies: DJ-017, DJ-018. Scope: indexing, search, preview, peak caches, bounded read-ahead, cancelable analysis, generation-safe seeking.

Acceptance: realistic multi-hour library/session has bounded memory and responsive UI; analysis cannot interrupt playback; source moves and stale worker results handled; cache deletion preserves edits.

### DJ-024 Add MP3 and FLAC import/export presets

Type: feature. Dependencies: DJ-013, DJ-017, dependency review. Scope: decoder/encoder selection, seek/priming behavior, tags, bit depth/rate, normalization and dither choices.

Acceptance: sample-rate/duration/channel/tag fixtures, clipping policy, long output and cancel/disk-full tests; lossy export compared after alignment using explicit tolerance; encoder license/build settings documented. WAV remains the reference render.

### DJ-025 Package portable sets and robust recovery

Type: feature epic. Dependencies: DJ-016, DJ-023. Scope: collect-media bundle, safe extraction, manifest migration, take recovery, compatibility/freeze path design.

Acceptance: transfer to another machine and relink/collected-media reopen; content hashes verified; malformed paths/oversized data rejected; backup preserved across migration; explicit incomplete take recovery.

### DJ-026 Ship and validate a Mac alpha

Type: quality epic. Dependencies: M3 and selected M4 feature issues. Scope: packaging/signing plan, permissions/device lifecycle, accessibility, release notes, hardware matrix and user workflow validation.

Acceptance: install/open/save/export/update path tested on declared supported machines; no broad support claims without evidence; S4 firmware/settings and limitations published; source/build/license notices available for the chosen distribution.

## M5 platform and device epics

### DJ-027 Qualify Windows audio and S4 integration

Type: platform epic. Dependencies: DJ-017 and accepted core interfaces. Scope: Windows builds, WASAPI and current vendor ASIO, HID, paths/permissions/installer, project parity.

Acceptance: critical M2 workflow rerun on documented Windows/device/driver versions; master/cue, latency, device loss and stress measured; current driver-block reports rechecked; no reliance on weakening OS protections.

### DJ-028 Add generic MIDI mapping and a second controller

Type: controller epic. Dependencies: DJ-009, DJ-010, DJ-017. Scope: mapping schema, learn, absolute/relative controls, modifiers, pickup, output feedback; select hardware actually available for testing.

Acceptance: second model has fixtures and physical qualification; same semantic recording replays independently of mapping/device; MIDI addresses never replace stable project parameter IDs; feature matrix clearly scoped by model/generation.

## M6 web delivery epics

### DJ-029 Implement the selected web or hybrid host

Type: platform epic. Dependencies: DJ-004, DJ-005, DJ-017, explicit product-scope decision. Scope: one chosen route—native engine + web UI, or browser arrangement/AudioWorklet/Wasm host—with versioned bridge/project semantics.

Acceptance: browser/native project fixtures agree within declared scope; capability detection and permission failure usable; UI stalls do not control audio time; native bridge isolates device access; no duplicated divergent automation rules.

### DJ-030 Qualify web long sets and conditional hardware support

Type: quality epic. Dependencies: DJ-029. Scope: WEB-04–WEB-07 where applicable, bounded decode/export, file/storage recovery, tab lifecycle, browser matrix.

Acceptance: 60-minute render with measured bounded memory; no chunk-boundary audio discontinuity; save/reopen/relink works with permissions changing; S4 live support advertised only on qualified configurations; Safari scope accurately described.

## M7 advanced epics

### DJ-031 Expand to four decks and deeper S4 features

Type: feature epic. Dependencies: stable M3/M4 baseline. Scope: four-deck control layers and routing first; controller screen rendering and motor/haptic protocols as separately gated subprojects.

Acceptance: four active decks pass audio/capture stress; deck/lane identity changes do not rewrite old takes; each screen/motor feature has protocol, bounds, lifecycle and physical qualification evidence before enabling by default.

### DJ-032 Qualify Linux and track advanced integrations

Type: platform/research epic. Dependencies: stable core and a Linux test environment. Scope: audio backend/device permissions, S4 and generic MIDI, packaging; separate later proposals for plugins, stems, external audio, DVS, streaming, or collaboration.

Acceptance: Linux build and core workflow report on declared backend/hardware; each advanced integration gets its own design and acceptance criteria before implementation. These optional proposals do not block the Mac/Windows prepared-set product.

## First implementation slice after planning

Start DJ-001–DJ-005 as bounded experiments, then deliver DJ-006–DJ-009 and a minimal DJ-011–DJ-013 scripted capture/render path before the full editor. Keep DJ-017's evidence requirements visible throughout. This sequence tests the hardware and the defining feature while the UI and product scope are still inexpensive to change.
