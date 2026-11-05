# Product and user experience

Status: Concrete interactions proposed; Audacity 4 redesign lessons are a user-confirmed UI direction. Updated 2026-11-04. User requirements are recorded in the [README](../README.md).

## Product definition

A DJ performance editor combines familiar deck controls with a song arrangement timeline. A user can build a set entirely with a mouse, perform it using a controller, or switch between the two. The saved result contains source references, placements, playback decisions, and automation, so a mistake in one transition does not require recording the whole set again.

**User-confirmed scope, 2026-11-04:** a DJ performance editor with selected DAW tools. MIDI composition, instruments and studio multitrack recording are not current requirements. The [roadmap completeness review](research/roadmap-completeness-review.md) distinguishes covered workflows, proposed additions and missing validation; it does not expand the accepted feature scope.

The main promise is **perform → inspect → edit → replay → export**. Good controller behavior matters because a prepared transition should still feel natural to play. Reliable project recovery matters because the editable performance is the primary creative artifact.

The [competitive workflow audit](research/competitive-workflow-audit.md) finds substantial existing overlap. This product definition is a workflow target, not a claim of novelty or a decision to build every component. First evaluate adoption, contribution or extension; the detailed custom UI below remains proposed.

## Three workflows

1. **Arrange a set.** Browse local files, preview a track, drag it onto a row, move it without resizing it, draw independent volume/HP/LP envelopes, and audition the overlap. Source-offset/trim editing, musical alignment and EQ are staged additions with separate interaction contracts.
2. **Capture a performance.** Load tracks into decks, arm automation recording, perform a transition, and stop. The timeline shows the resulting clips and parameter changes. Optional master recording preserves an audio reference.
3. **Replace a section.** Select a transition, arm only the parameters to replace, hear a preroll, perform a new take, and keep or discard it. Unarmed parameters keep playing their existing automation. One undo restores the previous take.

An arrangement lane is not permanently the same thing as a physical deck. The v0.2 prototype uses independently placed clips on extensible rows, each owning volume, high-pass and low-pass automation. Its primary editor has no global A/B crossfader. Two-source hardware qualification does not limit the saved arrangement to two rows. Controller deck-to-clip assignment and later mix-bus routing need explicit identity and feedback; they must not change old takes.

## Interface direction

Use the **Audacity 4 redesign lessons as the primary UI direction**: a usable default, visible editing affordances, reversible operations, fewer pointer modes and accessible presentation. The [video research and timestamped design rationale](research/audacity-4-design-lessons.md) translate that direction into Fader-specific proposals. Ableton and the competitor audit remain references for automation and DJ workflows. The choice of UI framework and engine remains subject to the adoption/reuse decision.

```text
Project    Save   Undo/Redo   Play/Stop   Record actions   Audio arm   Export
Position / tempo / snap / zoom                       Audio setup   Panels
Time / bars ruler ---------------------- playhead --------------------------
Loop strip                  [ practice loop ]
Files / Search | Row 1    [ header: Track 1 ------------------------ ]
Preview / BPM  |          < waveform / time selection               > | Details
Folders        |   Volume/HP/LP: selected clip-owned envelope          | Owner
               | Row 2                   [ header: Track 2 -------- ] | Value
               |   Volume/HP/LP: selected clip-owned envelope          | Read
               | Retake range + return region; armed targets named    |
Compact mixer: controlled source • pickup • Read/Override/Record • Resume
Controller connection • master/cue status • active recording/errors
```

The implemented v0.2 arrangement, waveform envelopes, browser and flat dark palette are described in [theme and interaction](theme-and-interaction.md). The richer layout shown above remains a proposal, not an implemented screenshot. The timeline dominates the default **Edit** layout. Files and details can collapse; show selected/used automation lanes with a visible reveal action. Keep a compact mixer available for rehearsal. Audio setup contains device/rate/buffer configuration, while connection, routing failures, capture errors, armed targets and override state stay visible. No customization wizard is required before importing and editing.

The proposed POC layout improvements are collapsible panels and Reset layout. The current v0.2 host has a documented flat dark theme and a 1250 × 920 minimum window; see its pinned [theme report](https://github.com/coder13/Fader/blob/3a3b58456065f6cfed4c9f23f1a0252047d80a89/docs/theme-and-interaction.md). Later **Edit/Perform** workspace presets may change emphasis, but never transport, capture, routing, automation ownership or undo history. Layout/theme preferences stay separate from the saved musical project. Full docking and configurable toolbars are deferred until they solve observed tasks.

The file browser needs folders, search, supported-format status, duration, BPM/grid state, preview, and drag/drop. It must use the existing music-folder layout without moving or converting source files. Large-library indexing and metadata analysis run in the background. The completed POC browses a chosen folder and imports MP3/WAV; the alpha qualifies the whole collection and broader common formats. See [audio import](audio-import-and-library.md).

## Editing rules

These are product interaction targets, not a claim that every gesture exists in v0.2. Current movement preserves clip duration; trim, time selection and further editing tools need separate implementation and acceptance scope.

- A clip header moves the clip. Distinguish waveform time selection from editing the selected envelope. Future selected/focused clips expose visible trim handles, with keyboard/numeric alternatives for small targets. Trimming preserves the source and can be reversed by extending the handles. Future stretch has a separate labelled control; no hidden-modifier-only discovery.
- Selection, move, trim and zoom need no persistent global pointer modes. “Show automation” never arms recording. A later split tool can support a visible command and a temporary hold shortcut with an obvious cursor and Escape cancellation.
- A parameter's context action reveals its automation lane. Points support drag, exact numeric values, deletion, and undo.
- Gain uses decibels in the editor, with an explicit mute value; the engine defines how values interpolate and smooth.
- An automation mode label says **Read**, **Override**, or **Record**. A clear **Resume automation** action ends an override. Touch and latch become explicit modes later.
- Snapping supports off/beat/bar. Display musical and absolute time; do not infer that a displayed beat grid is verified.
- Moving a clip moves clip-owned automation. Bus/master automation stays at the set position unless the user explicitly includes it in the move.
- Delete leaves a gap by default. Future “Close gap on selected lanes” and “Close gap across set” are separate commands with an affected-material preview. Explicit clip groups have visible membership; overlap never implicitly groups clips or includes master automation. The full ripple/grouping feature set is not added to the POC.
- The playhead, edit selection, loop strip and retake/return region have distinct purposes and labels. Export explicitly chooses full set or a range and tail policy; a practice loop does not silently change export bounds.
- Preview a destination before a move. If the POC cannot support that overlap, explain the limit and show a valid placement instead of silently refusing it or pretending extra sources can play.
- Record controls distinguish automation from audio. Cue monitoring and hardware output volume are not silently recorded into the exported master.
- Keyboard access, scalable UI, visible focus, light/dark and high-contrast presentation, units and exact values are part of the design. Color is supplementary to labels/shapes for selection, armed/overridden state, cue and errors. No onboarding popup interrupts active performance or recording.

## Feature stages

| Capability | First complete POC | Following milestones |
| --- | --- | --- |
| Tracks and mixing | Independent clip volume and HP/LP, master/cue; two-source hardware qualification | Four decks, EQ/effects, flexible arrangement buses |
| Tempo and beat grids | Manual BPM/first beat; fixed project tempo; static per-clip varispeed before capture | Captured rate gestures, editable detected grids, key-lock quality, changing tempo and variable grids |
| Automation | Clip volume/HP/LP, starts/stops/source offsets, manual editing, range replacement; complete action capture remains open | Touch/latch, tempo/loop/jog/FX gestures, take comparison |
| Controller | S4 essential controls and LED feedback | Four-deck layers, jog behavior, screens and motors qualified individually |
| Files | MP3/WAV import, source relinking, set save/reopen, WAV export | AIFF/FLAC/AAC/ALAC/Vorbis/Opus import, full-library qualification, MP3/FLAC export, collect-media bundle |
| Platforms | Apple Silicon macOS | Windows, Linux; web editor/hybrid based on experiments |
| MIDI output | Basic timestamped output proof | Mapping presets and clock/start/stop with measured timing |
| Plugin effects | Excluded | Version 2/3: VST3 song/lane/master racks, saved settings, parameter automation and processed export |

The goal of eventually having broad DJ features is preserved in the backlog. Plugin effects are a confirmed version 2/3 requirement: drag effects from a plugin browser onto a song or effects rack and edit their exposed parameters on the timeline. See [plugin effects](plugin-effects.md). Plugin hosting, streaming services, DRM media, DVS/timecode, stem separation, video, and cloud collaboration remain outside the POC.

## Usability evidence

Validate the [Audacity-derived interaction tasks](research/audacity-4-design-lessons.md#proposed-usability-acceptance-tasks) without coaching. Include first-use clip movement/trimming, editing immediately after zooming, deletion expectations, automation visibility versus recording, and distinct loop/retake/export bounds. Record first-attempt success, mode errors, help requests and timing. The video's audience findings and untested concepts are not Fader measurements.

The [UI evidence atlas](research/ui-evidence-atlas.md) records 33 inspected published images/crops. It suggests testing a selected-transition mixer, visible preview/record/override state, selective automation-lane display, and a move preview that makes automation ownership clear. These are design hypotheses, not measured usability improvements. Preserve the editing rules above until an explicit decision changes them; in particular, MixCanvas's documented lane-owned curves differ from this proposal.

Begin with the same [cross-application workflow](research/competitive-workflow-audit.md#proposed-adoption-and-reuse-gate) in existing candidates. Record blocked tasks, setup effort, parameter ownership and retake behavior. Historical forum complaints become current-build test questions, not presumed defects.

Run five tasks on the editor prototype: import two tracks, align an overlap, record a fade, correct the fade, save/reopen/export. Record confusion points, number of mode errors, and whether the user can distinguish a temporary override from a saved edit. The first research participant is the project owner using their S4. This experiment should guide the layout before a toolkit choice becomes expensive to reverse.
