# Existing music library inventory

Date: 2026-11-04. Status: completed **read-only inventory and OS metadata probing**. No Fader decoder exists yet; no full decode, audible playback, or compatibility certification was performed.

## Question and method

Identify actual formats and variants in the user's confirmed `~/Music` library to inform [import requirements](../audio-import-and-library.md). Enumerate files with `rg --files --hidden`, excluding `.DS_Store` and `.git`, without following directory symlinks. Inspect MP3/WAV candidates and ID3-header-bearing files under other suffixes using macOS `/usr/bin/afinfo`. Use `/usr/bin/file` for a small unusual-suffix check. Read headers/metadata only; do not rename, convert, retag, move, or play files.

The run used the observed Apple M3/macOS 26.6.2 host. Native metadata inspection is not the decoder planned for all product platforms. File enumeration follows `rg` ignore behavior; this is the enumerated snapshot, not a guarantee about symlink targets or inaccessible/ignored locations. Counts are files/paths, including samples and duplicate copies, not unique songs.

## Aggregate results

| Observation | Count |
| --- | ---: |
| Enumerated files, including non-audio | 4,267 |
| Files named with `.mp3` suffix | 3,554 |
| Files named with `.wav` suffix | 319 |
| Additional ID3-bearing candidates under other suffixes | 4 |
| Audio candidates with readable OS metadata | 3,877 |
| Detected MPEG audio candidates | 3,558 |
| Detected WAV candidates | 319 |
| Metadata probe failures/timeouts for selected candidates | 0 |

Of the four additional MPEG candidates, two have `.crdownload` suffixes and two have other nonstandard suffixes. Partial-download candidates remain pending/incomplete even if their headers are readable; metadata success does not demonstrate complete audio data. Non-audio material includes software presets, databases, images, and archives and is not a song-import failure.

| Audio property reported by OS metadata | Count |
| --- | ---: |
| 44.1 kHz | 3,807 |
| 48 kHz | 57 |
| 88.2 kHz | 12 |
| 96 kHz | 1 |
| Mono | 16 |
| Stereo | 3,861 |
| WAV 16-bit integer | 158 |
| WAV 24-bit integer | 158 |
| WAV 32-bit float | 3 |

MP3's compressed representation is not reported as a PCM bit depth. Header probing did not establish the library's CBR/VBR distribution, complete-stream integrity, exact seek accuracy, or cross-platform decoder agreement. No media or track-name manifest is included in this report.

## Decision

Promote MP3 import into the completed POC; retain WAV as the controlled audio-engine fixture and initial export format. Handle mono/stereo, float WAV, and all observed rates in qualification rather than assuming every track is 16-bit/44.1 kHz stereo. Content probing must supplement filename filtering; pending downloads need a distinct state.

DJ-035 covers representative MP3/WAV POC loading and seeking. DJ-036 requires full decode verification of supported, complete files in the collection and a specific local explanation for every failure, plus broader common-format fixtures. This inventory passes its discovery goal; playback and application compatibility remain untested.
