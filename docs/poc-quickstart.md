# Native arrangement prototype quickstart

Version 0.2 is a software experiment that runs without a controller. The Mac/S4 hardware qualification remains open.

## Build on macOS

Install the Xcode Command Line Tools and CMake 3.22 or newer. The first configure fetches pinned JUCE 9.0.5 (`d60b7a78030661574a615b586d9325ba7a7fa7dd`). To use an existing checkout, add `-DFADER_JUCE_PATH=/path/to/JUCE`.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
codesign --force --deep --sign - build/Fader_artefacts/Release/Fader.app
open build/Fader_artefacts/Release/Fader.app
```

See [reproducible builds and CI](build-and-ci.md) for shared `core` / `native` presets, clean-checkout tests and retained build metadata. Hardware, Windows and Linux application support remain unqualified. Pure core tests need no JUCE/network: configure a separate build with `-DFADER_BUILD_APP=OFF`.

## Try the workflow

1. Open **Set ... → Preferences** (Cmd/Ctrl+,). Music defaults to `~/Music/DJ` when present; select another root if needed. Select Traktor's `collection.nml`, or disable its import. **Apply & close** saves library preferences. Audio-device changes apply immediately; master uses channels 1/2 and headphone cue requires 3/4 on the same device.
2. Search the library by title, artist, BPM or key; sort with column headers and filter with the folder list. Traktor metadata is labeled. Missing/unknown values stay visible. Drag a song to the arrangement, double-click to add at the playhead on a new row, or drop files from Finder.
3. Move a clip by its **header**. Horizontal movement changes its start; vertical movement changes its row. Neither changes duration, waveform scale or clip-owned automation. Scroll for more time/rows; use **Zoom** to change scale explicitly.
4. Select **Volume**, **Low cut (HP)** or **High cut (LP)** above the grid. Click its waveform to add a point; drag points; right-click a point and choose **Delete automation point** to remove it. Only the selected parameter is overlaid, independently for every clip. Volume is shown in dB in the inspector; filters show cutoff Hz and explicit Off endpoints.
5. Click empty timeline space to seek. **Play** reads the saved curves. Moving an inspector slider during playback makes a temporary **OVERRIDE**; **Resume automation** returns to the saved curves. Stopped slider edits write a point at the playhead within that clip, otherwise at its beginning.
6. Select a clip and press **Record clip**. Its volume/HP/LP controls are captured; other clips continue their saved curves. **Clip ... → Show retake range** reveals From/To in absolute seconds; 0 means current/clip end. Hiding these fields preserves the range; **Clear retake range** resets it. **Finish take** keeps the gestures. Only touched parameters are replaced, and **Undo** restores the previous edit/take.
7. **Save** writes a schema 3 `.faderset` containing stable identities, source/decoder fingerprints, placement, grid/render snapshots and named automation. Older sets retain their original renderer; saving upgrades storage, so choose a new filename to keep the old file. **Set ... → Open set** verifies media identity. Keep sources with the project when moving it. Unsaved changes prompt before New/Open/quit; there is no autosave yet.
8. **Set ... → Export WAV** renders a stereo 24-bit/48 kHz WAV through the same engine. Clipping is rejected with a request to lower gains. The destination is replaced only after successful rendering, header/storage checks and finalization. The transport button becomes **Cancel export**; the mode label shows export progress. Native inspection of these new controls is still pending; the API cancellation/error path is tested.

The flat dark interface keeps transport, automation selection, Save and Undo visible. Right-click a clip for its actions, or use **Clip ...** / Shift+F10. Right-click empty timeline space for set actions. **Library ...** contains Refresh and Preferences; right-click a library song for **Add to arrangement**. Shortcuts: Cmd/Ctrl+N/O/S/Z for New/Open/Save/Undo, Cmd/Ctrl+Shift+E for export, and Space for Play/Stop outside text fields. See the [theme and menu rules](theme-and-interaction.md).

For quiet synthetic material, run `python3 tools/make_demo.py` and open `build/demo/demo.faderset`. No user music is included. Media import clears older undo snapshots to bound retained source/peak snapshots; other edits keep up to 24 undo states.

## Probes

```sh
build/fader_probe_artefacts/Release/fader_probe --devices
build/fader_probe_artefacts/Release/fader_probe --library
build/fader_probe_artefacts/Release/fader_probe --audio-smoke
build/fader_probe_artefacts/Release/fader_probe --inspect /path/to/audio.mp3
build/fader_probe_artefacts/Release/fader_probe --render build/demo/demo.faderset build/demo/mix.wav
```

`--resave-project <input> <output>` checks migration with the same project reader/writer and cannot bypass decoder-domain mismatches. `--library` prints aggregate counts only. `--inspect` scans/validates one file with bounded scratch PCM and waveform peaks. `--qualify-stream <audio>` compares sequential streaming and seeks against an independent sequential decoder. `--streaming-soak <A> <B>` processes two sources with a 30-minute offset and reports cache/RSS; use synthetic long WAVs from `scripts/make-streaming-fixtures.py`. `--audio-smoke` runs two seconds of silent clip/filter rendering on the default device; driver xrun value -1 means unavailable. This does not measure acoustic latency or establish sustained reliability.

## Deliberate limits

- The grid expands as clips move, within 64 clips, 4,096 row positions and 24 hours. It is not mathematically infinite. Fixed zoom prevents moving the final song from rescaling every clip.
- Imported files stream through 512 KiB PCM caches per clip (32 MiB maximum), plus at most 512 KiB waveform peaks per source (32 MiB/set). Codec/runtime/UI memory is additional. Full-file scan/hash still makes import proportional to file length. See the [streaming contract](streaming-contract.md).
- MP3 and WAV are the initial import targets. Other registered JUCE decoders may work, but broader formats and every file in the music library are not qualified. Mono duplicates to stereo; multichannel/incomplete sources are rejected.
- Traktor import covers metadata, first nonnegative grid marker and cue count. Full cue/loop/playlist import, beat analysis and editing variable grids remain pending. No source files or Traktor data are modified.
- Snap is Off, 1 second or a fixed 120-BPM bar. Imported BPM/grid is displayed, not automatically used for sync, warping or variable-tempo snap.
- The prototype filters are two cascaded one-pole stages per side, 12 dB/octave, without resonance. Volume and filter targets smooth over 5 ms. Pitch-preserving stretching and production resampling remain future work.
- MP3 and CoreAudio-decoded sources use a session PCM disk cache when admitted (8 GiB process budget). The uncached fallback can be slow; preparation identifies that mode. Live missing PCM produces a 5 ms dry-source fade and a **READ GAP** warning. Offline export waits for PCM or fails explicitly.
- Seeking uses a worker and at most two seconds of filter preroll. **Cancel** invalidates preparation; a ruler click during ordinary playback/preparation replaces the destination. Timeline editing stops playback. See the [seek contract](seek-contract.md) for measured scope and limits.
- Recorded targets use held segments; drawn points use linear interpolation. Retakes restore the original target through smoothing. Saved later points are preserved, but baseline audio immediately outside punch-out may differ during that return.
- Legacy global-mixer sets retain their original renderer. Start a new set for clip-owned editing. Storage migration to [format 3](project-format-v3.md) preserves renderer choice; it does not approximate the old crossfader mix with new clip envelopes.
- No controller input/feedback, raw audio recording, VST hosting, transport/rate capture, MP3 export, autosave, missing-source relinking UI or complete keyboard envelope editor yet. Cue is monitoring state and is not saved automation. Audio-device selection lasts for the current session; library preferences persist.

See [current evidence](experiments/2026-11-04-clip-arrangement.md), [POC qualification gates](poc-plan.md), and [ADR 0002](decisions/0002-clip-owned-automation.md). The source uses AGPL-3.0-or-later; the long-term stack/reuse choice remains provisional.

## Interrupted recordings

If input/capture queues overflow or the device clock changes, Fader retains captured actions separately and shows **INCOMPLETE**. Existing envelopes stay intact. Use **Set ... → Save incomplete take...** to archive those actions and the original project as `.fadercapture`, or explicitly discard them before recording again. The archive is not a complete playable set, has no audio embedded, and cannot yet be reopened through the UI. This is not autosave or crash recovery; those remain [#16](https://github.com/coder13/Fader/issues/16).
