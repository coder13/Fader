# Playback clock and capture failure qualification

Issue: [DJ-008](https://github.com/coder13/Fader/issues/8). Scope: software A08 scheduling and A06 project-owned callback discipline, plus simulated capture/device failure behavior. No physical controller, device output or acoustic measurement was used.

## Configuration and reproduction

Local: Apple Silicon arm64, Darwin 25.6.0/macOS 26.6.2, AppleClang 21.0.0.21000334, CMake 4.4.4, Release, C++20, JUCE 9.0.5 at `d60b7a78030661574a615b586d9325ba7a7fa7dd`. Native and core presets are described in [build instructions](../build-and-ci.md). The source commit and clean CI results will be appended after verification; local results alone are not the closure evidence.

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
