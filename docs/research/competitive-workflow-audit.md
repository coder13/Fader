# Editable DJ performance: competitive workflow audit

Research date: 2026-11-04. Status: **Research findings and proposed decision process**, not an accepted product pivot or a completed software evaluation.

## Conclusion

Fader's broad combination of a DJ mixer, timeline, automation, and export already exists in several forms. The remaining question is whether an existing application can support this user's **Mac + S4 MK3 → editable performance → selective correction → dependable saved/exported result** without excessive setup or maintenance.

The strongest next investigations are **GlideMix for arranger reuse** and **Mixxx for performance-engine reuse**. Density is the closest commercial example of reconstructing a hardware DJ performance. DJ.Studio is a strong benchmark for preparing and editing transitions. MixCanvas is a relevant open-source implementation, but its current platform, automation ownership, and plugin policy create material conflicts with Fader's requirements.

Do not start another general-purpose DJ engine on the strength of this research alone. First run the same bounded workflow through the strongest candidates. A personal solution using commercial software and an open-source Fader implementation are different decisions: the former could solve the user's immediate problem, but does not satisfy the project's confirmed open-source requirement.

## Scope and evidence quality

The user's attached conversation screenshots supplied the candidate list and earlier hypotheses. Their recommendations were treated as context to verify, not new instructions or accepted changes to requirements.

This pass read official manuals, product pages, repository documentation, releases, historical manuals, and public user discussions. It also visually inspected **33 published UI images/crops across nine products**, including two small animated REAPER examples. The [UI evidence atlas](ui-evidence-atlas.md) identifies every image, its source, and the observed interaction implications. Some images are marketing composites or older manual illustrations; an attractive screenshot does not establish functioning controls.

Evidence labels used below:

- **Documented:** explicitly described by the vendor/project. Functionality still needs testing.
- **Observed:** visible in a published UI image; this establishes presentation, not behavior or reliability.
- **Inference:** our interpretation or engineering judgment.
- **Unknown:** not established by the reviewed evidence. An undocumented feature is not automatically absent.
- **Measured:** none in this audit. No application was installed, source-built, timed, audio-compared, or exercised with the user's S4.

Version anchors: GlideMix release **v1.1.26** was available; its live manual identified Beta 1.1.25. MixCanvas release **v1.8.2** supplied a Windows executable. DJ.Studio's Studio View documentation describes 5.0 but mixes August/October 2026 images. Mixxx evidence uses the 2.5 manual, with some older skin images. Ableton evidence uses the Live 12 manual. REAPER's site offered 7.84, while the inspected examples illustrate features introduced in earlier releases. Density's reviewed support articles span 2024–2025; its exact current installed build and platform coverage were not established. [GlideMix release](https://github.com/cristitm76/glidemix-releases/releases/tag/v1.1.26), [MixCanvas release](https://github.com/sebedard-creator/MixCanvas/releases/tag/v1.8.2), [DJ.Studio manual](https://help.dj.studio/en/articles/8106005-studio-view-editing-arranging-controls), [REAPER guide index](https://www.reaper.fm/userguide.php).

## Compare the actual workflow

This matrix screens candidates; it does not award numerical scores to untested software.

| Candidate | What the reviewed evidence establishes | Exact Mac/S4 question | Main decision risk |
| --- | --- | --- | --- |
| **GlideMix** | Arrangement, recorded software controls, cue/audience playback, effects; Audacity-derived GPL source archives | Mac build offered; manual explicitly says controller knobs/jogs are not mapped | Whether a controller adapter can enter the existing automation path cleanly |
| **Mixxx** | Live DJ engine and documented S4 MK3 HID/audio integration | Best documented hardware starting point; exact Mac qualification still required | Full-set semantic capture, editing, seek reconstruction, and offline replay are a substantial unproven addition |
| **Density** | Rebuilds a supported hardware set from performance data and original music | S4 MK3 is not on the reviewed capture hardware list | Supported capture ecosystem differs from the user's controller/software setup |
| **DJ.Studio** | Prepared transition workflow and editable automation | Automation FAQ explicitly excludes MIDI-controller automation | Cannot assume its timeline is a live controller recorder |
| **MixCanvas** | Open-source arrangement, tempo and automation, project persistence/export | Published release is Windows; Mac/S4 path unqualified | Porting plus behavior changes, rather than one missing mapping |
| **Ableton Live** | Performance/arrangement automation with explicit override/return | Generic MIDI mapping does not establish S4 MK3 HID integration | Configuration and song preparation may overwhelm the desired workflow |
| **REAPER** | Extensible DAW with automation, routing, scripts and plugins | No turnkey S4 workflow established here | A template/extension could help, but DJ behavior and controller integration remain work |
| **VirtualDJ** | Automix editor adjusts a pair's transition positions and overlap | Not qualified in this audit | Pairwise automix editing is not evidence of captured full-performance automation |

Primary sources for these distinctions are linked in the candidate sections below. Audacity is a visual/editing reference, rather than a demonstrated solution to controller-performance capture. MixMeister and The Bridge are historical references, not recommended current Mac/S4 solutions.

## GlideMix: most important arranger to evaluate first

The manual documents per-song lanes, editable beat grids, recorded control movements with ARM, manual override and RE-ENABLE, separate audience/cue playback, and VST3/AU processing. It explicitly distinguishes using a DJ controller as an audio interface from mapping its knobs and jog wheels. This is a confirmed current limitation in the documentation, rather than an unanswered compatibility question. [Manual](https://glidemix.com/manual).

The inspected UI has a large arrangement above a library/project table, plus a dedicated transition mixer. Its EQ/fader panel supplies a familiar place to perform an overlap without making decks dominate the main window. The automation, beat-grid, routing and plugin views are individually documented in the [atlas](ui-evidence-atlas.md#glidemix-seven-images). Some mixer controls are dim placeholders according to the manual; visual presence alone must not be scored as support.

The project offers paid Mac/Windows binaries and GPL source tarballs. The v1.1.26 release includes a source archive; the release repository is not a conventional browsable development tree, and the project says it does not provide source-build support. The README's “Main FX” image is explicitly a private development preview and is excluded from the shipped-feature assessment. [Repository](https://github.com/cristitm76/glidemix-releases), [release assets and notes](https://github.com/cristitm76/glidemix-releases/releases/tag/v1.1.26).

**Inference:** adding a hardware input layer to an already usable arranger could remove more work than extracting an engine and building another editor. But “add controller support” is not yet an effort estimate. We must establish:

1. Can a clean Mac build be reproduced from the published source, with its exact dependencies and license notices?
2. Is there a common parameter-change path for mouse input, recorded automation, and external control, or would a new adapter bypass important state?
3. Can fader pickup, automation override, and return be made predictable without rewriting the editing model?
4. Can a selected range and parameter be reperformed while preserving everything else, including an undoable original?
5. Does this remain maintainable across upstream releases, and is contribution practical?

Its early beta status and limited independent evidence make recovery and long-session tests especially valuable. Recent release notes describe fixes; they are neither proof that those defects persist nor proof of production reliability. GlideMix should lead the **reuse investigation**, not be declared the winner before testing.

## Mixxx: strongest performance and hardware foundation

The S4 MK3 mapping is documented from Mixxx 2.4 onward. The 2.5 controller page describes HID integration and four-channel audio routing; display support is absent and motor support is beta/disabled by default. These distinctions matter more than a blanket “S4 supported” badge. [S4 MK3 documentation](https://manual.mixxx.org/2.5/en/hardware/controllers/native_instruments_traktor_kontrol_s4_mk3).

Its inspected interface is organized around current decks, waveforms, mixer, cue/loop controls and a library. It provides the performance side of the workflow, while a multi-song editable set needs a different overview. [UI manual](https://manual.mixxx.org/2.5/en/chapters/user_interface).

There is relevant prior work: the 2020 GSoC Macros project investigated recording/replaying actions associated with tracks. Its original PR #2989 is closed and unmerged, and marked “Moved”; this audit did not trace every successor. That historical proposal is useful design evidence, not proof of a released full-set editor. [Project notes](https://github.com/mixxxdj/mixxx/wiki/Macros-Project), [original PR](https://github.com/mixxxdj/mixxx/pull/2989).

**Inference:** Mixxx is the better starting point if spontaneous controller performance is the hardest constraint. However, capture cannot stop at logging incoming HID/MIDI. The saved performance needs source identity, transport positions, initial parameter state, accepted actions, time anchors, and later loop/seek/rate semantics. Replaying into a running DJ application and rendering a saved arrangement offline are different integration problems.

The bounded spike should trace one fader through the applied engine state, persist it with initial state, replay it after a seek, then find an offline-render seam. If reliable replay requires an invasive fork of transport, library and UI layers, report that cost before building a timeline. Existing hardware support is a large advantage, not evidence that the missing half is small.

## Density: closest commercial performance-reconstruction precedent

Density's recorder captures performance information rather than finished audio and reconstructs a set using the original tracks. The reviewed computer-recording guide lists specific Pioneer/AlphaTheta players and mixers, using network player information plus mixer USB data. It does not list the S4 MK3. The REC-1 FAQ also describes metadata/actions rather than audio recording. [Computer recording guide](https://support.density.one/hc/en-us/articles/18511009482007-How-to-record-a-set-using-your-computer), [REC-1 FAQ](https://support.density.one/hc/en-us/articles/20285787225495-REC-1-Frequently-Asked-Questions).

The DAW manual describes moving/reordering songs, transition presets, parameter breakpoints, loop edits and a shared tempo through transitions. Adjusting some lane controls changes subsequent points, making edit scope an important trial question. [DAW manual](https://support.density.one/hc/en-us/articles/19861699745175-Density-DAW-Manual).

Its UI makes the performed mix visible as staggered songs with stacked parameter lanes. The close-ups expose the density of recorded data, including EQ and effects. This supports Fader's emphasis on inspectability while illustrating why “show every parameter” can consume the screen. [Published product images](https://density.one/), [atlas](ui-evidence-atlas.md#density-four-images).

**Inference:** this substantially weakens any claim that performance-to-editable-set is a new category. It does not establish a solution for this user's hardware. Before paying for or recommending a trial, establish the current supported capture configuration, platform build, and whether a standalone mouse-authored project is useful enough on its own. Reproducing the feel of external mixer DSP is a separate question from reconstructing its control values; this audit performed no audio comparison.

## DJ.Studio: transition editing benchmark, with a capture boundary

The current Studio View documentation presents two main deck lanes, sample lanes, phrase/bar-level views and a separate tracklist. It distinguishes temporary adjustments during playback from saved editing while paused or recording. The inspected images show these mode messages explicitly. [Studio View](https://help.dj.studio/en/articles/8106005-studio-view-editing-arranging-controls).

Its automation editor supports recorded on-screen controls, editable curves and effect blocks. Its FAQ explicitly says MIDI controllers are not currently supported for automation. The useful distinction is **recording a mouse gesture versus recording a hardware DJ performance**. [Automation editor](https://help.dj.studio/en/articles/8213331-automation-editor-and-effect-blocks).

Documented exports include audio and an Ableton project carrying automation/effects; trial export requires a license. Ableton export is an outbound handoff, not evidence of importing a performed Traktor set. Likewise, exporting DJ playlists/cues does not reproduce recorded fader movement. [Exporting mixes](https://help.dj.studio/en/articles/8106079-exporting-mixes).

**Inference:** DJ.Studio is worth testing as the quickest route to a finished prepared mix and as a UI benchmark. Phrase-level editing, a selected-transition workspace and a compact details pane are promising references. Its preview/edit distinction also exposes a risk: users must know whether a turn of a knob changes the saved project. Test comprehension of the actual current labels rather than assuming that more visible controls solve it.

For Fader, the research supports an explicit automation-state model. It does not justify competing on auto-transition presets, video, stems or streaming integrations before the controller-correction loop is proven.

## MixCanvas: relevant source, significant product mismatches

The README documents a Rust audio backend with a Tauri/React interface, three stereo lanes, editable beat grids, tempo changes, save/export and AGPL-3.0-only licensing. Crucially, automation belongs to the **lane** and remains in place when a clip moves. It also explicitly excludes a VST ecosystem. The reviewed release distributes Windows software; general MIDI/S4 integration was not established. [README](https://github.com/sebedard-creator/MixCanvas), [v1.8.2](https://github.com/sebedard-creator/MixCanvas/releases/tag/v1.8.2).

The inspected timeline is compact, with a visible transport, effect pads and a right-side browser. The beat-grid editor exposes a guided sequence of operations; the bounce screen makes mastering controls visible. These are useful interaction examples even if this is not the right base. [Atlas](ui-evidence-atlas.md#mixcanvas-four-images).

Source architecture notes describe different live/output protection and offline processing paths, including export gain behavior. Therefore, an export button is not proof that live monitoring and rendered output use identical processing. Some architecture passages describe planned work, so current code/build verification is necessary before treating the document as implementation truth. [Architecture notes](https://github.com/sebedard-creator/MixCanvas/blob/main/architecture.md).

**Inference:** the attractive technology match does not outweigh the cost of a Mac port, S4 adapter, automation-ownership changes, and future plugin hosting. Investigate it after GlideMix/Mixxx unless a source build reveals a much cleaner extension boundary. For example, moving a transition eight bars later must not silently leave a bass cut behind; the application can legitimately choose lane ownership, but the user must understand that choice. Fader's proposed clip/bus distinction is different, not universally superior.

## DAWs and neighboring approaches

**Ableton Live** provides recorded automation, editable envelopes, manual override and re-enabling automation. Its manual distinguishes Session clip automation from Arrangement track automation; the three inspected illustrations make the two contexts visible. This is an excellent reference for ownership and mode design. It is also a plausible personal solution if a deliberately small template makes the workflow comfortable. [Live 12 automation](https://www.ableton.com/en/live-manual/12/automation/).

**REAPER** offers extensive routing, automation, scripting and plugin support. Its published examples show selecting media and envelopes together and surfacing render diagnostics. A two-lane template plus a focused extension deserves a small usability trial before dismissing general DAWs as too complicated. Extensibility is not equivalent to an open-source application base. The current PDF guide was located but could not be parsed by the web tool because of its size; this audit does not claim a complete guide review. [Product capabilities](https://www.reaper.fm/), [guide index](https://www.reaper.fm/userguide.php).

For either DAW, a generic “supports MIDI” claim does not settle S4 MK3 control. NI's MIDI-mode article lists S4 MK1/MK2 but not MK3, and says unlisted controllers cannot be switched using that procedure. A HID adapter or another documented integration would need verification. Audio-interface use is a separate capability. [NI MIDI-mode support](https://support.native-instruments.com/support/solutions/articles/69000880031-native-instruments-switching-your-controller-to-midi-mode).

**VirtualDJ** has an Automix Editor for previewing a track pair and changing transition boundaries/overlap. The inspected view is deliberately focused on that pair. It should be compared with the “correct this transition” step, but does not by itself demonstrate editable capture of the user's live mixer actions. Exact S4 compatibility and any broader capture mechanism were not qualified here. [Automix Editor](https://virtualdj.com/manuals/virtualdj/editors/automixeditor.html).

**Audacity** remains useful for waveform-first clarity, direct audio editing and the user's familiarity. Its current homepage presents a redesigned timeline; “Audacity-like” needs concrete interaction requirements rather than assuming one unchanging visual style. GlideMix already explores the Audacity-derived DJ-arranger route. [Audacity](https://www.audacityteam.org/).

## Historical precedents change the novelty claim

- **Traktor 2.x:** NI's Traktor 2.6 quick reference, printed page 9, describes editable mix recording, selective overdub and audio rendering. This is stronger evidence than relying on memory of old versions. The earlier screenshot's specific “2.5, page 13” citation was not established here; use this verified reference instead. [Original NI PDF](https://www.native-instruments.com/fileadmin/redaktion_upload/pdf/Traktor26_Quick_Reference.pdf).
- **Serato/Ableton The Bridge:** recorded sets could become Ableton projects, with mixer automation dependent on supported Rane hardware. This was a hardware/software partnership, not a universal recording format. Treat its legacy compatibility separately from today's Live/Serato products. [Ableton Mixtapes](https://www.ableton.com/en/products/bridge/mixtapes/), [Serato legacy support](https://support.serato.com/hc/en-us/sections/200491074-The-Bridge).
- **MixMeister:** its official site now announces end of life and no further OS-support updates. It remains a workflow reference, not a safe assumption for the user's current Mac. [Official notice](https://www.mixmeister.com/).

The opportunity, if demonstrated, is a maintained and approachable implementation for a particular workflow and hardware target. “DJ software plus a timeline” is not a defensible novelty claim.

## What users report, and what it actually supports

These are self-selected public anecdotes, not a representative survey or a defect database. Product versions, setup and incentives are inconsistent. Vendor replies are attributed separately from user experience.

| Discussion | Evidence worth retaining | Limits and test derived from it |
| --- | --- | --- |
| [DJ app with post-mix editing, June 2024](https://www.reddit.com/r/DJs/comments/1dgczm0/dj_app_with_post_mix_editting/) | A user asks for recorded transitions and controls to remain editable; DAW preparation feels unlike spontaneous DJing | Strong match to a use case, no market-size conclusion. Compare preparation effort as well as editing effort |
| [Density experiences, August 2024](https://www.reddit.com/r/DJs/comments/1f2cm04/has_anyone_tried_out_densityone/) | Early praise for understandable gridding/editing; concern over capture connections during changeovers. Developer explains MIDI plus ProLink and distinguishes supported standalone equipment from DJ-software data | Early enthusiasm is not long-term reliability evidence. Test setup/relink burden, supported hardware and reconstruction fidelity |
| [DJ.Studio after a year, July 2025](https://www.reddit.com/r/djstudio/comments/1lwryqp/my_experience_after_a_year/) | Original post values accessible mix preparation; replies include controller-capture requests and editing/export concerns | Original includes a referral link. Later UI changes can invalidate complaints. Test the current build without assuming historical omissions remain |
| [Would you recommend DJ.Studio, April 2025](https://www.reddit.com/r/Beatmatch/comments/1k6dunf/would_you_recommend_djstudio/) | Mixed views on ease versus DAW flexibility | Some recommendations contain referral incentives. Measure this user's task completion, not comment totals |
| [DJ.Studio frustrations](https://www.reddit.com/r/djstudio/comments/1sbu0ot/what_are_your_frustrations_with_djstudio/) | Reports include import waits, unwanted playback changes and difficult long exports; vendor requests support reports | Mixed dates/builds and unverified causes. Convert into import-cancel, cursor-safety, recovery and long-export scenarios |

No substantial independent longitudinal evidence was established for GlideMix or MixCanvas in this pass. That is an evidence gap, not a negative review. Product-authored comparison blogs were not used as neutral competitor benchmarks. Likewise, a launch article saying it has not tried Density is not a hands-on review. [Density launch coverage](https://www.digitaldjtips.com/density-record-edit-master-dj-sets/).

## UI conclusions to carry into testing

The atlas supports several design hypotheses, not proven usability wins:

1. **Keep the arrangement readable; focus the mixer on the selected transition.** GlideMix and DJ.Studio show alternatives to dedicating most screen area to permanent decks. Test whether selection always makes the controlled song clear.
2. **Expose capture state beside the control.** DJ.Studio's preview/edit messages and GlideMix/Ableton's override/return concepts show why a global record lamp is insufficient. A physical fader can disagree with the saved curve even while playback sounds correct.
3. **Show fewer automation lanes by default, with a dependable “reveal recorded” action.** Density's detailed lanes help inspection but grow vertically; hidden lanes must remain discoverable.
4. **Make ownership visible before moving material.** MixCanvas's lane behavior and Ableton's separate contexts demonstrate that similar curves can have different semantics. A move preview should show which curves and bus changes will travel.
5. **Treat retakes as a distinct task.** An editable envelope and a record button do not establish safe selected-parameter replacement, automatic return, take comparison or one-step undo. This remains a key unresolved comparison question.
6. **Keep monitoring and rendering intelligible.** Separate audition output, saved master changes and export-only processing. Explicit limiter/normalization controls matter when judging why a bounce sounds different.
7. **Make correction and recovery first-class.** Guided grid correction, relinking, undo and a useful render failure report can matter more than another transition preset.

## Proposed adoption and reuse gate

Preserve the [confirmed requirements](../../README.md#confirmed-requirements). The proposed near-term objective is: **make a naturally performed transition easy to inspect, revise and reuse, using an existing application or engine wherever practical.** This narrows the next experiment, not the user's long-term requirements.

Run this common task with two synthetic/redistributable tracks, then representative local MP3/WAV files. Keep private music and paths out of published evidence. Record exact builds, settings, controller firmware, and exceptions using the [experiment template](../experiments/README.md).

| Task | Evidence to collect | Why it discriminates |
| --- | --- | --- |
| Import, locate downbeats and arrange overlap | Preparation time; manual grid steps; any conversion or source mutation | Measures friction before the attractive demo starts |
| Perform a 30-second gain/crossfader transition | Capture route; initial state; what actually gets saved | Separates software-knob recording, audio recording, and hardware-action capture |
| Correct just one fade | Steps; untouched curves; audible before/after; undo | Tests edit scope and discoverability |
| Replace only eight bars of one parameter | Preroll; pickup; entry/return boundaries; keep/discard; one undo | Tests the central retake requirement |
| Move the transition eight bars | Which clips, tempo and curves move; conflicts made visible | Exposes ownership assumptions |
| Cue without changing the exported master | Physical channel map and saved project state | Separates monitoring from performance |
| Save, close, reopen without controller; relink moved copies | Missing-media behavior and retained edits | Tests portability rather than same-session success |
| Export and compare | Declared export processing, reference audio, timing, completion/errors | Distinguishes “exports” from faithful reproduction |

EQ-only replacement is a useful **additional competitor probe**, not an expansion of Fader's gain/crossfader POC. Similarly, a generic MIDI controller can help isolate a DAW workflow but cannot pass the S4 requirement. Keep setup time separate from repeated-task time; record blocked steps rather than hiding them through substitutions. No timing results exist yet.

Suggested order and decision rules:

1. **GlideMix:** establish the mouse workflow, current limitations, source build and controller insertion point. Prefer an upstream contribution or maintained extension if the existing model survives the retake/ownership tests.
2. **Mixxx:** qualify the user's S4, then bound the capture/replay/render investigation. Prefer this route if live performance works well and a maintainable project/editor boundary can be demonstrated.
3. **DJ.Studio and one DAW template:** benchmark finishing and correcting a prepared set. A satisfactory personal workflow can be useful even if it does not meet open-source distribution goals.
4. **Density:** qualify current capture/platform support before treating it as an actionable S4 trial. Use it as a reconstruction/UI reference meanwhile.
5. **MixCanvas:** investigate source reuse if its platform and model changes appear cheaper than the stronger candidates; do not select it simply because the frontend stack is familiar.

For each investigation, agree a small effort cap before implementation and record **adopt, contribute, extend, reject with reason, or still unknown**. Do not let an unbounded port or fork become the default through sunk cost. Preserve actual blocker evidence and the smallest change that could remove each blocker.

If application reuse fails, evaluate **Tracktion Engine before a fully custom sequencing core**. Its C++20/JUCE module supplies a sequence-oriented data model and examples; Tracktion and JUCE have separate licensing considerations. It remains an engine candidate, not a finished DJ product. The source-build, controller, capture and replay questions still apply. [Official repository](https://github.com/Tracktion/tracktion_engine).

The existing C++/JUCE architecture and POC remain a conditional fallback. Acceptance of that route should follow this gate, with explicit reasons why adoption or a narrower contribution does not satisfy the user's workflow.
