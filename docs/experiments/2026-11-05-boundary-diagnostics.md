# Boundary and CPU diagnostic candidate

Status: implementation candidate `806f4252b7c442e077a0ea9463b360b3814d97ad`; macOS/Windows native and Linux core CI pass.
This is instrumentation, not a repair or a successful hour qualification.
The [completed 128-frame run](2026-11-05-software-stress-followup.md) remains failed.
The separate 256-frame run has finished with its original checkout and binary; its failed targets remain separate.
No local build or additional host ran alongside it.

## Why these observations

All three missed blocks in the repeated hour begin at `4096*n - 128`.
Thirty-four of its thirty-nine tail blocks share that remainder.
Their almost punctual wakes make a sleep-backend change a poor first response.
The existing internal timer measures wall time, which cannot distinguish computation from descheduling.
Painter wall durations also lack enough viewport context to reproduce their largest tails.

The candidate adds callback populations for blocks that end at a 4,096-frame project boundary and for other blocks.
It reports counts and maximum internal wall duration for both populations before sorting histories.
This names a project-frame relationship, not a claim that it is the decoder's logical page size or the native memory page size.
No stream page size, DSP algorithm, source density or timing target changes.

## Optional thread CPU clock

`--cpu-diagnostics` reads the calling thread's CPU clock before and after each process call and painter update.
macOS uses `clock_gettime(CLOCK_THREAD_CPUTIME_ID)` behind a macOS 10.12 availability guard.
The same POSIX branch is provided for Linux; the native CI scope here is macOS and Windows.
Windows uses `GetThreadTimes` on the current-thread pseudo-handle and sums user and kernel time.
Microsoft specifies 100 ns units, which do not establish the accounting precision of a short observation. [Primary API documentation](https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-getthreadtimes)
The local Apple SDK declarations and availability were inspected in `_time.h`.

Each CPU reading stores steady-clock timestamps immediately before and after the native query.
The report preserves both query brackets and the enclosing CPU delta.
It also gives the minimum and maximum wall windows that can contain the two CPU samples.
The native query itself takes time; the two clocks are not sampled simultaneously.
These are enclosing thread observations, not exact DSP CPU measurements.
Windows accounting can be too coarse to interpret a short call, even when reads succeed.

The report exposes the API name, units, reported resolution when available, resolution error, and failed sample counts.
Failed or backwards readings remain missing rather than becoming zero CPU work.
CPU completeness is separate from musical and wall-timing integrity.
The CLI returns code 4 for an incomplete requested CPU observation after writing its report.

Instrumented runs always set `performanceQualificationEligible` to false and cannot set `hourTargetPassed`.
The clock reads and additional history add observer cost.
Timing, save and integrity results remain visible, but a diagnostic observation cannot be presented as the original performance pass.
The existing real-time policy and steady-clock sleep/spin mechanism remain unchanged.

## Painter context

The first 256 slow or late painter iterations retain their index, scheduled/actual start and finish,
playhead, viewport position, selected parameter and editor-copy clip displacement.
The slow threshold is a 20 ms update/paint duration or a start more than 20 ms late.
Omitted rows are counted, while maximum execution and maximum start lateness retain their context after the bound.
CPU mode also retains the maximum enclosing CPU observation.
Invalid or duplicate painter metadata is rejected and counted.

This can distinguish expensive positions or drawing parameters from a long wall interval with little observed CPU use.
That distinction remains subject to the clock's precision and query brackets.
It cannot identify a kernel or memory fault without further evidence.
The rendering stays offscreen; native presentation is still unmeasured.

## Bounds and checks

The fixed host queue grows by its fixed CPU-observation fields; it remains bounded at 4,096 entries.
CPU histories are allocated and touched before timing only when the diagnostic mode is selected.
At an hour/128 frames, the callback CPU history adds 10,800,000 bytes and painter CPU history adds 1,440,000 bytes.
Painter and callback tail lists are each capped at 256 records.
All histories are released before capture commit/save, and the process peak still includes their earlier lifetime.
The declared memory budget is not raised.

Model checks cover failed CPU reads, backwards CPU time, overlapping query brackets, boundary-population indexing,
painter context pairing, duplicate painter records and maxima after tail truncation.
The native stress smoke adds a separate two-second CPU-mode run and checks complete native clock coverage,
musical/wall integrity, painter context and the explicit performance exclusion.
Default and instrumented results are kept as separate objects.
Existing callback allocation tests remain enabled on the unchanged production core.
The checked native results below supply functional collection evidence with explicit platform limits.

## macOS artifact check

[CI 42036581905](https://github.com/coder13/Fader/actions/runs/42036581905) passes all seven macOS suites and five Linux core suites.
Downloaded macOS metadata and JUnit match exact source `806f4252b7c442e077a0ea9463b360b3814d97ad`, pinned JUCE and UI fixtures OFF.
There are no failed, erroneous or skipped native suites.
The [verified macOS aggregate](data/2026-11-05-boundary-mac-ci.json) records the build and diagnostic results.

The native mode retains all 750 callback CPU observations and all 98 actual painter CPU observations, with zero CPU read errors.
All 247 controls survive, and the musical/wall-timing integrity check passes.
The API reports a 42 ns nominal resolution on that runner; the precision caveat still applies.
Independent checks validate CPU deltas, query brackets, frame-boundary populations and painter context on 260 serialized host rows and 56 painter rows.
Maximum rows may repeat frames already present in the bounded tail lists.

The ordinary-priority shared-runner diagnostic has 477 virtual misses, 53 late paint starts and 364 omitted host tail rows.
It explicitly marks performance qualification ineligible and the hour target false.
This proves functional collection and visible limits, not performance, an owner-Mac cause, or a timing repair.
The separate Windows artifact check below completes the native functional check.

## Windows and Linux artifact checks

The same clean CI passes all seven Windows suites and five Linux core suites.
Downloaded [Windows/Linux metadata and JUnit](data/2026-11-05-boundary-windows-linux-ci.json) match exact source `806f4252b7c442e077a0ea9463b360b3814d97ad`, pinned JUCE and UI fixtures OFF, with no failures, errors or skips.
Linux covers the unchanged core; it does not compile this native diagnostic branch in that headless job.

Windows retains all 750 callback and 100 painter CPU readings with zero read errors, all 247 controls, and correct serialized query brackets/context.
All 260 host and three painter maximum rows pass independent arithmetic checks.
The ordinary-priority shared runner reports three virtual misses and 74 omitted host tail rows; performance eligibility remains false.

However, all 750 callback CPU deltas are zero, while painter deltas jump in 15.625 ms amounts.
The `GetThreadTimes` accounting is too coarse here to diagnose the cost of one short callback.
Zero does not mean the callback did no CPU work.
`complete` means every requested native query returned an ordered reading, not that the accounting precision resolves the operation.
Reported resolution is unknown on Windows, as recorded by its explicit unsupported-resolution status.
The current detailed CPU investigation therefore uses the macOS clock; Windows short-call CPU attribution remains unqualified.
Do not derive a blocked-time estimate from those zero Windows deltas.

## Independent observation reader

The [Python reader](../../scripts/verify-stress-observations.py) checks serialized timing arithmetic without starting a playback host.
It accepts a probe report, an aggregate object selected with `--key timed`, or the full native CTest log.
Use the full log because successful-test output in JUnit can be truncated.

The reader checks absolute frame targets, marker order, internal/call durations, matched metrics, tail selection and omitted-row accounting.
It checks CPU deltas and query brackets, painter context, boundary populations and explicit exclusion from performance qualification.
Global maxima can repeat tail rows and remain visible in the output.
CPU deltas above an enclosing wall interval are counted, because coarse accounting cannot establish exact short-call CPU use.

The reader checks retained rows and reported counts only.
It cannot prove unseen rows, global-maximum selection, source/binary identity, clock precision or performance.
A consistent incomplete trace passes arithmetic checks while its omitted count remains visible.
Use the exact-source driver and original performance gates separately.

```sh
python3 scripts/verify-stress-observations.py /path/to/evidence/timed.json \
  --output /path/to/evidence/trace-verification.json
python3 scripts/verify-stress-observations.py /path/to/LastTest.log \
  --ctest-log --key cpuDiagnosticSmoke
python3 -m unittest discover -s scripts/tests -v
```

Eight reader tests cover corrupted frames, reordered brackets, altered CPU deltas, missing coverage, duplicate rows and false eligibility.
They also cover coarse/zero CPU accounting, visible truncation, non-finite JSON and rejection under optimized Python.
Local checks pass against the completed 128/256 reports and exact `806f425` native logs.
CI now runs the reader tests and checks both native smoke objects after CTest.
This tooling does not change the production engine or the active diagnostic build.

## Commands after the current fallback finishes

Build and test the candidate in an isolated build after the current measurement ends.
Do not overwrite its running probe or source tree.
Use a clean committed checkout and the existing driver for source and binary identity:

```sh
python3 scripts/run-software-stress.py \
  --source-root /path/to/clean/candidate \
  --build-source /path/to/candidate-build-source \
  --probe /path/to/candidate-build-source/build/native/fader_probe_artefacts/Release/fader_probe \
  --build-info /path/to/candidate-build-source/build/native/fader-build-info.json \
  --fixtures /path/to/unchanged/synthetic-fixtures \
  --output /path/to/new/diagnostic-evidence \
  --seconds 3600 --block 128 --scheduler realtime --cpu-diagnostics
```

Use the full hour so the stored curves, timeline and baseline timing histories retain the original scale.
The extra CPU observations still add memory and query cost, so this instrumented hour cannot satisfy the original performance gate.
Shorter diagnostics change the timeline and history size and must be labeled accordingly.
Preserve all counts and uncertainties even if no deadline is missed.
Select an actual repair from the resulting evidence, then repeat qualification without the CPU instrumentation.
