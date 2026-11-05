# Capture implementation plan

Status: preparation for [#12](https://github.com/coder13/Fader/issues/12), not implemented or qualified.
The [capture contract](capture-contract.md) defines required behavior.
This plan turns the reviewed code into an implementation order while #40 runs.
It does not replace a failed prerequisite or count as capture acceptance evidence.

## Changes required by the current code

| Current boundary | Required change |
| --- | --- |
| `Engine` puts an `Event` into its capture queue after applying it. | Publish an applied-action ordinal with each captured target. An overflow must leave a detectable hole, including when the lost action is the last action. |
| `Playback` and `AudioPlayback` render directly into the host's output pointers. | Capture a complete stereo master before host channel selection. A null or absent device channel must not become a missing recorded channel. |
| The UI timer drains the only event queue into a 200,000-event vector. | Give the journal worker sole ownership of that drain. The UI reads progress; it never competes for events. |
| `Main::stop` compiles the take on the UI thread. | Detach the callback, drain/seal the recording, then compile/save on a worker. A failed compile must leave the original journal available. |
| Playback failure flags include input and device failures. | Map causes to separate automation and PCM states. A rejected gesture can invalidate intended automation while the recorded master still contains exactly the rendered sound. |

The snapshot must retain the renderer's parameter meaning.
Clip volume/high-pass/low-pass targets and a legacy mixer crossfader cannot share a misleading parameter name.
Resolve runtime target indices to stable project/clip IDs and semantic names through the immutable snapshot.
Do not send strings or owning references through the callback queues.

## First queue and buffer proposal

These are candidate capacities for measurement, not supported-scale claims.
Allocate and touch all storage before Ready; reject an invalid configuration before attaching a callback.

| Item | Candidate capacity | Payload cost or time at 48 kHz |
| --- | --- | --- |
| Applied-action queue | 16,384 entries; action plus 64-bit ordinal | About 512 KiB if the runtime entry is 32 bytes; measure actual layout. This replaces the current capture queue. |
| Master PCM queue | 1,024 entries, each holding at most 64 stereo float32 frames | 512 KiB of PCM plus packet headers. With full packets it holds 65,536 frames, about 1.365 seconds. |
| Master render scratch | Four planes of up to 4,096 float frames | 64 KiB, prepared outside the callback. Two planes are master, two are cue. |
| Journal worker batch | At most 256 actions | Bounded serialization work; no per-take accumulation during recording. |
| PCM worker batch | At most 1,024 frames | 8 KiB of stereo payload; combine queue packets on the worker. |

Queue metadata, alignment, stream objects, snapshots and decoder memory are additional costs.
Report `sizeof` and actual process peak rather than treating payload sums as total memory.
Small final packets consume a full queue slot, so 1.365 seconds is not a guarantee for every packet pattern.
Test ordinary 64/128/256/512-frame callbacks and partial final blocks explicitly.

The first implementation can declare a 4,096-frame host-call maximum for recording.
An unexpected larger call must stop that PCM recording with an explicit reason while preserving its prefix.
It must not allocate a larger buffer, corrupt playback or invalidate the independent automation journal.
Confirm that the native host is configured within this bound before Ready.

## Master bus and ownership

During PCM capture, render into the prepared four-plane buffer, copy only master left/right into PCM packets,
and route the same rendered samples to the host's available outputs.
Headphone cue stays on its separate planes.
The recording must not depend on whether the host exposes two or four outputs, a null output pointer, or monitoring preferences.
Audio-only mode still applies live controls to the heard master without creating an editable action take.

Use an automation worker and a PCM worker with separate queues and error state.
A blocked or failed journal write must not prevent the PCM worker from draining its queue, or vice versa.
A slow disk shared by both streams can still make both queues overflow; each stream must report its actual failure.
Do not promise that independent workers can overcome a blocked storage device.

The capture controller owns the prepared session until the callback is detached and both workers finish.
The callback holds a non-owning tap with that lifetime guarantee.
It never releases the last reference to the session, snapshot, stream, packet storage or decoder.
Worker shutdown and file finalization must not join on the UI or callback thread.

## Applied-action loss boundaries

Assign an ordinal at the point where `Engine` actually applies an armed, valid target.
Increment it before attempting the capture-queue push, not only after a successful push.
Keep the first failed ordinal and the final attempted ordinal in the detached segment summary.
The summary is necessary when no later event arrives to expose the last lost action.
Input acceptance, application and archival are different counts and must remain separate.

For example, received ordinals `1, 2, 4` prove that the automatic replay prefix ends at `2`.
A loss flag or detached final ordinal must also expose a missing final `3` when only `1, 2` reach the worker.
Do not renumber retained actions to conceal either case.
Same-frame targets retain their applied order, even if later curve compilation keeps only the final target.

After a gap, preserve the validated prefix and explicit loss information.
Any separately retained suffix must stay labeled as incomplete history and must not be joined automatically across the gap.
An unsealed journal after a process interruption is recoverable history, not a completed take.
The reader must distinguish an interrupted file from a deliberately sealed incomplete recording.

## Persistence spike before freezing a file format

Keep the source set and initial snapshot immutable.
Use a new recording destination rather than truncating an existing set, source or recording.
The original applied actions remain separate from the compiled editable curves.
Recovery must validate snapshot metadata without opening media; the current project reader already has a metadata-only path.

The action journal needs a versioned header, bounded chunk length, chunk ordinal, payload integrity check and explicit terminal record.
Serialize integer fields and float bits explicitly; do not write padded C++ structs as a portable format.
Reject oversized lengths and invalid target references before allocating payload storage.
Stop at the first malformed or truncated chunk instead of scanning ahead for a plausible magic value.
The final byte layout and integrity algorithm require an independently specified cross-platform fixture before adoption.

Compare these PCM persistence options in a small fault-injection spike:

| Candidate | Benefit | Required evidence |
| --- | --- | --- |
| Float32 WAV plus an append-only checkpoint sidecar | Produces a conventional recording while reusing the pinned writer. | Recover after a torn header update; validate sample count, data offset and payload independently of a damaged current header. |
| Append-only checked PCM chunks, followed by WAV materialization | Uses the same recovery framing as actions and avoids rewriting a live audio header. | Measure finalization time and extra disk space; preserve the original chunks if WAV delivery fails. |

Prefer the smallest option that passes partial-write, process-termination and recovery tests on both native platforms.
Keep capture completeness separate from the success of a derived WAV delivery.
Neither option changes the required float32 stereo master or permits normalization and clipping.

The pinned JUCE WAV writer's `flush()` rewrites its header and restores the write position.
It is not a substitute for checking and flushing the underlying file stream.
Inspection used `juce_WavAudioFormat.cpp` at the [pinned revision](https://github.com/juce-framework/JUCE/blob/d60b7a78030661574a615b586d9325ba7a7fa7dd/modules/juce_audio_formats/codecs/juce_WavAudioFormat.cpp).
JUCE also documents that destruction alone does not force buffered file data to disk;
use explicit flushing and inspect stream status. [FileOutputStream documentation](https://docs.juce.com/master/classjuce_1_1FileOutputStream.html)
RF64 code is present in the pinned writer, but recordings beyond the tested size remain unqualified.

## Checkpoint and stop protocol

The first checkpoint candidate is one second of recorded audio or one second of recording time for actions.
Treat this as a requested interval, not a bound on storage latency or potential loss.
Each worker exposes received, written and OS-flush-confirmed positions separately.
Only advance the last confirmed position after the stream flush succeeds and its error status remains clear.
Keep the previous confirmed checkpoint when a write or flush fails.
Tests must cover a writer that stalls for longer than the queue's measured capacity.

On normal Stop, detach the callback before reading its terminal counters.
Drain accepted packets, compare final ordinals and frame counts, seal each stream, then publish separate results.
The UI shows Finalizing and remains responsive.
It may report that original actions are archived even when editable-project compilation fails its existing manifest limit.
It must not report that failed project application as a successful saved set.

For the first continuous-segment implementation, pause, seek, source replacement and clock changes cannot silently edit the snapshot.
Either implement a new anchored segment/action or end the affected recording with a visible reason before the operation.
Keep this policy explicit in the controls and terminal record.
Do not imply scratch, loop or tempo semantics before those engines exist.

## Qualification order

1. Add applied ordinals and terminal loss counters, retaining existing playback/capture parity and callback allocation tests.
2. Add the master tap, fixed queues and fake workers. Compare PCM against independent hosted playback across block sizes, cue settings, missing host pointers, nonzero starts and partial final blocks.
3. Prototype persistence with injected short writes, flush failures and stalls. Choose the PCM layout from those results and freeze portable fixtures before building the recovery reader.
4. Kill child recorder processes at each header/chunk/checkpoint/terminal boundary. Recover without source media, preserve the prior set, and distinguish validated bytes from confirmed checkpoints.
5. Integrate the three modes, armed-target display, per-stream finalization/failure messages and recovery UI. Share native screenshots from synthetic fixtures.
6. Repeat dense capture/compile/save and integrated sustained load with the declared memory, latency and storage limits. Run the same source/project fixtures on macOS and Windows before closing the software scope.

Physical controller behavior remains a separate gate.
The checklist above is planned work; none of it is marked passed by this document.
