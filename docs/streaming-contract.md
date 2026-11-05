# Bounded source streaming

Tracking: [DJ-038](https://github.com/coder13/Fader/issues/38). This contract replaces the prototype's full-track PCM admission limits. See the [qualification report](experiments/2026-11-04-bounded-streaming.md) for measured scope and remaining gaps.

## Storage and ownership

Imported audio retains a source descriptor, sample rate/length and waveform peaks. It does not retain decoded track PCM. Import still scans the entire file in 8,192-frame blocks to validate finite samples and build peaks, and hashes it before/after the scan. This bounds memory, not loading time. Project reopening verifies the saved SHA-256. Waveforms use an explicit source-frame stride that grows with duration; changing the cache resolution never changes clip duration.

An engine owns a `StreamBank`, with two decode workers and a separate cache for every streamed clip. Two clips may read different positions in the same file. Each clip has eight pages of 8,192 stereo float frames: **512 KiB PCM**, **32 MiB maximum across 64 clips**. Peaks are capped at **512 KiB per source**, with a **32 MiB set admission limit**. MP3 workers additionally use a 64 KiB discard buffer each. Codec indexes/metadata, automation, UI, runtime and transient old/new engine generations consume additional memory; these are not a global process-RSS cap. The long-file qualification sets a separate 128 MiB peak-RSS budget for its two-source probe.

The grid now permits 24 hours, 64 clips and 4,096 rows. Source rate remains 8–384 kHz and clip rate 0.25–4.0. Import rejects a source exceeding the 24-hour timeline. Existing in-memory synthetic fixtures still work, but normal file import always uses streaming.

## Render boundary

`request`, `sample` and `release` have one exclusive render/preparation consumer. Atomics publish requests and page ownership. The worker writes a page only while it owns the Writing state; the consumer pins Ready pages as Reading until the block ends. Neither side can overwrite samples the consumer is reading. The callback performs no file access, decoder call, lock, wait, allocation, thread join or cache reclamation. It requests source pages as the source cursor advances. Two workers open decoders lazily, prefetch the eight-page window, and poll at 1 ms when idle.

Each engine has separate cache/decoder storage and a generation ID. A completed read outside the latest requested window is discarded. Cancellation prevents further publication/serving; destruction joins the workers before reclaiming memory, outside the callback. Cancellation is observed between decoder chunks. An OS read already in progress may still block the control-side join until it returns. This is not yet the responsive seek/close contract of [#39](https://github.com/coder13/Fader/issues/39).

## Live gaps and offline readiness

When requested PCM is absent, the mixer fades its last available dry sample to zero over **240 project frames (5 ms at 48 kHz)**. Availability recovery blends from the last concealed sample to current source audio over the same duration. Filters and routing then process that signal normally. Master and cue use the same concealed source; these are not a promise that filter tails vanish in exactly 5 ms. Missing clip-frames and source failures are reported separately from automation-capture loss.

The transport shows **READ GAP**, **OVR GAP** or **REC GAP**, with a persistent status explanation. A valid automation capture can survive an audio gap; its saved curves may render differently when the source becomes available. Capture queue/lifecycle faults retain the separate **INCOMPLETE** behavior.

Offline rendering calls `prepareSources` before each 1,024-frame processing span. Preparation waits for all required samples, including interpolation neighbors, with a 30-second timeout. Decode/open errors, cancellation and timeout fail explicitly before destination replacement. Prepared rendering must report zero missing frames. Live rendering never calls this readiness barrier.

## Decoder correctness and limits

WAV uses the decoder's source-frame addressing. The pinned JUCE MP3 decoder failed random access at the tail of generated VBR files despite successful sequential decoding. MP3 workers therefore preserve sequential decoder state, discard intervening frames when moving forward, and reopen/replay from frame zero for a backward jump. Discarding is cancellable in 8,192-frame chunks and uses fixed scratch memory. This preserves the existing decoder sample domain but can make compressed seeks slow; fast checkpoint/decoder work belongs to #39.

The MP3 length includes the pinned decoder's encoder-delay/padding convention. The tests establish alignment with that decoder's sequential output, not gapless agreement with every other application. Per-read size/mtime checks catch ordinary source changes or disappearance; they are not cryptographic detection of an adversary preserving both attributes. Full hashes are checked at import/reopen. Broader codecs, network/removable storage, all-library compatibility, maximum simultaneous clips and sustained wall-clock deadlines remain separate qualification tasks.
