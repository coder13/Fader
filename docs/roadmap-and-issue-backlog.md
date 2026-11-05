# Roadmap and published GitHub issues

Initially published 2026-11-04 with 43 issues, then expanded at the user's request to **64 issues and 10 milestones** in [coder13/Fader](https://github.com/coder13/Fader/issues). Fourteen new product issues join existing milestones; seven optional decision issues remain unscheduled. GitHub holds current status. Planning IDs `DJ-001`–`DJ-064` are stable: the original 43 are indexed below and [DJ-044–DJ-064 have a complete supplementary index](product-completion-backlog.md). Milestones use exit criteria rather than speculative dates.

The [completeness review](research/roadmap-completeness-review.md) now maps to [21 published follow-up issues](product-completion-backlog.md), covering editing/DJ/library contracts, native v1 release acceptance and usability/visual validation. [DJ-044](https://github.com/coder13/Fader/issues/44) records the selected release subset; issue publication does not make every optional feature a v1 requirement. [DJ-057](https://github.com/coder13/Fader/issues/57) gates the complete selected workflow on Mac and Windows.

## Current evidence and immediate work

The [v0.2 native prototype](https://github.com/coder13/Fader/blob/3a3b58456065f6cfed4c9f23f1a0252047d80a89/README.md) already has clip-owned volume/high-pass/low-pass envelopes, fixed-duration clip movement, waveform editing, capture/retakes, versioned save/reopen, MP3/WAV import, WAV export, a metadata browser, Preferences, read-only Traktor metadata and a flat dark theme. [Measured evidence](https://github.com/coder13/Fader/blob/3a3b58456065f6cfed4c9f23f1a0252047d80a89/docs/experiments/2026-11-04-clip-arrangement.md) and [theme verification](https://github.com/coder13/Fader/blob/3a3b58456065f6cfed4c9f23f1a0252047d80a89/docs/theme-and-interaction.md) describe the tested slice. This is **partial POC evidence**, not a qualified production stack. The prototype source is now on main; the linked reports identify their tested builds. Source integration does not close unmeasured hardware or long-run gates.

The user authorized continuing without the controller. C++20/JUCE remains the provisional implementation route. Bounded reuse/web experiments inform the production decision; they do not require discarding the working prototype or restarting research before every software improvement. The exact product license remains deferred; the current experimental source has its declared license.

Next, in order:

1. [Stream long media through bounded caches](https://github.com/coder13/Fader/issues/38) and complete the existing mixer/real-time audit.
2. [Make seeks cancellable and state-correct](https://github.com/coder13/Fader/issues/39).
3. [Measure sustained playback/capture under load](https://github.com/coder13/Fader/issues/40) and [bounded long export](https://github.com/coder13/Fader/issues/13).
4. In parallel where independent: [early Windows build smoke](https://github.com/coder13/Fader/issues/41), [owner workflow session](https://github.com/coder13/Fader/issues/42) and [recovery/relinking](https://github.com/coder13/Fader/issues/16).
5. When hardware is available: [S4 interface/routing qualification](https://github.com/coder13/Fader/issues/2), then [essential control and feedback mapping](https://github.com/coder13/Fader/issues/9).

## Gate from POC to product

[DJ-017 / issue #17](https://github.com/coder13/Fader/issues/17) is the explicit go/no-go decision. It requires the complete [A01–A16 matrix](poc-plan.md), owner workflow evidence, a recorded architecture decision, and named remaining limitations. Passing a UI test or short audio smoke does not pass this gate. Record failures and any 256-frame fallback rather than quietly changing the 128-frame targets.

The POC must demonstrate arrange → perform → edit → retake → save/reopen without controller → WAV export, using independent clip volume/HP/LP. It must also establish bounded streaming/seeking, representative MP3/WAV correctness, capture/render parity, 60-minute playback/export, recoverable failures, physical master/cue and S4 controls. Browser delivery, advanced S4 screens/motors, Linux and VST3 are not native POC blockers. An early Windows build catches portability problems; full Windows/device qualification follows in M5.

M3–M5 turn the validated core into a usable Mac/Windows product. Later issues are scoped epics to split when selected, not promises of a release date. M6 web delivery and M7 Linux/advanced performance can proceed independently. VST3 **product version 2/3** is M8/M9; these version names do not refer to the POC's M2/M3 or prototype v0.2.

## Milestones

| Milestone | Published issues | Exit criterion |
| --- | ---: | --- |
| [M0 · POC: architecture and feasibility](https://github.com/coder13/Fader/milestone/1) | 6 | Validate the provisional C++20/JUCE direction with bounded reuse, native/web UI and Windows build experiments. Record evidence and tradeoffs; no framework rewrite or license decision is assumed. Browser research can proceed separately from the native POC. Exit: explicit architecture recommendation with remaining risks and dependencies. |
| [M1 · POC: streaming audio and S4 harness](https://github.com/coder13/Fader/milestone/2) | 8 | Prove bounded streaming, cancellable correct seeking, event scheduling, S4 MK3 HID controls/LEDs, Core Audio master/cue and independent MIDI output. Software tasks proceed without hardware. Exit: repeatable subsystem evidence; hardware support requires the physical device. |
| [M2 · POC: editable sets and qualification](https://github.com/coder13/Fader/milestone/3) | 10 | Complete arrange → perform → edit → retake → save/reopen → export with independent clip volume/HP/LP. Add capture/recovery gaps and qualify representative MP3/WAV, 60-minute playback/export and real S4 behavior. Exit: A01–A16 results plus architecture go/no-go and owner workflow evidence. This is the gate into product development. |
| [M3 · Product alpha: musical mixing](https://github.com/coder13/Fader/milestone/4) | 11 | Editable detected/imported beat grids, pitch-preserving stretch, tempo maps, EQ and reproducible cues/loops/jog actions, then MIDI clock. Exit: defined musical timing, capture/seek/export semantics and measured sound quality. Begins after the POC decision. |
| [M4 · Product alpha: reliable Mac workflow](https://github.com/coder13/Fader/milestone/5) | 13 | Persistent library/indexing and preview, broad audio import with whole-library qualification, complete Traktor import, portable projects, export presets, recovery and a packaged accessible Mac alpha. Exit: finish and reopen a realistic set with explicit supported formats and no unexplained library omissions. |
| [M5 · Product v1: Windows and more controllers](https://github.com/coder13/Fader/milestone/6) | 3 | Qualify Windows audio/device/build/install paths and generic MIDI mappings on another controller. Exit: critical prepared-set workflow, audio/cue/latency and failure cases pass on named Windows/device configurations. Early Windows compile smoke is in M0. |
| [M6 · Product: web or hybrid delivery](https://github.com/coder13/Fader/milestone/7) | 2 | Choose browser preparation companion, browser audio host or native-engine/web-UI host from the feasibility evidence; implement only the selected scope. Exit: shared project semantics, long render/storage recovery and an honest browser/hardware capability matrix. Does not block native v1. |
| [M7 · Later: advanced performance and Linux](https://github.com/coder13/Fader/milestone/8) | 2 | Separately qualify four-deck control layers, deeper S4 features and Linux. Screens and motors are independent opt-in gates. Exit per feature/platform; optional Linux and advanced hardware do not block the Mac/Windows prepared-set product. |
| [M8 · Product v2: VST3 effects](https://github.com/coder13/Fader/milestone/9) | 1 | Host VST3 effects on clips/lanes/master with parameter automation, saved state, isolated scanning, latency/tail handling and processed export. Exit: named Mac/Windows plugin workflows pass including missing-plugin recovery. Excluded from the POC; does not depend on Linux or web delivery. |
| [M9 · Product v3: plugin resilience and portability](https://github.com/coder13/Fader/milestone/10) | 1 | Harden the qualified VST3 host with freeze/unfreeze, render fallbacks and a measured runtime-isolation decision; optional routing/formats need separate designs. Exit: preserved editable state, usable missing-plugin fallback and documented failure/latency behavior. |

Seven optional decision issues ([DJ-058–DJ-064](product-completion-backlog.md#optional-future-decisions--no-release-milestone)) have no milestone. The milestone counts above include the new product work; M3 adds scope/sync/cue/harmonic/controller/metering contracts, M4 adds editing/library/delivery/design qualification, and M5 adds the integrated native v1 gate.

## Tracking rules

Issues carry phase, type and area labels. `priority:next` identifies streaming, seeking and sustained-load work; `needs:hardware` identifies the first physical S4 tasks. All issues are initially open because their full acceptance evidence is outstanding, even when some implementation already exists. Each issue says what is already built and what remains.

Dependencies are linked issue checklists. A dependency means its relevant contract/evidence is needed to finish the downstream work; it does not prohibit independent implementation or test preparation. Web scope is conditional on measured capability. No due dates, owners or universal device/format compatibility are invented.

Close an issue only after recording the exact build/configuration, repeatable steps, expected/actual results and limitations. Keep user media, track lists, absolute private paths and device serials out of published evidence. Update design documents and the next milestone when an experiment changes the architecture. Review existing issue markers before retrying publication so retries do not create duplicates.

## Published task index

## M0 · POC: architecture and feasibility

<a id="dj-001-choose-project-license-and-pin-initial-dependencies"></a>

### DJ-001 Inventory pinned dependencies and confirm distribution licensing

GitHub: [#1](https://github.com/coder13/Fader/issues/1). Labels: `phase:poc`, `type:spike`, `area:research`.

**Current evidence:** Open source is confirmed. The experimental source uses AGPL-3.0-or-later and pinned JUCE 9.0.5; the user deferred a final licensing preference.

**Scope:** Record the actual dependency/build inventory and obligations before adding copied implementation code or shipping binaries. Keep routine POC development moving; do not treat this as a request to choose a different license now.

**Acceptance:**

- [ ] List exact versions, source locations and applicable licenses for the core, host, HID, codecs and future Windows driver/SDK dependencies.
- [ ] Record a maintainer distribution decision before the alpha release; document notices and unresolved constraints without assuming all open-source licenses are interchangeable.

**Dependencies:** No blocking issue dependencies; use the current prototype and recorded evidence.

<a id="dj-003-evaluate-mixxx-capture-and-render-reuse"></a>

### DJ-003 Bound the Mixxx and existing-engine reuse investigation

GitHub: [#3](https://github.com/coder13/Fader/issues/3). Labels: `phase:poc`, `type:spike`, `area:research`.

**Current evidence:** Published research exists; source-build/capture/render trials have not been completed. The C++/JUCE prototype remains provisional.

**Scope:** Time-box a pinned Mixxx build and one input-to-engine trace, then investigate semantic capture/headless rendering. Evaluate Tracktion Engine only if the application reuse investigation leaves a concrete sequencing gap.

**Acceptance:**

- [ ] Set an effort cap before starting; record build result, exact revisions and concrete insertion seams.
- [ ] Report coupling, state/render ownership, maintenance cost and licensing; end with findings rather than an unbounded fork.
- [ ] Feed [DJ-037](https://github.com/coder13/Fader/issues/37) with a smallest-change comparison against the existing working prototype.

**Dependencies:** [DJ-001](https://github.com/coder13/Fader/issues/1)

<a id="dj-004-test-browser-s4-access-and-mastercue-routing"></a>

### DJ-004 Measure browser S4 access and physical master/cue routing

GitHub: [#4](https://github.com/coder13/Fader/issues/4). Labels: `phase:poc`, `type:spike`, `area:platform`.

**Current evidence:** Web platform research is documented; browser hardware and routing are unmeasured. This investigation does not block the native POC exit.

**Scope:** Run WEB-01–WEB-03 on macOS Chromium against the native S4 baseline and inventory Safari capabilities using a minimal AudioWorklet harness.

**Acceptance:**

- [ ] Record actual input/LED behavior, permission refusal, unplug/reconnect and four physical output channels; API presence alone is not a pass.
- [ ] Choose a provisional live-browser, native-bridge or preparation-only scope with explicit unsupported paths.
- [ ] Defer live browser reliability claims until WEB-04–WEB-07 and [DJ-030](https://github.com/coder13/Fader/issues/30) pass.

**Dependencies:** [DJ-002](https://github.com/coder13/Fader/issues/2)

<a id="dj-005-compare-minimal-native-and-web-timeline-interactions"></a>

### DJ-005 Compare the native timeline with a bounded web UI prototype

GitHub: [#5](https://github.com/coder13/Fader/issues/5). Labels: `phase:poc`, `type:spike`, `area:editor`.

**Current evidence:** The native v0.2 editor demonstrates clip-owned envelopes and a shared flat dark theme. No comparative usability or web bridge cost has been measured.

**Scope:** Reuse the same two-clip scenario to compare native editing with a small web/hybrid interaction prototype; do not build a second production audio engine.

**Acceptance:**

- [ ] Try fixed-size horizontal/vertical movement, zoom/scroll, envelope selection/editing, keyboard access and file drop with the owner.
- [ ] Record mode errors, responsiveness, accessibility gaps and bridge/maintenance cost.
- [ ] Document whether native remains the initial host; any host change needs evidence and an updated ADR.

**Dependencies:** No blocking issue dependencies; use the current prototype and recorded evidence.

<a id="dj-037-evaluate-existing-applications-and-focused-extensions"></a>

### DJ-037 Record the architecture decision using POC and reuse evidence

GitHub: [#37](https://github.com/coder13/Fader/issues/37). Labels: `phase:poc`, `type:spike`, `area:research`.

**Current evidence:** The user chose to build a C++20/JUCE POC; its UI and core fixtures work. This is evidence for continuing the experiment, not a permanent stack decision.

**Scope:** Compare the working prototype with bounded existing-application/engine trials using the same arrange, record, edit, retake, reopen and export task. Update ADR 0001 with retain/extend/reuse recommendations and concrete tradeoffs.

**Acceptance:**

- [ ] Record exact versions, task outcomes and smallest blocking gaps for selected candidates; distinguish published research from hands-on evidence.
- [ ] Use the existing native prototype as a baseline; evaluate Rust/Qt/web boundaries only against specific integration or usability problems, not language preference alone.
- [ ] Document why to keep or change the stack, cost of ownership and unresolved hardware/streaming risks; do not restart implementation automatically.
- [ ] Final production go/no-go is [DJ-017](https://github.com/coder13/Fader/issues/17); commercial personal adoption does not replace the open-source product requirement.

**Dependencies:** [DJ-003](https://github.com/coder13/Fader/issues/3), [DJ-005](https://github.com/coder13/Fader/issues/5)

### DJ-041 Run an early Windows build and project-parity smoke test

GitHub: [#41](https://github.com/coder13/Fader/issues/41). Labels: `phase:poc`, `type:quality`, `area:platform`.

**Current evidence:** The app is built/tested on Apple Silicon macOS only. Cross-platform toolkit choice is not Windows qualification.

**Scope:** Add a Windows compile/test job for the core, native host and representative media fixtures before architecture assumptions become costly. This is an early portability check, not S4 certification.

**Acceptance:**

- [ ] A clean supported Windows toolchain configures/builds the app and runs headless/project tests in CI or a documented Windows environment.
- [ ] Check path/Unicode behavior, decoder availability and project fixtures; record OS/toolchain/dependency versions and any blockers.
- [ ] Keep actual audio, ASIO/WASAPI, installer and S4 tests in [DJ-027](https://github.com/coder13/Fader/issues/27); failures feed the POC architecture decision.

**Dependencies:** [DJ-006](https://github.com/coder13/Fader/issues/6)

## M1 · POC: streaming audio and S4 harness

<a id="dj-002-qualify-s4-mk3-interfaces-on-the-target-mac"></a>

### DJ-002 Qualify the actual S4 MK3 HID and Core Audio interfaces

GitHub: [#2](https://github.com/coder13/Fader/issues/2). Labels: `phase:poc`, `type:spike`, `area:controller`, `needs:hardware`.

**Current evidence:** The S4 MK3 is identified, but hardware is currently unavailable. No physical control or audio-routing qualification has passed.

**Scope:** Collect firmware/descriptors and safe read-only report fixtures; enumerate actual rates/channels and prove one LED plus four physical outputs. No firmware update, screen or motor work is required.

**Acceptance:**

- [ ] Record the exact Mac, OS, controller firmware, USB path and device settings; bind specifically to the intended interface (A01).
- [ ] Verify master 1/2 and cue 3/4 individually at low gain; distinguish software buses from hardware monitoring (A03).
- [ ] Publish sanitized report fixtures and a supported/unsupported control map; record failure and next experiment if access fails.

**Dependencies:** No blocking issue dependencies; use the current prototype and recorded evidence.

<a id="dj-006-create-nativeheadless-build-and-audio-host"></a>

### DJ-006 Make the native and headless build reproducible

GitHub: [#6](https://github.com/coder13/Fader/issues/6). Labels: `phase:poc`, `type:feature`, `area:platform`.

**Completed:** Clean-checkout Linux core and Mac native CI passed at `fb2428fc2fad53a1faa614d958c3ac998744691c`, including retained metadata/JUnit/CTest evidence and simulated device setup failures. See [qualification report](experiments/2026-11-04-reproducible-builds.md). This does not qualify hardware or sustained playback.

**Scope:** Provide clean checkout builds, pinned dependencies, CI headless fixtures and off-thread diagnostics. Preserve the UI-independent core and documented supported build environment.

**Acceptance:**

- [x] Build the app and headless tests from a clean checkout with documented tools and no personal machine paths.
- [x] Run existing tests in CI; record compiler, dependency revisions and architecture.
- [x] Verify audio initialization/error reporting and collect callback metrics off-thread; do not equate successful compilation with hardware support.

**Dependencies:** No blocking issue dependencies; use the current prototype and recorded evidence.

<a id="dj-007-implement-bounded-two-source-pcm-playback-and-mixing"></a>

### DJ-007 Qualify two-source mixing, gain and master/cue semantics

GitHub: [#7](https://github.com/coder13/Fader/issues/7). Labels: `phase:poc`, `type:feature`, `area:audio`.

**Completed:** Synthetic source-rate/gain/master/cue and export fixtures passed locally and in Linux/Mac CI at `b18e22aa807ae5642f1920dffd20da1fb4be002d`. See [mixer contract](mixer-contract.md) and [qualification evidence](experiments/2026-11-04-mixer-qualification.md). Full decoding, production resampling quality and physical routing remain separate gates.

**Scope:** Audit the existing mixer/resampler and define source-rate, gain, filter, clipping and monitoring behavior. A global A/B crossfader is not the primary editor model; preserve legacy v1 playback compatibility.

**Acceptance:**

- [x] Synthetic mono/stereo and source-rate fixtures verify gain, duration and declared resampling behavior.
- [x] Master and cue buses are independently testable; cue is absent from export (A03/A06).
- [x] All callback work is bounded and nonblocking; streaming memory limits are addressed by [DJ-038](https://github.com/coder13/Fader/issues/38).

**Dependencies:** [DJ-006](https://github.com/coder13/Fader/issues/6)

**Product coverage follow-up:** These product follow-ups preserve this issue's existing POC scope. They do not become new blockers for the POC merely by being linked here. [DJ-052](https://github.com/coder13/Fader/issues/52).

<a id="dj-008-implement-engine-clock-event-scheduling-and-telemetry"></a>

### DJ-008 Validate engine-clock scheduling, queue behavior and telemetry

GitHub: [#8](https://github.com/coder13/Fader/issues/8). Labels: `phase:poc`, `type:feature`, `area:audio`.

**Current evidence:** The core already has frame-based processing and bounded commands; physical input timing and failure behavior need broader qualification.

**Scope:** Audit clock domains, input timestamp conversion, same-frame ordering, late commands, queue capacity and publication/reclamation. Controller input must bypass UI timing.

**Acceptance:**

- [ ] Exercise events at block boundaries and across nonzero seeks/restarts; distinguish engine frames from project positions (A08).
- [ ] Inject queue pressure, overflow and device clock changes; report capture incompleteness visibly and preserve valid data.
- [ ] Audit/instrument project-owned callbacks for allocation/free, locks, file access and unbounded work; record telemetry off-thread (A06).

**Dependencies:** [DJ-006](https://github.com/coder13/Fader/issues/6)

<a id="dj-009-map-essential-s4-controls-and-feedback"></a>

### DJ-009 Map essential S4 controls to clip automation and feedback

GitHub: [#9](https://github.com/coder13/Fader/issues/9). Labels: `phase:poc`, `type:feature`, `area:controller`, `needs:hardware`.

**Current evidence:** No S4 input/feedback implementation is qualified. Independent clip volume and HP/LP replace the old global-crossfader editor design.

**Scope:** Implement the HID adapter, explicit two-deck-to-clip assignment, play/cue, independent channel volume and supported filter gestures, pickup, LED state and reconnect handling.

**Acceptance:**

- [ ] Normalize saved input fixtures and route accepted semantic events directly to the engine with stable target identity.
- [ ] Physical faders use pickup; LED/play/cue state follows the engine; reconnect never jumps gain or starts playback (A01/A02/A12).
- [ ] Document exactly supported controls; screens, motors and four-deck layers remain later work.
- [ ] If the physical crossfader is mapped, define how it emits clip-volume events; it must not replace independent saved envelopes.

**Dependencies:** [DJ-002](https://github.com/coder13/Fader/issues/2), [DJ-007](https://github.com/coder13/Fader/issues/7), [DJ-008](https://github.com/coder13/Fader/issues/8)

**Product coverage follow-up:** These product follow-ups preserve this issue's existing POC scope. They do not become new blockers for the POC merely by being linked here. [DJ-050](https://github.com/coder13/Fader/issues/50).

### DJ-010 Prove independent MIDI output

GitHub: [#10](https://github.com/coder13/Fader/issues/10). Labels: `phase:poc`, `type:feature`, `area:controller`.

**Current evidence:** No independent MIDI output proof has been implemented; S4 HID feedback is a different transport.

**Scope:** separate MIDI endpoint enumeration and timestamped note/CC scheduler, virtual-loopback test, offline suppression.

**Acceptance:**

- [ ] note/CC sequence ordered at the virtual endpoint with timing distribution recorded; cancel pending events on stop/disconnect; offline rendering emits no physical MIDI; explicitly distinguish S4 HID feedback from MIDI output. A14.

**Dependencies:** [DJ-008](https://github.com/coder13/Fader/issues/8)

### DJ-038 Stream long audio through bounded decode and PCM caches

GitHub: [#38](https://github.com/coder13/Fader/issues/38). Labels: `phase:poc`, `type:feature`, `area:audio`, `priority:next`.

**Original evidence:** The prototype fully decoded sources with 256 MiB/source and 1 GiB/set caps. The [bounded-streaming implementation and measured results](experiments/2026-11-04-bounded-streaming.md) replace those limits. Software/CI and native visual checks passed; fast seek and sustained-load work remain separate issues.

**Scope:** Add worker-side decoding/prefetch and a bounded source-window cache; maintain a nonblocking render path and deterministic source-frame semantics. Keep offline readiness distinct from live starvation.

**Acceptance:**

- [ ] Play two long overlapping sources in a multi-hour arrangement with a declared memory budget; report peak resident memory/cache size rather than loading complete tracks.
- [ ] Inject slow/unavailable reads: the audio callback never waits; live starvation is visible and uses a defined fade, while offline rendering waits or fails explicitly.
- [ ] Cancellation and cache generation ownership prevent stale reads/use-after-free after source changes, seek or close.
- [ ] Preserve representative MP3/WAV sequential/seek alignment, waveform duration and existing automation/render fixtures.

**Dependencies:** [DJ-007](https://github.com/coder13/Fader/issues/7)

### DJ-039 Make long seeks fast, cancellable and state-correct

GitHub: [#39](https://github.com/coder13/Fader/issues/39). Labels: `phase:poc`, `type:feature`, `area:audio`, `priority:next`.

**Current evidence:** [Latest-request preparation, bounded filter preroll and session MP3 caches are implemented and locally measured](experiments/2026-11-04-cancellable-seeking.md). Cached fixtures meet the declared timing targets; uncached long MP3 misses them. Exact-source Linux/macOS CI passes; native UI inspection remains pending.

**Scope:** Choose bounded preroll or cached DSP checkpoints and generation-aware decode scheduling. Publish a prepared state only for the latest requested seek.

**Acceptance:**

- [ ] Measure warm/cold seek p50/p95/max on declared long-set fixtures; set the interactive target before measurement and report misses.
- [ ] Rapidly seek, cancel, change source and close; stale work never starts playback or mutates the latest project.
- [ ] Nonzero start PCM and envelope state match sequential reference within the declared tolerance for volume/HP/LP; define limits for future stretch/plugins.
- [ ] Preparation and cancellation are visible, memory/work remain bounded and the UI stays responsive.

**Dependencies:** [DJ-008](https://github.com/coder13/Fader/issues/8), [DJ-038](https://github.com/coder13/Fader/issues/38)

## M2 · POC: editable sets and qualification

<a id="dj-011-implement-versioned-project-model-and-initial-state"></a>

### DJ-011 Harden the versioned clip-owned project model

GitHub: [#11](https://github.com/coder13/Fader/issues/11). Labels: `phase:poc`, `type:feature`, `area:storage`.

**Current evidence:** [Storage schema 3](project-format-v3.md) adds stable project/clip/asset IDs, named curves, decoded-domain identity, render/grid snapshots and strict read/write validation. Local migration/render/type tests and [clean Linux/macOS CI](https://github.com/coder13/Fader/actions/runs/41990487133) pass. Legacy v1/v2 renderer behavior is preserved during storage migration.

**Scope:** Validate stable clip/asset/parameter addressing, initial state, units/time domains, malformed input and future render-version compatibility using the existing model.

**Acceptance:**

- [x] Round-trip placement, metadata, curve values/hold semantics and source identity; moving a clip preserves its own curves.
- [x] Reject nonfinite values, invalid references, oversize input and unsupported schema writes; exclude device handles from saved data.
- [x] Define asset/project/source-frame conversion, legacy preservation and migration policy; snapshot grid/render settings rather than depending on mutable library state.

**Dependencies:** [DJ-008](https://github.com/coder13/Fader/issues/8)

<a id="dj-012-capture-applied-actions-and-preserve-original-takes"></a>

### DJ-012 Complete automation, audio-only and combined performance capture

GitHub: [#12](https://github.com/coder13/Fader/issues/12). Labels: `phase:poc`, `type:feature`, `area:automation`.

**Current evidence:** Clip volume/HP/LP automation capture works in the software prototype. Raw master recording, durable take journaling and complete transport/source-action capture remain open.

**Scope:** Record accepted engine-timed targets and necessary discrete transport/source actions; independently arm master PCM recording through a bounded worker queue. Preserve the original take and initial state.

**Acceptance:**

- [ ] Automation-only, audio-only and combined modes work; headphone monitoring and hardware output volume stay outside master automation.
- [ ] Targets retain step timing plus declared smoothing; nonzero-position capture uses correct project anchors (A08/A09).
- [ ] Queue overflow, disk failure and interrupted capture mark only affected recordings incomplete; retained valid data is recoverable (A12).
- [ ] Physical S4 capture is integrated/qualified in [DJ-009](https://github.com/coder13/Fader/issues/9) and [DJ-017](https://github.com/coder13/Fader/issues/17); software capture development does not wait for hardware.

**Dependencies:** [DJ-008](https://github.com/coder13/Fader/issues/8), [DJ-011](https://github.com/coder13/Fader/issues/11)

<a id="dj-013-render-projects-offline-and-compare-with-playback"></a>

### DJ-013 Qualify offline rendering against live-core playback

GitHub: [#13](https://github.com/coder13/Fader/issues/13). Labels: `phase:poc`, `type:feature`, `area:audio`.

**Current evidence:** [Independent offline host qualification](experiments/2026-11-05-offline-render.md) passes local applied-event/range parity, checked storage-error retention and a real 60-minute export with delayed reads, zero PCM error and ~13 MiB RSS. Clean CI and native Cancel export/progress inspection remain pending.

**Scope:** Use the same render core in an independent offline host. Define range, tail, clipping, alignment and render-version policy; wait for decoded source readiness outside the callback.

**Acceptance:**

- [ ] Baseline pre-device float PCM matches offline within 1e-6 at 64/128/256/512 frame blocks with identical event timing (A09/A10).
- [ ] Render without UI/controller/audio device; exclude cue; do not mutate the active project.
- [ ] Render a 60-minute arrangement with measured bounded memory; slow decoding waits rather than inserting live-starvation silence (A15).
- [ ] Cancel, unreadable media, disk-full and destination replacement retain a prior valid output or give an explicit failure.

**Dependencies:** [DJ-007](https://github.com/coder13/Fader/issues/7), [DJ-008](https://github.com/coder13/Fader/issues/8), [DJ-011](https://github.com/coder13/Fader/issues/11), [DJ-038](https://github.com/coder13/Fader/issues/38)

<a id="dj-014-build-the-two-lane-arrangement-editor"></a>

### DJ-014 Finish arrangement editing affordances without resizing clips

GitHub: [#14](https://github.com/coder13/Fader/issues/14). Labels: `phase:poc`, `type:feature`, `area:editor`.

**Current evidence:** Scrollable fixed-zoom rows, header dragging, waveform overlays, parameter selection, library drop and dark menus are implemented; handler tests cover clip movement.

**Scope:** Harden selection, exact placement and keyboard access around the existing clip-owned model. Fix defects discovered in the owner session; keep any future trim gesture distinct from moving a clip.

**Acceptance:**

- [ ] Drag songs horizontally and vertically while preserving duration, waveform scale and local envelopes; undo restores placement.
- [ ] Selection, point creation/drag/deletion and menu opening have distinct reversible behavior; opening menus does not edit or seek.
- [ ] Provide understandable exact positions and keyboard alternatives; document current grid/zoom/snap limits.
- [ ] Long library/analysis work stays off the UI/audio paths; owner workflow evidence is tracked in [DJ-042](https://github.com/coder13/Fader/issues/42).

**Dependencies:** [DJ-011](https://github.com/coder13/Fader/issues/11)

**Product coverage follow-up:** These product follow-ups preserve this issue's existing POC scope. They do not become new blockers for the POC merely by being linked here. [DJ-045](https://github.com/coder13/Fader/issues/45), [DJ-046](https://github.com/coder13/Fader/issues/46), [DJ-055](https://github.com/coder13/Fader/issues/55), [DJ-056](https://github.com/coder13/Fader/issues/56).

<a id="dj-015-edit-recorded-automation-and-replace-a-range"></a>

### DJ-015 Qualify selective retakes, override and return boundaries

GitHub: [#15](https://github.com/coder13/Fader/issues/15). Labels: `phase:poc`, `type:feature`, `area:automation`.

**Current evidence:** The software prototype records clip volume/HP/LP, exposes Read/Override/Record, Resume and optional retake ranges. Touched-parameter replacement and undo have tests.

**Scope:** Verify a nonzero-position replacement against the original take, including declared smoothing/preroll behavior, visible range/arming and physical pickup integration.

**Acceptance:**

- [ ] Edit and partly reperform a transition; unarmed parameters and stored points outside the effective interval remain unchanged.
- [ ] Define and display any return region; test baseline-state reconstruction and boundary PCM continuity (A09).
- [ ] One undo restores the previous take; stop/cancel/keep behavior and hidden active-range indication are unambiguous.
- [ ] Verify no silent capture loss and connect the physical pickup behavior from [DJ-009](https://github.com/coder13/Fader/issues/9) before closing the complete hardware POC.

**Dependencies:** [DJ-012](https://github.com/coder13/Fader/issues/12), [DJ-014](https://github.com/coder13/Fader/issues/14)

**Product coverage follow-up:** These product follow-ups preserve this issue's existing POC scope. They do not become new blockers for the POC merely by being linked here. [DJ-046](https://github.com/coder13/Fader/issues/46).

<a id="dj-016-save-reopen-relink-and-recover-the-poc-set"></a>

### DJ-016 Add autosave, missing-media relinking and crash recovery

GitHub: [#16](https://github.com/coder13/Fader/issues/16). Labels: `phase:poc`, `type:feature`, `area:storage`.

**Current evidence:** Versioned save/reopen, source hashes and atomic export exist. There is no autosave or missing-source relinking UI yet.

**Scope:** Extend project persistence with atomic save/backups, a bounded recovery journal and explicit missing/changed-media workflow. Keep the original library read-only.

**Acceptance:**

- [ ] Reopen without controller; relocate matching files and relink; reject wrong-content substitutions (A11).
- [ ] Interrupt save and recording; recover the latest valid project/take without overwriting the previous good version (A12).
- [ ] Unsupported versions and malformed projects fail visibly without being silently rewritten.
- [ ] Validate Unicode/case/path aliases and worker cancellation; document recovery limits.

**Dependencies:** [DJ-011](https://github.com/coder13/Fader/issues/11), [DJ-012](https://github.com/coder13/Fader/issues/12)

<a id="dj-017-qualify-and-document-the-complete-mac-poc"></a>

### DJ-017 Decide whether the Mac/S4 POC is ready for product development

GitHub: [#17](https://github.com/coder13/Fader/issues/17). Labels: `phase:poc`, `type:quality`, `area:research`.

**Current evidence:** The native prototype demonstrates feasibility; no complete hardware/long-run qualification or final production architecture decision exists.

**Scope:** Run the complete A01–A16 matrix and owner workflow against a pinned build. Reconcile reuse/stack findings, current limits and the product release scope before declaring the POC complete.

**Acceptance:**

- [ ] Record pass/fail/unknown for every A01–A16 gate, exact machine/device/firmware/settings, fixture recipe, metrics and uncertainty.
- [ ] Demonstrate MP3/WAV arrange → S4 transition → envelope edit → nonzero retake → save/reopen without controller → WAV export.
- [ ] Include sustained streaming/seek stress, bounded 60-minute export, capture/replay parity and failure/recovery evidence; screenshots alone cannot close this issue.
- [ ] Record pass at 128 frames, documented fallback at 256, or explicit failure; never silently relax thresholds.
- [ ] Publish a go/no-go decision for C++/JUCE, UI/core boundaries and remaining architecture risks; identify product blockers and split the next epics.
- [ ] Keep hardware-dependent gates open until measured. Web, VST3, Linux and advanced S4 features are not native POC blockers.

**Dependencies:** [DJ-009](https://github.com/coder13/Fader/issues/9), [DJ-010](https://github.com/coder13/Fader/issues/10), [DJ-013](https://github.com/coder13/Fader/issues/13), [DJ-015](https://github.com/coder13/Fader/issues/15), [DJ-016](https://github.com/coder13/Fader/issues/16), [DJ-035](https://github.com/coder13/Fader/issues/35), [DJ-037](https://github.com/coder13/Fader/issues/37), [DJ-039](https://github.com/coder13/Fader/issues/39), [DJ-040](https://github.com/coder13/Fader/issues/40), [DJ-041](https://github.com/coder13/Fader/issues/41), [DJ-042](https://github.com/coder13/Fader/issues/42)

<a id="dj-035-import-mp3-and-the-existing-library-in-the-completed-poc"></a>

### DJ-035 Qualify representative MP3/WAV library import and seeking

GitHub: [#35](https://github.com/coder13/Fader/issues/35). Labels: `phase:poc`, `type:quality`, `area:library`.

**Current evidence:** MP3/WAV import and metadata browsing work; 949 existing DJ files matched Traktor metadata. Inventory/matching does not prove complete decoding.

**Scope:** Test the selected decoder against representative user-library files locally plus redistributable CBR/VBR MP3 and PCM WAV fixtures. Preserve source files and source-frame positioning.

**Acceptance:**

- [ ] Prove sequential decode versus random seek/cue alignment, priming/padding and duration; waveform, playback and export agree (A16).
- [ ] Test supported bit depths/rates, malformed tags, wrong suffixes, truncation and missing files with distinct diagnostics.
- [ ] Save/reopen/export retains placement and source identity; use WAV for strict engine parity.
- [ ] Publish aggregate counts only; full common-format and whole-library qualification belongs to [DJ-036](https://github.com/coder13/Fader/issues/36).

**Dependencies:** [DJ-007](https://github.com/coder13/Fader/issues/7), [DJ-011](https://github.com/coder13/Fader/issues/11), [DJ-014](https://github.com/coder13/Fader/issues/14)

### DJ-040 Measure sustained playback, capture and failure recovery under load

GitHub: [#40](https://github.com/coder13/Fader/issues/40). Labels: `phase:poc`, `type:quality`, `area:audio`, `priority:next`.

**Current evidence:** Short silent device smoke, synthetic timing and callback-allocation checks exist. They do not establish 60-minute reliability or end-to-end latency.

**Scope:** Create a repeatable software stress harness plus device runs with two active sources, dense automation, UI work, library jobs and delayed decoding. Add fault injection and off-thread metrics.

**Acceptance:**

- [ ] Run 60 minutes at 48 kHz/128 frames with zero detected underruns/missed deadlines and no capture loss (A04); report a 256-frame fallback separately.
- [ ] Target callback p99.9 below half the 2.67 ms budget and maximum below deadline; report tails and measurement limitations (A05/A06).
- [ ] Exercise queue pressure, cancellation, missing media and device loss; retain valid takes and show failures (A12).
- [ ] Measure input receipt, scheduled/applied frames and predicted output separately; physical/loopback latency requires hardware and is finalized in [DJ-017](https://github.com/coder13/Fader/issues/17) (A07).
- [ ] Archive reproducible scripts and aggregate results tied to the exact build; do not publish private library paths.

**Dependencies:** [DJ-008](https://github.com/coder13/Fader/issues/8), [DJ-013](https://github.com/coder13/Fader/issues/13), [DJ-038](https://github.com/coder13/Fader/issues/38), [DJ-039](https://github.com/coder13/Fader/issues/39)

### DJ-042 Run the owner prepared-set usability session

GitHub: [#42](https://github.com/coder13/Fader/issues/42). Labels: `phase:poc`, `type:quality`, `area:editor`.

**Current evidence:** The dark native UI, menus and Preferences have been inspected. A structured owner session with real transitions has not been completed.

**Scope:** Observe a real two-song preparation task without coaching every action. Use the Audacity/Traktor research and current clip-owned automation rules as hypotheses, not proof of usability.

**Acceptance:**

- [ ] Import and search by title/artist/BPM/key; arrange an overlap, draw volume/HP/LP, record and replace part of a transition, move clips, save/reopen and export.
- [ ] Record confusion, accidental edits, task completion and visibility of Read/Override/Record and retake range; verify menu discoverability.
- [ ] Check keyboard/focus, contrast, minimum-window layout and drag behavior; create bounded follow-up fixes with priorities.
- [ ] Keep track-level observations local; commit an anonymized report and remaining blockers (A13).

**Dependencies:** [DJ-014](https://github.com/coder13/Fader/issues/14), [DJ-015](https://github.com/coder13/Fader/issues/15), [DJ-016](https://github.com/coder13/Fader/issues/16), [DJ-035](https://github.com/coder13/Fader/issues/35)

**Product coverage follow-up:** These product follow-ups preserve this issue's existing POC scope. They do not become new blockers for the POC merely by being linked here. [DJ-054](https://github.com/coder13/Fader/issues/54), [DJ-055](https://github.com/coder13/Fader/issues/55), [DJ-056](https://github.com/coder13/Fader/issues/56).

## M3 · Product alpha: musical mixing

### DJ-018 Add beat analysis and editable grids

GitHub: [#18](https://github.com/coder13/Fader/issues/18). Labels: `phase:product`, `type:epic`, `area:audio`.

**Current evidence:** Planned after the POC; not implemented or qualified by the current prototype.

**Scope:** BPM/beat candidates, manual first beat/BPM correction, multiple source anchors, confidence/version provenance, per-project grid snapshots.

**Acceptance:**

- [ ] fixed and variable-tempo fixtures; phase/downbeat and BPM distinguished; user correction survives reanalysis/reopen; detector failures visibly editable; sync drift quantified over long tracks.

**Dependencies:** [DJ-017](https://github.com/coder13/Fader/issues/17)

**Product coverage follow-up:** These issues make the related product contracts and qualification evidence explicit. Reuse shared implementation and fixtures rather than duplicating engines or workflows. [DJ-047](https://github.com/coder13/Fader/issues/47), [DJ-049](https://github.com/coder13/Fader/issues/49).

### DJ-019 Select and integrate pitch-preserving time stretch

GitHub: [#19](https://github.com/coder13/Fader/issues/19). Labels: `phase:product`, `type:spike`, `area:audio`.

**Current evidence:** Planned after the POC; not implemented or qualified by the current prototype.

**Scope:** compare Rubber Band/Signalsmith, adapter with latency/startup/tail/reset/rate-change behavior, representative listening corpus.

**Acceptance:**

- [ ] measured quality/latency/CPU decision; both decks meet selected callback budget; key-lock is distinct from varispeed; seeks/preroll and offline parity characterized; licensing/version recorded.

**Dependencies:** [DJ-017](https://github.com/coder13/Fader/issues/17)

**Product coverage follow-up:** These issues make the related product contracts and qualification evidence explicit. Reuse shared implementation and fixtures rather than duplicating engines or workflows. [DJ-047](https://github.com/coder13/Fader/issues/47), [DJ-049](https://github.com/coder13/Fader/issues/49).

### DJ-020 Add project tempo maps and musical anchors

GitHub: [#20](https://github.com/coder13/Fader/issues/20). Labels: `phase:product`, `type:epic`, `area:automation`.

**Current evidence:** Planned after the POC; not implemented or qualified by the current prototype.

**Scope:** step changes then defined tempo ramps, beat/time transforms, beat-anchored clips/automation, sync/nudge/resume behavior.

**Acceptance:**

- [ ] conversions round-trip with bounded error; edits move beat-anchored objects as documented; absolute objects remain fixed; tempo changes survive capture/replay/export; no click or unexpected phase jump at segment boundaries.

**Dependencies:** [DJ-018](https://github.com/coder13/Fader/issues/18), [DJ-019](https://github.com/coder13/Fader/issues/19)

**Product coverage follow-up:** These issues make the related product contracts and qualification evidence explicit. Reuse shared implementation and fixtures rather than duplicating engines or workflows. [DJ-047](https://github.com/coder13/Fader/issues/47), [DJ-048](https://github.com/coder13/Fader/issues/48).

### DJ-021 Capture EQ, filters, loops, cues, and jog trajectories

GitHub: [#21](https://github.com/coder13/Fader/issues/21). Labels: `phase:product`, `type:epic`, `area:automation`.

**Current evidence:** Planned after the POC; not implemented or qualified by the current prototype.

**Scope:** Extend the existing clip volume/HP/LP implementation with three-band EQ, then discrete cues/loops and relative jog/rate trajectories. Define touch/latch and action ownership before implementation. Time-dependent features additionally require [DJ-019](https://github.com/coder13/Fader/issues/19) and [DJ-020](https://github.com/coder13/Fader/issues/20).

**Acceptance:**

- [ ] each new parameter/action has versioned units/interpolation, capture and seek reconstruction fixtures; source trajectory survives loops/reverse; latency and audible continuity measured; unsupported modes cannot masquerade as reproducible capture.

**Dependencies:** [DJ-017](https://github.com/coder13/Fader/issues/17)

**Product coverage follow-up:** This epic remains the owner of new parameter/action capture and touch/latch semantics. The linked issues specify sync and per-mode cue/loop/jog behavior; their selected contracts and fixtures must be satisfied as part of this epic's completion. [DJ-047](https://github.com/coder13/Fader/issues/47), [DJ-048](https://github.com/coder13/Fader/issues/48).

### DJ-022 Schedule external MIDI clock and transport

GitHub: [#22](https://github.com/coder13/Fader/issues/22). Labels: `phase:product`, `type:epic`, `area:controller`.

**Current evidence:** Planned after the POC; not implemented or qualified by the current prototype.

**Scope:** clock at 24 pulses per quarter note, Start/Continue/Stop, transport changes, queue cancellation.

**Acceptance:**

- [ ] jitter/drift distributions at fixed tempo and ramps; seek/loop/restart policy documented; virtual and physical tests distinguished; offline export sends no hardware clock; external-instrument audio requires capture for export.

**Dependencies:** [DJ-010](https://github.com/coder13/Fader/issues/10), [DJ-020](https://github.com/coder13/Fader/issues/20)

## M4 · Product alpha: reliable Mac workflow

### DJ-023 Add long-library caching and analysis jobs

GitHub: [#23](https://github.com/coder13/Fader/issues/23). Labels: `phase:product`, `type:epic`, `area:library`.

**Current evidence:** Planned after the POC; not implemented or qualified by the current prototype.

**Scope:** Build persistent indexing/search, preview, peak/analysis caches, background job scheduling and cancellation on top of the bounded streaming engine from [DJ-038](https://github.com/coder13/Fader/issues/38). Analysis must not interrupt playback; cached data must not overwrite saved project edits.

**Acceptance:**

- [ ] realistic multi-hour library/session has bounded memory and responsive UI; analysis cannot interrupt playback; source moves and stale worker results handled; cache deletion preserves edits.

**Dependencies:** [DJ-017](https://github.com/coder13/Fader/issues/17), [DJ-038](https://github.com/coder13/Fader/issues/38)

**Product coverage follow-up:** These issues make the related product contracts and qualification evidence explicit. Reuse shared implementation and fixtures rather than duplicating engines or workflows. [DJ-051](https://github.com/coder13/Fader/issues/51).

### DJ-024 Add MP3 and FLAC export presets

GitHub: [#24](https://github.com/coder13/Fader/issues/24). Labels: `phase:product`, `type:epic`, `area:audio`.

**Current evidence:** Planned after the POC; not implemented or qualified by the current prototype.

**Scope:** encoder selection, delay/padding behavior, tags, bit depth/rate, normalization and dither choices. MP3 import is in [DJ-035](https://github.com/coder13/Fader/issues/35); broad import is in [DJ-036](https://github.com/coder13/Fader/issues/36).

**Acceptance:**

- [ ] sample-rate/duration/channel/tag fixtures, clipping policy, long output and cancel/disk-full tests; lossy export compared after alignment using explicit tolerance; encoder license/build settings documented. WAV remains the reference render.

**Dependencies:** [DJ-013](https://github.com/coder13/Fader/issues/13), [DJ-017](https://github.com/coder13/Fader/issues/17)

**Product coverage follow-up:** These issues make the related product contracts and qualification evidence explicit. Reuse shared implementation and fixtures rather than duplicating engines or workflows. [DJ-052](https://github.com/coder13/Fader/issues/52), [DJ-053](https://github.com/coder13/Fader/issues/53).

### DJ-025 Package portable sets and robust recovery

GitHub: [#25](https://github.com/coder13/Fader/issues/25). Labels: `phase:product`, `type:epic`, `area:storage`.

**Current evidence:** Planned after the POC; not implemented or qualified by the current prototype.

**Scope:** collect-media bundle, safe extraction, manifest migration, take recovery, compatibility/freeze path design.

**Acceptance:**

- [ ] transfer to another machine and relink/collected-media reopen; content hashes verified; malformed paths/oversized data rejected; backup preserved across migration; explicit incomplete take recovery.

**Dependencies:** [DJ-016](https://github.com/coder13/Fader/issues/16), [DJ-023](https://github.com/coder13/Fader/issues/23)

### DJ-026 Ship and validate a Mac alpha

GitHub: [#26](https://github.com/coder13/Fader/issues/26). Labels: `phase:product`, `type:epic`, `area:platform`.

**Current evidence:** Planned after the POC; not implemented or qualified by the current prototype.

**Scope:** packaging/signing plan, permissions/device lifecycle, accessibility, release notes, hardware matrix and user workflow validation. Include keyboard-only core tasks, screen-reader/focus testing, scalable layout, discoverable context actions and a tested install/update/recovery path. Confirm release dependency/licensing obligations from [DJ-001](https://github.com/coder13/Fader/issues/1).

**Acceptance:**

- [ ] install/open/save/export/update path tested on declared supported machines; no broad support claims without evidence; S4 firmware/settings and limitations published; source/build/license notices available for the chosen distribution.

- [ ] Use the selected product contract in #44: demonstrate the selected editing, organization, monitoring and delivery workflows on the Mac alpha build, with linked implementation evidence.
- [ ] Attach current usability repair/retest evidence (#54), owner visual approval (#55), and the Mac size/keyboard/screen-reader matrix (#56); no unresolved finding may lose work, conceal capture failure or prevent a core advertised task.

**Dependencies:** [DJ-018](https://github.com/coder13/Fader/issues/18), [DJ-019](https://github.com/coder13/Fader/issues/19), [DJ-020](https://github.com/coder13/Fader/issues/20), [DJ-021](https://github.com/coder13/Fader/issues/21), [DJ-023](https://github.com/coder13/Fader/issues/23), [DJ-024](https://github.com/coder13/Fader/issues/24), [DJ-025](https://github.com/coder13/Fader/issues/25), [DJ-036](https://github.com/coder13/Fader/issues/36), [DJ-043](https://github.com/coder13/Fader/issues/43), [DJ-001](https://github.com/coder13/Fader/issues/1)

**Product coverage follow-up:** These issues make the related product contracts and qualification evidence explicit. Reuse shared implementation and fixtures rather than duplicating engines or workflows. [DJ-044](https://github.com/coder13/Fader/issues/44), [DJ-045](https://github.com/coder13/Fader/issues/45), [DJ-046](https://github.com/coder13/Fader/issues/46), [DJ-051](https://github.com/coder13/Fader/issues/51), [DJ-052](https://github.com/coder13/Fader/issues/52), [DJ-053](https://github.com/coder13/Fader/issues/53), [DJ-054](https://github.com/coder13/Fader/issues/54), [DJ-055](https://github.com/coder13/Fader/issues/55), [DJ-056](https://github.com/coder13/Fader/issues/56), [DJ-057](https://github.com/coder13/Fader/issues/57).

### DJ-036 Qualify broad audio import against the existing collection

GitHub: [#36](https://github.com/coder13/Fader/issues/36). Labels: `phase:product`, `type:epic`, `area:library`.

**Current evidence:** Planned after the POC; not implemented or qualified by the current prototype.

**Scope:** AIFF, FLAC, AAC, ALAC, Vorbis, Opus plus inventory-driven variants; cross-platform decoder matrix; full collection scan/decode with no unexplained omissions. [Import specification](https://github.com/coder13/Fader/blob/3a3b58456065f6cfed4c9f23f1a0252047d80a89/docs/audio-import-and-library.md).

**Acceptance:**

- [ ] every supported readable audio candidate decodes to completion; failures receive per-file local diagnostics; seek/priming/duration, malformed tags, headers/extensions, rates/depths and multichannel policy covered by fixtures; originals unchanged; aggregate report committed without songs, track lists or private paths. Repeat critical formats on Windows in [DJ-027](https://github.com/coder13/Fader/issues/27) and qualify browser capabilities in M6.

**Dependencies:** [DJ-023](https://github.com/coder13/Fader/issues/23), [DJ-035](https://github.com/coder13/Fader/issues/35)

### DJ-043 Complete read-only Traktor library and playlist import

GitHub: [#43](https://github.com/coder13/Fader/issues/43). Labels: `phase:product`, `type:epic`, `area:library`.

**Current evidence:** The POC imports title/artist/album/genre, BPM, Open Key and a first grid marker; it counts cues. Full playlists/cues/loops and variable grids are not imported.

**Scope:** Extend the importer with explicit per-field provenance/conflicts and supported playlists, cue/loop markers and multiple grid anchors. Keep Traktor and music files unchanged.

**Acceptance:**

- [ ] Synthetic NML fixtures cover supported versions, missing/relocated/Unicode paths and repeated tracks without title-only guessing.
- [ ] Browser playlists and imported markers survive reindex/reopen; saved sets retain their grid snapshots and user corrections.
- [ ] Unsupported or ambiguous data is reported rather than silently invented; document exactly imported fields.
- [ ] Verify input hashes remain unchanged and publish only aggregate real-library findings.

**Dependencies:** [DJ-018](https://github.com/coder13/Fader/issues/18), [DJ-023](https://github.com/coder13/Fader/issues/23)

**Product coverage follow-up:** These issues make the related product contracts and qualification evidence explicit. Reuse shared implementation and fixtures rather than duplicating engines or workflows. [DJ-051](https://github.com/coder13/Fader/issues/51).

## M5 · Product v1: Windows and more controllers

### DJ-027 Qualify Windows audio and S4 integration

GitHub: [#27](https://github.com/coder13/Fader/issues/27). Labels: `phase:product`, `type:epic`, `area:platform`.

**Current evidence:** Planned after the POC; not implemented or qualified by the current prototype.

**Scope:** Windows builds, WASAPI and current vendor ASIO, HID, paths/permissions/installer, project parity.

**Acceptance:**

- [ ] critical M2 workflow rerun on documented Windows/device/driver versions; master/cue, latency, device loss and stress measured; current driver-block reports rechecked; no reliance on weakening OS protections.

- [ ] Qualify the selected M3/M4 product workflows and critical broad-format fixtures on Windows using the #44 matrix, not only the M2 POC scenario; record project/render parity, accessibility and install/update/recovery results for the final #57 release gate.

**Dependencies:** [DJ-017](https://github.com/coder13/Fader/issues/17), [DJ-041](https://github.com/coder13/Fader/issues/41)

**Product coverage follow-up:** These issues make the related product contracts and qualification evidence explicit. Reuse shared implementation and fixtures rather than duplicating engines or workflows. [DJ-044](https://github.com/coder13/Fader/issues/44), [DJ-057](https://github.com/coder13/Fader/issues/57).

### DJ-028 Add generic MIDI mapping and a second controller

GitHub: [#28](https://github.com/coder13/Fader/issues/28). Labels: `phase:product`, `type:epic`, `area:controller`.

**Current evidence:** Planned after the POC; not implemented or qualified by the current prototype.

**Scope:** mapping schema, learn, absolute/relative controls, modifiers, pickup, output feedback; select hardware actually available for testing.

**Acceptance:**

- [ ] second model has fixtures and physical qualification; same semantic recording replays independently of mapping/device; MIDI addresses never replace stable project parameter IDs; feature matrix clearly scoped by model/generation.

**Dependencies:** [DJ-009](https://github.com/coder13/Fader/issues/9), [DJ-010](https://github.com/coder13/Fader/issues/10), [DJ-017](https://github.com/coder13/Fader/issues/17)

## M6 · Product: web or hybrid delivery

### DJ-029 Implement the selected web or hybrid host

GitHub: [#29](https://github.com/coder13/Fader/issues/29). Labels: `phase:product`, `type:epic`, `area:platform`.

**Current evidence:** Planned after the POC; not implemented or qualified by the current prototype.

**Scope:** one chosen route—native engine + web UI, or browser arrangement/AudioWorklet/Wasm host—with versioned bridge/project semantics.

**Acceptance:**

- [ ] browser/native project fixtures agree within declared scope; capability detection and permission failure usable; UI stalls do not control audio time; native bridge isolates device access; no duplicated divergent automation rules.

**Dependencies:** [DJ-004](https://github.com/coder13/Fader/issues/4), [DJ-005](https://github.com/coder13/Fader/issues/5), [DJ-017](https://github.com/coder13/Fader/issues/17)

### DJ-030 Qualify web long sets and conditional hardware support

GitHub: [#30](https://github.com/coder13/Fader/issues/30). Labels: `phase:product`, `type:epic`, `area:platform`.

**Current evidence:** Planned after the POC; not implemented or qualified by the current prototype.

**Scope:** WEB-04–WEB-07 where applicable, bounded decode/export, file/storage recovery, tab lifecycle, browser matrix.

**Acceptance:**

- [ ] 60-minute render with measured bounded memory; no chunk-boundary audio discontinuity; save/reopen/relink works with permissions changing; S4 live support advertised only on qualified configurations; Safari scope accurately described.

**Dependencies:** [DJ-029](https://github.com/coder13/Fader/issues/29)

## M7 · Later: advanced performance and Linux

### DJ-031 Expand to four decks and deeper S4 features

GitHub: [#31](https://github.com/coder13/Fader/issues/31). Labels: `phase:product`, `type:epic`, `area:controller`.

**Current evidence:** Planned after the POC; not implemented or qualified by the current prototype.

**Scope:** four-deck control layers and routing first; controller screen rendering and motor/haptic protocols as separately gated subprojects.

**Acceptance:**

- [ ] four active decks pass audio/capture stress; deck/lane identity changes do not rewrite old takes; each screen/motor feature has protocol, bounds, lifecycle and physical qualification evidence before enabling by default.

**Dependencies:** [DJ-020](https://github.com/coder13/Fader/issues/20), [DJ-021](https://github.com/coder13/Fader/issues/21), [DJ-026](https://github.com/coder13/Fader/issues/26)

### DJ-032 Qualify Linux builds, audio and controller support

GitHub: [#32](https://github.com/coder13/Fader/issues/32). Labels: `phase:product`, `type:epic`, `area:platform`.

**Current evidence:** Planned after the POC; not implemented or qualified by the current prototype.

**Scope:** Qualify Linux builds, audio backends, device permissions, S4/generic MIDI and packaging on a declared test environment. This issue is Linux qualification only; optional advanced integrations now have independent decision issues below. VST3 remains in #33/#34.

**Acceptance:**

- [ ] Publish Linux build/install and core prepared-set workflow results on declared distribution, audio backend and hardware versions; measure routing, device lifecycle, timing and recovery for the advertised configuration.
- [ ] Qualify supported S4/generic MIDI behavior separately; document unsupported controls and packaging/permission limits. Linux remains optional and does not block the Mac/Windows prepared-set product.

**Dependencies:** [DJ-017](https://github.com/coder13/Fader/issues/17)

**Product coverage follow-up:** Optional product directions are tracked independently below. Their decisions and implementations do not block Linux qualification or native v1. [DJ-058](https://github.com/coder13/Fader/issues/58), [DJ-059](https://github.com/coder13/Fader/issues/59), [DJ-060](https://github.com/coder13/Fader/issues/60), [DJ-061](https://github.com/coder13/Fader/issues/61), [DJ-062](https://github.com/coder13/Fader/issues/62), [DJ-063](https://github.com/coder13/Fader/issues/63), [DJ-064](https://github.com/coder13/Fader/issues/64).

## M8 · Product v2: VST3 effects

### DJ-033 Host VST3 effects with editable automation and saved state

GitHub: [#33](https://github.com/coder13/Fader/issues/33). Labels: `phase:product`, `type:epic`, `area:plugins`.

**Current evidence:** Confirmed user requirement for product version 2, explicitly excluded from the POC. No plugin host is implemented.

**Scope:** installed-plugin browser, isolated scanning, stereo song/lane/master racks, plugin editors, order/bypass, exposed parameter capture/edit/replay, state persistence, latency compensation, and offline export. [Design](https://github.com/coder13/Fader/blob/3a3b58456065f6cfed4c9f23f1a0252047d80a89/docs/plugin-effects.md).

**Acceptance:**

- [ ] a supported VST3 effect can be added to a song, automated from its GUI/controller or drawn lane, saved/reopened and exported; two instances retain independent state and automation; tempo context, bypass delay, dynamic latency and tails pass fixtures; missing/incompatible effects preserve state and cannot silently change export; Mac/Windows builds qualified individually. POC has no new plugin dependency.

**Dependencies:** [DJ-020](https://github.com/coder13/Fader/issues/20), [DJ-021](https://github.com/coder13/Fader/issues/21), [DJ-027](https://github.com/coder13/Fader/issues/27)

**Product coverage follow-up:** These issues make the related product contracts and qualification evidence explicit. Reuse shared implementation and fixtures rather than duplicating engines or workflows. [DJ-064](https://github.com/coder13/Fader/issues/64).

## M9 · Product v3: plugin resilience and portability

### DJ-034 Harden plugin hosting and portable effect renders

GitHub: [#34](https://github.com/coder13/Fader/issues/34). Labels: `phase:product`, `type:epic`, `area:plugins`.

**Current evidence:** Confirmed product version 3 direction, following the qualified version 2 effects host. Runtime isolation is an experiment, not an existing guarantee.

**Scope:** evaluate runtime process isolation and its latency cost; broaden plugin matrix; freeze/unfreeze and real-time-render fallback; optional sidechains/sends and additional formats after separate decisions.

**Acceptance:**

- [ ] failure recovery and buffering behavior documented for supported process modes; frozen audio carries provenance and original editable chain state; project reopening without a plugin offers an explicit usable fallback; browser-only editing preserves native plugin state and can preview frozen audio; offline/render-mode differences and non-deterministic effects are characterized rather than advertised as bit-identical.

**Dependencies:** [DJ-033](https://github.com/coder13/Fader/issues/33)

**Product coverage follow-up:** These issues make the related product contracts and qualification evidence explicit. Reuse shared implementation and fixtures rather than duplicating engines or workflows. [DJ-064](https://github.com/coder13/Fader/issues/64).
