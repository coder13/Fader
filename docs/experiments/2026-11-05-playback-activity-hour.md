# Playback activity hour: callback targets pass, painter target fails

The completed uninstrumented hour records zero virtual deadline misses and retains all 107,999 controls.
Callback headroom, predicted software latency, source/capture integrity, save and memory pass their declared checks in this observation.
However, 2,030 painter starts are over 20 ms late, so the overall hour gate fails.
[#40](https://github.com/coder13/Fader/issues/40) remains open.
A single observation on an ordinary desktop does not establish a long-term failure rate or identify the cause of a pause.

The [full aggregate](data/2026-11-05-playback-activity-hour.json) includes final metadata, faults, retained rows, preservation checks and operator activity.
The [policy design and functional CI](2026-11-05-playback-activity.md) describe the scoped user-initiated activity.
The failed [128/256-frame baselines](2026-11-05-software-stress-followup.md) and [CPU diagnostic](2026-11-05-boundary-hour.md) remain preserved.

## Source, workload and completion

The clean measured source is `1c07b9c9f7650b0694bf3ef99e63dc1dac0c3b0d`.
Compiled policy code is `7e089d03fa3241d151e76bee0db11b90d0ee12a8`; later candidate commits contain documentation only.
The source-tree SHA-256 is `e0b81c5aea9741cc216e2303fda74961a0a93610ec7ee1820c51c1728499438e`.
The probe SHA-256 is `db88032ee2271816d394318e4ff591c0d05ff1168becfbb87b5af4792b66f569`.
All 153 tracked files match the build source at the start and after completion; the source and executable hashes still match at the end.
These checks supplement the recorded no-mutation policy; they are not a continuous filesystem trace.

The host is an Apple M3 with eight physical/logical cores, 16 GiB RAM and macOS 26.6.2.
The Release build uses AppleClang 21, C++20, CMake 4.4.4, arm64, pinned JUCE `d60b7a7` and local UI fixtures ON.
The candidate passes all seven local suites and checked macOS/Windows native 7/7 plus Linux core 5/5 CI.
This hour uses the original 3,600-second timeline, stored curve density, 48 kHz/128-frame virtual host, delayed decoding, controls, background cadence and 50-paint/s workload.
CPU diagnostics are disabled and `performanceQualificationEligible` is true.
No audio or MIDI device opens; the production arrangement painter renders offscreen.

The scoped activity begins before the timed origin and ends after workers join.
The report records `macProcessActivityRequested: true`.
It records the request, not proof that the OS honors it or that App Nap caused earlier pauses.
No DSP, drawing algorithm, cache layout, duration, startup origin, density or budget changes for this candidate.
No local build, sanitizer or second playback host runs alongside the driver.
Ordinary user/system activity is uncontrolled; source review, remote CI work, small Python/file checks and the idle restored preview are recorded in the aggregate.

The timed probe finishes at `2026-11-05T18:04:56.630667Z`.
The whole driver and production archive finish at `2026-11-05T18:05:40.458343Z`, exit 3.
The observed interval is 3,600.018272917 seconds.
Only after the exact process handle reports terminal completion are the original files copied and checked.

## Callback and capture results

| Metric | Result |
| --- | --- |
| Callback and host observations | 1,350,000 each; no invalid, duplicate or missing observations |
| Virtual deadline misses | 0 |
| Callback p99.9 / maximum | 59.209 microseconds / 2.654792 ms |
| Callback plus checksum/sink maximum | 2.656167 ms |
| Maximum host wake lateness | 0.721542 ms |
| Predicted receipt-to-software-output p95 / maximum | 5.166167 ms / 29.422667 ms |
| Planned / attempted / accepted / captured controls | 107,999 each |
| Input, capture-retention and callback/control/host metric losses | 0 |
| Source gaps/failures, non-finite samples and quiet blocks | 0 |
| PCM pages / delayed reads | 1 MiB / 42,202 reads with the declared extra 1 ms delay |

Callback p99.9 is below half the 2.666667 ms interval and the maximum is below that interval.
The maximum is close to the deadline: the retained maximum-process-call block finishes only 10.458 microseconds before its virtual deadline.
This is not a broad safety margin or a physical driver result.
Predicted p95 passes the original 10 ms target under the one-block virtual buffer assumption; the reported maximum is larger.
Physical control, converters and acoustic latency are not measured.

The complete host trace retains 20 tail rows with none omitted, plus global maxima.
Fifteen of those tails end at a 4,096-frame boundary, including 12 halfway through a logical 8,192-frame source page.
The maximum internal callback duration is 2.654792 ms in the boundary population and 1.960625 ms elsewhere.
This remains a frame correlation, not a native page-fault observation.
The independent reader checks 22 serialized host rows and 258 painter rows; arithmetic and context checks pass.

The background worker records 361 library jobs, 361 analysis jobs and 121 seek cancellations.
These are actual counts, including work admitted near the endpoint before shutdown; the original cadence is unchanged.
They are not normalized down to earlier runs' 360/360/120 counts.

## Save, preservation and memory

The compiled project contains 150,137 points and saves in 0.423835209 seconds.
Its original 7,443,828 bytes remain below the unchanged 8 MiB manifest limit.
Peak probe RSS is 96,993,280 bytes, exactly 92.5 MiB, below the original 128 MiB budget.
The peak before capture commit is 88,162,304 bytes.
This is the stress probe's process peak, not a native full-library window footprint or parent/helper process budget.

The production-rebased archive contains 7,443,882 bytes.
Independent checks match every non-path field, JSON type and all 150,145 parsed float64 values bit for bit.
Both asset references name the same original physical source files on this host.
The original SHA-256 is `35ec8a10e62f024f08cfb917021e6d472119efc60511fcf78c08dbe115211910`.
The archive SHA-256 is `c93533df3aa1aa68807f64d0be43623174500552a44ab853fdec2f0f8422fdc6`.

Exact executable bytes, the original compiled project and all six original fault files are preserved locally with hashes and path context.
This hour executes the older driver at the original build path.
The post-run copy matches its initial executable hash; it does not claim that this hour used the newer retained-copy execution path.
Likewise, the original project is copied after production rebasing and matched to the original hash recorded before rebasing.
The independently checked preservation helper comes from `079a37b` with implementation unchanged from qualified `bb994cc`.
The preserved project is a compiled curve set, not the durable original-action journal required by #12, and metadata preservation is not PCM-parity proof.

## Painter failure

All 180,000 paints complete, but 2,030 starts exceed the allowed 20 ms lateness.
Update-and-paint wall p99.9 is 55.738083 ms and maximum is 525.232167 ms.
Maximum start lateness is 1,716.274292 ms.
Average paint count does not satisfy the zero-late-start requirement.

The longest paint is index 108,671, viewport X 2,013, low-pass selected, with the editor clip displaced by one second.
It starts 736.520375 ms late and takes 525.232167 ms.
The largest start lateness occurs at index 108,686, viewport X 2,058, low-pass selected and no clip displacement.
That paint itself takes 9.805875 ms.

The bounded painter trace retains 256 of 3,461 slow/late rows and reports 3,205 omissions.
Execution and wake maxima survive truncation; the distributions retain all actual paint samples.
This is not a complete painter-tail timeline.
This run does not collect painter CPU time, so its long wall intervals cannot distinguish expensive drawing from descheduling or other pauses.
The earlier CPU diagnostic is a separate run and must not be substituted for missing CPU data here.

## Decision and next work

Keep the original targets and the failed overall result.
Zero observed callback misses improves this observation over the failed baseline, but does not prove a causal power-policy repair or qualify every future run.
Painter cadence is now the remaining failed check in this particular software hour.
Review the painter's work and host scheduling with a diagnostic that preserves the full timeline and curve density.
A short timing option that shrinks the project cannot stand in for that full-scale drawing load.
Qualify an actual repair with another full original-workload uninstrumented hour, retaining this result.
Native presentation, physical output and the remaining capture/product gates stay separate.
