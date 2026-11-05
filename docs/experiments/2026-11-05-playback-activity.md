# Scoped playback activity candidate

Status: implementation candidate with local and clean CI functional checks complete.
Sustained timing qualification remains pending; this is not an established repair.
The [completed CPU hour](2026-11-05-boundary-hour.md) shows long wall intervals with much smaller enclosing CPU deltas.
This motivates a process-policy experiment before changing DSP, drawing or cache layout.
It does not establish App Nap, memory faults or a particular scheduler cause.

## Policy and lifetime

The native playback bridge creates JUCE's `ScopedLowPowerModeDisabler` on the control thread before callback attachment.
The control thread releases it after callback detachment and outside the audio manager's callback lock.
The attachment error path also detaches and releases the activity.
No audio callback creates, destroys or queries this activity.
An unplanned device stop still follows the existing failure and control-thread cleanup path.
Physical attachment and device-loss behavior remain outside the present hardware-free qualification.

The virtual stress host creates the same scoped activity before its timed origin.
Its cleanup joins the callback and other workers before ending the activity, including exception exits.
The activity remains scoped through capture finalization and report construction.
The report names the policy and records `macProcessActivityRequested`, rather than claiming that the OS honors it.
The selected JUCE option is `NSActivityUserInitiatedAllowingIdleSystemSleep` on macOS.
Other platforms use JUCE's existing no-op implementation.

Apple describes activities as hints for lengthy user-requested work.
The request must end when the work finishes, and user preferences can override it. [Apple guidance](https://developer.apple.com/library/archive/documentation/Performance/Conceptual/power_efficiency_guidelines_osx/PrioritizeWorkAtTheAppLevel.html), [ProcessInfo](https://developer.apple.com/documentation/foundation/processinfo)
The pinned JUCE source supplies the exact implementation. [JUCE activity source](https://github.com/juce-framework/JUCE/blob/d60b7a78030661574a615b586d9325ba7a7fa7dd/modules/juce_events/native/juce_ScopedLowPowerModeDisabler.cpp)
An activity request is not a deadline guarantee or evidence of the process's prior power state.

## Qualification required

Build the app and probe, then run all existing native and core suites.
Keep callback allocation checks and the independent observation reader enabled.
Check clean macOS/Windows native and Linux core CI, including the exact metadata and full logs.
The Windows implementation remains a no-op and cannot count as a Windows performance repair.

Use a clean committed source and byte-identical build source for the original uninstrumented 128-frame hour.
Keep the same two sources, stored curves, control rates, origin, background work, painter cadence and original budgets.
Preserve the failed baselines and diagnostic, including their exact executables and raw files.
Record all outcomes, including a failed or inconclusive candidate.
A successful observation cannot by itself identify App Nap as the cause or establish a long-term failure rate.
Native presentation and physical audio/MIDI remain separate gates.

## Functional checks completed

Exact candidate `7e089d03fa3241d151e76bee0db11b90d0ee12a8` builds the app, probe and tests locally.
All seven local suites pass in 12.47 seconds, including the existing callback allocation checks.
Both local smoke objects report the macOS activity request and pass independent observation arithmetic checks.

[Clean CI 42058168225](https://github.com/coder13/Fader/actions/runs/42058168225) passes macOS/Windows native 7/7 and Linux core 5/5.
The Linux job also passes all eight reader tests.
Downloaded metadata, JUnit and full logs match the exact source, pinned JUCE, Release build and UI fixtures OFF.
There are zero failed, erroneous or skipped suites.
The [checked aggregate](data/2026-11-05-playback-activity-ci.json) retains the platform scopes, clock limits and ordinary-priority timing failures.

Both native archived reader outputs match fresh checks of their full logs exactly.
macOS reports the request as true, while Windows reports false for its declared no-op implementation.
The shared macOS wall/CPU smokes still report 496/507 virtual misses and 54/53 late painter starts.
Windows reports 11/1 virtual misses with coarse CPU accounting.
These are functional collection checks and do not qualify sustained performance or physical playback.

The original uninstrumented hour remains required at its original workload and limits.
The completed CPU diagnostic and all failed baselines remain preserved.
No production DSP, drawing algorithm, source cache layout or persistent OS configuration changes in this candidate.
