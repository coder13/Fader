# Product references

Reviewed 2026-11-04. These are workflow references, not a claim of feature parity or an exhaustive competitor audit. Proposed product behavior appears in [Product and UX](../product-and-ux.md).

| Reference | Verified observation | Design implication |
| --- | --- | --- |
| Audacity | Its timeline supplies a shared horizontal time ruler, and its envelope tool edits amplitude with control points. | Make clips, time, and automation directly inspectable; keep transport and selection obvious. [Timeline](https://manual.audacityteam.org/man/timeline.html), [Envelope tool](https://manual.audacityteam.org/man/envelope_tool.html) |
| Ableton Live | Arrangement automation can be recorded and edited as breakpoints; manual control can override playback, with an explicit return to automation. Tempo can be automated. | Define recording, override, and return behavior before building the UI. [Automation manual](https://www.ableton.com/en/manual/automation/) |
| Mixxx | Beat analysis provides constant-tempo and variable-tempo approaches; incorrect grids require correction. | Always permit manual grid correction and persist the grid used by a set. [Beat detection](https://manual.mixxx.org/2.5/en/chapters/preferences/beat_detection) |
| DJ.Studio | Supports prepared mixes and WAV/MP3/FLAC export, as well as Ableton export with automation. | Timeline set preparation already exists as a category. Our proposed emphasis is the seamless open-source controller-performance-to-editable-set workflow; validate that advantage with users. [Exporting mixes](https://help.dj.studio/en/articles/8106079-exporting-mixes) |
| MixMeister | The official site now says the products have reached end of life and will not receive operating-system support updates. | Treat as a historical workflow reference; prioritize maintainable builds and project longevity. [Official announcement](https://www.mixmeister.com/) |

Traktor, rekordbox, and Mixxx remain references for deck ergonomics, cues, beatmatching, controller mapping, and cue routing. We have not performed a comprehensive hands-on comparison of their current versions, and should not claim that none supports any comparable editing workflow.

## Proposed differentiator to test

The useful question is whether a DJ can perform a transition naturally, see exactly what was captured, change one part without damaging the rest, and hear the same result after reopening or exporting. This combines controller performance with editable arrangement. It requires a stronger project model than a master audio recorder or a raw MIDI event log.

The first user test should compare completing that workflow with the user's current Traktor + audio-editing process. Measure time to correct a transition and confidence in the saved result, rather than comparing feature counts.
