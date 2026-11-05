# Mixer qualification (DJ-007)

Work dated 2026-11-04 Pacific / 2026-11-05 UTC. Tracking: [issue #7](https://github.com/coder13/Fader/issues/7).

Tested implementation: `b18e22aa807ae5642f1920dffd20da1fb4be002d`. [CI run 41967530872](https://github.com/coder13/Fader/actions/runs/41967530872) passed Linux core **2/2** and Mac native **3/3** suites, including the application/probe build and ad-hoc bundle-signature verification. Downloaded both evidence artifacts and checked the matching source/JUCE revisions, zero-failure/zero-skip JUnit reports and CTest logs. Later changes merge documentation only; render code and test targets remain identical to the tested revision.

| Configuration | Result |
| --- | --- |
| Local arm64, Darwin 25.6.0, AppleClang 21.0.0.21000334, CMake 4.4.4 | Native 3/3 suites passed; added PCM-overload fixture also passed in the media suite |
| Linux CI x86_64, GCC 13.3.0, CMake 3.31.6, Release C++20 | Core and realtime suites passed |
| Mac CI arm64, Darwin 24.6.0, AppleClang 17.0.0.17000013, CMake 4.4.4, Release C++20 | Core, realtime and media suites passed; app/probe built; bundle signature verified |

Native dependency: JUCE 9.0.5 at `d60b7a78030661574a615b586d9325ba7a7fa7dd`. Commands are the shared `core` / `native` CMake and CTest presets in [build documentation](../build-and-ci.md).

## Acceptance evidence

1. **Source rate, gain and duration:** generated mono/stereo float WAVs at 44.1/48/96 kHz retain original rate/count, correct mono duplication/stereo separation, floor-rounded 48 kHz project duration and per-channel gain through actual export/decode. Affine-source fixtures verify the existing interpolator at 8/44.1/48/96 kHz, rates 0.5/1/2, nonzero source offset/start and the end boundary. No user media was read or copied.
2. **Independent master/cue:** separate four-output assertions prove dry pre-volume/pre-filter cue, cue summation, master-only metering and unused-output clearing. Export has two channels at 48 kHz; toggling cue produces byte-identical files. PCM overload fails without replacing its destination; float WAV retains ±1.5. Legacy version 1 equal-power crossfader behavior remains covered.
3. **Bounded mixer callback:** source audit found no allocation/free, blocking locks, file/device/UI access or retry-until-success loops in the project-owned render path. Work is bounded by the host block size, 64-clip/curve-point admission limits and the 4,096-command budget. Existing realtime tests passed 20,000 calls without ordinary `new`/`delete` during processing. The [mixer contract](../mixer-contract.md) records the paths and limits rather than treating this fixture as a comprehensive scheduler/allocation proof.

## Result and limits

The current POC mixer behavior is qualified for these software fixtures; no render algorithm changed. Linear interpolation has no band-limited anti-alias stage, rate remains pitch-coupled, and there is no limiter/normalizer. This does not establish final resampling sound quality or subjective listening approval. Full source decoding remains in place; [#38](https://github.com/coder13/Fader/issues/38) owns bounded streaming and [#40](https://github.com/coder13/Fader/issues/40) owns sustained-load qualification. Physical master/cue routing and device latency remain untested. Product metering/cue controls have their own follow-up [#52](https://github.com/coder13/Fader/issues/52).
