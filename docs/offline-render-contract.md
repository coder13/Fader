# Offline render contract

Tracking: [DJ-013](https://github.com/coder13/Fader/issues/13). Implementation/qualification in progress. The long-render budget below is set before running that measurement.

`OfflineRender` is an independent worker-owned host of the same `Engine` used by `Playback`. It opens no UI, audio device, controller or MIDI output. It waits for all required source windows before each process call and fails on missing frames, decoder failure, cancellation or non-finite master output. The process method remains nonblocking; readiness waits belong only to the offline owner. Each call renders at most 1,024 project frames into caller-owned stereo master buffers.

## Sound, range and version

Default export is project frame zero through the exclusive arrangement end, including gaps. The API also accepts an explicit nonempty `[first,end)` within that arrangement. A nonzero first frame reconstructs the entire preceding DSP history using the same engine rather than the bounded interactive seek approximation. This can be slow at distant starts and retains bounded PCM memory. Every referenced source must remain readable; a range is not a missing-media recovery mechanism.

Render order, source interpolation, volume/HP/LP smoothing and automation semantics use the saved renderer configuration. Current DSP is unchanged. Neither host introduces an alignment offset, normalization, dithering, tempo/stretch mode or extra effect chain. Output is stereo at 48 kHz: either 24-bit PCM, rejecting peaks over 1, or 32-bit float retaining finite values above 1. The float result is the pre-device master; headphone cue is excluded. MIDI and hardware commands are not part of an export. The project and active playback engine are not modified.

The current renderer stops a clip at its exclusive end and appends no filter tail after the requested export end. Range export is an exact slice of that renderer's uninterrupted output, not a new tail policy. New stretchers, delay/reverb, resonant effects and plugins need explicit render-version/state/tail contracts. The UI currently exports the full arrangement; this work adds no range controls.

For event parity, submit timestamped semantic controls to `Playback`, capture the actually applied project-frame events, compile them with `commitTake`, then render that plan offline. Compare master PCM within **1e-6** at **64/128/256/512** frame live/offline blocks, including boundary/same-frame events, delayed clip starts, source offsets/rates, volume/HP/LP and nonzero ranges. Do not use analog output, starvation-concealed live audio or lossy encoding as this baseline.

## Output transaction and errors

The WAV encoder writes a temporary sibling file. A checked stream latches every failed write or seek, including errors the pinned writer ignores while rewriting headers. Native file output is unbuffered so short writes are directly observable. The encoder's final flush and destructor header rewrite must both complete before the underlying file is flushed, its status checked, and it is closed. The temporary WAV is reopened to verify sample rate, channels and frame count. Only then may it replace the destination.

Cancel, decode/readiness failure, clipping in integer output, non-finite audio, header/write/seek/flush/validation failure, or replacement failure must produce an explicit exception. A prior destination stays intact before publication. Cancellation is checked before preparation, during source waits/reads, between bounded render chunks and immediately before replacement. A request arriving during the final replacement cannot retroactively undo publication of a valid completed file. Already running OS reads may delay worker teardown; the UI callback never joins those readers.

`ExportDestination` allows deterministic faults in the actual production encoder/transaction path. Qualification injects initial/final-header failure, data failure, a partial data write, final seek failure, flush failure, validation/finalization failure and refused publication. These are storage-error simulations, not claims that a physical volume was filled. A real blocked filesystem destination is also tested. Inputs are protected from replacement through their original path or an existing physical alias. Temporary-file cleanup is checked on failed runs.

The pinned JUCE writer contains an RF64 path for large output. This issue's large-file measurement is a 60-minute 32-bit stereo WAV below 4 GiB; it does not qualify every RF64 consumer or every 24-hour export. File/directory crash durability and recoverable backup/journaling are separate from successful ordinary replacement.

## Long-run budget and evidence scope

Before measurement, declare a **128 MiB peak process RSS** ceiling for a two-source probe, including import, export and verification; **1 MiB PCM pages** (512 KiB per clip); fixed ≤1,024-frame render buffers and bounded encoder scratch. This is a declared fixture budget, not a global cap for arbitrary 64-clip/UI/library sessions.

Use actual long WAV source files from the synthetic streaming recipe. Render exactly **172,800,000 project frames (60 minutes)** to a real 32-bit stereo file; A spans the hour and B overlaps from minute 15. Both have saved volume/HP/LP curves and enabled runtime cue. Add **1 ms delay per decoder read** only to the export sources to force readiness waits. After completion, compare the entire output against an independent prepared `Playback` host, record PCM error, nonzero master/cue checksums, frame count/file size, cache/RSS, delayed-read count and elapsed times. Check the original project manifest is unchanged. Accelerated software rendering does not qualify wall-clock callback deadlines, acoustic latency or a physical audio device.

Reproduce with `fader_probe --export-soak A.wav B.wav output.wav`. The probe imports/hashes sources, writes a substantial temporary/output artifact and verifies all samples using bounded blocks. Use synthetic fixtures; no user library paths or audio are committed. Final exact source, CI and measurements are recorded in the experiment report.
