# Playback clock and capture failure qualification

Issue: [DJ-008](https://github.com/coder13/Fader/issues/8). Scope: software A08 scheduling and A06 project-owned callback discipline, plus simulated capture/device failure behavior. No physical controller, device output or acoustic measurement was used.

## Configuration and reproduction

Local: Apple Silicon arm64, Darwin 25.6.0/macOS 26.6.2, AppleClang 21.0.0.21000334, CMake 4.4.4, Release, C++20, JUCE 9.0.5 at `d60b7a78030661574a615b586d9325ba7a7fa7dd`. Native and core presets are described in [build instructions](../build-and-ci.md). Source under qualification: `9acd69b45a42b405b2fac01fe7a721318aafd3b2`. [Clean CI run](https://github.com/coder13/Fader/actions/runs/41971504468) passed on the exact commit. Its downloaded build metadata, JUnit XML and CTest logs were checked against that SHA and the JUCE pin; no tests failed or were skipped.

Run `cmake --preset native`, `cmake --build --preset native`, `ctest --preset native --output-junit results.xml`. Core-only Linux uses the `core` presets. All automated fixtures synthesize PCM or a fake JUCE backend; they do not open the machine's audio devices. The separate `--audio-smoke` probe was updated to use the shared callback path but was **not run** for this issue.

UI fixture: generate a new directory with `python3 scripts/make-ui-fixture.py /tmp/fader-clock-ui`; configure with `-DFADER_ENABLE_UI_FIXTURES=ON`; build; launch the existing preview executable with `--incomplete-take-fixture "/tmp/fader-clock-ui/Synthetic timing.faderset"`. Default/CI builds leave the flag off. The fixture uses two clips referring to one 30-second 48 kHz stereo PCM file. Its synthetic signal is an exponentially decaying 110 Hz pulse every half second plus a quiet 440 Hz tone. The script records the generated file's hash; no user audio is distributed.

## Results

| Requirement | Expected and observed locally |
| --- | --- |
| Block boundaries/nonzero origin | Block sizes 1, 64, 128 and 256 produced the same six applied events at project frames 4800, 4927, 4927, 4927, 4928 and 5056; engine elapsed was 512 while project position was 5312. |
| Same-frame and out-of-order input | UI precedes controller at equal mapped frame; each producer retains sequence order. An earlier timestamp enqueued after a future timestamp still executes first. |
| Restart/late input | Old-segment input rejected without failing the new take. Input 123 frames late applied at project frame 12128 and reported that lateness. |
| Separate producer threads | Two real threads submitted 1,000 controls each while processing ran independently of the UI; all 2,000 applied actions survived. |
| Input/scheduler pressure | 4,096-entry input lane retained its valid entries and rejected the next; a full future heap rejected the next 4,096 controls and reported loss. Host timestamp overflow was rejected. |
| Capture pressure | Five batches of 4,096 retained the 16,384-action queue prefix and set capture-incomplete status. |
| Telemetry pressure | 1,100 callbacks with no consumer retained 1,024 metrics and counted 76 metric drops without failing musical capture. |
| Device lifecycle | Fake stop, same-rate restart, 44.1 kHz negotiation and error stopped the segment. Intentional detach stayed healthy; a real fault preceding detach remained visible and preserved captured actions. |
| Allocation and bounded work | Ordinary/aligned C++ allocator interposition covers engine, playback, queue pressure and stopped processing. The [source audit](../playback-clock.md#callback-audit) identifies callback work bounds and framework limitations. |
| Archive integrity | Ordered same-frame actions, original envelopes and source identities preserved; invalid action order could not replace an existing archive. Normal project loader rejected an incomplete archive. |

Native UI inspection used the real application with the opt-in synthetic fixture. The red **INCOMPLETE** mode and status were visible; pressing Record refused to replace the retained take. The Set menu opened **Save incomplete take archive**; saving cleared the incomplete guard and displayed the current no-reopen limitation. Inspection of the saved JSON confirmed `complete: false`, the stop flag, the actual applied action at frame zero, and the unchanged baseline envelope. Screenshots were shared in the development conversation. The owner's open set was saved to a local recovery copy before updating the same preview bundle; it was reopened after the fixture and its saved bytes remained unchanged.

## Decision and limits

The software contract distinguishes engine/project/host clock domains and preserves accepted data on the tested failures. It does not establish physical input latency, callback deadlines under saturation, full-session recording durability, transport/rate action capture, or a finished recovery workflow. Telemetry measures scheduling/mixing before publication, not framework dispatch or audible output. The C++ allocator fixture does not interpose direct C allocation or OS internals. JUCE still locks its callback dispatcher. Full decode and linear-time seek preparation remain unchanged; [#38](https://github.com/coder13/Fader/issues/38) and [#39](https://github.com/coder13/Fader/issues/39) are next.

## Additional concurrency check

A focused driver calling `testPlayback()` passed under Apple ThreadSanitizer with no race report. It compiled `src/Engine.cpp`, `src/Playback.cpp`, `tests/PlaybackTests.cpp` and a driver containing `int main() { testPlayback(); }`, using `clang++ -std=c++20 -g -fsanitize=thread -fno-omit-frame-pointer -I include`. This exercises both real producer threads, scheduler/capture/telemetry pressure and synthetic lifecycle cases; it does not instrument JUCE or the UI.

An initial attempt to run the entire older `CoreTests.cpp` in Debug/TSan hit the macOS stack guard on entry: the fixture keeps many large `Engine` queue objects on its stack. The focused playback fixtures are heap-owned. Whole-suite sanitizer qualification is therefore not claimed; heap-owning/splitting the longstanding fixtures remains a [Windows/Debug build follow-up](https://github.com/coder13/Fader/issues/41). Release CTest and callback allocation checks are separate from this additional targeted race check.

## Verified closure evidence

| Build | Configuration | Result |
| --- | --- | --- |
| Local native | Release; AppleClang 21; native preset; UI fixture flag ON | 3/3 CTest suites passed; native UI/archive check passed separately. |
| Clean Linux CI | ubuntu-24.04 x86_64; GNU 13.3.0; CMake 3.31.6; Release core preset; UI fixtures OFF | 2/2 suites passed; scheduling/concurrency and ordinary/aligned callback allocation evidence present in logs. |
| Clean macOS CI | macos-15 arm64; AppleClang 17.0.0.17000013; CMake 4.4.4; Release native preset; UI fixtures OFF | App/probe/media tools compiled; 3/3 suites passed; application bundle signature verification passed. |

The `linux-core-evidence` and `mac-native-evidence` artifacts from run **41971504468** identify source **9acd69b45a42b405b2fac01fe7a721318aafd3b2** and JUCE **d60b7a78030661574a615b586d9325ba7a7fa7dd**. The subsequent completion commit changes documentation only. The project-end fixture additionally verifies engine frame 256 while the project cursor remains clamped at 100. DJ-008's stated software acceptance is met; the limitations above remain explicit and are not treated as passed hardware or production gates.
