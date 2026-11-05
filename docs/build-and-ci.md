# Reproducible builds and CI

Tracking: [DJ-006](https://github.com/coder13/Fader/issues/6). The core can be built and tested without JUCE, a GUI session, an audio device or any media library. The native target builds the application, diagnostic probe and synthetic media/editor/device-setup tests. Hardware qualification is separate.

## Required tools and commands

Use a C++20 compiler, CMake 3.22 or newer, Git for native dependency fetches, and the build tool selected by CMake. On macOS, install Xcode Command Line Tools (`xcode-select --install`); the default generator uses Make. A full Xcode installation is not required for the local command-line build. CI uses the `ubuntu-24.04` and `macos-15` runner images and records the compiler actually selected on each run.

From a clean checkout:

```sh
cmake --preset core
cmake --build --preset core
ctest --preset core

cmake --preset native
cmake --build --preset native
ctest --preset native
```

The native configure fetches JUCE **9.0.5**, commit `d60b7a78030661574a615b586d9325ba7a7fa7dd`. There is no dependency on a personal checkout path. Offline developers can pass `-DFADER_JUCE_PATH=/path/to/JUCE`; the checkout must be at the pinned commit. Treat local dependency modifications as a separate experiment. The core preset never fetches JUCE. Use `-DBUILD_TESTING=OFF` for a build without test targets. Put personal overrides in ignored `CMakeUserPresets.json`.

On macOS the app is `build/native/Fader_artefacts/Release/Fader.app`. To sign locally and launch:

```sh
codesign --force --deep --sign - build/native/Fader_artefacts/Release/Fader.app
open build/native/Fader_artefacts/Release/Fader.app
```

This is an ad-hoc development signature, not a signed/notarized product distribution. Windows compilation and actual device/driver behavior have their own issues (#41 and #27).

## Evidence retained per run

`.github/workflows/build.yml` runs on code/configuration pushes and pull requests; pushes changing only `docs/` or root Markdown files are skipped. It checks out the commit into a fresh runner, configures with the presets, builds and executes CTest. Actions are pinned by commit. Jobs have bounded timeouts and read-only repository permissions. No user music, controller, audio output, secrets or test-tone playback is required by the tests.

Each build produces `fader-build-info.json` with source commit (or source-archive marker), project version, compiler/version, CMake/generator, OS/architecture, build type and the JUCE pin. It intentionally omits personal filesystem paths. CI retains that metadata, JUnit results and CTest logs for 14 days; the run identifies the runner image and exact build commit. Runner images and installed compilers receive updates, so this is repeatable source/dependency/configuration evidence, not a byte-identical binary guarantee.

The core tests exercise capture/edit/replay, project invariants, filter state and block-size independence. The realtime fixture records process-call durations into preallocated storage and reports sorted timing/allocation results after processing. Its synthetic timing is not a device deadline guarantee. Native media tests exercise save/reopen/export, source identity, Traktor fixtures, actual editor event handlers and a simulated JUCE device backend. Audio-setup cases include no output, driver refusal, 128→256 fallback, wrong negotiated rate, mono-only output and preservation of a chosen buffer. They use the same setup function as playback/Preferences/probe.

The optional `fader_probe --audio-smoke` explicitly opens an output. It stores callback timing in a fixed array, removes the callback, then sorts and writes metrics on the calling thread. It reports actual negotiated buffer/rate and driver xrun availability; it does not establish sustained reliability or acoustic latency. Hardware work is excluded from the current issue sequence.

References: [CMake preset schema supported by the minimum version](https://cmake.org/cmake/help/v3.22/manual/cmake-presets.7.html), [GitHub runner labels and image limitations](https://docs.github.com/en/actions/reference/runners/github-hosted-runners).
