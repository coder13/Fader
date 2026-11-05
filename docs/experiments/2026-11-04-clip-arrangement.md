# Clip arrangement, filter automation and Traktor library experiment

Date: 2026-11-04. Source: branch `codex/clip-automation-browser`, Fader 0.2.0. Base: `c54887137a4ed5fc053ba381464ccf72d0acdab3`. Machine: Apple M3 MacBook Air, 16 GB, macOS 26.6.2 (25G83). C++20, Apple Clang 21, JUCE 9.0.5 at `d60b7a78030661574a615b586d9325ba7a7fa7dd`. Hardware remains unavailable; the user authorized software-only work.

## Question and implemented slice

Can the prepared-set workflow use independent volume/high-pass/low-pass envelopes per clip, a stable-scale arrangement and the user's existing DJ/Traktor library?

The app now has scrollable time/rows, explicit zoom, fixed-duration header dragging, selected-parameter curves on waveforms, independent clip controls/capture, a searchable/sortable library, and music/Traktor/audio Preferences. The implementation follows the hierarchy and ownership findings in the [UI atlas](../research/ui-evidence-atlas.md) and [workflow audit](../research/competitive-workflow-audit.md).

## Repeatable checks

- Native Release CTest: 3/3 pass (core, realtime core, media/editor integration). Tests cover legacy playback, version 2 save/reopen, source hashes, atomic export/cancellation, filter attenuation, block-size independence, nonzero-seek filter state, selective retakes and live/export agreement within 1e-6.
- Editor event tests invoke the real arrangement handlers: move a 12-second clip two seconds later and one row down; duration and envelopes stay identical. Add/drag/delete a high-pass point using clip-local time; other clips/parameters stay unchanged. Zoom at the origin keeps it visible.
- AddressSanitizer/UndefinedBehaviorSanitizer core configuration: 2/2 pass. JUCE/UI and media were tested in Release, not under these sanitizers.
- Allocation instrumentation: 20,000 process calls with two clips, volume/filters and cue; zero ordinary C++ new/delete in the measured callback. Synthetic Release timing: p50 7.542 µs, p95 8.416 µs, p99.9 24.125 µs, max 55.166 µs. This is offline timing, not a device deadline guarantee or exhaustive allocator audit.
- Silent Core Audio smoke: 48,000 Hz, 128 frames, 752 callbacks over two seconds; core p99 19.625 µs, max 23.5 µs. Driver xrun count unavailable (-1). Reported output latency 1,552 frames (~32.3 ms); configured buffer size is not total latency. No acoustic/loopback measurement or sustained-load qualification.
- Native window inspection verified arrangement hierarchy, visible clip envelopes, zoom, browser metadata and Preferences. Fixed zoom anchoring, filter readouts and Preferences contrast during inspection. Native save verified independent HP/LP edits while both clip durations and unrelated envelopes stayed unchanged. A save/export check exposed a macOS directory-alias problem; saving now canonicalizes the relative-path base, with a symlink-directory regression test. Direct pointer automation was inconsistent, so handler regression tests provide the repeatable movement evidence; an owner mouse/trackpad session is still required. The installed build subsequently saved a synthetic set through the native dialog, reopened it through the loader and exported WAV successfully. A startup stack sample identified synchronous folder discovery before the window appeared; saved settings now take priority and preference loading/discovery runs on a worker. Folder navigation is derived from scan results without a second filesystem enumeration on the UI thread.

## Read-only library findings

The selected DJ root contained 949 existing audio candidates. The selected Traktor collection had 955 entries overall. The browser matched metadata for all 949 existing files and included one missing entry under the chosen root: 950 rows total, all with BPM/key and 948 with a usable first grid marker. These are metadata counts, not a full decode qualification.

Six matches required physical-file identity rather than exact path strings. The importer narrows by file identifier and verifies filesystem equivalence, avoiding guesses based on titles or basenames. A synthetic hard-link fixture checks this path. The collection's content hash was unchanged by scanning; fixtures also enforce read-only behavior. No music, private absolute paths or track lists are included in the repository.

Imported fields: title/artist, album/genre, duration, tempo, Open Key, earliest nonnegative type-4 grid marker; other cues are counted. Full cues, loops, playlists, variable grids and analysis are not implemented. Traktor field/key interpretation was cross-checked against primary [Mixxx Traktor importer](https://github.com/mixxxdj/mixxx/blob/main/src/library/traktor/traktorfeature.cpp), [key enum](https://github.com/mixxxdj/mixxx/blob/main/src/proto/keys.proto) and [key conversion](https://github.com/mixxxdj/mixxx/blob/main/src/track/keyutils.cpp), plus the local NML structure. No Mixxx implementation was copied.

## Interpretation and remaining gates

This passes the bounded software experiment. It does not complete the Mac/S4 POC. Filters are simple 12 dB/octave proofs; snap uses seconds or a fixed 120-BPM bar. Exact seeking rebuilds history on a worker and can be slow. Full decoding is bounded by 256 MiB/source and 1 GiB unique PCM/set; this is not yet a long-set streaming engine. The grid is extensible within 64 clips, two hours and 4,096 row positions.

Priority next checks: owner usability test with real songs and retakes; streaming/cancellable seek, autosave/relinking; editable imported grids and tempo-aware snap; S4 control/cue routing and sustained performance when hardware is available. Version 1 sets preserve their original renderer and are playback/export-only in this editor. See [quickstart](../poc-quickstart.md) for all current limits and [ADR 0002](../decisions/0002-clip-owned-automation.md) for persistent semantics.
