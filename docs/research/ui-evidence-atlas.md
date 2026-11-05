# UI evidence atlas

Inspected 2026-11-04. Companion to the [competitive workflow audit](competitive-workflow-audit.md).

This inventory records **33 distinct published images/crops across nine products** that were opened and visually inspected. It includes two small REAPER animated examples viewed as frames. It is not 33 independent workflows, an accessibility audit, or hands-on testing. UI images and trademarks belong to their respective publishers. Images are linked to their original hosts, not copied into this repository; links may change. Observations below describe visible presentation; hypotheses describe what to test.

## GlideMix: seven images

Source: [official manual](https://glidemix.com/manual). The manual and release numbers differ slightly; do not assign every image to v1.1.26. The repository's private “Main FX” preview is excluded.

| Image inspected | Visible observation | Question for Fader/reuse evaluation |
| --- | --- | --- |
| [Full arrangement](https://glidemix.com/img/mix.jpg) | Staggered song lanes; timeline above a dense library area; tempo/energy overview | Can selected-transition controls remain compact enough for a laptop? |
| [Transition mixer](https://glidemix.com/img/mixer-panel.jpg) | Large mixer-style popup, EQ curves, vertical faders and REC | Is the controlled song obvious? Which dim controls are implemented? |
| [Automation](https://glidemix.com/img/auto.jpg) | Curve over waveform and a context menu for separate automation lanes | Can recorded parameters be revealed without searching menus? |
| [Perform view](https://glidemix.com/img/perform.jpg) | Red and white playback cursors, room/cue controls and meters | Can the user tell audience playback from private audition without relying only on color? |
| [Beat-grid editor](https://glidemix.com/img/grid.jpg) | Detailed transients plus song overview and many correction tools | Is correcting one wrong downbeat understandable on first use? |
| [Project table](https://glidemix.com/img/project-tab.jpg) | Song table below timeline and a transition selector | Does reordering preserve intended transitions and selected automation? |
| [Plugin windows](https://glidemix.com/img/plugins.jpg) | Effects sidebar and floating plugin interfaces over the arrangement | Can automation be armed and inspected while an effect window is open? |

![GlideMix arrangement, from the official manual](https://glidemix.com/img/mix.jpg)

The arrangement/mixer split is useful evidence for Fader's compact-mixer proposal. It does not prove controller support; the manual explicitly says knobs and jog wheels are not mapped.

## DJ.Studio: seven images

Source: [Studio View manual](https://help.dj.studio/en/articles/8106005-studio-view-editing-arranging-controls). Images are served through temporary signed Intercom URLs, so the durable parent article and observed filenames are recorded instead of expiring tokens. The article mixes dated screenshots and should not be treated as one version-pinned UI session.

| Image inspected in that article | Visible observation | Question to test |
| --- | --- | --- |
| `CleanShot 2026-08-13 at 14_32_14@2x.png` | Two main decks, several sample lanes, highlighted overlap, bottom details panel | Does the overlap clearly identify what will be edited? |
| `SCR-20261009-mloz-2.png` | Arrangement overview, navigation and phrase-scale presentation | Can users move between whole-set and one-transition views without losing selection? |
| `preview effects.png` | Playback mode message says adjustments are not saved | Does the message prevent accidental loss of a good gesture? |
| `edit mode.png` | Paused editing message says changes are saved automatically | Is the change from preview to edit predictable? |
| `image.png` | Close arrangement view with samples and a highlighted transition | Do additional layers obscure the two songs being mixed? |
| `SCR-20261009-mukz-2.png` | Tracklist with artwork and metadata columns | Is ordering a set easier here than dragging long clips? |
| `SCR-20261009-mvvz-2.png` | Video/background/spectrum controls in the details area | Can unrelated production features stay out of the primary correction task? |

The important observation is the visible distinction between previewing and saving a change. The screenshot cannot establish how well the distinction works with real users or whether a hardware action follows the same path.

## Density: four images

Source: [official product page](https://density.one/). These are promotional UI excerpts with no reliable build identifier.

| Image inspected | Visible observation | Question to test |
| --- | --- | --- |
| [Set editor](https://density.one/static/media/edit-set-img.a612de029fe518b23724.png) | Song navigator, staggered waveforms, overview and parameter curves | Is a transition easy to locate in a long set? |
| [Captured mixer data](https://density.one/static/media/mixer-set-data.43908c58f3cac7d5c5cb.png) | Dense sequences of automation points over a waveform | Can users simplify or correct a gesture without damaging its shape? |
| [EQ lanes](https://density.one/static/media/EQ%20lanes%201.051a953d06f91e3fde3d.png) | Separate high/mid/low lanes consume substantial vertical space | Should the default show used/selected parameters only? |
| [Effect selector](https://density.one/static/media/Fx_2%201.811504918123f4bbc980.png) | Effect choices displayed beside the timeline | Are reconstructed effects audibly equivalent to the source hardware? |

![Density set editor, from its product page](https://density.one/static/media/edit-set-img.a612de029fe518b23724.png)

This is the clearest visual precedent for making a recorded hardware mix inspectable. Audio fidelity, exact capture support and selective retakes require separate evidence.

## MixCanvas: four images

Source: [repository README](https://github.com/sebedard-creator/MixCanvas). Images on `main` are not guaranteed to match the v1.8.2 binary exactly. The grid screenshot contains the author's example local path; it is not transcribed here.

| Image inspected | Visible observation | Question to test |
| --- | --- | --- |
| [Timeline](https://raw.githubusercontent.com/sebedard-creator/MixCanvas/main/assets/screenshots/timeline.png) | Three pale waveform lanes; console/transport above; browser to the right | Are many small icon controls understandable without prior knowledge? |
| [Beat-grid editor](https://raw.githubusercontent.com/sebedard-creator/MixCanvas/main/assets/screenshots/beatgrid-editor.png) | Numbered correction workflow, waveform, BPM/downbeat fields | Does the guided sequence reduce grid-correction errors? |
| [Mix FX](https://raw.githubusercontent.com/sebedard-creator/MixCanvas/main/assets/screenshots/mix-fx.png) | Compact grid of effect pads and labels | Can users understand what is being recorded and which lane owns it? |
| [Bounce](https://raw.githubusercontent.com/sebedard-creator/MixCanvas/main/assets/screenshots/bounce.png) | Format choices and mastering/limiter settings are explicit | Can a user reproduce monitoring settings in the export? |

![MixCanvas timeline, from its repository](https://raw.githubusercontent.com/sebedard-creator/MixCanvas/main/assets/screenshots/timeline.png)

The timeline looks close to the proposed broad layout. Its documented lane-owned automation remains a separate semantic choice; visual similarity does not establish interchangeable project behavior.

## Mixxx: three images

Source: [2.5 UI manual](https://manual.mixxx.org/2.5/en/chapters/user_interface). The manual describes the LateNight PaleMoon skin using some older illustrations; these are not a claim that every current skin looks identical.

| Image inspected | Visible observation | Question to test |
| --- | --- | --- |
| [Full interface](https://manual.mixxx.org/2.5/en/_images/mixxx-interface.png) | Deck waveforms and mixer above a large browser | Where would an editable-set overview fit without harming live operation? |
| [Deck sections](https://manual.mixxx.org/2.5/en/_images/deck-sections.png) | Cue, loop and tempo controls grouped around the current deck | Which controls belong in a compact editor strip versus a performance workspace? |
| [Grid controls](https://manual.mixxx.org/2.5/en/_images/latenight-beatgrid-controls.png) | Grid correction is adjacent to deck waveforms | Are correction tools discoverable without cluttering performance mode? |

## Ableton Live: three images

Source: [Live 12 automation manual](https://www.ableton.com/en/live-manual/12/automation/).

| Image inspected | Visible observation | Question to test |
| --- | --- | --- |
| [Clip automation](https://ableton-production.imgix.net/live-manual/12/ClipAutomationEnvelopeL12.png?auto=&w=1280) | Dedicated envelope area, parameter selection and linked-state label | Is automation's owner and time basis apparent? |
| [Arrangement automation lane](https://ableton-production.imgix.net/live-manual/12/AutomationLaneL12.png?auto=&w=1280) | Named parameter and curve aligned with arrangement content | Can users inspect one parameter without losing musical context? |
| [Waveform envelope](https://ableton-production.imgix.net/live-manual/12/AutomationEnvelopeL12.png?auto=&w=852) | A small number of clearly visible volume points over audio | Can a correction be made directly, with exact values when needed? |

These examples support separating clip and arrangement contexts. They do not establish S4 MK3 compatibility or spontaneous DJ-library ergonomics.

## VirtualDJ: two images

Source: [Automix Editor manual](https://virtualdj.com/manuals/virtualdj/editors/automixeditor.html).

| Image inspected | Visible observation | Question to test |
| --- | --- | --- |
| [Editor](https://virtualdj.com/img/295326/66267/AutomixEditor.png) | Pair list, two waveforms, overlap and PFL controls | Does focusing on one pair make correction faster? |
| [Overlap adjustment](https://virtualdj.com/img/295330/32158/AutomixEditorHow3.png) | Transition overlap is the main manipulation target | Can the pair view coexist with full-set timing and automation ownership? |

This is evidence for a focused transition editor, not a general performance-capture timeline.

## REAPER: two small animated examples

Source: [official homepage](https://www.reaper.fm/). These are small feature animations; inspected frames support narrow observations, not a full interface assessment.

| Image inspected | Visible observation | Question to test |
| --- | --- | --- |
| [Razor edits](https://www.reaper.fm/v7img/razor.gif) | Selection spans waveform and an automation envelope | Can the user include/exclude automation when moving a range? |
| [Render statistics](https://www.reaper.fm/v7img/renderstats.gif) | Render-related curves appear in a dedicated visual report | What minimal diagnostics make an export trustworthy? |

## Audacity: one image

Source: [official homepage](https://www.audacityteam.org/).

The [current hero image](https://www.audacityteam.org/_astro/hero-surface-studio-darkened.Bg6vZ4zE_Z1E7Af.webp) is a perspective product mockup showing colored clip lanes, a shared ruler, track controls and transport. It supports the waveform-first direction but is not a detailed interaction or accessibility reference. “Audacity-like” should mean readable time, clips and editing affordances, rather than copying a remembered version's appearance.

## What the images cannot establish

No image establishes latency, audio quality, exact hardware support, keyboard accessibility, undo correctness, crash recovery or live/export parity. Several illustrations omit the state needed to evaluate retakes. Use the [common-task protocol](competitive-workflow-audit.md#proposed-adoption-and-reuse-gate) to close those gaps. A missing screenshot is not evidence that a feature is missing, and a drawn control is not evidence that it works.
