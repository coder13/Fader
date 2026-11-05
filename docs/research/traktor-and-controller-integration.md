# Traktor S4 MK3 and controller integration

Research date: 2026-11-04. Status: proposed design; no hardware integration has been implemented or qualified.

The user has confirmed a **Native Instruments Traktor Kontrol S4 MK3** with four mixer channels. The observed development machine is an Apple M3 MacBook Air (Mac15,13), 16 GB RAM, running macOS 26.6.2 (25G83). These observations identify the first test configuration; they are not a compatibility test.

## Recommended first target

Build a native **HID adapter for S4 MK3 controls**, use **Core Audio for its audio interface**, and keep general-purpose **MIDI input/output as a separate subsystem**. Start with two software decks and ordinary controls on the four-channel hardware. Leave motor drive, haptic effects, controller screens, and four simultaneous decks for later milestones.

S4 MK3 is not a generic MIDI controller. NI's official MIDI-mode table says unlisted controllers cannot switch to MIDI mode, and lists S4 MK1/MK2 but not MK3. A statement by NI staff explicitly confirms S4 MK3 never implemented MIDI mode. HID is the working third-party path demonstrated by Mixxx, so a MIDI-only controller POC would miss the user's hardware. [NI MIDI modes](https://support.native-instruments.com/support/solutions/articles/69000880031-native-instruments-switching-your-controller-to-midi-mode), [NI staff explanation](https://community.native-instruments.com/discussion/9022/why-does-the-s4mk3-still-not-have-midi-mode/p3).

Mixxx 2.5 documents class-compliant audio and HID support on macOS, Windows, and Linux. Its suggested audio routing is master on channels 1–2 and headphones on 3–4. The controller requires its external power supply. These are useful starting expectations to verify on our application and exact firmware. [Mixxx S4 MK3](https://manual.mixxx.org/2.5/en/hardware/controllers/native_instruments_traktor_kontrol_s4_mk3).

The native transport terminology needs care: HID standardizes the transport, **not the meaning of NI's reports**. We still need device-specific parsing, initialization, output reports, state management, and tests. A controller appearing in an OS device list is not evidence that our mappings, audio routes, or timing work.

## Evidence by controller generation

This matrix prevents future work from treating the Traktor product family as one protocol. “Evidence” refers to upstream documentation, not qualification of our software.

| Model | Control integration evidence | Audio/platform implications | Priority |
| --- | --- | --- | --- |
| **S4 MK3 — user's device** | HID buttons, knobs, faders, LEDs; separate USB bulk screen transport; no MIDI mode | Class-compliant audio; independently qualify master and cue routes | First POC |
| S2 MK3 | HID mapping exists; firmware MIDI mode entered by holding left FLX while connecting | Integrated interface; main 1–2, headphones 3–4 in Mixxx | Second all-in-one candidate |
| S3 | HID mapping exists; NI documents firmware MIDI mode | Class-compliant audio/HID on macOS/Linux; Windows backend must be qualified | Later four-deck device |
| S2 MK2 | HID; NI also documents MIDI-mode switching | Class-compliant on macOS/Linux; Windows driver path documented | Later legacy generation |
| S4 MK2 | HID for surface; NI documents MIDI mode | Has separate rear-panel MIDI ports; an enumerated MIDI port is not necessarily surface control | Later legacy generation |
| S2/S4 MK1 | NI lists MIDI modes, but these rely on legacy compatibility assumptions | NI states their operability ended with macOS 10.15; unsuitable current-Mac baseline | Excluded initial target |
| X1 MK1 | NI MIDI-mode entry documented | Same legacy macOS problem | Excluded initial target |
| X1 MK2/MK3 | NI documents model-specific MIDI-mode entry | Qualify as an additional control surface; do not infer S4-equivalent audio or jog capabilities | Later modular surface |
| Z1 original | HID and audio class compliance documented; NI lists MIDI mode | Integrated master/cue audio; Windows NI service can contend for LED control | Later compact mixer |
| Z1 MK2 | NI lists MIDI-mode entry | Do not reuse original Z1 HID reports without qualification | Later compact mixer |

Sources for the matrix: [NI MIDI-mode table](https://support.native-instruments.com/support/solutions/articles/69000880031-native-instruments-switching-your-controller-to-midi-mode), [S2 MK3 firmware MIDI instructions](https://support.native-instruments.com/support/solutions/articles/69000879412-traktor-switching-your-kontrol-s2-mk3-to-midi-mode), [Mixxx S2 MK3](https://manual.mixxx.org/2.5/en/hardware/controllers/native_instruments_traktor_kontrol_s2_mk3), [Mixxx S3](https://manual.mixxx.org/2.5/en/hardware/controllers/native_instruments_traktor_kontrol_s3), [Mixxx S2 MK2](https://manual.mixxx.org/2.5/en/hardware/controllers/native_instruments_traktor_kontrol_s2_mk2), [Mixxx S4 MK2](https://manual.mixxx.org/2.5/en/hardware/controllers/native_instruments_traktor_kontrol_s4_mk2), [Mixxx Z1](https://manual.mixxx.org/2.5/en/hardware/controllers/native_instruments_traktor_kontrol_z1), [NI macOS compatibility](https://support.native-instruments.com/support/solutions/articles/69000879296-compatibility-of-native-instruments-products-on-macos).

NI currently lists S4 MK3 as Apple Silicon compatible and compatible with macOS 26. Its product page retains a specific warning about malfunction and possible controller damage on **macOS 11 Big Sur**. That is not the observed macOS version, but it belongs in any future supported-platform matrix. [NI macOS compatibility](https://support.native-instruments.com/support/solutions/articles/69000879296-compatibility-of-native-instruments-products-on-macos), [S4 product requirements](https://www.native-instruments.com/products/traktor-kontrol-s4).

## What can be replayed and exported

The application should record **resolved musical controls**, such as deck gain, EQ, crossfader, cue jumps, and track rate, after interpreting the HID/MIDI mapping. Persisting only raw USB reports would tie a set to a device, firmware, mapping version, and transient modifier state.

There are three distinct classes of physical action:

| Action | Set recording rule |
| --- | --- |
| Software mix change, e.g. deck EQ or channel fader | Record the parameter value actually applied by the engine, with its effective timeline position |
| Monitor-only hardware change, e.g. physical output level | Keep outside the rendered mix; identify it as monitoring rather than editable set automation |
| External audio entering the system | Record audio if it must survive replay; automation cannot recreate a microphone or external player |

This distinction is material on the S4 MK3. NI says MAIN and CUE VOL affect hardware output; S3/S4 MK3 also have hardware cue monitoring. Mixxx's source separately exposes optional software mapping for main, booth, and headphone controls and warns that using both paths applies changes to both software and hardware. This is **not a claim that all four deck-channel faders or EQs are hardware-only**. [NI hardware volume explanation](https://support.native-instruments.com/hc/en-us/articles/210273405-Traktor-does-not-react-to-the-Main-Level-or-Cue-Volume-knobs-on-my-Traktor-Kontrol-Controller), [Mixxx S4 MK3 preset](https://github.com/mixxxdj/mixxx/blob/2.5/res/controllers/Traktor%20Kontrol%20S4%20MK3.hid.xml).

Recommendation: export the software master bus, upstream of monitor volume. During hardware qualification, exercise every candidate parameter and compare software state, digital recording, and physical output. Do not label a set “fully captured” if an audible external signal bypassed capture. NI documents that S4 MK3 microphone and line inputs share channels, which must be accounted for when later adding external-input recording. [NI microphone routing](https://support.native-instruments.com/support/solutions/articles/69000879459-how-to-use-a-microphone-with-the-traktor-kontrol-s4).

For an absolute fader moved after automation has changed its software value, use pickup/soft takeover by default: require it to cross the current value before taking control. Define separate behavior for relative encoders and jog movement. A physical knob generally lacks touch sensing, so an automation “touch” gesture needs a documented inactivity timeout or explicit record/override control. Preserve gesture boundaries and the accepted engine values; offer an unsimplified take before reducing automation points.

## Native adapter design

Recommended separation:

```text
S4 HID input -> device reader -> S4 report parser -> mapping -> control commands
Other MIDI  -> MIDI backend ----------------------^             |
                                                               v
UI edits ------------------------------------------> engine scheduling
                                                               |
                                       applied values/events -> take recorder

engine state -> feedback snapshot -> rate-limited HID/MIDI output worker
engine audio -> Core Audio -> master/cue device channels
```

All device discovery, report reads/writes, mapping scripts, logging, reconnect handling, and output throttling stay outside the audio callback. Commands cross bounded queues; capture records what the engine accepted. Queue exhaustion must be observable. Coalesce expendable meter feedback before transport/button events; never silently discard a recording-critical event and claim complete capture.

HIDAPI is a reasonable cross-platform transport candidate: its maintained implementation covers macOS, Windows, and Linux and offers BSD-style licensing among its license options. Linux requires suitable device permissions/udev rules. It does not supply an S4 protocol implementation. [HIDAPI repository](https://github.com/libusb/hidapi), [HIDAPI licenses](https://github.com/libusb/hidapi/blob/master/LICENSE.txt).

The inspected Mixxx 2.5 preset identifies S4 MK3 as vendor `0x17cc`, product `0x1720`, usage page `0xff01`, usage `0x0`, interface `0x3`. Use these as evidence-backed discovery hints; inspect actual descriptors and firmware before binding. The S2 MK3 has a different product ID (`0x1710`) and usage (`0x1`). Do not bind all NI devices by vendor ID alone. [S4 preset](https://github.com/mixxxdj/mixxx/blob/2.5/res/controllers/Traktor%20Kontrol%20S4%20MK3.hid.xml), [S2 preset](https://github.com/mixxxdj/mixxx/blob/2.5/res/controllers/Traktor%20Kontrol%20S2%20MK3.hid.xml).

Store mapping ID/version, transport, model, firmware, stable endpoint identity where available, absolute/relative encoding, range, inversion, dead zone, modifier behavior, and feedback capability. Normalize to stable application parameter IDs. A reconnected device gets a fresh state handshake and pickup state; reconnect must not unexpectedly start playback or overwrite an existing take.

Use Mixxx's mapping and parser as inspectable reference material. It is not a drop-in library: it depends on Mixxx's engine/control APIs. Mixxx is GPL-2.0-or-later; reuse needs a chosen compatible project license, retained notices, and file/dependency review. The user's open-source preference makes reuse worth investigating but does not itself select a license. [Mixxx license](https://github.com/mixxxdj/mixxx/blob/main/LICENSE), [S4 mapping source](https://github.com/mixxxdj/mixxx/blob/2.5/res/controllers/Traktor-Kontrol-S4-MK3.js).

### Timing and MIDI output

On macOS, Core MIDI timestamps use the host clock (`mach_absolute_time()`); output event timestamps can schedule delivery. Correlate that host clock with audio sample time using a maintained clock mapping. For HID, record the earliest host receipt timestamp available and measure polling/USB uncertainty rather than claiming knowledge of the exact physical movement time. [Apple MIDITimeStamp](https://developer.apple.com/documentation/coremidi/miditimestamp), [Apple MIDI event timestamp](https://developer.apple.com/documentation/coremidi/midieventpacket/timestamp).

RtMidi is a portable MIDI I/O candidate, but its API documents immediate output and delta-time input timestamps. It does not by itself implement a transport scheduler. If adopted, preserve a backend-specific scheduling path or own a measured scheduling worker. [RtMidi README](https://github.com/thestk/rtmidi/blob/master/README.md).

Treat MIDI output as three separate features: controller feedback; user-authored note/CC events to other software/hardware; and clock/transport synchronization. MIDI Timing Clock is 24 pulses per quarter note, with separate Start/Continue/Stop messages. Generate it from the project tempo map; explicitly define tempo ramps, seeks, loops, and transport restart behavior. Record/output MIDI intent in the project, but render external synthesizer audio only if it has been captured. [MIDI Association message overview](https://midi.org/about-midi-part-3midi-messages).

S4 HID output can prove LEDs without any MIDI endpoint. Add a virtual MIDI loopback test to prove general MIDI output independently. Do not send hardware MIDI while rendering an offline export faster than real time.

## S4 MK3 qualification POC

The following are proposed acceptance criteria, not measured results.

| Gate | Experiment and pass evidence |
| --- | --- |
| Inventory | Save OS/chip/RAM, firmware, USB descriptors, audio channel/rate capabilities, selected buffer size, USB hub/cable path, app revision, and mapping version. Record firmware before considering any update. |
| Read-only surface | Enumerate and open the exact HID interface. Capture short report traces for play/cue, both channel faders, crossfader, EQ, tempo faders, encoder turns, modifier presses, and non-motor jog motion. Save fixtures with expected normalized values. |
| Output feedback | Drive one known LED, then play/cue LEDs and bounded-rate meters. Verify app-state changes update hardware, including after reconnect. No motor or screen commands in this gate. |
| Audio | Send distinguishable test signals to master and cue; confirm routing by listening/measurement at low monitor gain. Verify rates and buffer sizes supported by this machine/device. |
| Mix capture | Perform a transition between two local tracks with channel faders, EQ, and crossfader. Save/reopen; replay without the controller. Compare recorded parameter/event sequence with the accepted engine sequence, then compare real-time and offline digital renders under the same render configuration. |
| Pickup | Change a value through automation, then move its physical fader from a mismatched position. Confirm there is no abrupt jump before takeover; verify relative encoder behavior separately. |
| Device loss | Unplug/replug control and audio paths during playback and recording. No crash, stuck touch state, unexpected playback start, or destructive overwrite; interrupted capture is clearly marked. Reconnect requires explicit output recovery if the audio device disappeared. |
| Timing/load | Sweep supported 64/128/256-frame buffers at 48 kHz. Log audio deadlines/xruns, control-to-engine delay, render revision, CPU, queue high-water marks, and late-event counts during a 30-minute two-deck run plus sustained controller movement. Success requires no unreported event loss. Overall latency thresholds belong in the engine benchmark plan. |
| MIDI output | Verify note/CC ordering and Start/Clock/Stop through a virtual loopback. Measure clock interval distribution at fixed and changing tempos; do not treat loopback as proof of external-device latency. |

During the first run, keep Traktor and other controller consumers closed. Later test contention explicitly and give a useful device-in-use error. Do not require users to uninstall unrelated software as an application design strategy. Test direct USB first, then the user's normal hub; the exact power/cable path is part of a reproducible performance report.

### Deferred S4 capabilities

Mixxx 2.5 treats motor/haptic support as beta and disables it by default. Its stable manual does not support controller screen rendering. The development 2.7 manual adds screens but says the required bulk/QML facilities are not available out of the box on macOS/Windows. Therefore screens and motors are separate research milestones, not prerequisites for capturing a useful transition. [Stable S4 documentation](https://manual.mixxx.org/2.5/en/hardware/controllers/native_instruments_traktor_kontrol_s4_mk3), [Development S4 documentation](https://manual.mixxx.org/2.7/en/hardware/controllers/native_instruments_traktor_kontrol_s4_mk3).

The web edition must retain the same normalized controller/automation model, but browser hardware APIs and native audio routing require separate capability qualification. A generic Web MIDI implementation cannot receive S4 MK3 HID controls. A desktop bridge or supported WebHID path would be a distinct adapter, not evidence of universal browser support.

## Windows follow-on and current driver evidence

Support both an evaluated vendor-ASIO path and WASAPI exclusive where the chosen audio framework permits. NI's support article reports Windows 11 blocking some S2/S4 MK3 ASIO drivers and recommends WASAPI exclusive. Separately, its current download catalog lists S4 MK3 USB audio driver `5.2.0.87` and ASIO/control panel `5.2.1.94`, dated 2026-09-04. These sources do not establish whether the latest driver resolves every reported block. Recheck and test the exact Windows/driver pair at that milestone; do not disable OS security as a compatibility strategy. [NI Windows issue](https://support.native-instruments.com/support/solutions/articles/69000880969-kontrol-s2-s4-mk3-asio-driver-block-after-windows-11-update), [NI current driver catalog](https://support.native-instruments.com/support/solutions/articles/69000882238-native-instruments-drivers-other-files).

## Remaining questions

- What firmware is installed on the user's S4 MK3, and what descriptors does it expose on the observed Mac?
- Can all initial mix parameters be captured and replayed without relying on an active hardware mixer state?
- Which headphone mix controls are implemented in hardware on this firmware, and what is the correct digital cue-bus configuration?
- What input/output latency and stable buffer size are measured with two decoded tracks and the selected time-stretcher?
- Can our HID backend coexist with installed NI background components without requiring system changes?
- Does the eventual web edition use browser hardware APIs, an optional desktop bridge, or offer preparation/playback without S4 integration initially?

These questions should produce benchmark reports and fixtures as milestone artifacts. They should not be answered by assuming behavior from the Traktor brand name or another generation's mapping.
