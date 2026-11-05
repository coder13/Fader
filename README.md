# Fader

An open-source DJ application for preparing, performing, editing, and exporting sets. The central feature is a performance that remains editable: record musical actions and automation, revise a transition on a timeline, and render the finished set without performing it again.

**Current stage: native POC validation.** A [C++20/JUCE v0.2 prototype](https://github.com/coder13/Fader/blob/3a3b58456065f6cfed4c9f23f1a0252047d80a89/README.md) implements clip-owned automation, an arrangement editor, library/Traktor metadata, save/reopen and WAV export. Its software tests pass; sustained streaming, recovery, Windows and S4 hardware gates remain open. [10 milestones and 43 issues](docs/roadmap-and-issue-backlog.md) now track the path from POC to product. Fader is the working project name.

## Start here

| Document | Purpose |
| --- | --- |
| [Roadmap completeness review](docs/research/roadmap-completeness-review.md) | Feature coverage, proposed backlog gaps and evidence needed for a polished release |
| [Competitive workflow audit](docs/research/competitive-workflow-audit.md) | Deeper comparison, source evidence, adoption/reuse decision gate |
| [UI evidence atlas](docs/research/ui-evidence-atlas.md) | 33 inspected published UI images/crops across nine products |
| [Audacity 4 design lessons](docs/research/audacity-4-design-lessons.md) | Video/transcript research translated into Fader interaction proposals |
| [Product and UX](docs/product-and-ux.md) | User workflows, Audacity-inspired interface, feature priorities |
| [Architecture](docs/architecture.md) | Real-time engine, editor, hardware, and export boundaries |
| [Set format and automation](docs/set-format-and-automation.md) | Editable performance semantics, time domains, persistence |
| [Plugin effects for v2/v3](docs/plugin-effects.md) | VST3 racks, parameter automation, state, latency, and export |
| [Audio import and library compatibility](docs/audio-import-and-library.md) | Common formats, existing-library workflow, and qualification |
| [POC plan](docs/poc-plan.md) | Smallest useful Mac + S4 MK3 demonstration and pass/fail evidence |
| [Roadmap and issue backlog](docs/roadmap-and-issue-backlog.md) | Published milestones, dependencies and POC-to-product exit gate |
| [Stack decision](docs/decisions/0001-platform-and-engine.md) | Conditional C++ fallback after application/engine reuse evaluation |
| [Risks and open questions](docs/risks-and-open-questions.md) | Unresolved decisions and how to resolve them |
| [Experiment report template](docs/experiments/README.md) | Evidence format for hardware, timing, and design decisions |
| [Audio engine research](docs/research/audio-engine-and-stack.md) | Frameworks, real-time audio, DSP, licensing |
| [Controller research](docs/research/traktor-and-controller-integration.md) | Exact S4 MK3 integration and other controller families |
| [Web research](docs/research/web-platform.md) | Browser, Electron, Tauri, Web Audio/Wasm, device access |
| [Product references](docs/research/product-references.md) | Candidate index and workflow references |

## Confirmed requirements

- Build a **DJ performance editor with selected DAW tools**. MIDI composition, instruments and studio multitrack recording are not current product requirements.
- Open source; macOS first, Windows required, Linux a bonus; investigate web as a platform.
- Initial hardware is **Native Instruments Traktor Kontrol S4 MK3**.
- Arrange local audio on extensible rows; move clips without resizing them and draw independent volume/HP/LP envelopes directly on each waveform.
- Use a flat dark interface, discoverable context menus, a Traktor-style metadata browser and read-only Traktor import; configure the DJ folder in Preferences.
- Model the UI on the lessons of the Audacity 4 redesign: understandable defaults, direct and reversible editing, discoverable controls, and accessible presentation; validate the concrete DJ interactions.
- Use the existing `~/Music` library directly: MP3/WAV import in the completed POC, broader common DJ formats and collection-wide qualification for the usable alpha.
- Record a performance as automation, optionally alongside audio; record replacement sections of a prepared set.
- Save portable, versioned set projects and export WAV, MP3, and further formats in stages.
- Support tempo changes, controller input/feedback, and external MIDI output in the roadmap.
- Version 2/3: host third-party audio effects, starting with VST3, with editable parameter automation and saved plugin state; excluded from the POC.

## Current direction

Continue the C++20/JUCE prototype while testing the risks that could change the production architecture: bounded streaming, fast cancellable seeking, sustained audio/capture, real S4 HID/master-cue behavior and early Windows compilation. The [published roadmap](docs/roadmap-and-issue-backlog.md) contains the next work and the [POC exit decision](https://github.com/coder13/Fader/issues/17).

The [competitive audit](docs/research/competitive-workflow-audit.md) remains evidence for bounded reuse investigations. The working prototype is now the comparison baseline; language/framework choice is provisional until the qualification and architecture decision. Web/hybrid delivery remains a measured scope decision. VST3 effects are planned for product versions 2 and 3, after the native POC.

## Keeping these documents useful

This repository is the source of truth for design. Each implementation issue should link its relevant design and acceptance criteria. Update the design when behavior changes, and add an architecture decision record for decisions with lasting consequences. Use statuses **Proposed**, **Accepted**, or **Superseded**; a recommendation does not become accepted just because it appears here.

Record experiments under `docs/experiments/` with date, commit, dependency versions, machine/OS, firmware, settings, measurements, and conclusion. Keep research claims linked to primary sources and recheck changing claims at the point of adoption. Preserve rejected alternatives and their reasons in decision records. Keep hardware support claims tied to completed qualification reports.

Repository: [coder13/Fader](https://github.com/coder13/Fader). The user authorizes direct commits and pushes of documentation updates to `main`; see [working conventions](AGENTS.md). [GitHub issues](https://github.com/coder13/Fader/issues) and [milestones](https://github.com/coder13/Fader/milestones) are published; the roadmap maps stable planning IDs to them. The experimental implementation declares AGPL-3.0-or-later; the maintainer's final product licensing preference remains deferred.
