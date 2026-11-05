# Active issue work order

Updated 2026-11-04 after reviewing all 64 GitHub issues and the user's request to incorporate the new issues into prioritization. Work proceeds one issue at a time, preserving the full non-hardware objective. This is an execution order, not a reduction of the roadmap or a change to acceptance criteria. GitHub holds current completion status.

## Immediate POC critical path

**#6, #7, #8, #11 and #38 are completed.** #39 and #13 await native visual inspection on an unlocked Mac. Their software/CI checks pass. **#40 is in progress** with the [declared software workload and short-run results](experiments/2026-11-05-software-stress.md).

| Order | Issue | Why it comes here |
| --- | --- | --- |
| Completed | [#8](https://github.com/coder13/Fader/issues/8), event timing and capture failure | [Qualified clock/capture integrity](experiments/2026-11-04-playback-clock.md), including the interruption/stop race and retention of partial-take data. |
| Completed | [#38](https://github.com/coder13/Fader/issues/38), bounded source streaming | [Bounded PCM, long-file/codec qualification and native gap-warning verification passed](experiments/2026-11-04-bounded-streaming.md). Import still scans/hashes whole files. |
| 1 · in progress | [#39](https://github.com/coder13/Fader/issues/39), cancellable state-correct seeking | [Implementation, local tests and timing evidence](experiments/2026-11-04-cancellable-seeking.md) are published; exact-source Linux/macOS CI passes; native Preparing/Cancel inspection awaits an unlocked Mac. The uncached MP3 fallback is explicitly slower. |
| Completed | [#11](https://github.com/coder13/Fader/issues/11), stable project model | [Format 3 identities, decoder-domain preservation, strict validation and migration](project-format-v3.md) pass local and clean Linux/macOS CI qualification. #39 remains separately open for native visual inspection. |
| 3 · in progress | [#13](https://github.com/coder13/Fader/issues/13), live/offline render parity | [Local event/range parity, failure retention and a real 60-minute export pass](experiments/2026-11-05-offline-render.md). Clean Linux/macOS CI passes. Native Cancel export/progress inspection awaits an unlocked Mac; independent #40 software preparation may proceed against the qualified backend. |
| 4 · in progress | [#40](https://github.com/coder13/Fader/issues/40), sustained load/failure qualification | The [128/256-frame hours](experiments/2026-11-05-software-stress-followup.md) preserve controls/audio and pass save/memory but still fail timing and painter starts. Investigate the 128-frame boundary tails and the 256-frame startup wake delays with checked CPU/painter diagnostics; keep physical output/acoustic claims separate. |

The new product issues explicitly preserve these priorities. A failed prerequisite is repaired before advancing. CI waits can be used for independent review or fixture preparation; no dependent issue is closed without its evidence.

## Complete the software POC

[#41](https://github.com/coder13/Fader/issues/41) is completed for its early software portability scope. [Windows/macOS common-project evidence](experiments/2026-11-05-windows-portability.md) passes clean CI, while actual driver/UI/installer qualification remains later work. Repair the failed #40 sustained targets, then complete the capture/edit/recovery path in dependency order: [#12](https://github.com/coder13/Fader/issues/12), [#14](https://github.com/coder13/Fader/issues/14), [#15](https://github.com/coder13/Fader/issues/15), [#16](https://github.com/coder13/Fader/issues/16), [#35](https://github.com/coder13/Fader/issues/35) and the owner session [#42](https://github.com/coder13/Fader/issues/42). Owner observations require the owner's actual participation; automated tests cannot stand in for them.

Also finish virtual MIDI output [#10](https://github.com/coder13/Fader/issues/10), dependency/reuse/native-versus-web evidence [#1](https://github.com/coder13/Fader/issues/1), [#3](https://github.com/coder13/Fader/issues/3), [#5](https://github.com/coder13/Fader/issues/5), and architecture synthesis [#37](https://github.com/coder13/Fader/issues/37). The maintainer's deferred license preference is not silently assumed. These remain required evidence, not discarded work. The complete POC gate [#17](https://github.com/coder13/Fader/issues/17) stays open while hardware or owner evidence is missing.

## Incorporate the new product issues

**[#44](https://github.com/coder13/Fader/issues/44) is the first product-planning issue, before broader M3/M4 implementation.** It selects the v1 subset and support/scale targets using the POC evidence; it must not postpone the critical streaming/seek/load path. Optional portions remain linked with explicit decisions rather than vanishing from the backlog.

After that contract, take the next dependency-ready issue from these groups. Preserve child-feature evidence when closing the existing larger epics.

| Priority group | Existing foundations and new follow-ups | Intended outcome |
| --- | --- | --- |
| Musical timing and safe mixing | #18–#22 with [#47](https://github.com/coder13/Fader/issues/47), [#48](https://github.com/coder13/Fader/issues/48), [#52](https://github.com/coder13/Fader/issues/52); software portions of [#50](https://github.com/coder13/Fader/issues/50) | Defined sync/cue/loop behavior, capture/replay correctness, clear levels and safe deck ownership. |
| Everyday prepared-set editing and library | #23, #36, #43 with [#45](https://github.com/coder13/Fader/issues/45), [#46](https://github.com/coder13/Fader/issues/46), [#51](https://github.com/coder13/Fader/issues/51); selected [#49](https://github.com/coder13/Fader/issues/49) harmonic features | Precise edits, useful organization and complete local-library workflows. |
| Delivery and recovery | #24, #25 with [#53](https://github.com/coder13/Fader/issues/53) | Reopen, relocate and deliver the selected outputs reliably. |
| Design validation and release usability | #42 feeds [#55](https://github.com/coder13/Fader/issues/55); #45/#46/#51 feed [#54](https://github.com/coder13/Fader/issues/54); #14/#55 feed [#56](https://github.com/coder13/Fader/issues/56) | Owner-approved layout, repaired usability findings, keyboard/accessibility and display-size evidence. These are release work, not optional polish. |
| Integrated native release | #26/#27 and [#57](https://github.com/coder13/Fader/issues/57) | Fresh end-to-end Mac/Windows qualification of the selected features. Compilation alone cannot close the release gate. |

The layout/accessibility test plan should inform earlier UI changes, but #54–#56 are not marked complete without their required real observations, approval and tests. Hardware-dependent parts of mixed issues remain explicitly open; progress on independent software does not qualify the physical device.

## Later tracks and current exclusions

Web delivery #29/#30, Linux #32 and VST3 #33/#34 retain their separate stages; VST3 stays v2/v3. Non-hardware optional decisions #58/#60/#61/#63/#64 remain in scope for evidence-backed adopt/defer/reject outcomes after the core product path. They are decision tasks, not automatic feature-build commitments.

Physical S4/browser routing #2/#4/#9, second-controller qualification #28, advanced S4 #31, DVS #59 and external-input hardware trials in #62 are excluded from the present hardware-free run. Physical portions of Windows #27, Linux #32, web #30, deck ownership #50 and release gates remain unqualified. Virtual endpoints and synthetic clock/device-failure fixtures are allowed software tests. None may be described as successful physical-device qualification.
