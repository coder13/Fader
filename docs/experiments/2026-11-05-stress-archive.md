# Automatic preservation of stress evidence

Status: implemented and checked on macOS/Windows native CI and Linux core/helper CI.
This supports [#40](https://github.com/coder13/Fader/issues/40) evidence retention.
It does not change production C++, the workload, a timing target or the running activity experiment.

## Problem and behavior

The old driver hashes the executable but leaves its only copy in the build folder.
It rebases the captured project through the production writer, without retaining the original bytes.
Another preflight overwrites the fixture-side fault files and a later capture replaces the original set.
Earlier completed runs have been preserved manually; the new driver automates that work.

Before preflight, copy the probe into the new evidence folder and verify its SHA-256 against the recorded input hash.
Execute this retained copy for preflight, timed playback and production rebasing.
Preserve its executable mode and retain the `.exe` suffix on Windows.
An existing evidence file is never replaced.
A short or changed copy fails before it can be recorded as verified.

After a successful preflight, preserve all six known fault files before timed playback starts.
After a successful capture save, preserve the exact original set bytes before production rebasing.
This is the unrebased compiled project, not the original-action journal required by #12.
Saving these bytes does not preserve every original control action or prove PCM parity.
A failed save must not cause a stale file from a previous run to be archived as the new take.
A reported successful save with a missing file is an explicit evidence failure.
These copies and checks run outside the measured probe interval.

The original relative asset paths retain their original directory context in separate local JSON files.
Those context files and diagnostic error logs can contain absolute paths and must stay local or be redacted before publication.
Public aggregate metadata contains retained filenames, sizes and hashes instead of those directory paths.
The synthetic sources are not copied by this helper.
Copy completion and hashes do not promise recovery after power loss.

## Independent archive check

Read the preserved original bytes and the production-rebased archive as JSON.
Only asset paths may differ, and both references must name the same existing physical file on this host.
Use the filesystem's [file-identity check](https://docs.python.org/3/library/pathlib.html#pathlib.Path.samefile), rather than [path-text equality](https://docs.python.org/3/library/pathlib.html#general-properties) that can fold case on Windows.
Windows also supports [case sensitivity per directory](https://learn.microsoft.com/en-us/windows/wsl/case-sensitivity).
Hard-link aliases are accepted; a distinct file with equal bytes is still a different destination for this rebase check.
Missing source files prevent verification; this helper is not the media-independent recovery reader.
All other fields, array order, JSON types and parsed float64 bits must match, including signed zero.
Reject duplicate keys, non-finite numbers, malformed JSON and missing asset paths.
Do not ignore another field just because it is also named `path`.

This checks metadata preservation after rebasing.
It does not validate a project schema, decode media, prove source contents, compare PCM or qualify timing.
An archive check cannot turn a failed timed result into a pass.
An evidence failure returns failure even when the timed probe also reports a missed performance target.
Final aggregate and standalone metadata share the same finished timestamp.

## Checks completed and remaining

Eight new filesystem/corruption tests and the eight existing observation-reader tests cover the candidate.
All 16 pass locally in 0.178 seconds after the file-identity change.
They cover changed source files, exact retained bytes, wrong hashes, short copies, existing destinations, missing fault files, stale captures and missing successful saves.
They also cover Unicode paths, hard-link aliases, distinct files with equal bytes, missing sources, changed IDs/lengths, JSON type changes, signed zero, non-asset path fields and malformed/non-finite metadata.
No native host, build or sanitizer runs for these Python helper tests.

The new comparator also reads the completed CPU hour's preserved original and production archive.
It matches two asset destinations and all 150,040 parsed floating-point values, with every non-path type and field unchanged.
The original SHA-256 is `ada2d15a2e785c60da03eb246244b8e35b26338fd70983641611852ccdade447`.
The rebased archive SHA-256 is `6057bee3f66b57b13a2a56304f966d727c168c3c47a23d001a51f8d6a147c535`.
This small read-only check is recorded as operator activity during the separate timed run.

## Clean native driver checks completed

[CI 42074528793](https://github.com/coder13/Fader/actions/runs/42074528793) passes at exact source `bb994cc90fcde2ea988ecd58711ad49261f0691a`.
Downloaded metadata, JUnit and full logs match that source, the pinned JUCE revision, Release builds and UI fixtures OFF.
There are no failures, errors or skipped suites.
The [checked aggregate](data/2026-11-05-stress-archive-ci.json) records configurations and remaining limits.

| Platform | Existing CTest suites | Python helper tests | Retained-probe driver |
| --- | --- | --- | --- |
| macOS arm64, AppleClang 17 | 7 pass | 16 pass, 0.173 seconds | Faults, two-second capture, production rebase and observation check pass |
| Windows x64, MSVC 19.51 | 7 pass | 16 pass, 0.463 seconds | Faults, two-second capture, production rebase and observation check pass |
| Linux x86_64, GCC 13 | 5 core suites pass | 16 pass, 0.211 seconds | No native driver run |

The native driver uses four-second synthetic sources and the normal-priority two-second workload.
Each runner retains all 247 planned controls, six original fault files, its executable, original compiled capture and production archive.
Downloaded executable and original-file hashes match the recorded values.
Standalone metadata and aggregate metadata match, including the finished timestamp.
Independent comparisons match all non-path JSON fields/types and 235 parsed float64 values on Mac, 286 on Windows.
Each runner's archive is compared with its own original project; this is not cross-host equality of jitter-dependent captures.
Both existing native smoke objects and the new driver object pass fresh observation checks that match their archived reader outputs.

File identity is checked by the native runner and recorded in its results.
The downloaded artifact omits the original source files, so that remote physical identity is not independently reestablished locally.
The downloaded original/archive fields, types, floating-point bits and retained-file hashes are independently checked.
Windows also passes the real hard-link, distinct-equal-byte-file and missing-source regression.

The driver records 506 virtual misses and 60 late painter starts on Mac, and 12 misses with no late painter starts on Windows.
Both explicitly report `hourTargetPassed: false`.
Existing suite smoke timing and coarse Windows CPU-accounting limits remain in the aggregate.
These ordinary-priority shared runners qualify evidence retention, not sustained timing or physical audio/MIDI.
The separate original-workload activity hour remains pending and unchanged.
