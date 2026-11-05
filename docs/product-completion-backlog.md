# Product completion issues

Published 2026-11-04 at the user's request following the [completeness review](research/roadmap-completeness-review.md): **21 new issues, DJ-044–DJ-064**. Fourteen product issues join existing milestones; seven optional decision issues have no release milestone. The repository now has **64 issues across 10 milestones**, including those seven unscheduled decisions. GitHub holds current status.

Creating issues authorizes planning and tracking; it does not establish implementation, measured usability or acceptance of every optional feature. [DJ-044](https://github.com/coder13/Fader/issues/44) selects and records the native v1 feature contract. [DJ-057](https://github.com/coder13/Fader/issues/57) is the integrated Mac/Windows release gate. VST3 remains v2/v3; the existing POC streaming/seek/load priorities remain unchanged.

## Review-to-issue map

| Review | Published issues |
| --- | --- |
| R1 | [DJ-044](https://github.com/coder13/Fader/issues/44), [DJ-057](https://github.com/coder13/Fader/issues/57) |
| R2 | [DJ-045](https://github.com/coder13/Fader/issues/45), [DJ-046](https://github.com/coder13/Fader/issues/46) |
| R3 | [DJ-047](https://github.com/coder13/Fader/issues/47), [DJ-048](https://github.com/coder13/Fader/issues/48), [DJ-049](https://github.com/coder13/Fader/issues/49), [DJ-050](https://github.com/coder13/Fader/issues/50) |
| R4 | [DJ-051](https://github.com/coder13/Fader/issues/51) |
| R5 | [DJ-052](https://github.com/coder13/Fader/issues/52), [DJ-053](https://github.com/coder13/Fader/issues/53) |
| R6 | [DJ-054](https://github.com/coder13/Fader/issues/54) |
| R7 | [DJ-055](https://github.com/coder13/Fader/issues/55), [DJ-056](https://github.com/coder13/Fader/issues/56) |
| R8 | [DJ-058](https://github.com/coder13/Fader/issues/58), [DJ-059](https://github.com/coder13/Fader/issues/59), [DJ-060](https://github.com/coder13/Fader/issues/60), [DJ-061](https://github.com/coder13/Fader/issues/61), [DJ-062](https://github.com/coder13/Fader/issues/62), [DJ-063](https://github.com/coder13/Fader/issues/63), [DJ-064](https://github.com/coder13/Fader/issues/64) |

## Publication and tracking

Every new issue has current-evidence context, scope, acceptance criteria, dependency links or an explicit independent-decision status, existing area/type/phase labels and a stable publication marker. Existing epics link to their follow-ups. Mac alpha and Windows acceptance now include the selected product workflows and design evidence. DJ-032 now owns Linux qualification only; optional integrations have separate decisions.

The original [roadmap](roadmap-and-issue-backlog.md) retains DJ-001–DJ-043 and the POC gates. Dependencies describe contracts needed for completion, not a ban on independent preparation. Optional decision issues close only with an evidence-backed adopt/defer/reject outcome; adoption requires scoped implementation and qualification tasks before release assignment.

## M3 · Product alpha: musical mixing

### DJ-044 Define the native v1 feature and release contract

GitHub: [#44](https://github.com/coder13/Fader/issues/44). Labels: `area:research`, `phase:product`, `type:spike`.

**Current evidence:**

The completeness review found a missing or incomplete product acceptance contract. The current prototype supplies a partial baseline; this issue does not claim the selected workflow is already qualified.

**Scope:**

Define a complete DJ performance editor release around prepare, perform, repair, reopen and deliver. Map every review capability to an implementation owner, a release stage and explicit acceptance evidence. This is a scope decision, not a full music-production DAW expansion.

Release placement is a planning target. DJ-044 records the selected feature subset; any deferred portion stays explicitly linked. Independent design/test preparation may proceed before dependencies close. Selected implementation issues require working behavior and evidence, not only a design document.

**Acceptance:**

- [ ] Publish a feature-to-issue matrix separating confirmed requirements, selected v1 features, later work and explicit exclusions; record the reasons for each decision.
- [ ] Specify supported Mac/Windows configurations, project/library scale targets, input methods and the representative end-to-end release scenario before qualification.
- [ ] Select the arrangement, automation, DJ-control, harmonic, library, monitoring and delivery subsets covered by the new issues; keep deferred portions linked and visible rather than dropping them.
- [ ] Preserve the POC gate and immediate streaming/seek/load priorities. Web, Linux, advanced S4, optional integrations and VST3 v2/v3 remain separately staged.
- [ ] Define release-blocker severity and evidence freshness: any behavior change invalidates affected previous qualification. The final v1 gate must cover the selected M3/M4 workflows on both platforms.

**Dependencies:**

No blocking implementation dependency. Use the published review and current product evidence.

### DJ-047 Specify and qualify sync, grid correction and quantized actions

GitHub: [#47](https://github.com/coder13/Fader/issues/47). Labels: `area:audio`, `phase:product`, `type:feature`.

**Current evidence:**

The completeness review found a missing or incomplete product acceptance contract. The current prototype supplies a partial baseline; this issue does not claim the selected workflow is already qualified.

**Scope:**

Make the user-facing synchronization contract explicit on top of the existing beat-grid, stretch and tempo-map epics. Define tempo versus phase sync, leader/follower ownership, manual nudge, sync release/rejoin, half/double tempo and quantized launches.

Release placement is a planning target. DJ-044 records the selected feature subset; any deferred portion stays explicitly linked. Independent design/test preparation may proceed before dependencies close. Selected implementation issues require working behavior and evidence, not only a design document.

**Acceptance:**

- [ ] Document each selected sync mode and leader-selection rule, including what happens when the leader stops/unloads, the grid is corrected or project tempo changes.
- [ ] Provide manual first-beat/downbeat and half/double-tempo correction with confidence/provenance; distinguish source-grid edits from project tempo edits and preserve saved snapshots.
- [ ] Specify quantization units, off behavior and late-input policy for starts/cues/loops; capture the actual resolved source position and applied frame rather than rerunning sync heuristics during replay.
- [ ] Measure phase/drift and audible discontinuities on fixed/variable-tempo and deliberately incorrect-grid fixtures; set tolerances before testing and report failures.
- [ ] Verify mouse/controller operation, nudge/rejoin, nonzero seek, save/reopen and export through tempo changes with visible mode/target state.

**Dependencies:**

- [ ] [#18 · Add beat analysis and editable grids](https://github.com/coder13/Fader/issues/18)
- [ ] [#19 · Select and integrate pitch-preserving time stretch](https://github.com/coder13/Fader/issues/19)
- [ ] [#20 · Add project tempo maps and musical anchors](https://github.com/coder13/Fader/issues/20)
- [ ] [#44 · Define the native v1 feature and release contract](https://github.com/coder13/Fader/issues/44)

### DJ-048 Define and qualify hotcue, loop, beat-jump and jog behavior

GitHub: [#48](https://github.com/coder13/Fader/issues/48). Labels: `area:automation`, `phase:product`, `type:epic`.

**Current evidence:**

The completeness review found a missing or incomplete product acceptance contract. The current prototype supplies a partial baseline; this issue does not claim the selected workflow is already qualified.

**Scope:**

Provide explicit per-action contracts beneath #21 for selected hotcues, saved/manual/beat loops, loop resizing/moving, beat jumps, reverse, slip and jog/scratch modes. Supported actions must remain editable and reproducible.

Release placement is a planning target. DJ-044 records the selected feature subset; any deferred portion stays explicitly linked. Independent design/test preparation may proceed before dependencies close. Selected implementation issues require working behavior and evidence, not only a design document.

**Acceptance:**

- [ ] Publish a per-mode support matrix with user controls, quantize interaction, source trajectory, capture requirements and unsupported cases. Implement the selected subset; link deliberately deferred modes.
- [ ] Persist hotcue names/positions and saved-loop bounds with source identity and imported/user provenance; define update/delete conflicts with read-only Traktor metadata.
- [ ] Exercise loop entry/exit, resize/move, beat jumps and source boundaries. For slip/reverse/jog modes, retain enough trajectory and initial state to reproduce the audible result.
- [ ] Fixture each supported action at nonzero starts, block boundaries and tempo changes; verify seek reconstruction, save/reopen, live/offline agreement and declared continuity/latency tolerances.
- [ ] Show the active loop/mode and destination or return position; distinguish temporary monitoring from saved actions. Unsupported capture modes cannot appear successfully recorded.
- [ ] Physical jog qualification remains per controller in #9/#28/#31; software fixtures do not certify scratch feel or hardware latency.

**Dependencies:**

- [ ] [#12 · Complete automation, audio-only and combined performance capture](https://github.com/coder13/Fader/issues/12)
- [ ] [#19 · Select and integrate pitch-preserving time stretch](https://github.com/coder13/Fader/issues/19)
- [ ] [#20 · Add project tempo maps and musical anchors](https://github.com/coder13/Fader/issues/20)
- [ ] [#47 · Specify and qualify sync, grid correction and quantized actions](https://github.com/coder13/Fader/issues/47)
- [ ] [#44 · Define the native v1 feature and release contract](https://github.com/coder13/Fader/issues/44)

### DJ-049 Add harmonic preparation and explicit key-shift behavior

GitHub: [#49](https://github.com/coder13/Fader/issues/49). Labels: `area:library`, `phase:product`, `type:epic`.

**Current evidence:**

The completeness review found a missing or incomplete product acceptance contract. The current prototype supplies a partial baseline; this issue does not claim the selected workflow is already qualified.

**Scope:**

Distinguish imported musical-key metadata, analysis, user correction, compatible-key browsing and independent pitch shifting. Select the useful harmonic-preparation subset without treating key lock as key analysis or transposition.

Release placement is a planning target. DJ-044 records the selected feature subset; any deferred portion stays explicitly linked. Independent design/test preparation may proceed before dependencies close. Selected implementation issues require working behavior and evidence, not only a design document.

**Acceptance:**

- [ ] Record the selected key-analysis/pitch-shift scope and algorithm/version/quality decision; preserve imported key provenance and user corrections through reanalysis and reopen.
- [ ] Show consistent key notation and unknown/low-confidence states; define compatible-key search and any notation conversion using fixtures.
- [ ] If independent transposition is selected, separate it visibly from key lock and varispeed, define semitone/cents units and capture its automation with saved render settings.
- [ ] Run labeled/synthetic analysis fixtures and a redistributable listening corpus; report ambiguity, quality, CPU and latency rather than promising perfect detection.
- [ ] Verify original library files remain unchanged and saved projects retain their key/pitch settings across reindex, nonzero seek and export. Publish only aggregate findings for private media.

**Dependencies:**

- [ ] [#18 · Add beat analysis and editable grids](https://github.com/coder13/Fader/issues/18)
- [ ] [#19 · Select and integrate pitch-preserving time stretch](https://github.com/coder13/Fader/issues/19)
- [ ] [#23 · Add long-library caching and analysis jobs](https://github.com/coder13/Fader/issues/23)
- [ ] [#44 · Define the native v1 feature and release contract](https://github.com/coder13/Fader/issues/44)

### DJ-050 Protect live deck loading and controller-to-clip assignment

GitHub: [#50](https://github.com/coder13/Fader/issues/50). Labels: `area:controller`, `phase:product`, `type:feature`.

**Current evidence:**

The completeness review found a missing or incomplete product acceptance contract. The current prototype supplies a partial baseline; this issue does not claim the selected workflow is already qualified.

**Scope:**

Make load/replace/eject and deck-to-clip reassignment safe and understandable during playback and capture. Preserve the independent clip-envelope model and existing pickup/reconnect contracts.

Release placement is a planning target. DJ-044 records the selected feature subset; any deferred portion stays explicitly linked. Independent design/test preparation may proceed before dependencies close. Selected implementation issues require working behavior and evidence, not only a design document.

**Acceptance:**

- [ ] Show which clip each controlled deck addresses, the active source, capture/override state and pickup status; feedback follows effective engine state.
- [ ] Define and implement playing/recording-deck load and eject protection with an explicit intentional override; unrelated playback continues and failed loads preserve the prior valid state.
- [ ] Reassign controls without rewriting previous takes or applying a stale command to the new target; a new loaded source receives the correct new clip identity.
- [ ] Test rapid selection/load/reassignment, missing media, device loss/reconnect and parameter pickup with semantic fixtures; qualify physical behavior on named supported controllers.
- [ ] If mapping the physical crossfader, document assignment/law and resolved clip-volume events. It must not replace independent saved envelopes or leak cue/hardware volume into export.

**Dependencies:**

- [ ] [#9 · Map essential S4 controls to clip automation and feedback](https://github.com/coder13/Fader/issues/9)
- [ ] [#11 · Harden the versioned clip-owned project model](https://github.com/coder13/Fader/issues/11)
- [ ] [#44 · Define the native v1 feature and release contract](https://github.com/coder13/Fader/issues/44)

### DJ-052 Qualify mix metering, gain staging and cue controls

GitHub: [#52](https://github.com/coder13/Fader/issues/52). Labels: `area:audio`, `phase:product`, `type:epic`.

**Current evidence:**

The completeness review found a missing or incomplete product acceptance contract. The current prototype supplies a partial baseline; this issue does not claim the selected workflow is already qualified.

**Scope:**

Define product-level channel/master meters, headroom, loudness analysis, clipping/limiter policy and headphone monitoring. Coordinate with #24 so playback and export processing differences are explicit.

Release placement is a planning target. DJ-044 records the selected feature subset; any deferred portion stays explicitly linked. Independent design/test preparation may proceed before dependencies close. Selected implementation issues require working behavior and evidence, not only a design document.

**Acceptance:**

- [ ] Document the signal path, meter tap points, units, peak hold/clipping indicators and reference levels; gain/normalization state is visible and persisted where it changes the master sound.
- [ ] Select loudness/auto-gain and limiter behavior, including bypass, latency, tails and whether export applies different processing. Analysis must never silently change a playing track's gain.
- [ ] Implement the selected cue selection/mix/level behavior with clear master-versus-headphone state; verify cue does not enter master export and hardware output controls are not falsely captured.
- [ ] Use known-level and overlapping-source fixtures to check meter readings, gain/EQ/filter headroom, clipping, limiter behavior and normalization consistency.
- [ ] Listen and measure live/offline output with explicit settings; record peak/loudness/latency/CPU results and unsupported paths. Preserve source files and source-level metadata provenance.

**Dependencies:**

- [ ] [#7 · Qualify two-source mixing, gain and master/cue semantics](https://github.com/coder13/Fader/issues/7)
- [ ] [#21 · Capture EQ, filters, loops, cues, and jog trajectories](https://github.com/coder13/Fader/issues/21)
- [ ] [#44 · Define the native v1 feature and release contract](https://github.com/coder13/Fader/issues/44)

## M4 · Product alpha: reliable Mac workflow

### DJ-045 Complete arrangement editing and navigation workflows

GitHub: [#45](https://github.com/coder13/Fader/issues/45). Labels: `area:editor`, `phase:product`, `type:epic`.

**Current evidence:**

The completeness review found a missing or incomplete product acceptance contract. The current prototype supplies a partial baseline; this issue does not claim the selected workflow is already qualified.

**Scope:**

Add the selected everyday editing tools beyond fixed-duration clip movement: split, reversible trim, source slip, duplicate/copy/paste, multiselect and explicit transition groups, markers/regions, fades and practical timeline navigation.

Release placement is a planning target. DJ-044 records the selected feature subset; any deferred portion stays explicitly linked. Independent design/test preparation may proceed before dependencies close. Selected implementation issues require working behavior and evidence, not only a design document.

**Acceptance:**

- [ ] Document and implement each selected gesture with discoverable commands plus keyboard/numeric alternatives; ordinary horizontal/vertical movement never resizes a clip or alters its local automation.
- [ ] Split/trim/slip/duplicate operations preserve source identity, envelope time basis, hidden data needed for reversible extension and stable IDs for independent copies; define boundary interpolation and fade behavior.
- [ ] Move/copy a multiclipped transition with a preview of included clips/curves and any bus/master data left behind; one undo restores all affected objects. Overlap does not implicitly group material.
- [ ] Provide selected marker/region and navigation commands, fit selection/set, zoom/scroll and beat/bar/off snapping; practice-loop, retake and export ranges remain distinct.
- [ ] Verify edit → audition → undo/redo → save/reopen → export on synthetic overlapping clips and a dense 60-minute arrangement. Qualify CPU/UI responsiveness and unsupported-overlap feedback within declared limits.
- [ ] Keep ripple/time-stretch gestures out of ordinary move/trim. If excluded from v1, record the explicit decision and later issue rather than claiming support.

**Dependencies:**

- [ ] [#14 · Finish arrangement editing affordances without resizing clips](https://github.com/coder13/Fader/issues/14)
- [ ] [#20 · Add project tempo maps and musical anchors](https://github.com/coder13/Fader/issues/20)
- [ ] [#44 · Define the native v1 feature and release contract](https://github.com/coder13/Fader/issues/44)

### DJ-046 Add precise automation editing and take comparison

GitHub: [#46](https://github.com/coder13/Fader/issues/46). Labels: `area:automation`, `phase:product`, `type:epic`.

**Current evidence:**

The completeness review found a missing or incomplete product acceptance contract. The current prototype supplies a partial baseline; this issue does not claim the selected workflow is already qualified.

**Scope:**

Extend point editing with exact values, selected-point/curve copying and scaling, explicit interpolation, safe dense-curve reduction and comparison with the original performance take. Reuse #21 for touch/latch engine semantics.

Release placement is a planning target. DJ-044 records the selected feature subset; any deferred portion stays explicitly linked. Independent design/test preparation may proceed before dependencies close. Selected implementation issues require working behavior and evidence, not only a design document.

**Acceptance:**

- [ ] Edit parameter values and time positions numerically with units, valid ranges and an explicit mute representation; keyboard and pointer edits use the same undoable commands.
- [ ] Copy/paste, offset or scale selected automation without changing unrelated clips/parameters or silently changing step-target capture into ramps; define clipping, baseline and point-collision behavior.
- [ ] Display interpolation and ownership; hiding or revealing curves never changes arming, Read/Override/Record or transport.
- [ ] Any optional simplification runs off the audio callback with declared parameter/audio error bounds, preserves endpoints and sharp gestures, and can restore the original take.
- [ ] Audition original versus edited/retaken versions with common range/level conditions; keep/discard and undo preserve the original evidence.
- [ ] Save/reopen, nonzero seek and offline render fixtures confirm chosen edits and take selection survive and unrelated stored data remains unchanged.

**Dependencies:**

- [ ] [#15 · Qualify selective retakes, override and return boundaries](https://github.com/coder13/Fader/issues/15)
- [ ] [#21 · Capture EQ, filters, loops, cues, and jog trajectories](https://github.com/coder13/Fader/issues/21)
- [ ] [#44 · Define the native v1 feature and release contract](https://github.com/coder13/Fader/issues/44)

### DJ-051 Add persistent playlists, crates and library organization

GitHub: [#51](https://github.com/coder13/Fader/issues/51). Labels: `area:library`, `phase:product`, `type:epic`.

**Current evidence:**

The completeness review found a missing or incomplete product acceptance contract. The current prototype supplies a partial baseline; this issue does not claim the selected workflow is already qualified.

**Scope:**

Add Fader-owned organization on top of persistent indexing and read-only Traktor import: selected playlists/crates, play queue/history, ratings/tags and duplicate/missing-source handling.

Release placement is a planning target. DJ-044 records the selected feature subset; any deferred portion stays explicitly linked. Independent design/test preparation may proceed before dependencies close. Selected implementation issues require working behavior and evidence, not only a design document.

**Acceptance:**

- [ ] Create, rename, reorder and remove selected playlist/crate structures; distinguish removing a reference from deleting source audio. Original music and Traktor files remain unchanged.
- [ ] Persist ratings/tags, queue and history where selected; provide search/filter/sort and explicit provenance/conflict rules for imported versus Fader-owned metadata.
- [ ] Keep stable identity through rescan, path relocation, reimport and duplicate files; never merge distinct tracks by title alone. Handle missing entries visibly.
- [ ] Define preview routing and preview-versus-live-deck state; long organization/index operations stay cancellable and off audio/UI-critical paths.
- [ ] Verify restart, cache rebuild, interrupted save and realistic library-scale scenarios preserve organization and project edits; publish synthetic fixtures and aggregate private-library results.

**Dependencies:**

- [ ] [#23 · Add long-library caching and analysis jobs](https://github.com/coder13/Fader/issues/23)
- [ ] [#43 · Complete read-only Traktor library and playlist import](https://github.com/coder13/Fader/issues/43)
- [ ] [#44 · Define the native v1 feature and release contract](https://github.com/coder13/Fader/issues/44)

### DJ-053 Define and implement set delivery exports beyond the master file

GitHub: [#53](https://github.com/coder13/Fader/issues/53). Labels: `area:storage`, `phase:product`, `type:epic`.

**Current evidence:**

The completeness review found a missing or incomplete product acceptance contract. The current prototype supplies a partial baseline; this issue does not claim the selected workflow is already qualified.

**Scope:**

Select and implement useful delivery outputs: tracklist/cue sheet, chapter/region metadata, individual region renders and separate lane/stem renders. Reuse codec/export machinery; rendered stems are distinct from source separation.

Release placement is a planning target. DJ-044 records the selected feature subset; any deferred portion stays explicitly linked. Independent design/test preparation may proceed before dependencies close. Selected implementation issues require working behavior and evidence, not only a design document.

**Acceptance:**

- [ ] Record which delivery outputs belong in v1 versus later releases, with format, timing/rounding, metadata, naming and compatibility contracts.
- [ ] For selected tracklist/chapter/cue outputs, derive timings from the edited arrangement and represent repeats, overlaps and tempo changes unambiguously; use synthetic public track metadata.
- [ ] For selected range or lane renders, define alignment, pre/post-fader and effects routing, shared buses, tails and normalization. Do not promise that separately nonlinear-processed outputs sum to the master.
- [ ] Keep loop/retake/export ranges distinct. Preview destinations and overwrite behavior; cancellation or disk failure preserves existing valid outputs and reports incomplete jobs.
- [ ] Qualify duration, boundaries, channel counts, tags, reopen/portability and declared tolerances against the master render on Mac/Windows. Link deferred outputs rather than silently omitting them.

**Dependencies:**

- [ ] [#13 · Qualify offline rendering against live-core playback](https://github.com/coder13/Fader/issues/13)
- [ ] [#24 · Add MP3 and FLAC export presets](https://github.com/coder13/Fader/issues/24)
- [ ] [#25 · Package portable sets and robust recovery](https://github.com/coder13/Fader/issues/25)
- [ ] [#44 · Define the native v1 feature and release contract](https://github.com/coder13/Fader/issues/44)

### DJ-054 Run target-user usability rounds and retest blocking findings

GitHub: [#54](https://github.com/coder13/Fader/issues/54). Labels: `area:editor`, `phase:product`, `type:quality`.

**Current evidence:**

The completeness review found a missing or incomplete product acceptance contract. The current prototype supplies a partial baseline; this issue does not claim the selected workflow is already qualified.

**Scope:**

Extend the owner session with formative testing by approximately five target users spanning DJ-first and DAW-familiar experience, then fix and retest serious problems. Use the current design and accessibility work; repeat affected tasks after subsequent UI changes.

Release placement is a planning target. DJ-044 records the selected feature subset; any deferred portion stays explicitly linked. Independent design/test preparation may proceed before dependencies close. Selected implementation issues require working behavior and evidence, not only a design document.

**Acceptance:**

- [ ] Set task definitions and success/time/error goals before the sessions. Test import/search, overlap preparation, automation edit, record/retake/return, move a transition, undo, reopen/recover and export without coaching.
- [ ] Include first use and a dense 60-minute project; record first-attempt completion, assistance, accidental edits, mode/ownership misunderstandings and timing separately from setup time.
- [ ] Collect visual preference and density/readability feedback separately from task performance; distinguish owner approval from evidence across unfamiliar users.
- [ ] Publish anonymized findings with severity, exact build/configuration and linked fixes; obtain consent for retained recordings and keep private track details local.
- [ ] Resolve and retest all findings that lose work, hide capture failure or prevent a core task on a supported configuration. Report remaining nonblocking issues and sample-size limits.
- [ ] Maintain build-specific evidence for the Mac alpha and final cross-platform release; issue closure must include repairs/retests, not only a list of newly discovered problems.

**Dependencies:**

- [ ] [#42 · Run the owner prepared-set usability session](https://github.com/coder13/Fader/issues/42)
- [ ] [#45 · Complete arrangement editing and navigation workflows](https://github.com/coder13/Fader/issues/45)
- [ ] [#46 · Add precise automation editing and take comparison](https://github.com/coder13/Fader/issues/46)
- [ ] [#51 · Add persistent playlists, crates and library organization](https://github.com/coder13/Fader/issues/51)

### DJ-055 Compare arrangement layouts and approve the visual design system

GitHub: [#55](https://github.com/coder13/Fader/issues/55). Labels: `area:editor`, `phase:product`, `type:epic`.

**Current evidence:**

The completeness review found a missing or incomplete product acceptance contract. The current prototype supplies a partial baseline; this issue does not claim the selected workflow is already qualified.

**Scope:**

Build on the existing flat dark theme. Compare two or three arrangement-first layout variants using the same music/automation/tasks, then document and implement the selected hierarchy and visual system.

Release placement is a planning target. DJ-044 records the selected feature subset; any deferred portion stays explicitly linked. Independent design/test preparation may proceed before dependencies close. Selected implementation issues require working behavior and evidence, not only a design document.

**Acceptance:**

- [ ] Compare browser/inspector placement, selected-transition controls and automation density using identical synthetic projects; preserve transport, capture, routing and ownership semantics between variants.
- [ ] Review empty first run, two-track overlap, dense 60-minute set, selected curves, Read/Override/Record, retake range, missing media, export, Preferences and disconnected-controller states.
- [ ] Record task evidence and owner visual preferences separately; document the chosen layout and reasons without claiming an objectively best possible design.
- [ ] Define shared typography/spacing, semantic colors, icons, focus/selection/disabled/error/recording states and control hierarchy; keep frequent actions discoverable and critical state visible.
- [ ] Implement the approved direction and keep representative screenshot baselines at declared sizes/scales; check visual regressions with synthetic media.
- [ ] Record explicit owner visual approval and unresolved polish items. Approval does not replace accessibility, usability or audio qualification.

**Dependencies:**

- [ ] [#42 · Run the owner prepared-set usability session](https://github.com/coder13/Fader/issues/42)

### DJ-056 Qualify responsive layout, keyboard editing and accessibility

GitHub: [#56](https://github.com/coder13/Fader/issues/56). Labels: `area:editor`, `phase:product`, `type:quality`.

**Current evidence:**

The completeness review found a missing or incomplete product acceptance contract. The current prototype supplies a partial baseline; this issue does not claim the selected workflow is already qualified.

**Scope:**

Qualify the full core workflow across supported logical window sizes, display scales and access methods. The documented 1250 × 920 prototype minimum and incomplete keyboard envelope editing are starting evidence, not the desired final support boundary.

Release placement is a planning target. DJ-044 records the selected feature subset; any deferred portion stays explicitly linked. Independent design/test preparation may proceed before dependencies close. Selected implementation issues require working behavior and evidence, not only a design document.

**Acceptance:**

- [ ] Declare supported sizes/scales before testing; evaluate 1280 × 720, 1440 × 900 and a large display, text scaling and bright/dim use, recording justified support limits rather than silently clipping controls.
- [ ] Core import, selection/move, numeric envelope editing, point deletion, menus, retake/override, save/reopen and export work keyboard-only with visible focus, predictable order and no traps.
- [ ] Verify accessible names/roles/values and state announcements with named screen readers on supported platforms; compare with pointer operation and preserve text-entry shortcuts.
- [ ] Meet declared text/control contrast targets and reinforce selection, armed/override/cue/errors without color alone; verify scaled labels/targets and alternative controls for small clips.
- [ ] Critical recording, missing-media and routing state remains visible when panels collapse or layout changes; workspace preferences never change musical state or undo history.
- [ ] Fix and retest blocking problems on exact builds; attach the size/platform/input matrix and explicit remaining limitations for #26 and the final release gate.

**Dependencies:**

- [ ] [#14 · Finish arrangement editing affordances without resizing clips](https://github.com/coder13/Fader/issues/14)
- [ ] [#55 · Compare arrangement layouts and approve the visual design system](https://github.com/coder13/Fader/issues/55)

## M5 · Product v1: Windows and more controllers

### DJ-057 Gate native v1 on complete Mac and Windows product workflows

GitHub: [#57](https://github.com/coder13/Fader/issues/57). Labels: `area:platform`, `phase:product`, `type:quality`.

**Current evidence:**

The completeness review found a missing or incomplete product acceptance contract. The current prototype supplies a partial baseline; this issue does not claim the selected workflow is already qualified.

**Scope:**

Make the explicit product release decision after the selected M3/M4 workflows are implemented and qualified on both native platforms. POC success and Windows compilation alone do not qualify a finished release.

Release placement is a planning target. DJ-044 records the selected feature subset; any deferred portion stays explicitly linked. Independent design/test preparation may proceed before dependencies close. Selected implementation issues require working behavior and evidence, not only a design document.

**Acceptance:**

- [ ] Freeze the scoped feature/support matrix from DJ-044 and map every promised capability to its implementation and current evidence; record any explicitly deferred optional subset with a linked backlog item.
- [ ] On declared Mac and Windows builds, complete a realistic 60-minute prepared-set workflow: organize/import, grid/sync, arrange, control/capture, edit/retake, recover/relink, reopen without controller and deliver the selected export outputs.
- [ ] Repeat relevant streaming, seek, capture, render parity, latency/quality and failure tests with all selected features enabled; set budgets/tolerances before runs and report regressions and supported fallbacks.
- [ ] Verify clean install, permissions, update/rollback or recovery, project migration and support diagnostics; retain license/notices, quickstart/release notes and exact device/OS/plugin-free baseline support details.
- [ ] Require fresh usability, visual-approval and accessibility evidence on the release behavior; no unresolved issue may lose work, conceal capture failure or block a core workflow on an advertised configuration.
- [ ] Publish a go/no-go report with linked evidence and known nonblocking limitations. Leave the gate open on failure or missing evidence; optional web/Linux/integrations and VST3 v2/v3 do not become implicit v1 prerequisites.

**Dependencies:**

- [ ] [#17 · Decide whether the Mac/S4 POC is ready for product development](https://github.com/coder13/Fader/issues/17)
- [ ] [#26 · Ship and validate a Mac alpha](https://github.com/coder13/Fader/issues/26)
- [ ] [#27 · Qualify Windows audio and S4 integration](https://github.com/coder13/Fader/issues/27)
- [ ] [#28 · Add generic MIDI mapping and a second controller](https://github.com/coder13/Fader/issues/28)
- [ ] [#44 · Define the native v1 feature and release contract](https://github.com/coder13/Fader/issues/44)
- [ ] [#45 · Complete arrangement editing and navigation workflows](https://github.com/coder13/Fader/issues/45)
- [ ] [#46 · Add precise automation editing and take comparison](https://github.com/coder13/Fader/issues/46)
- [ ] [#47 · Specify and qualify sync, grid correction and quantized actions](https://github.com/coder13/Fader/issues/47)
- [ ] [#48 · Define and qualify hotcue, loop, beat-jump and jog behavior](https://github.com/coder13/Fader/issues/48)
- [ ] [#49 · Add harmonic preparation and explicit key-shift behavior](https://github.com/coder13/Fader/issues/49)
- [ ] [#50 · Protect live deck loading and controller-to-clip assignment](https://github.com/coder13/Fader/issues/50)
- [ ] [#51 · Add persistent playlists, crates and library organization](https://github.com/coder13/Fader/issues/51)
- [ ] [#52 · Qualify mix metering, gain staging and cue controls](https://github.com/coder13/Fader/issues/52)
- [ ] [#53 · Define and implement set delivery exports beyond the master file](https://github.com/coder13/Fader/issues/53)
- [ ] [#54 · Run target-user usability rounds and retest blocking findings](https://github.com/coder13/Fader/issues/54)
- [ ] [#55 · Compare arrangement layouts and approve the visual design system](https://github.com/coder13/Fader/issues/55)
- [ ] [#56 · Qualify responsive layout, keyboard editing and accessibility](https://github.com/coder13/Fader/issues/56)

## Optional future decisions · no release milestone

### DJ-058 Decide whether to add stem separation and stem performance

GitHub: [#58](https://github.com/coder13/Fader/issues/58). Labels: `area:audio`, `phase:product`, `type:spike`.

**Current evidence:**

The review identified an optional future direction without a standalone decision. No implementation, access or hardware support is implied.

**Scope:**

Evaluate source separation as an optional DJ workflow independently of lane/stem export. This is a decision issue, not a v1 implementation promise.

This issue closes with an evidence-backed adopt/defer/reject decision. Implementation requires separately scoped issues if adopted. It is not a native v1 or POC blocker.

**Acceptance:**

- [ ] Define the desired preparation/live-control workflow and representative inputs; compare precomputed versus real-time separation with measured quality, resource use, latency and failure behavior.
- [ ] Specify source/model provenance, cache invalidation, editable stem controls, portability and deterministic replay/export requirements for any selected route.
- [ ] Check current model/dependency and distribution constraints before adoption; use redistributable fixtures and aggregate private-library results.
- [ ] Record adopt/defer/reject with reasons; if adopted, publish a bounded design and implementation/qualification issues before assigning a release milestone.

**Dependencies:**

No blocking implementation dependency. Use the published review and current product evidence.

### DJ-059 Decide whether to add DVS and timecode control

GitHub: [#59](https://github.com/coder13/Fader/issues/59). Labels: `area:controller`, `phase:product`, `type:spike`.

**Current evidence:**

The review identified an optional future direction without a standalone decision. No implementation, access or hardware support is implied.

**Scope:**

Evaluate DVS/timecode as an optional control path with its own hardware and capture model, independent of S4 HID or ordinary MIDI support.

This issue closes with an evidence-backed adopt/defer/reject decision. Implementation requires separately scoped issues if adopted. It is not a native v1 or POC blocker.

**Acceptance:**

- [ ] Name the intended timecode media, audio interface, control modes and user workflow; distinguish playback capability from reproducible editable capture.
- [ ] Measure calibration, latency, drift, signal loss and recovery on explicitly available hardware before making support claims; record unknowns if hardware is unavailable.
- [ ] Specify how applied source trajectories and mode changes survive capture, seek/replay and offline export without requiring live timecode.
- [ ] Record adopt/defer/reject, current integration constraints and estimated maintenance surface; adoption requires separate implementation and physical qualification issues.

**Dependencies:**

No blocking implementation dependency. Use the published review and current product evidence.

### DJ-060 Decide whether to integrate streaming music services

GitHub: [#60](https://github.com/coder13/Fader/issues/60). Labels: `area:library`, `phase:product`, `type:spike`.

**Current evidence:**

The review identified an optional future direction without a standalone decision. No implementation, access or hardware support is implied.

**Scope:**

Evaluate optional streaming-library workflows separately from local-file import. No service, account dependency, offline access or export entitlement is assumed.

This issue closes with an evidence-backed adopt/defer/reject decision. Implementation requires separately scoped issues if adopted. It is not a native v1 or POC blocker.

**Acceptance:**

- [ ] Identify candidate provider workflows and verify current official API, access, account, caching and recording/export terms at evaluation time.
- [ ] Specify authentication, unavailable tracks, changed catalog versions, network failure, source identity and project portability behavior.
- [ ] State exactly which actions remain editable/replayable/exportable when media cannot be retained; do not present a successful portable set when access is missing.
- [ ] Record provider-specific adopt/defer/reject decisions. Any adoption gets bounded implementation, privacy/data-handling and qualification issues before a release commitment; preserve local-only operation.

**Dependencies:**

No blocking implementation dependency. Use the published review and current product evidence.

### DJ-061 Decide whether to add project collaboration or cloud sharing

GitHub: [#61](https://github.com/coder13/Fader/issues/61). Labels: `area:storage`, `phase:product`, `type:spike`.

**Current evidence:**

The review identified an optional future direction without a standalone decision. No implementation, access or hardware support is implied.

**Scope:**

Evaluate optional collaboration/sharing against the existing portable-set workflow. Do not add mandatory accounts or cloud storage to local editing.

This issue closes with an evidence-backed adopt/defer/reject decision. Implementation requires separately scoped issues if adopted. It is not a native v1 or POC blocker.

**Acceptance:**

- [ ] Define the selected problem: sharing a portable file, asynchronous review or concurrent editing; compare it with the existing collect-media bundle.
- [ ] Specify ownership/access, media availability, privacy/deletion, project versioning, conflict handling, undo and offline recovery for any selected mode.
- [ ] Test a bounded synthetic two-user scenario including conflicting edits and interrupted connectivity; preserve deterministic render semantics and valid local work.
- [ ] Record adopt/defer/reject and cost/maintenance implications; adoption requires a separate architecture decision and implementation/qualification issues.

**Dependencies:**

No blocking implementation dependency. Use the published review and current product evidence.

### DJ-062 Decide whether to capture microphones and external audio

GitHub: [#62](https://github.com/coder13/Fader/issues/62). Labels: `area:audio`, `phase:product`, `type:spike`.

**Current evidence:**

The review identified an optional future direction without a standalone decision. No implementation, access or hardware support is implied.

**Scope:**

Evaluate optional microphone/line input, external synth or hardware-effect return capture for DJ sets. This does not expand Fader into a studio multitrack DAW.

This issue closes with an evidence-backed adopt/defer/reject decision. Implementation requires separately scoped issues if adopted. It is not a native v1 or POC blocker.

**Acceptance:**

- [ ] Define required input/routing/monitoring modes and selected channels; prevent feedback paths and distinguish direct hardware monitoring from the captured signal.
- [ ] Specify sample-clock alignment, latency compensation, recording assets, dropouts and interrupted-take recovery; external sound must be recorded as media for portable replay/export.
- [ ] Run a bounded available-device or loopback experiment and label hardware-dependent gaps; compare recorded alignment with the live reference.
- [ ] Record adopt/defer/reject and selected limits. Adoption requires separate input-capture, routing, persistence and hardware-qualification issues.

**Dependencies:**

No blocking implementation dependency. Use the published review and current product evidence.

### DJ-063 Decide whether to add sample decks or one-shot performance

GitHub: [#63](https://github.com/coder13/Fader/issues/63). Labels: `area:audio`, `phase:product`, `type:spike`.

**Current evidence:**

The review identified an optional future direction without a standalone decision. No implementation, access or hardware support is implied.

**Scope:**

Evaluate optional samples, jingles and one-shot/loop launching as a DJ workflow without adding a general instrument or MIDI-composition subsystem.

This issue closes with an evidence-backed adopt/defer/reject decision. Implementation requires separately scoped issues if adopted. It is not a native v1 or POC blocker.

**Acceptance:**

- [ ] Specify useful slot count, trigger/gate/loop behavior, launch quantization, choke/voice rules and controller interaction; set voice/CPU/memory limits.
- [ ] Define asset identity, source trajectory, gain/routing and accepted trigger capture so a performance can be edited, sought, reopened and exported.
- [ ] Prototype a bounded synthetic launch scenario including rapid triggers, stop/restart and missing media; report resource/latency and usability results.
- [ ] Record adopt/defer/reject with reasons; adoption requires independent implementation and capture/render qualification issues.

**Dependencies:**

No blocking implementation dependency. Use the published review and current product evidence.

### DJ-064 Decide built-in performance effects and extended mix routing

GitHub: [#64](https://github.com/coder13/Fader/issues/64). Labels: `area:plugins`, `phase:product`, `type:spike`.

**Current evidence:**

The review identified an optional future direction without a standalone decision. No implementation, access or hardware support is implied.

**Scope:**

Select any future built-in DJ effects, group/return buses, sends and sidechains separately from the confirmed VST3 effects plan in #33/#34. Do not move VST3 into the POC or make these choices v1 blockers.

This issue closes with an evidence-backed adopt/defer/reject decision. Implementation requires separately scoped issues if adopted. It is not a native v1 or POC blocker.

**Acceptance:**

- [ ] Document concrete DJ workflows and choose built-in effects/routing features only where needed; compare with the planned clip/lane/master VST3 racks.
- [ ] Specify effect/route identity, order, pre/post-fader taps, automation ownership, feedback restrictions, latency compensation, bypass and bounded tails for the selected graph.
- [ ] Define capture, seek/preroll, missing-effect/frozen fallback and offline export behavior; distinguish sum-to-master expectations from nonlinear or shared-bus processing.
- [ ] Record adopt/defer/reject per feature with current dependency/licensing findings. Adoption gets individual design, implementation and sound/reliability qualification issues; retain #33/#34 as the plugin release owners.

**Dependencies:**

No blocking implementation dependency. Use the published review and current product evidence.
