# Short CPU diagnostic with full timeline density

Status: implementation candidate; local build, seven native suites and 19 Python checks pass.
Clean cross-platform CI and the first full-size concurrent diagnostic remain pending.
This supports investigation of the [failed painter gate](2026-11-05-playback-activity-hour.md), not a performance pass.

## Keep project size separate from run time

The ordinary short stress option builds a short project with fewer stored points.
It cannot represent the full hour's arrangement drawing load.
The new CPU-only `--timeline-seconds` option keeps a longer project while limiting the measured host interval.
Both clips retain their full length, source mapping and four stored points per second on each of six curves.
The timed host still runs the same input rates, delayed sources, background cadence, scrolling, alternating clip movement and 50-paint/s work for its requested interval.
Timing-history array lengths follow that interval, so a short run cannot qualify the original hour's memory or sustained timing targets.

An expanded timeline requires `--cpu-diagnostics`, must be at least as long as the timed interval and cannot exceed 3,600 seconds.
Invalid settings fail before media import.
The native parser rejects partial integers and duplicate options.
CPU instrumentation always forces `performanceQualificationEligible` and `hourTargetPassed` false.
The independent reader also rejects a short run that claims an hour pass, a reduced project/point declaration, or an expanded timeline without CPU diagnostics.
Existing default timing and project-size behavior remains unchanged.

For a five-minute diagnostic with the full 60-minute drawing scale:

```sh
python3 scripts/run-software-stress.py \
  --source-root /path/to/clean/candidate \
  --build-source /path/to/build-source \
  --probe /path/to/build-source/build/native/fader_probe_artefacts/Release/fader_probe \
  --build-info /path/to/build-source/build/native/fader-build-info.json \
  --fixtures /tmp/fader-stress-fixtures \
  --output /tmp/fader-full-timeline-cpu-evidence \
  --seconds 300 --timeline-seconds 3600 --block 128 --scheduler realtime --cpu-diagnostics
```

The equivalent probe option is `--timeline-seconds=3600`.
The expected original project has 172,800,000 frames and 86,406 stored points before capture compilation.
The report names the actual project frames, stored points and requested diagnostic timeline.
Keep the measured checkout/build source/binary unchanged until the whole driver and archive finish.

## CPU and scheduling context

Retain the existing per-callback and per-paint CPU readings with their wall brackets, painter context, bounded tails, omissions and maxima.
Add five host policy snapshots: painter before/during the activity, virtual callback thread before/after its loop, and painter after workers join.
These queries run outside the production Engine/Playback/device callback, not on every paint or callback.
Their wall brackets and query cost are recorded.
Owned Mach thread-port references are released after each query.
No query changes a priority or persistent system setting.

On Mac, record `pthread_get_qos_class_np`'s requested class/relative priority and its error status.
The SDK states that QoS overrides are not visible through this interface; the value is not continuous effective priority.
Also record the Mach time-constraint query status, returned default-policy flag, raw period/computation/constraint ticks, timebase and port-release status.
A returned default policy must not be described as a thread's active real-time constraint.
The sampled endpoints cannot prove a policy was held throughout an interval or identify a kernel wait or page fault.
Other platforms report these Mac-specific policy queries as unsupported.
Existing Windows CPU-accounting precision limits still apply.

## Checks completed and remaining

All targets build locally and seven native suites pass in 13.87 seconds.
The new native case times two seconds of a four-second project, checks all 102 original stored points, then reloads the capture and verifies both clip lengths plus every untouched second-clip point beyond the timed interval.
CPU coverage is complete and both qualification flags stay false.
Invalid native settings are checked against a missing fixture folder to prove rejection happens before import.
Direct CLI checks reject missing CPU mode, malformed integers and duplicate timeline options.
The native callback allocation checks remain enabled.

The 19 Python tests include wrapper guards, declared project density/duration, exclusion and signed pre-origin policy brackets with lifecycle ordering.
The independent reader accepts all three local smoke objects, including the expanded-timeline object.
All five Mac snapshots report successful pthread queries and thread-port release; error/default-policy fields remain visible.
These ordinary local smokes use the default thread mode and do not prove the requested real-time host policy.

CI now retains independent checks of `fullTimelineDiagnosticSmoke` on Mac and Windows alongside both existing objects.
Inspect exact metadata, full logs, JUnit and artifacts before treating cross-platform collection as checked.
Then run the full-size short diagnostic to distinguish drawing CPU from elapsed wall pauses and choose an actual repair.
The original full-hour painter failure remains open; any repair still needs the full original uninstrumented hour.
