# Roadmap completeness and design-evidence review

Reviewed 2026-11-04. **Evidence:** live GitHub bodies for all 43 issues, their 10 attached milestones, the design documents on `main` at `ea3e246`, and the linked prototype reports at `3a3b58456065f6cfed4c9f23f1a0252047d80a89`. All 43 issues were open; all 10 milestones had zero closed issues. Open status does not mean no implementation exists. This review inspected plans and recorded evidence; it did not rerun the app, audio tests or user sessions.

**Confirmed by the user during this review:** Fader is a **DJ performance editor with selected DAW tools**. It is not currently scoped as a full music-production DAW with MIDI composition, instruments and multitrack recording. This clarification belongs to the product requirements. At the time of review, the additions below were **proposals**, not accepted implementation scope or measured results. The user subsequently authorized issue creation; the [21 published follow-ups](../product-completion-backlog.md) now track them. DJ-044 selects the release subset; optional features remain decisions rather than commitments.

## Assessment

The backlog provides a credible engineering path to the central promise: arrange → perform → inspect → edit → retake → reopen → export. It does not yet specify a complete, polished release. Coverage is strongest for audio correctness, automation ownership, persistence, hardware qualification and platform risk. Everyday editing, detailed DJ behavior, library organization, release-wide acceptance and visual/usability validation need more explicit treatment.

No finite issue list proves that a layout is the best possible. The existing issues can produce evidence of task success, fewer errors and visual preference if their tests are performed. They do not currently require enough evidence to support a claim of exceptional usability or visual polish.

## What the milestones deliver

| Milestones | Planned result | Review |
| --- | --- | --- |
| M0–M2, 24 issues | Architecture experiments, streaming/S4 core, editable-set POC and qualification | Strong risk-first plan. [#17](https://github.com/coder13/Fader/issues/17) is a substantive POC exit gate. |
| M3, 5 issues | Beat grids, stretch, tempo, EQ/cues/loops/jog, MIDI clock | Right capabilities, but broad epics need explicit user-facing behavior and smaller acceptance tasks. |
| M4, 6 issues | Library, broader import, exports, recovery and packaged Mac alpha | Good product foundation. Library organization and complete editing workflows are less specified than decoding and persistence. |
| M5, 2 issues | Windows qualification and another controller | Platform expansion, not a complete cross-platform v1 release gate. |
| M6–M7, 4 issues | Conditional web delivery, four decks/deeper S4 and Linux | Independent later tracks; milestone numbering must not imply all are prerequisites for v1 or VST3. |
| M8–M9, 2 issues | VST3 effects, then resilience/freeze/portability | Correctly preserves the confirmed version 2/3 requirement. Effects hosting does not imply instruments or MIDI composition. |

The existing [roadmap](../roadmap-and-issue-backlog.md) deliberately describes later work as epics to split when selected. That is reasonable. The gap is not the number of issues; it is whether every promised workflow has a scoped owner, closure evidence and a release gate.

## Capability coverage

“Covered” means planned explicitly, not completed. “Partial” means a related issue or design exists but does not fully define the workflow. “Unscoped” means no explicit acceptance contract was found across the reviewed issues.

| Capability | Current coverage | Remaining decision or acceptance gap |
| --- | --- | --- |
| Editable performance, clip-owned volume/HP/LP, selective retakes | Covered: [#11](https://github.com/coder13/Fader/issues/11), [#12](https://github.com/coder13/Fader/issues/12), [#14](https://github.com/coder13/Fader/issues/14), [#15](https://github.com/coder13/Fader/issues/15) | Strong central workflow; hardware and full qualification remain open. |
| Long playback, fast seek, offline parity, recovery | Covered: [#13](https://github.com/coder13/Fader/issues/13), [#16](https://github.com/coder13/Fader/issues/16), [#38](https://github.com/coder13/Fader/issues/38)–[#40](https://github.com/coder13/Fader/issues/40) | Preserve these immediate priorities. Short software tests do not replace long-run/device measurements. |
| Practical arrangement editing | Partial: #14 plus proposed editing rules | Explicit split, reversible trim, source slip, duplicate/copy/paste, multiselect, transition grouping, markers/regions, fades, zoom-to-selection and musical snap contracts. Select the needed subset; keep movement distinct from resizing. Ripple editing can remain later. |
| Automation editing beyond basic points | Partial: #14, #15, [#21](https://github.com/coder13/Fader/issues/21) | Numeric precision, copy/scale curves, interpolation display, density reduction and take comparison need scoped decisions. Touch/latch is named in #21, not absent. |
| BPM, grids, key lock, tempo maps | Covered: [#18](https://github.com/coder13/Fader/issues/18)–[#20](https://github.com/coder13/Fader/issues/20) | Specify sync leader/follower and rejoin behavior, half/double tempo, variable-grid correction and launch quantization. Key lock does not establish independent key shifting or key analysis. |
| Cues, loops, jog and reverse | Partial: #21 | Define hotcue storage/naming, saved loops, loop size/move, beat jump, quantize, slip and scratch/jog policies individually. Capture/seek/export must reproduce each supported action. Not every mode must ship in v1. |
| Gain, EQ, monitoring and output quality | Partial: [#7](https://github.com/coder13/Fader/issues/7), #21, [#24](https://github.com/coder13/Fader/issues/24) | Gain staging, channel/master meters, loudness analysis, headroom, clipping/limiter policy and cue mix need explicit product contracts. Normalization/dither are already named in #24. |
| Library discovery and import | Covered: [#23](https://github.com/coder13/Fader/issues/23), [#35](https://github.com/coder13/Fader/issues/35), [#36](https://github.com/coder13/Fader/issues/36), [#43](https://github.com/coder13/Fader/issues/43) | Persistent Fader playlists/crates, queue/history, ratings/tags, duplicate handling and organization edits are unscoped. Importing Traktor playlists is not the same as managing Fader playlists. Keep source files read-only. |
| Harmonic preparation | Partial: imported key metadata and architecture references to analysis | Key detection/correction, notation, compatible-key search and independent transposition need explicit selection and tests if wanted. Imported key labels do not prove these capabilities. |
| S4, MIDI mappings and platform support | Covered: [#2](https://github.com/coder13/Fader/issues/2), [#9](https://github.com/coder13/Fader/issues/9), [#27](https://github.com/coder13/Fader/issues/27), [#28](https://github.com/coder13/Fader/issues/28), [#31](https://github.com/coder13/Fader/issues/31), [#41](https://github.com/coder13/Fader/issues/41) | Explicit playing-deck load protection, deck-to-clip reassignment feedback and supported performance controls. A crossfader mapping, if selected, must preserve independent clip envelopes. |
| Export and portability | Covered foundation: #13, #24, [#25](https://github.com/coder13/Fader/issues/25) | Decide whether tracklist/cue-sheet, chapter markers, individual regions or separate lane/stem renders are needed. Stem export is distinct from source separation. |
| Effects and routing | Covered VST3 plan: [#33](https://github.com/coder13/Fader/issues/33), [#34](https://github.com/coder13/Fader/issues/34) | Built-in performance effects, group/return buses and sidechains are not fully promised. Sends/sidechains are explicitly optional in #34. Keep VST3 in v2/v3. |
| Microphone, external audio and samplers | Unscoped implementation; reproducibility constraint appears in design | Optional later product choices. External input must become a recorded asset if it should survive replay/export. |
| Stems, DVS, streaming, collaboration | Named as optional proposals in [#32](https://github.com/coder13/Fader/issues/32) | Mentions do not constitute designed features. Give each a separate decision and acceptance contract only if selected. |
| MIDI composition, piano roll, instruments, studio multitrack recording/comping | Outside the confirmed product direction | Do not add as completion blockers merely to match a general DAW. External MIDI output in [#10](https://github.com/coder13/Fader/issues/10)/[#22](https://github.com/coder13/Fader/issues/22) does not imply these features. |

The feature prompts above were cross-checked against the primary [Mixxx 2.5 interface manual](https://manual.mixxx.org/2.5/en/chapters/user_interface.html), which distinguishes key controls, quantize/slip, hotcues, loops, gain, samplers and monitoring. The [Ableton Live 12 manual](https://www.ableton.com/en/live-manual/12/welcome-to-live/) separates arrangement/audio editing, MIDI composition, recording, comping and routing. These references help identify separate workflows; their feature lists do not define Fader's requirements.

## Specific planning defects

1. **No complete native v1 exit gate.** [#26](https://github.com/coder13/Fader/issues/26) gates a Mac alpha. #27 explicitly reruns the critical M2 workflow on Windows. M5 has only Windows and controller issues. Add a product-wide release decision that covers the selected M3/M4 features on both supported platforms, realistic projects, installation/update, recovery, accessibility, sound quality and usability. Do not treat passing M2 on Windows as qualifying every later feature.
2. **Some daily workflows lack acceptance owners.** #14 deliberately stays near the existing POC editor; it is not a commitment to all future arrangement operations. #23 primarily addresses indexing/cache/jobs; #43 imports Traktor data. Broader editing and organization need bounded product issues if selected.
3. **Design documents disagree about the current model.** At the reviewed baseline, `product-and-ux.md` still described two fixed deck lanes and a gain/crossfader POC; `set-format-and-automation.md` still called POC mixer automation bus-owned. README and #7/#9/#11/#14 require clip-owned volume/HP/LP on extensible rows. This review's documentation update corrects those current-state statements and distinguishes future proposals. Historical research remains historical evidence.
4. **UI scope is broader than UI acceptance.** #26 names screen readers, focus and scaling, but its sole acceptance bullet principally checks install/open/save/export/update. #42 records confusion and creates follow-ups; it does not require that all blocking findings be repaired and retested. Give the release gate explicit evidence and blocker-closure requirements.

## What the layout evidence does and does not show

There is already a documented visual system. The pinned [theme report](https://github.com/coder13/Fader/blob/3a3b58456065f6cfed4c9f23f1a0252047d80a89/docs/theme-and-interaction.md) records shared colors, flat controls, menus, focus outlines, explicit performance states and native inspection. It also records a **1250 × 920 minimum window**, incomplete keyboard envelope editing and unqualified smaller-screen/accessibility behavior. Those are concrete targets for follow-up, not evidence that the UI is bad on every device.

The [prototype experiment](https://github.com/coder13/Fader/blob/3a3b58456065f6cfed4c9f23f1a0252047d80a89/docs/experiments/2026-11-04-clip-arrangement.md) records handler tests and native inspection, while explicitly leaving an owner mouse/trackpad session open. Tests of movement correctness cannot establish whether a new user discovers the gesture. The [UI atlas](ui-evidence-atlas.md) and [Audacity research](audacity-4-design-lessons.md) provide hypotheses and references, not Fader user results.

[#5](https://github.com/coder13/Fader/issues/5) compares native/web interaction and maintenance cost. [#42](https://github.com/coder13/Fader/issues/42) tests one owner workflow. #26 includes later accessibility and release work. Together these are useful beginnings, but none requires comparing visual/layout alternatives, testing people unfamiliar with the product, or approving a polished design across representative states.

Proposed evidence before calling the UI polished:

- **Layout alternatives:** compare two or three arrangement-first variants using the same clips, automation and tasks. Test browser/inspector placement and selected-transition controls without changing the underlying musical semantics.
- **Representative states:** empty first run; two-track transition; a dense 60-minute set; selected/overlapping curves; record/override/retake; missing media; export; preferences; controller disconnected. Use synthetic public media.
- **Usability rounds:** start with #42, then a small formative round with approximately five target users spanning DJ-first and DAW-familiar experience. Record first-attempt completion, assistance, accidental edits, mode misunderstandings and timing. This is issue discovery, not statistical proof about all DJs. Fix the most serious findings and repeat the affected tasks.
- **Separate visual preference from task performance:** ask users and the owner to compare hierarchy, readability, density and visual appeal after doing the tasks. Record the chosen direction and reasons. A dark palette alone does not establish a polished product.
- **Display and access matrix:** declare supported logical window sizes/scales before testing; consider 1280 × 720, 1440 × 900 and a large display, plus text scaling and bright/dim environments. These are proposed test sizes, not a claim of present support. Verify keyboard-only editing, focus, screen-reader names and non-color state cues.
- **Proposed release rule:** zero unresolved findings that lose work, conceal capture failure or prevent a core task on a supported configuration. Set timing/completion goals before testing, preserve actual results, and rerun failures. Screenshot regressions support consistency but cannot replace task tests.

## Review recommendations and published follow-up

R1–R8 remain review IDs, not GitHub issue numbers. After this review, the user authorized publication of DJ-044–DJ-064. The [review-to-issue map and full acceptance criteria](../product-completion-backlog.md#review-to-issue-map) record the result. Existing epics now link to those issues, Mac/Windows release criteria are strengthened, and Linux qualification is separate from optional integrations. The immediate streaming/seeking/load work remains the current engineering priority.

| Review ID | Proposed change | Placement and completion evidence |
| --- | --- | --- |
| R1 | Define native v1 scope and add a release decision | Before promising v1: selected features and explicit exclusions, each mapped to issues; final integrated Mac/Windows scenario, installation/update/recovery and zero unresolved release blockers. Extend #26/#27 evidence into this gate. |
| R2 | Specify the selected arrangement-editing toolset | Product alpha: split/trim/source-slip, duplicate/copy/paste, multiselect/groups, markers, fades and navigation as selected. Move a transition without changing its sound; undo/save/reopen/export preserve the result. Record deferred operations explicitly. |
| R3 | Split musical-performance contracts from #18–#21 | M3: sync/quantize, hotcues/loops, jog/slip/beat-jump and harmonic preparation as selected. Each supported action gets UI/controller behavior plus capture, seek and export fixtures. |
| R4 | Add Fader library organization | M4: selected playlist/crate, queue/history and tag workflows persist across rescan/reopen; source files remain unchanged; imported and Fader-owned data have explicit provenance. |
| R5 | Specify levels, monitoring and delivery quality | Extend #7/#24 with product tasks: visible meters, clipping/headroom and normalization behavior, selected loudness/limiter policy and cue controls. Known-level fixtures and listening checks verify playback/export differences are intentional. |
| R6 | Add usability iteration beyond the owner session | Begin after #42; repeat before v1. Publish anonymized findings, severity, repairs and retests; core tasks must work without coaching on supported inputs. |
| R7 | Extend the visual system and qualify layouts | Build on the existing theme; compare variants, define spacing/type/icon/control-state rules, qualify small/dense/scaled/accessibility states, obtain owner visual approval and maintain representative screenshot checks. Feed #26 and R1. |
| R8 | Separate optional future product decisions | Keep stems/DVS/streaming/collaboration, external inputs/samplers, richer routing and four-deck performance individually selectable. Split Linux qualification from unrelated optional feature planning when scheduling. Preserve #33/#34 for VST3 v2/v3. |

The published scope issue DJ-044 must select the v1 subset before implementation commitments are finalized. A complete Fader release should let a target DJ finish, revise and deliver a real set confidently; it need not implement every feature found in either a DAW or a live DJ application.
