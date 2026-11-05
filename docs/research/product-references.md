# Product references

Reviewed 2026-11-04. These are workflow references, not a claim of feature parity or an exhaustive competitor audit. Proposed product behavior appears in [Product and UX](../product-and-ux.md).

For the deeper comparison, read the [competitive workflow audit](competitive-workflow-audit.md) and [UI evidence atlas](ui-evidence-atlas.md). They distinguish documented features, visual observations, inferences and untested questions, and propose an adoption/reuse gate before new-engine implementation.

| Reference | Verified observation | Design implication |
| --- | --- | --- |
| GlideMix | Audacity-derived arranger with published Mac/Windows binaries and GPL source archives. | First investigate an existing arranger plus focused controller integration. [Repository and releases](https://github.com/cristitm76/glidemix-releases) |
| Density | Reconstructs a supported hardware performance from recorded data and original music. | Strong precedent for the core workflow; qualify the capture ecosystem before assuming S4 compatibility. [Capture guide](https://support.density.one/hc/en-us/articles/18511009482007-How-to-record-a-set-using-your-computer) |
| MixCanvas | Open-source Rust/Tauri arranger with a Windows release. | Evaluate platform and editing semantics as well as technology overlap. [Repository](https://github.com/sebedard-creator/MixCanvas) |
| REAPER | Extensible DAW with automation, routing, scripts and plugin support. | Test a small template before assuming a new application is necessary. [Official capabilities](https://www.reaper.fm/) |
| Audacity | Its timeline supplies a shared horizontal time ruler, and its envelope tool edits amplitude with control points. | Make clips, time, and automation directly inspectable; keep transport and selection obvious. [Timeline](https://manual.audacityteam.org/man/timeline.html), [Envelope tool](https://manual.audacityteam.org/man/envelope_tool.html) |
| Ableton Live | Arrangement automation can be recorded and edited as breakpoints; manual control can override playback, with an explicit return to automation. Tempo can be automated. | Define recording, override, and return behavior before building the UI. [Automation manual](https://www.ableton.com/en/manual/automation/) |
| Mixxx | Beat analysis provides constant-tempo and variable-tempo approaches; incorrect grids require correction. | Always permit manual grid correction and persist the grid used by a set. [Beat detection](https://manual.mixxx.org/2.5/en/chapters/preferences/beat_detection) |
| DJ.Studio | Supports prepared mixes and WAV/MP3/FLAC export, as well as Ableton export with automation. | Timeline set preparation already exists as a category. Our proposed emphasis is the seamless open-source controller-performance-to-editable-set workflow; validate that advantage with users. [Exporting mixes](https://help.dj.studio/en/articles/8106079-exporting-mixes) |
| MixMeister | The official site now says the products have reached end of life and will not receive operating-system support updates. | Treat as a historical workflow reference; prioritize maintainable builds and project longevity. [Official announcement](https://www.mixmeister.com/) |

Traktor, rekordbox, and Mixxx remain references for deck ergonomics, cues, beatmatching, controller mapping, and cue routing. We have not performed a comprehensive hands-on comparison of their current versions, and should not claim that none supports any comparable editing workflow.

## Proposed differentiator to test

The useful question is whether a DJ can perform a transition naturally, see exactly what was captured, change one part without damaging the rest, and hear the same result after reopening or exporting. This combines controller performance with editable arrangement. It requires a stronger project model than a master audio recorder or a raw MIDI event log.

The first user test should compare existing candidates and the user's current Traktor + audio-editing process using the [common-task protocol](competitive-workflow-audit.md#proposed-adoption-and-reuse-gate). Measure setup effort, correction steps and confidence in the saved result, rather than comparing feature counts. Adoption or contribution is a valid research outcome; any change to confirmed product requirements needs an explicit decision.
