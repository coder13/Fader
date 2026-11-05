# Fader

An open-source DJ application for preparing, performing, editing, and exporting sets. The central feature is a performance that remains editable: record musical actions and automation, revise a transition on a timeline, and render the finished set without performing it again.

**Current stage: native arrangement prototype, v0.2.** This branch adds a scrollable DAW-style arrangement with fixed-duration clip dragging; clip-owned volume, high-pass and low-pass envelopes drawn on waveforms; a searchable metadata browser; Preferences; and read-only Traktor collection import. Recording/retakes, set persistence, MP3/WAV import and WAV export use the same rendering core. See the [quickstart](docs/poc-quickstart.md), [current evidence and limits](docs/experiments/2026-11-04-clip-arrangement.md), and [clip-automation decision](docs/decisions/0002-clip-owned-automation.md). The complete Mac/S4 POC has not passed: controller integration and sustained performance qualification remain pending. Fader is the working project name.

## Start here

| Document | Purpose |
| --- | --- |
| [Competitive workflow audit](docs/research/competitive-workflow-audit.md) | Deeper comparison, source evidence, adoption/reuse decision gate |
| [UI evidence atlas](docs/research/ui-evidence-atlas.md) | 33 inspected published UI images/crops across nine products |
| [Product and UX](docs/product-and-ux.md) | User workflows, Audacity-inspired interface, feature priorities |
| [Theme and interaction](docs/theme-and-interaction.md) | Flat dark palette, shared controls, context menus and shortcuts |
| [Architecture](docs/architecture.md) | Real-time engine, editor, hardware, and export boundaries |
| [Set format and automation](docs/set-format-and-automation.md) | Editable performance semantics, time domains, persistence |
| [Plugin effects for v2/v3](docs/plugin-effects.md) | VST3 racks, parameter automation, state, latency, and export |
| [Audio import and library compatibility](docs/audio-import-and-library.md) | Common formats, existing-library workflow, and qualification |
| [POC plan](docs/poc-plan.md) | Smallest useful Mac + S4 MK3 demonstration and pass/fail evidence |
| [Roadmap and issue backlog](docs/roadmap-and-issue-backlog.md) | Dependency-ordered milestones and draft GitHub issues |
| [Stack decision](docs/decisions/0001-platform-and-engine.md) | Conditional C++ fallback after application/engine reuse evaluation |
| [Risks and open questions](docs/risks-and-open-questions.md) | Unresolved decisions and how to resolve them |
| [Experiment report template](docs/experiments/README.md) | Evidence format for hardware, timing, and design decisions |
| [Audio engine research](docs/research/audio-engine-and-stack.md) | Frameworks, real-time audio, DSP, licensing |
| [Controller research](docs/research/traktor-and-controller-integration.md) | Exact S4 MK3 integration and other controller families |
| [Web research](docs/research/web-platform.md) | Browser, Electron, Tauri, Web Audio/Wasm, device access |
| [Product references](docs/research/product-references.md) | Candidate index and workflow references |

## Confirmed requirements

- Open source; macOS first, Windows required, Linux a bonus; investigate web as a platform.
- Initial hardware is **Native Instruments Traktor Kontrol S4 MK3**.
- Arrange local audio on a scrollable grid; move clips in time and between tracks without changing their length. Draw independent volume, high-pass and low-pass automation on each waveform.
- Use a Traktor-inspired table with title, artist, BPM, key and other metadata. Configure the DJ subfolder in Preferences and import existing Traktor metadata read-only.
- Use a flat, dark, clean interface informed by Traktor and Audacity. Keep common editing controls visible; put occasional actions in discoverable context menus.
- Use the existing `~/Music` library directly: MP3/WAV import in the completed POC, broader common DJ formats and collection-wide qualification for the usable alpha.
- Record a performance as automation, optionally alongside audio; record replacement sections of a prepared set.
- Save portable, versioned set projects and export WAV, MP3, and further formats in stages.
- Support tempo changes, controller input/feedback, and external MIDI output in the roadmap.
- Version 2/3: host third-party audio effects, starting with VST3, with editable parameter automation and saved plugin state; excluded from the POC.

## Current recommendation

The user requested a POC and authorized continuing without hardware. The native code on this branch is a bounded experiment in capture/edit/replay, not a final decision to reject reuse or commit to a production stack. The research recommendations below remain open.

**Evaluate adoption or a focused contribution before building another audio engine.** The [deeper audit](docs/research/competitive-workflow-audit.md) identifies GlideMix as the leading arranger-reuse investigation and Mixxx as the strongest documented S4 performance foundation. Density and DJ.Studio are important workflow benchmarks; MixCanvas and a focused DAW template are additional alternatives. None has passed the user's complete workflow here. Published UI/manual evidence is not hands-on qualification.

The next proposed milestone is an adoption/reuse decision: perform or record one transition, correct one parameter, replace a range, move the transition, save/reopen and export. Record exact blockers. Personal adoption of commercial software may solve the immediate workflow but does not replace the confirmed open-source product requirement.

If existing applications prove unsuitable, evaluate Tracktion Engine before a custom **C++20 headless audio core with a thin JUCE desktop host**. The existing native architecture remains a conditional fallback, subject to license/dependency review and measured S4 HID/Core Audio results. Web UI and browser-only hardware experiments remain proposals. No stack or product pivot has been accepted.

The first complete POC imports two tracks, records a short S4 transition, edits its automation, saves/reopens it, and exports WAV. Full controller screens, motors, four-deck performance, and a complete DAW are later milestones.

## Keeping these documents useful

This repository is the source of truth for design. Each implementation issue should link its relevant design and acceptance criteria. Update the design when behavior changes, and add an architecture decision record for decisions with lasting consequences. Use statuses **Proposed**, **Accepted**, or **Superseded**; a recommendation does not become accepted just because it appears here.

Record experiments under `docs/experiments/` with date, commit, dependency versions, machine/OS, firmware, settings, measurements, and conclusion. Keep research claims linked to primary sources and recheck changing claims at the point of adoption. Preserve rejected alternatives and their reasons in decision records. Keep hardware support claims tied to completed qualification reports.

Repository: [coder13/Fader](https://github.com/coder13/Fader). The user authorizes direct commits and pushes of documentation updates to `main`; see [working conventions](AGENTS.md). GitHub issues have not been published; the local backlog is ready for review and publication. The experimental implementation carries AGPL-3.0-or-later in [LICENSE](LICENSE), compatible with the chosen JUCE route. The user deferred license preferences; this does not record a final product licensing decision.
