# Native seek and export observations

Native macOS inspection completes the remaining functional UI checks for [#39](https://github.com/coder13/Fader/issues/39) and [#13](https://github.com/coder13/Fader/issues/13).
Preparing, Cancel, superseding seeks, edit/source replacement, close, export progress and export cancellation work on the declared synthetic fixtures.
These observations supplement the backend qualification reports.
They do not qualify physical audio/MIDI, presentation timing or sustained callback performance.

The [observation aggregate](data/2026-11-05-native-seek-export.json) contains no user music, track lists or recovery-file contents.
Native synthetic screenshots were shared in the development chat.
The aggregate records the observed labels and independent file comparisons, rather than screenshot image files.

## Build and session preservation

The inspected native code is `7e089d03fa3241d151e76bee0db11b90d0ee12a8`.
It uses the local Release AppleClang 21/arm64 build, CMake 4.4.4, macOS 26.6.2 and pinned JUCE `d60b7a7`.
The explicit developer flag `FADER_ENABLE_UI_FIXTURES=ON` supplies the delayed preparation fixture.
All seven local native suites pass, and clean macOS/Windows native 7/7 and Linux core 5/5 CI pass at that exact source.
Downloaded metadata, JUnit and full logs are checked separately in the [candidate report](https://github.com/coder13/Fader/blob/1c07b9c9f7650b0694bf3ef99e63dc1dac0c3b0d/docs/experiments/2026-11-05-playback-activity.md).
The relevant Main, Arrangement, preparation, offline-render and WAV-export sources match main `0d33f6d` byte for byte.

The preview keeps its name, bundle ID and path: `Fader Dark Preview`, `org.fader.poc.dark-preview`, `/private/tmp/Fader Dark Preview.app`.
A fresh user recovery copy is saved and checked before the replacement.
The updated bundle passes signature verification.
After the synthetic checks, the same app reopens the recovery set in editing mode and restores the previous zoom.
User sources remain unchanged, and user file contents and paths stay local.

## Seek, replacement and close

The production loader/writer converts the existing synthetic thirty-second rhythm fixture into the current project format.
`--seek-fixture` skips library scanning and device opening, and deliberately delays preparation for five seconds.
That delay makes the in-flight UI observable.
It is not a measured decoder seek time or a replacement for the declared warm/cold measurements.

The following native interactions pass:

- Start preparation and observe `Preparing` plus the `Cancel` transport action.
- Activate Cancel while that state is still verified in flight and observe `Playback preparation cancelled.` with `EDIT` and `Play`.
- Leave it cancelled beyond the former completion interval and observe no late restart.
- Request positions zero, ten and twenty-five seconds while preparing, then observe readiness at twenty-five seconds.
- Invoke Open from preparation, load a different synthetic source set, and observe the replacement stopped in `EDIT` at zero.
- Move a clip during verified preparation and observe immediate cancellation, the changed clip position and editing mode.
- Save the edited fixture, start another verified preparation and quit. The original process exits normally without forced termination.

The Open dialog can remain present long enough for the former request to finish before a replacement file is selected.
This observation establishes the replacement's final state, not the exact time of invalidation within the native picker.
The separate preparation tests cover ignored cancellation and late completion during source replacement.

The drag moves clip A from start zero/row zero to frame 240,000/row two.
Its length stays 576,000 frames, or twelve seconds.
An independent comparison of the saved fixture matches clip/source IDs and every other field after normalizing only that start, row and asset paths.
The clip moves without resizing or changing its automation.
It remains stopped after the former preparation interval.

The existing [seek report](2026-11-04-cancellable-seeking.md) supplies the memory/work bounds, long WAV/MP3 timings, PCM/control parity and race/lifetime tests.
The uncached slow path remains an explicitly reported fallback, not a passing fast-seek result.
General import/library cancellation and future plugin/stretch state remain separate work.

## Export progress, cancellation and project preservation

The native test loads a full-hour, two-source synthetic arrangement with 150,032 compiled points.
Its compact manifest is 7,438,509 bytes, within the existing 8 MiB limit.
An initially pretty-printed hand-written copy exceeds that limit and is correctly rejected without replacing the prior project.
The fixture is corrected by compact serialization, without increasing the limit.

An existing-target overwrite sheet does not respond to the computer-control tool's input.
The owner manually cancels that sheet before rendering starts.
The actual running cancellation test therefore uses a new destination, with no overwrite sheet.
This test does not claim native cancellation against an existing destination.
Existing-destination retention remains covered by the backend failure and transaction tests.

The native mode reports `Export 1%`, then `Export 28%`, with a visible `Cancel export` action.
Activating Cancel returns the app to `EDIT`, restores its controls and displays `Export cancelled`.
The new final destination is absent, and no export temporary file remains in the test directory.
The separate prior valid WAV remains unchanged, but it was not the destination of the running export.

A native Save action writes a new comparison set after cancellation.
An independent recursive comparison matches all non-path fields, JSON types and all 150,040 parsed floating-point values bit for bit.
All 150,032 automation points and source identities remain intact.
Both copies resolve the same synthetic sources.
The production-saved copy also matches the earlier production archive's SHA-256 exactly.

The existing [offline report](2026-11-05-offline-render.md) supplies the completed hour export, full independent PCM comparison, bounded memory and injected failure/retention checks.
This native test supplies the remaining visible progress/click behavior, rather than repeating that completed render measurement.

## Limits and completion scope

These are functional observations on one development Mac, with deliberate synthetic fixtures and no device opening.
They are not response-time measurements, a native GUI memory-budget result, or owner approval of the product layout.
A short diagnostic process sample shows event-loop waiting and painting during earlier input-routing trouble, rather than a sampled lock/join hang.
It cannot establish responsiveness at every instant.
The same sample reports physical footprint `50.2M` and peak `166.5M` in its own units.
Those figures are a different metric and surface size from the virtual stress process and do not qualify its RSS budget.

[#40](https://github.com/coder13/Fader/issues/40) remains open for its failed sustained timing targets and candidate qualification.
Physical-device gates and the later owner workflow/accessibility/release tasks remain separate.
The native functional gaps named by #39 and #13 are now complete within these stated limits.
