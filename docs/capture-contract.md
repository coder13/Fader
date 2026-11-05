# Performance capture contract

Status: proposed implementation for [#12](https://github.com/coder13/Fader/issues/12).
This document records requirements and a prototype design, not a capture qualification result.
The current app captures clip controls in memory, but it lacks raw master recording and a durable take journal.
The [clock contract](playback-clock.md) and [project format](project-format-v3.md) remain authoritative for applied timing and saved sound.

## Required behavior

The user can choose automation-only, audio-only or combined capture before recording starts.
Automation captures the selected clip parameters independently from the master PCM arm.
PCM means the uncompressed digital audio samples.
The audio recording contains the stereo master before device output, with no headphone cue or physical output-volume control.
The interface shows the armed mode and targets before Record.

| Mode | Original automation take | Stereo master audio |
| --- | --- | --- |
| Automation-only | Required | No file opened |
| Audio-only | No editable target take | Required |
| Combined | Required | Required |

The recorded controls retain actual applied frames, step targets and the versioned smoothing policy.
A nonzero start retains its project-position anchor and transport segment identity.
The original project snapshot and original actions survive later edits and curve compilation.
Headphone routing and hardware output controls remain outside master automation.
Physical S4 qualification remains in [#9](https://github.com/coder13/Fader/issues/9) and [#17](https://github.com/coder13/Fader/issues/17).

## Proposed ownership

A worker prepares recording destinations and the initial snapshot before it reports the capture session ready.
The snapshot retains project, clip and source identities, render policy, armed targets and range, sample rate, and engine/project anchors.
It excludes device handles, temporary decode-cache locations and monitoring state.
The snapshot uses the project reader's metadata validation without requiring unavailable media to become playable during recovery.

The callback sends applied events and copied stereo master blocks through separate bounded queues.
It performs no file I/O, allocation, lock, reference reclamation, JSON work or encoder call.
The recording worker owns file writes, integrity checks, checkpoints and finalization.
The UI observes progress and failure state without draining the only copy of a take.
Queue and memory capacities must be declared before qualification.

Audio-only controls still change the heard master, even though they do not create an editable automation take.
Combined capture consumes the same applied actions and master samples as normal playback.
Clip indices resolve through the immutable segment snapshot to stable clip IDs and semantic parameter names.
Same-frame actions retain their applied order in the original journal, even when the editable curve uses the final target.

## Proposed journal and audio rules

Append framed chunks with sequence numbers and integrity checks on the worker.
Record enough length and version information to reject malformed, oversized or unsupported chunks before allocation.
Keep the initial snapshot immutable and store the original applied actions separately from editable curves.
A recovery reader accepts only the validated contiguous prefix and reports where validation stopped.
An applied-action sequence must expose capture-queue gaps before the reader can call retained data a contiguous prefix.
The current `Event` queue does not provide that sequence, so implementation must add it or an equivalent explicit loss boundary.

Use a declared checkpoint interval and report the last confirmed checkpoint during recovery.
Process-termination tests must distinguish queued data, written data and data confirmed by an OS flush.
Do not infer power-loss guarantees from a successful process-crash test.
The final format, chunk sizes and checkpoint interval require implementation and measurements before they become a file contract.

The proposed raw format is stereo float32 WAV at the project rate, preserving finite headroom without normalization.
Recording excludes unused padding after the declared final project frame.
It records the actual rendered master, including source-gap fades, rather than reconstructing those samples from automation.
Source-gap warnings remain separate from loss of captured actions or recorded PCM.
Large-file/RF64 behavior needs its own evidence before claiming recordings beyond tested file sizes.

## Failure and transport rules

Automation and audio each have a completeness state and a retained-data location.
A failure in one recording must not silently discard the other recording.
After a missing PCM block, retain the contiguous prefix and report the gap instead of joining later audio as continuous time.
After an event gap, retain validated actions with explicit loss information instead of calling the journal complete.
Preserve the original project when a take cannot be compiled or saved within its current storage limits.

Stop, pause, seek, source assignment and clock changes need explicit segment boundaries or supported semantic action records.
A source change creates a new source/clip identity in the captured history.
An unsupported live action must be refused visibly or end the affected segment explicitly.
It must not mutate the original snapshot while leaving a misleading replayable take.
Clock restart or rate change cannot resume the previous segment silently.
These rules do not introduce tempo, scratch, loop or live-rate semantics before their respective engines exist.

Finalization runs off the UI thread after callback detachment and drains accepted records before sealing results.
Cancellation and closure preserve recoverable partial files and identify incomplete recordings.
The UI reports whether automation, audio, both, or neither completed.
It distinguishes an archived original take from a successfully applied and saved editable project.
The current 200,000-event UI limit and 8 MiB project-manifest limit require explicit scale checks, not silent truncation.

## Required qualification

Exercise all three modes through fake callback hosts without opening audio or MIDI devices.
Compare combined raw PCM with independently hosted playback and saved automation, including nonzero starts and partial final blocks.
Verify cue exclusion, finite headroom, held targets, same-frame ordering, and the declared smoothing at 64/128/256/512-frame blocks.
Keep ordinary and aligned callback allocation checks enabled with both recording queues active.

Inject queue overflow, failed writes/flushes, full retention storage, blocked writers, cancellation and simulated device loss.
Verify separate completion states and the exact retained prefix for each recording.
Terminate child writer processes at checkpoint boundaries and recover their validated data without altering the prior set.
Exercise missing sources and malformed/truncated journal chunks with bounded reads and allocations.
Use the same portable source/project fixtures on macOS and Windows.

Archive exact source, configuration, capacities, workloads, complete results and remaining limits.
Keep [#40](https://github.com/coder13/Fader/issues/40) sustained-load findings distinct from these new capture tests.
Do not mark #12 complete from this proposed design or from existing automation-only checks.
