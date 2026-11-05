# Theme and interaction rules

Status: **Accepted direction; implemented in the v0.2 prototype.** Updated 2026-11-04. The user requested a flat, dark, clean interface based on Traktor and Audacity, with rarely used buttons behind context menus. Easier use is the design goal; comparative usability has not been measured.

## Visual system

The [UI evidence atlas](research/ui-evidence-atlas.md) informs the implementation: waveform-first editing, a compact transport, a readable metadata table and selected-clip controls. One shared JUCE look-and-feel in [Theme.h](../src/Theme.h) supplies semantic colors and flat button, checkbox, slider, combo, field, table, scrollbar and menu drawing. New surfaces should reuse these roles rather than introduce local hex colors.

| Role | Color | Use |
| --- | --- | --- |
| Background | `#15181b` | Arrangement and outer workspace |
| Panel / raised / field | `#1d2125` / `#282d32` / `#191d21` | Inspector, controls and inputs |
| Text / muted | `#e6eaed` / `#a2acb6` | Primary content and secondary labels |
| Border / grid | `#3b434b` / `#2c3339` | Control boundaries and time/row guides |
| Accent | `#e6af62` | Selection, keyboard focus, active controls and volume envelope |
| Filter envelopes | `#6bbdd8` / `#8bcbb2` | High-pass / low-pass |
| Danger | `#ff8b80` | Recording state and missing-media notices |

Use flat rectangular surfaces, thin borders and restrained fills. Text, selected-button underlines, focus outlines and mode labels reinforce color. Waveforms remain visible behind the selected envelope. Native window chrome and file dialogs follow the operating system; this prototype does not replace them or offer a theme picker. The current layout minimum remains 1250 × 920; smaller screens and full accessibility qualification remain open.

## Action placement

| Surface | Persistent controls | Menu actions |
| --- | --- | --- |
| Workspace | Play/Stop, Record/Finish, Rewind, Undo, Save, parameter selection, Snap, Zoom | **Set ...**: New, Open, Save, Export WAV, Preferences, save/discard an incomplete take |
| Clip inspector | Name/metadata, volume, HP/LP, start and headphone cue | **Clip ...** or right-click: select displayed envelope, show/clear retake range, remove clip |
| Envelope point | Drag the point directly | Right-click near a point: Delete automation point |
| Library | Search, folder list, sortable metadata columns | **Library ...**: Refresh, Preferences; row right-click: Add to arrangement |

The labeled menu buttons make secondary actions discoverable without requiring right-click knowledge. Opening a clip menu selects the clip but does not create an undo entry, delete points or move the playhead. Point removal is an explicit, undoable action. Opening the ruler/empty-grid menu does not seek.

Retake controls expand only when requested. Hiding them does not clear their values: a nonzero range keeps a visible “Retake range active” label and changes Record to “Record range.” **Clear retake range** resets both fields. **Resume automation** appears while an override is active. Playback/capture state still uses explicit EDIT, READ, OVERRIDE and RECORD labels.

## Keyboard access

Cmd on macOS / Ctrl elsewhere: N New, O Open, S Save, Z Undo, comma Preferences, Shift+E Export. Space toggles playback outside text entry. Shift+F10 opens the selected clip menu outside text entry. Menu arrows, Return and Escape navigate, activate and dismiss. Buttons expose tooltips and keyboard focus borders. Complete keyboard envelope editing remains future work.

## Verification evidence

Release build on the existing Apple M3/macOS environment with pinned JUCE 9.0.5. All three CTest suites pass. Editor regression checks exercise actual mouse handlers: right-clicking a point preserves the curve and undo count; right-clicking the ruler does not seek. Existing clip-drag, automation ownership, save/reopen, filter and rendering checks still pass.

Native inspection covered the empty arrangement, a selected waveform, browser rows and Preferences. Cmd+comma and the Set menu both opened Preferences; Shift+F10 and the Clip menu both toggled retake visibility without changing the set. A low-contrast inherited sort arrow was replaced with the shared accent. No hardware, sustained-audio or cross-platform claims were added by this UI change. An owner usability session should still check menu discoverability and frequent editing tasks.

Interrupted captures show a red **INCOMPLETE** mode and an explanatory status. Valid retained actions stay separate from the saved envelopes. Recording again is blocked until they are archived or explicitly discarded. The archive currently has no reopen UI; this limitation is stated after saving. See the [capture contract](playback-clock.md).
