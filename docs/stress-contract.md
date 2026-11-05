# Software stress contract

This contract defines the workload for [#40](https://github.com/coder13/Fader/issues/40) before measurement.
A virtual host calls the production playback core at real clock intervals.
It sends PCM to a checksum sink and opens no audio or MIDI device.
The native device and acoustic gates remain open.

## Workload

Run 3,600 wall-clock seconds at 48 kHz with 128-frame blocks.
Report a separate 256-frame run without replacing the 128-frame result.
Short runs test the harness and do not satisfy the hour gate.

Use two continuous, synthetic stereo PCM WAV sources at 48 kHz.
Each source contains percussion and sustained tones throughout the run.
Each production decoder read receives an additional 1 ms delay.
The normal bounded stream cache serves both sources without a readiness barrier between callbacks.
Prime sources before the timed interval, as the native host does.

Store four points per second in each volume, high-pass and low-pass curve on both clips.
Both clips play for the entire timed interval.
Live inputs control clip 0, so clip 1 continues to evaluate all three stored curves.
Enable cue outputs and include all four output channels in the checksum.

Use separate input producers for the UI lane and controller lane.
The UI lane sends ten commands per second.
The controller lane sends 120 commands per second during the first five seconds of each 30-second period.
Cycle through volume, high-pass and low-pass commands with unique float values for correlation.
Stop new inputs 100 ms before the end so the last callback can capture them.
This is a synthetic gesture model, not an S4 report-rate claim.
Retain at most 200,000 capture events, matching the current app limit.
Report rejected, missing, duplicated or altered events explicitly.

Paint the production arrangement component into a 1,200 by 700 pixel image at a target of 50 paints per second.
Scroll the viewport and cycle the selected automation parameter.
Move a clip in an independent editor copy and restore its original position.
This exercises the production painter without native input or display presentation.
Report paint count and missed paint intervals separately.
The hour pass flag requires all scheduled paints and zero starts more than one paint interval late.
Report update-and-paint execution time and spacing between paint starts as exact distributions.
This is offscreen cadence; it does not measure native window presentation.

Scan a synthetic library of 1,000 files and decode a short analysis fixture every ten seconds.
Request and cancel a separate prepared seek every 30 seconds.
The library worker never reads the user's music or preferences.
Capture and callback metrics drain on another thread at a target interval of 1 ms.

## Clock and targets

Use the steady clock for input receipt and callback execution.
The virtual host follows absolute block boundaries and sleeps until 100 microseconds before each boundary.
It spins for the remaining interval.
The `normal` mode uses an ordinary thread with default priority.
The `realtime` mode requests JUCE's real-time thread policy for the virtual host only.
It requests the block period, one tenth of the period as expected computation, and one half as maximum computation.
The harness fails explicitly if JUCE cannot start that thread.
Its reported JUCE flag records the requested mode, not proof against later kernel demotion.
Neither mode changes persistent system configuration or measures driver behavior.
The [scoped playback-activity candidate](experiments/2026-11-05-playback-activity.md) also requests a macOS user-initiated process activity.
Its report records the request; the OS can override it.
The process activity ends after the virtual callback and its workers finish.
This candidate keeps all original workloads and targets and leaves the earlier failed results intact.
Never reset the host origin after a late wake.
Report wake lateness, callback duration and completion after the virtual deadline independently.
Report the duration of the checksum and telemetry submission outside the production callback separately.

Before calculating percentiles, join callback and host measurements by project frame.
Reject duplicate, invalid or missing measurements rather than shifting later associations.
Retain the first 256 blocks whose wake lateness or process-call duration reaches half the block interval,
plus every virtual deadline miss within that bound.
Each retained row contains the planned boundary, wake, process return, checksum completion, deadline and
matching internal callback duration.
Keep the global maximum wake, process call and deadline miss even if the row bound is reached.
Report the number of omitted rows and trace completeness separately from musical capture integrity.
An incomplete trace cannot pass the hour gate.
These durations use the steady clock and include time when the thread was descheduled;
they cannot establish DSP CPU cost or identify kernel behavior on their own.

Allocate and touch the three callback-history arrays and painter-history arrays before the timed origin.
The virtual callback publishes one fixed-size host measurement through a bounded queue per block.
Only the consumer retains outlier rows or indexes history.
The process peak memory measurement still includes this allocation and the entire run.

The 128-frame interval is 2.666667 ms.
Target zero virtual deadline misses, zero source starvation and zero capture loss for the entire hour.
Target callback p99.9 below half the interval and maximum below the interval.
Retain every callback duration for exact percentile calculation off the callback thread.
Report metric queue loss and sample count, so lost measurements cannot hide a tail.
Declare a process peak memory budget of 128 MiB and a PCM page budget of 1 MiB for two sources.
Report output checksum, finite-sample failures, quiet blocks, background job counts and source errors.

Match captured values to producer records after all producer threads join.
Keep intended producer wake time, actual input receipt and actual applied frame distinct.
Collect optional scheduling telemetry through a bounded queue.
Compare its scheduled frame with the callback anchor and the production nearest-frame rule.
Late inputs clamp to the callback's first project frame.
Compare this prediction with the captured frame for every accepted command.
Predict software output at the virtual callback anchor plus the sample offset and one block of sink buffering.
The one-block buffer is a declared virtual-host assumption, not a device latency estimate.
Report receipt-to-predicted-output p50, p95, p99 and maximum.
Target p95 below 10 ms under the declared virtual buffer assumption.
Keep intended-to-receipt producer jitter in a separate distribution.
No result measures physical control, converter or acoustic latency.

## Faults and evidence

Run separate fault cases before the timed workload.
Exercise input and capture queue overflow, blocked decode cancellation, missing media and simulated device loss.
Retain and save each valid captured prefix without presenting an incomplete take as complete.
Reopen the saved prefix and compare its automation points.
Keep fault results separate from the uninterrupted hour's counters.
Existing export transaction tests cover failed destination writes and retain prior valid files.

Archive the fixture recipe, commands, source revision, dependency pin, OS, toolchain and build configuration.
Archive aggregate JSON and a synthetic captured set without private media paths.
Write progress to standard error during the run.
Inspect the same live process handle after an observation timeout.
Record failures against these targets without changing the targets after measurement.
The issue stays open while device runs or required native observations remain incomplete.

A two-second prototype smoke run exposed five virtual deadline misses with the normal thread.
It retained 247 of 247 inputs and recorded zero source starvation frames.
This diagnostic preceded the final committed harness and does not qualify the hour gate.
The real-time mode tests scheduler policy as a remediation without changing the deadline or workload targets.
JUCE documents [real-time thread creation](https://docs.juce.com/master/classjuce_1_1Thread.html).
The pinned implementation checks the initial Mach policy request before it reports successful thread creation.
Apple describes [Mach scheduling](https://developer.apple.com/library/archive/documentation/Darwin/Conceptual/KernelProgramming/scheduler/scheduler.html) as soft real-time.
An accepted policy does not guarantee every future deadline.

## Commands

Build the native preset and run all tests before the sustained measurement.
The `software_stress_smoke` test runs isolated faults and two seconds of the timed workload.
It requires capture and source integrity, but it does not enforce performance targets on CI runners.
Use a clean committed checkout and byte-identical build source for the driver.
The driver writes the exact revision, binary hash, fixture hashes, build metadata and aggregate results.
It also archives the captured project through the production loader and writer.
The [checked evidence workflow](experiments/2026-11-05-stress-archive.md) retains the executable, original fault files and unrebased compiled project bytes automatically.
Its short native CI run tests the driver, not hour performance.
The following example uses a separate build-source folder for an existing local JUCE checkout.

```sh
python3 scripts/make-stress-fixtures.py /tmp/fader-stress-fixtures
python3 scripts/run-software-stress.py \
  --source-root /path/to/clean/Fader \
  --build-source /path/to/build-source \
  --probe /path/to/build-source/build/native/fader_probe_artefacts/Release/fader_probe \
  --build-info /path/to/build-source/build/native/fader-build-info.json \
  --fixtures /tmp/fader-stress-fixtures \
  --output /tmp/fader-stress-128-evidence \
  --seconds 3600 --block 128 --scheduler realtime
```

Run the 256-frame case into a new evidence directory.
The [full-timeline CPU diagnostic candidate](experiments/2026-11-05-full-timeline-painter-diagnostic.md) can keep the full project during a shorter observation.
Its instrumentation and shorter duration exclude it from hour qualification.
Run one host at a time and record other machine activity.
The peak process memory measurement includes capture commit and project save after the timed interval.
