# Mac and S4 MK3 proof of concept

Status: Proposed experiments and acceptance criteria. Updated 2026-11-04. **The software experiments exercise parts of these gates; the complete POC has not passed.** The user-authorized v0.2 slice adds independent clip volume/HP/LP, waveform envelopes, a scrollable arrangement, Preferences and Traktor metadata. [Current results](experiments/2026-11-04-clip-arrangement.md). These explicit additions supersede older two-lane/crossfader-only editor scope below; hardware gates remain open. See [2026-11-04 native prototype evidence](experiments/2026-11-04-native-prototype.md) for measured results and remaining work. The user authorized proceeding without the S4 hardware for now.

## Demonstration that counts as success

On the user's Mac with the S4 MK3, load two local MP3/WAV files from the existing library, arrange an overlap, monitor a deck in headphones, perform a 30-second transition with channel faders/crossfader, edit that recorded automation, and record a replacement portion. Save and close the set, disconnect the controller, reopen it, replay the edited result, and export a WAV using the same rendering core. Use WAV fixtures for strict engine comparisons and include MP3 in the user workflow.

The demo must show that the transition remains editable and that export agrees with the saved project. A controller moving an on-screen knob or an attractive timeline alone does not complete the POC.

## Test configuration

Observed machine: Apple M3 MacBook Air, 16 GB, macOS 26.6.2 (25G83). Confirmed controller: Native Instruments Traktor Kontrol S4 MK3. Firmware, USB connection, available channel/rate configurations, and real latency remain unknown. Record these during qualification without changing firmware as a prerequisite.

Use two short synthetic/redistributable stereo WAVs plus representative user-owned listening material. Start at 48 kHz/128 frames, with 256 frames as an explicitly reported fallback and 64 frames as an optional experiment. Do not imply that configured buffer duration measures total latency. Use the controller's one audio clock for master and cue.

## Sequence and stop conditions

Before committing to P1–P3 implementation, complete the proposed [adoption/reuse gate](research/competitive-workflow-audit.md#proposed-adoption-and-reuse-gate). Evaluate GlideMix's arranger/source-build/controller seams and Mixxx's capture/replay/render seams; benchmark the editing task in DJ.Studio or a focused DAW template. Qualify Density's capture configuration before treating it as an S4 candidate. Record adoption, contribution, extension or rejection with concrete blockers. This research has not run those trials.

The stages below describe the custom implementation fallback. Existing software that solves the personal workflow can end the need for that implementation, but a commercial-only solution does not fulfill an open-source distribution requirement. Reconcile that distinction explicitly in the decision record. Competitor EQ tests do not add EQ to the POC scope.

| Stage | Build or test | Exit evidence |
| --- | --- | --- |
| P0 Decision experiments | Adoption/reuse gate, license inventory, native S4 qualification, bounded reuse/source-build evaluation, browser capability/routing experiment | Recorded adopt/contribute/extend/build decision, stack/host decision if needed, and hardware feature matrix; failed assumptions recorded before editor build |
| P1 Native harness | Two WAV sources, gain/crossfader, Core Audio master/cue, HID play/fader input and LED feedback | Correct physical routing, timestamped semantic controls, basic callback metrics |
| P2 Capture/render core | Initial state + actions + curves, immutable project, offline WAV renderer | Capture/replay/persistence fixtures pass without UI/controller dependencies |
| P3 Minimal editor | Folder browser, drag/drop two clips, waveforms, gain/crossfader lanes, selection/undo | End-to-end demo including one range retake |
| P4 Qualification | Long playback/export, input stress, missing media, crash/device-loss recovery | Published local evidence report and explicitly listed limitations |

Do not continue polishing P3 if P1 cannot separate master/cue or P2 cannot replay the same saved actions. A browser experiment may be performed alongside P0 without committing the product to browser-only delivery. Hardware failure calls for diagnosis and an updated decision, not silent substitution of a MIDI-only demo.

## POC scope boundaries

Two internal stereo lanes/deck roles; manual BPM/first-beat metadata and grid display; fixed project tempo; a static varispeed ratio set per clip before capture; gains/crossfader and start/stop/source-offset capture; one range replacement workflow; MP3 and PCM WAV import; explicit save/reopen/relink; WAV export. Independently armed master PCM recording supports audio-only or combined recording, as well as comparisons. Live rate gestures are deferred until rate-event capture is implemented. The initial audio harness can remain WAV-only; the completed POC must also use MP3.

EQ, high-quality key lock, automatic beat analysis, tempo ramps, four simultaneous decks, arbitrary cue jumps/loops/scratching, controller screens/motors, MP3 export, and full library indexing follow this proof. Read-only controller traces may cover those controls earlier without promising their DSP or capture support. A generic MIDI note/CC output loopback is a small independent P1 experiment; full tempo-clock synchronization follows the tempo engine.

## Acceptance matrix

Targets are initial engineering hypotheses. A miss is reported with cause and remediation; targets are never silently changed to declare success. This matrix governs the complete native POC; shorter 30-minute subsystem runs in research docs are intermediate experiments.

| ID | Criterion | Measurement and pass evidence |
| --- | --- | --- |
| A01 | Exact device identification | Record descriptors/firmware and bind the S4 interface specifically; readable fixtures for required controls |
| A02 | Bidirectional control | Play/cue status and one LED follow engine state; fader pickup avoids a discontinuity; no motor/screen command required |
| A03 | Master/cue separation | Verify four physical output channels individually at low monitor gain; cue-only audio absent from master export |
| A04 | Sustained audio | 60-minute scripted two-source run with UI interaction and background work; zero detected underruns/missed deadlines; no capture loss |
| A05 | Callback headroom | At 128 frames/48 kHz, target p99.9 execution below half the 2.67 ms budget and maximum below deadline; report buffer fallback separately |
| A06 | Real-time discipline | Audit project-owned callback path for allocations/frees, I/O, locks and unbounded work; instrument queues/cache failures |
| A07 | Control latency | Target p95 below 10 ms from timestamped HID receipt to predicted audible output for gain/crossfader, excluding unknown physical-device delay; measure physical/loopback latency separately and report p50/p95/p99, method, and uncertainty |
| A08 | Correct scheduling | Synthetic events execute at the expected engine frame, including block boundaries and same-frame ordering; transport anchors map a nonzero-position retake correctly; late inputs counted |
| A09 | Edit/replay parity | Recorded gain/crossfader take, point edit, and range retake survive save/reopen; isolated step-target fixture proves no unintended interpolation before smoothing; return-region boundaries are continuous; baseline float PCM differs by at most `1e-6` after declared alignment, with matching event frames |
| A10 | Block independence | Render baseline fixture at 64/128/256/512-frame host blocks; no changed action timing; numerical differences within baseline tolerance |
| A11 | Portable project behavior | Reopen without controller; relocate and relink the same assets; reject wrong-content substitutions and unsupported schema writes |
| A12 | Failure handling | Unplug/replug, queue pressure, missing media, failed save, and interrupted recording do not crash or silently corrupt a take; recovery status visible |
| A13 | Usable workflow | User completes arrange → record → edit → retake → save/reopen → WAV export; undo restores the previous transition |
| A14 | MIDI output proof | Timestamped note/CC events reach a virtual endpoint in order; no outgoing hardware events during offline export; this does not certify physical MIDI latency |
| A15 | Bounded long render | Render a 60-minute set from repeated fixture placements without loading all output PCM at once; track peak memory; deliberately slow decoding stalls offline progress without inserting starvation silence; valid file/range/duration, cancel and disk-error behavior |
| A16 | Existing-library import | Load representative MP3/WAV from `~/Music` and synthetic CBR/VBR/bit-depth fixtures; header-based detection handles an audio file with a mismatched suffix; seek positions agree with sequential decoding; source files remain unchanged; errors are explicit |

For A07, record the device output-latency estimate, engine/DSP latency, timestamp uncertainty, and how predicted output time is derived. An internal timestamp is not a physical acoustic measurement. A hardware loopback/reference capture can measure the complete path where equipment permits; report the missing portion if it cannot.

For A09/A10, compare the live core's pre-device PCM with offline PCM using the same inputs and processing configuration. Analog loopback includes converters/noise and uses a different comparison. The simple fixture excludes stretch, effects, resampler differences, and lossy encoding; each gets a later defined test.

## Evidence bundle

Each experiment report includes configuration, pinned dependency/build identifiers, exact steps, fixtures or redistributable fixture recipe, expected versus actual results, logs/plots, and a decision. Latency results distinguish callback execution, input-receipt delay, estimated output latency, and physical end-to-end measurement. Capture completeness is reported separately from audio underruns.

At the end of P4, record whether the POC passed at 128 frames, passed with a documented fallback, or failed specific gates. List supported S4 controls individually. This is evidence for one configuration, not a declaration that every Mac, controller, or operating system is supported.
