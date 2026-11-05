# Automatic preservation of stress evidence

Status: implementation candidate; local helper checks pass, native driver CI remains pending.
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

Clean CI must execute the retained native probe on macOS and Windows, using four-second synthetic sources and a two-second normal-priority workload.
It must preserve the six fault files and original capture, pass production rebasing and the independent observation reader, and retain the full evidence directory.
All three CI platforms run the helper tests; Linux also runs the existing core suites.
No short CI observation qualifies the hour or physical audio/MIDI gates.
Inspect actual metadata, logs and retained files before calling this candidate complete.
