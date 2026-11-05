# Reproducible build qualification (DJ-006)

Work dated 2026-11-04 Pacific / 2026-11-05 UTC. Tracking: [issue #6](https://github.com/coder13/Fader/issues/6).

Implementation under test: `9ef17963a512b1bd1e24a5aa26b9718ef4db55d1`. Later documentation-only integration preserves the newer Audacity design research from main. [Build commands and limits](../build-and-ci.md) describe the repeatable setup.

## Scope and changes

The `core` preset builds the JUCE-independent engine and its two test targets. The `native` preset builds the Mac application, probe and all three test targets. `BUILD_TESTING=OFF` omits the test targets. Native builds fetch JUCE 9.0.5 at `d60b7a78030661574a615b586d9325ba7a7fa7dd`; the optional existing-checkout override rejects a different revision. No personal dependency path is committed.

CI runs fresh Linux core and Mac native checkouts, with pinned checkout/artifact actions, read-only repository permissions, bounded job/test timeouts, compiler/architecture/dependency metadata and JUnit/CTest evidence. Native CI also signs the development app ad hoc and verifies its bundle. This qualifies build reproducibility at the source/configuration level, not identical binary output across changing runner images.

Playback, Preferences and the optional device probe now share output setup and error reporting. Preferences previously discarded initialization errors. Tests inject a JUCE device backend and check missing output, driver refusal, 128-to-256-frame fallback, an unexpected sample rate, mono output and preservation of a selected buffer. Only the fake backend is registered for these checks.

## Local evidence

- AppleClang `21.0.0.21000334`, CMake `4.4.4`, Unix Makefiles, Darwin `25.6.0`, arm64, C++20 Release.
- `cmake --preset core`, build and CTest: **2/2 passed**.
- Native preset with the pinned local JUCE checkout: app/probe/tests built; CTest **3/3 passed**.
- The simulated device tests run in the media suite alongside existing media, project, Traktor and editor fixtures. A CoreMIDI service diagnostic was printed in the sandbox; it did not cause a test failure and is not a MIDI qualification result.
- The realtime fixture passed 20,000 process calls without ordinary `new`/`delete` during processing. Local timings: p50 7.584 µs, p95 8.917 µs, p99.9 31.666 µs, maximum 56.875 µs. These are synthetic offline timings, not physical output latency or a sustained deadline guarantee.
- The test-disabled configuration omitted test targets; an intentionally wrong dependency checkout was rejected with the required JUCE revision. New C++ files passed the repository formatter; changed local documentation links and `git diff --check` passed.

## Clean-checkout CI evidence

[Run 41965474848](https://github.com/coder13/Fader/actions/runs/41965474848) checks the exact implementation revision above.

The first run passed: Linux **2/2 tests**, Mac **3/3 tests**, app/probe build and ad-hoc bundle-signature verification. Linux used GCC 13.3.0, CMake 3.31.6 and x86_64; Mac used AppleClang 17.0.0.17000013, CMake 4.4.4, Darwin 24.6.0 and arm64. Both retained source/compiler/dependency metadata and CTest logs.

Artifact inspection found that the first workflow supplied a JUnit path relative to the source directory, but CTest resolved it within the preset's build directory. The test log and exit code were valid; the promised JUnit file was missing. The follow-up workflow uses `results.xml` and explicitly checks that the report exists before upload. The corrected [main-branch run 41966443612](https://github.com/coder13/Fader/actions/runs/41966443612), at `fb2428fc2fad53a1faa614d958c3ac998744691c`, passed both jobs. Both downloaded artifacts contain matching source/JUCE revisions, zero-failure/zero-skip JUnit reports (Linux 2 tests, Mac 3 tests), and CTest logs. Native CI again built the app and probe and passed the bundle-signature check. The source, test and CMake target files are identical to the first tested implementation; intervening changes integrate documentation and correct CI reporting.

## Callback and hardware boundaries

The engine callback uses its preallocated state and bounded queues. The optional audio probe stores callback durations into a fixed array, detaches the callback, then sorts and formats the report on the control thread. The realtime fixture similarly reports after processing. Device opening and error-string construction run on the control thread.

No physical audio test, S4 interaction, acoustic latency measurement or user-library scan was performed for this issue. The running editor was left untouched. This evidence does not qualify controller support, sustained playback, streaming, seeking, Windows native operation or product readiness. Those remain separate roadmap gates.
