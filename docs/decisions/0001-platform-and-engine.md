# ADR 0001 Platform and engine direction

Status: **Proposed**. Date: 2026-11-04. Decision owner: project maintainer. Acceptance requires the M0 findings and an explicit recorded decision.

## Context

The application is open source, targets macOS and Windows, and starts with a Traktor Kontrol S4 MK3. Its central feature is editable capture of a performance and reliable re-rendering. Web delivery is an explicit research requirement. The codebase is starting empty.

Low-latency audio and HID access must coexist with a rich timeline editor. Source media, parameter automation, and tempo mappings must survive changes in UI, device, or platform. The first experiment must establish hardware access and replay correctness before a broad feature build.

## Proposed decision

Use **C++20 for a small headless engine and JUCE for the initial native application harness**. Use Core Audio on macOS and a dedicated HID adapter for the S4. Keep the render core and project model independent of JUCE GUI classes. Use CMake with pinned dependencies if the spike confirms this direction.

JUCE is recommended for integration efficiency, not because C++ intrinsically produces lower latency than Rust. Both can support real-time processing; architecture, bounded work, device behavior, and DSP determine the result. JUCE does not implement the flagship arrangement/automation model for us.

Select an exact open-source license before substantial dependency/code reuse. **AGPL-3.0-or-later is a candidate for a JUCE-based application**, subject to the dependency inventory; it is not chosen by this ADR. JUCE 9.0.5's pinned license offers AGPLv3 and commercial routes. A preference for another licensing policy may favor Qt/PortAudio or Rust components. [JUCE license](https://raw.githubusercontent.com/juce-framework/JUCE/9.0.5/LICENSE.md).

Keep the product frontend undecided until a narrow native/web editor prototype answers the interaction and maintenance question. The first thin JUCE harness is disposable. Browser-only support is a separate capability-qualified host, rather than a reason to make the native engine depend on browser APIs.

## Alternatives and conditions for choosing them

| Alternative | Choose it if | Why not the current default |
| --- | --- | --- |
| Rust + CPAL + MIDI/HID libraries | Team preference/experience and measured integration work favor Rust; required DSP works cleanly | More audio/UI/DSP packages to assemble for this specific first proof |
| C++ + Qt + PortAudio | Qt materially improves the editor, or project licensing makes this route preferable | More plumbing than the small JUCE harness |
| Mixxx fork | Timeline capture and headless render can be added at lower maintenance cost than a new core | Coupling and architectural fit are unmeasured; existing application is not an assumed engine SDK |
| Native engine + Electron/Tauri UI | The timeline prototype improves iteration enough to justify the bridge | Adds host/IPC/build ownership before the core risks are retired |
| Browser AudioWorklet/Wasm | Exact S4 HID, master/cue routing, reliability, and latency pass the browser gates, or browser scope is arrangement-only | Safari hardware API gaps and device-specific output access block a universal hardware promise |
| Pure C application | A concrete embedded/minimal-runtime requirement appears | No corresponding product advantage identified |

Sources and fuller tradeoffs: [audio/stack research](../research/audio-engine-and-stack.md), [web research](../research/web-platform.md), [S4 research](../research/traktor-and-controller-integration.md).

## Decision experiments

1. **Hardware:** prove native S4 essential HID input/LED feedback and independent audio outputs on the actual Mac.
2. **Mixxx:** identify a supported stable build, trace capture insertion, attempt a scripted/headless render seam, and document the architectural delta. Stop after a bounded experiment with either a concrete reuse path or reasons against it.
3. **Web:** test Chromium S4 HID and four-channel output, and compare a tiny timeline interaction prototype. Safari should display a capability explanation, not attempt unavailable hardware APIs.
4. **Engine:** process two sources with timestamped gain/crossfader events and render the same scripted sequence offline. Measure deadlines before adding stretch.
5. **Licensing:** pin intended versions and review their license files, including copied mapping code, optional codecs, and future Windows ASIO packaging.

If native HID or multichannel audio fails, investigate interface access and firmware before replacing the controller requirement with generic MIDI. If web hardware gates fail, preserve web arrangement/preview as a viable scope. If a prototype is faster to build in Rust because of team expertise, that is legitimate decision evidence.

## Consequences

We accept C++ lifetime/concurrency discipline and a custom timeline/automation implementation. We gain a direct native device path and ready access to DSP libraries. A headless boundary enables offline export and potential Wasm reuse, but neither is automatic. We avoid maintaining two production engines until a second host is justified.

Revisit this decision after M0, after the complete M2 flagship POC, or if the chosen dependency/license combination cannot support the intended distribution.
