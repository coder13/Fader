# Audio import and existing-library compatibility

Status: User-confirmed requirement; proposed implementation and qualification. Updated 2026-11-04. The reference library is `~/Music`. A local inventory is evidence about files on disk, not proof that the unbuilt application can decode them.

## Requirement and delivery

Fader must use the user's existing song files directly, with common DJ-software formats and no prerequisite conversion/reorganization of the library. Import means preview, seek/cue accurately, display waveforms, analyze beats, place the file in a set, replay automation, reopen, and render it through the same decoded source timeline. Merely accepting a filename does not establish support.

The initial native audio harness may use WAV fixtures. The **completed M2 POC adds MP3 import alongside WAV**, because these are the observed library's main formats. WAV remains the first export format. The M4 library-ready alpha must qualify the broader format matrix and inventory the actual collection with no unexplained skipped audio. Import and export format coverage are separate decisions.

## Target format matrix

These are planned capabilities, not implemented claims.

| Format/container | Qualification target |
| --- | --- |
| MP3 | M2: CBR/VBR, relevant MPEG variants, tags, accurate seeking and encoder delay/padding handling |
| WAV/WAVE | M2: PCM 16/24/32-bit integer and 32-bit float; recognize RIFF variants and explicitly diagnose unsupported encodings; broaden RF64/extensible coverage in M4 |
| AIFF/AIF | M4: common integer/float PCM and endianness; distinguish compressed AIFC variants |
| FLAC | M4: lossless decoding, metadata and sample-accurate seek |
| AAC | M4: common AAC in `.aac` and MP4/M4A containers; probe codec/profile rather than extension |
| ALAC | M4: Apple Lossless in M4A/MP4, distinguished from AAC in the same container |
| Ogg Vorbis and Opus | M4: distinguish codecs inside Ogg containers; correct pre-skip/trimming and seeking |
| Additional legacy formats | Inventory-driven investigation, e.g. WMA/APE/WavPack; do not advertise before decoder/packaging/fixture qualification |

The common-format target is informed by [Mixxx's format list](https://manual.mixxx.org/2.5/en/chapters/library#compatible-file-formats). AlphaTheta's DJ hardware matrix also includes Apple Lossless, with support varying by device; it is a hardware comparison, not a blanket software compatibility statement. [AlphaTheta format matrix](https://rekordbox.com/en/support/faq/v7/#faq-q700035).

Qualify mono/stereo at 44.1/48/88.2/96 kHz, plus rates/variants found in the collection. The engine can resample supported input rates without rewriting the source. Multichannel sources need an explicit downmix/routing policy; never silently discard channels. Encrypted, incomplete, corrupt, missing, or genuinely unsupported files need distinct visible outcomes. DRM-protected downloads are not interchangeable with ordinary local audio files; that limitation also exists in established DJ applications. [NI file-loading guidance](https://support.native-instruments.com/support/solutions/articles/69000879453-traktor-error-message-filename-could-not-be-played-due-to-a-missing-or-corrupt-file-).

## Importer design

1. Recursively enumerate chosen roots and support individual drag/drop. Handle Unicode, spaces, long names, case differences, nested folders, symlinks without loops, and files unavailable on disconnected/cloud storage. Keep the original layout and audio untouched.
2. Probe headers/container/codec instead of using extensions as authority. The local inventory found MPEG audio under nonstandard suffixes, including partial downloads. Mark active/incomplete downloads as pending even if metadata is readable. Keep an explainable result for candidate files even when probing fails; skip software presets, databases, images and installers as non-audio.
3. Read metadata off-thread. Preserve user tags and report malformed tags separately from unreadable audio. Deduplicate indexing by source identity without deleting separate files or assuming identical titles mean identical audio.
4. Decode in workers through a versioned adapter with bounded PCM caches, source-frame seeking, priming/trimming, and consistent duration. Use the same decoder/sample-domain rules for waveform, beat analysis, playback and export. A playback position in VBR compressed bytes is not a source-frame position.
5. Reuse the project's asset hashes and relinking model. Global caches must not overwrite a saved set's source/grid identity. Do not write tags back to the music library by default.

Evaluate the chosen framework's decoder coverage against actual fixtures. If insufficient, add a pinned cross-platform decoder such as an appropriately configured FFmpeg build or smaller codec libraries; account for their dependency licenses and enabled components. Do not assume macOS decoding success proves Windows or browser support. Browser native codec availability needs a separate matrix or shared Wasm decoding. [FFmpeg formats](https://ffmpeg.org/ffmpeg-formats.html), [FFmpeg licensing](https://ffmpeg.org/legal.html).

## Library acceptance gate

Run a local full decode-to-end verification with our selected decoder, then exercise representative seek/cue/loop boundaries, waveform duration, set save/reopen and offline rendering. Every supported, readable song candidate must import, and every failure must have a specific local diagnostic. Compare start/end/seek samples against sequential decoding so MP3/AAC priming or VBR indexing does not shift prepared transitions.

Use a public fixture corpus covering the matrix, including mismatched extensions, malformed tags, truncation and unsupported codecs. Keep the user's music and track-level results local; publish only aggregate counts. Inventory counts include samples/caches and duplicate paths, not necessarily unique songs. Passing OS metadata probes is not full decoding or application validation.

The full-library pass belongs to M4; M2 proves representative MP3/WAV loading and set replay. See [inventory evidence](experiments/2026-11-04-library-inventory.md) and issues DJ-035/DJ-036 in the [backlog](roadmap-and-issue-backlog.md).
