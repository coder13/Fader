# Product and user experience

Status: Proposed. Updated 2026-11-04. User requirements are recorded in the [README](../README.md).

## Product definition

A DJ performance editor combines familiar deck controls with a song arrangement timeline. A user can build a set entirely with a mouse, perform it using a controller, or switch between the two. The saved result contains source references, placements, playback decisions, and automation, so a mistake in one transition does not require recording the whole set again.

The main promise is **perform → inspect → edit → replay → export**. Good controller behavior matters because a prepared transition should still feel natural to play. Reliable project recovery matters because the editable performance is the primary creative artifact.

## Three workflows

1. **Arrange a set.** Browse local files, preview a track, drag it onto a lane, adjust its start/source offset and duration, align beats, draw gain/EQ/crossfade changes, and audition the overlap.
2. **Capture a performance.** Load tracks into decks, arm automation recording, perform a transition, and stop. The timeline shows the resulting clips and parameter changes. Optional master recording preserves an audio reference.
3. **Replace a section.** Select a transition, arm only the parameters to replace, hear a preroll, perform a new take, and keep or discard it. Unarmed parameters keep playing their existing automation. One undo restores the previous take.

An arrangement lane is not permanently the same thing as a physical deck. The POC deliberately fixes two lanes to two deck roles; the mature product separates clip lanes, mix buses, and the two/four currently controlled decks. This prevents hardware layout from limiting the arrangement format.

## Interface direction

Use Audacity as the reference for readable waveforms, obvious transport, direct envelope manipulation, and a visible relationship between clips and time. Use Ableton's arrangement/automation concepts as a workflow reference. This is a visual and interaction direction, not a request to copy either application's implementation. Reference observations and sources are in [product references](research/product-references.md).

```text
Project name       Save   Undo   Play/Stop   Record actions [ ] Audio [ ]   Export
Time / bars ruler ---------------- playhead ------------------ tempo map -------
Files / Search   | Lane A   [ Track 1 waveform --------------------- ]
Preview / BPM    |   Gain      ───────────╲____
Folders         | Lane B                    [ Track 2 waveform ------------ ]
                |   Gain                    ____╱───────────────────────────
                | Master crossfader        A ─────╲───── B
Selected clip / automation details            Compact deck and mixer controls
Audio device • buffer • cue routing      Controller status      Recording state
```

This is a proposed layout, not an implemented screenshot. At normal use, the timeline should dominate the window. Show only selected/used automation lanes by default. Keep a compact deck/mixer strip available for rehearsal; do not fill the initial screen with four large turntable graphics. Tempo, selected range, snap mode, automation arm, and override state must remain visible.

The file browser needs folders, search, supported-format status, duration, BPM/grid state, preview, and drag/drop. It must use the existing music-folder layout without moving or converting source files. Large-library indexing and metadata analysis run in the background. The completed POC browses a chosen folder and imports MP3/WAV; the alpha qualifies the whole collection and broader common formats. See [audio import](audio-import-and-library.md).

## Editing rules

- Waveform dragging selects time; a clip header moves the clip; edges trim without modifying the source. Distinct affordances prevent accidental movement.
- A parameter's context action reveals its automation lane. Points support drag, exact numeric values, deletion, and undo.
- Gain uses decibels in the editor, with an explicit mute value; the engine defines how values interpolate and smooth.
- An automation mode label says **Read**, **Override**, or **Record**. A clear **Resume automation** action ends an override. Touch and latch become explicit modes later.
- Snapping supports off/beat/bar. Display musical and absolute time; do not infer that a displayed beat grid is verified.
- Moving a clip moves clip-owned automation. Bus/master automation stays at the set position unless the user explicitly includes it in the move.
- Record controls distinguish automation from audio. Cue monitoring and hardware output volume are not silently recorded into the exported master.
- Keyboard access, scalable UI, focus states, sufficient contrast, and parameter labels are part of the design. Do not use color alone to show armed or overridden state.

## Feature stages

| Capability | First complete POC | Following milestones |
| --- | --- | --- |
| Tracks and mixing | Two stereo sources, gain and crossfader, master/cue | Four decks, EQ/filter/effects, flexible arrangement buses |
| Tempo and beat grids | Manual BPM/first beat; fixed project tempo; static per-clip varispeed before capture | Captured rate gestures, editable detected grids, key-lock quality, changing tempo and variable grids |
| Automation | Gains/crossfader, starts/stops/source offsets, manual editing, range replacement | Touch/latch, tempo/loop/jog/FX gestures, take comparison |
| Controller | S4 essential controls and LED feedback | Four-deck layers, jog behavior, screens and motors qualified individually |
| Files | MP3/WAV import, source relinking, set save/reopen, WAV export | AIFF/FLAC/AAC/ALAC/Vorbis/Opus import, full-library qualification, MP3/FLAC export, collect-media bundle |
| Platforms | Apple Silicon macOS | Windows, Linux; web editor/hybrid based on experiments |
| MIDI output | Basic timestamped output proof | Mapping presets and clock/start/stop with measured timing |
| Plugin effects | Excluded | Version 2/3: VST3 song/lane/master racks, saved settings, parameter automation and processed export |

The goal of eventually having broad DJ features is preserved in the backlog. Plugin effects are a confirmed version 2/3 requirement: drag effects from a plugin browser onto a song or effects rack and edit their exposed parameters on the timeline. See [plugin effects](plugin-effects.md). Plugin hosting, streaming services, DRM media, DVS/timecode, stem separation, video, and cloud collaboration remain outside the POC.

## Usability evidence

Run five tasks on the editor prototype: import two tracks, align an overlap, record a fade, correct the fade, save/reopen/export. Record confusion points, number of mode errors, and whether the user can distinguish a temporary override from a saved edit. The first research participant is the project owner using their S4. This experiment should guide the layout before a toolkit choice becomes expensive to reverse.
