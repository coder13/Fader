# Playback timing and capture integrity

Status: implemented software contract for [DJ-008](https://github.com/coder13/Fader/issues/8). Hardware receipt latency, acoustic latency, sustained-load qualification and a real controller adapter remain unmeasured. See the [qualification report](experiments/2026-11-04-playback-clock.md).

## Clock domains

`Playback` wraps the immutable `Engine` snapshot. Every start/seek constructs a new nonzero segment identity off the callback; its engine frame starts at zero and its project origin is the requested seek position. Engine frames count rendered device frames, including blocks past the project end; the project cursor clamps at the set end. A device restart, rate change or stop invalidates the segment. Resuming requires a newly prepared segment, even at the same rate.

Controls carry a segment identity and receipt time in `steady_clock` nanoseconds. The callback pairs its current engine frame with a host-time reading, then maps inputs relative to that anchor. Conversion rounds to the nearest sample (half-frame ties away from zero), splitting seconds/remainder before multiplication to avoid overflow. A nonzero seek adds the project origin only when feeding the core. Capture stores the actual applied **project frame**, including late clamping; it does not mistake engine uptime for a project location.

Host receipt time is not the timestamp of audible output. Late events apply at the next processable frame and increment lateness counters; future inputs more than two seconds ahead are invalid. Inputs from old segments are rejected and counted without corrupting the new take. A future CoreMIDI/HID adapter must convert its clock or use this same receipt clock; native packet timestamps cannot simply be cast to nanoseconds.

## Ordering, capacities and ownership

| Boundary | Capacity and behavior |
| --- | --- |
| UI → audio | 4,096 entries, exactly one UI producer |
| Controller → audio | 4,096 entries, exactly one independent producer; no UI timer hop |
| Pending scheduler | Fixed heap of 4,096; at most 4,096 inputs ingested per callback, alternating lanes |
| Scheduler → core | At most 4,096 commands per callback; future commands stay in the scheduler until their block |
| Core → capture consumer | 16,384 applied actions; overflow is sticky and the retained prefix survives |
| Callback → metrics consumer | 1,024 POD records; telemetry loss is counted separately and does not invalidate a take |
| UI retained take | At most 200,000 actions; reaching the cap marks the take incomplete |

Inputs available before a block are ordered by mapped engine frame, then UI before controller, then producer sequence. This avoids a future FIFO entry hiding an earlier timestamp. An arrival after that block's bounded dequeue cannot retroactively change audio: it is clamped later and its actual frame is recorded. Same-frame applied order remains in the captured array; compilation uses the last target for that parameter/frame.

Construct, seek, arm and attach on the control/worker side before callbacks use the snapshot. Each producer owns one lane; multiple controller endpoints will need one serialized producer or more lanes. Stop producers, detach the callback, then destroy its `Playback`. The bridge holds a reference, not an atomic shared pointer. No snapshot publication or reference-count reclamation happens in `process()`. The current UI producer uses the message thread; controller-lane concurrency is synthetic until an adapter exists.

`AudioPlayback::detach()` takes JUCE's callback lock on the control thread. It suppresses only the synthetic `audioDeviceStopped()` caused by callback removal. A preceding genuine stop/error remains sticky, so immediate user Stop cannot turn an interrupted recording into a complete one. JUCE serializes its stop/start/error notifications with audio dispatch using that same lock.

## Incomplete recordings

Input/scheduler overflow, invalid input, capture overflow, device interruption/reconfiguration and retained-storage exhaustion mark a capture incomplete. The UI stops the recording, retains accepted applied actions and the original immutable project, and shows **INCOMPLETE**. It does not commit those actions over the saved envelopes. A new recording is refused until the retained take is saved or explicitly discarded. New/Open/Quit collect an active take before checking for unsaved work.

**Set → Save incomplete take** writes an atomic `.fadercapture` JSON archive: `format: fader-incomplete-capture`, schema 1, `complete: false`, original project with relative source references, segment/range, actual cursor/engine frames, failure flags, drop/lateness counts and ordered applied actions. No audio is embedded. Validation happens before replacing a destination. Normal project loading rejects this archive as a complete set. The archive currently has no reopen/repair UI and no automatic crash journal; those remain [DJ-016](https://github.com/coder13/Fader/issues/16). Saving an archive clears the retained-take guard but does not make the performance complete.

## Callback audit

The audited project-owned path is `AudioPlayback` IO/lifecycle callbacks → `Playback::process`/clock flags → fixed queues/heap → `Engine::process`/`processClips`/curve lookup, ramps and filters. It performs no file access, logging, mutex acquisition, allocation, owning-pointer replacement or reference reclamation. Lock-free atomic requirements are asserted at compile time. Curve lookup is binary search over validated arrays (up to 200,000 points per curve); mixing is bounded by the host block size, channel count and validated 64-clip limit. Fixed event budgets prevent an actively refilling producer from keeping the callback in a dequeue loop indefinitely. These are work bounds, not proof that a saturated block meets a device deadline.

The allocation fixture intercepts ordinary and aligned C++ `new`/`delete` around 20,000 engine calls, 20,000 playback calls, saturated queues and stopped callbacks. Direct C allocation and OS/library internals are not interposed. Source audit complements this test; neither claims that the entire JUCE/OS audio stack is lock-free. In particular, JUCE's manager holds its own callback-dispatch lock. Attach/detach take it only off the audio thread.

Metrics contain segment, starting engine/project frames, block size, remaining scheduled inputs and processing duration. Measurement covers scheduling/mixing or fault silence through the end of processing, excluding metric publication and framework dispatch. Counters publish approximate atomic snapshots without reader retry loops. The UI drains at most 1,024 records per timer tick; the diagnostic probe drains/sorts/reports off-thread, including metric drops. Timing percentiles describe retained synthetic/device-smoke samples, not acoustic latency. Audio underruns and capture completeness are separate measurements.
