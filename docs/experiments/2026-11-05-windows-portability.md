# Early Windows portability qualification

The initial native Windows x64 build passes all seven software test suites.
The same source passes Linux and macOS CI.
This is an early [#41](https://github.com/coder13/Fader/issues/41) portability result, not audio-device, installer or controller qualification.
The final build also reads and resaves one shared project fixture on both native hosts.

## Exact build and evidence

Tested source: `90d9abb90ae4ec5a63272bc75a325e285e6e6918`.
[Run 42005831656](https://github.com/coder13/Fader/actions/runs/42005831656) passes Linux 5/5, macOS 7/7 and Windows 7/7.
Downloaded metadata, JUnit and logs match that source and JUCE `d60b7a78030661574a615b586d9325ba7a7fa7dd`.
UI fixtures are OFF and JUnit reports no failures, errors or skipped cases.
The [initial aggregate](data/2026-11-05-windows-initial.json) records toolchains and decoder observations without private media paths.

| Configuration | Observed value |
| --- | --- |
| Windows | Windows Server 2025 Datacenter, 10.0.26100, build 26100 |
| Runner image | `win25-vs2026`, `20261030.294.1` |
| Visual Studio | Enterprise 2026, installation 18.11.12328.437 |
| Compiler | MSVC 19.51.36260.0, toolset 14.51.36231 |
| SDK | 10.0.26100.0 |
| CMake and generator | 4.4.5, Ninja |
| Build | C++20, Release, x64 |

The built executable is 7,226,368 bytes.
Its SHA-256 is `bd0fa31777d6d4f1884c7c89b69be2e6c18f3ad35ee41ad509909be5d9312a9b`.
A separate header read confirms the x64 PE machine value `0x8664` and PE32+ optional header `0x20b`.
The job retains the executable and its hash with the test evidence.
It does not launch the GUI or open an audio/MIDI device.

## Toolchain correction

The first Windows job in [run 42005118345](https://github.com/coder13/Fader/actions/runs/42005118345) failed before compilation.
Its requested VS2022 generator found no matching installation on the actual VS2026 runner.
The superseding push cancelled that run's unfinished macOS job, so it supplies no complete native pass.
Its retained image and Visual Studio metadata identify the mismatch.

The corrected workflow discovers the installed x64 compiler with `vswhere` and initializes its developer environment.
It transfers the compiler paths, include/library paths and toolset/SDK variables to later steps.
Ninja uses that compiler without depending on a particular Visual Studio generator version.
The workflow retains the actual image and compiler configuration for each run.
The [runner image documentation](https://github.com/actions/runner-images) is context, while the observed metadata establishes the tested installation.

## Paths, projects and media

Filesystem boundaries now convert explicitly between UTF-8 and native paths.
This covers library enumeration, identity matching, canonical project directories and asset locations.
Current project writes use relative forward-slash locations, and MSVC compiles C++ source text as UTF-8.
Large test engine instances use owned heap storage, preserving their assertions without increasing the app's stack limit.

The new native test creates files and directories with accents, Greek, Japanese and emoji characters.
It checks preferences, library enumeration, Traktor matching, project identities, metadata, save/reopen and relocation.
Its PCM16 WAV recipe has 8,192 stereo frames at 48 kHz and eight defined integer levels.
Each host requires the same encoded-file SHA and decoded-domain fingerprint.
It compares the complete master render and float WAV export exactly with the integer sample recipe.

Existing suites also cover captured step targets, nonzero anchors, filters, source rates, bounded streaming, cancelled preparation and export failures.
The callback guard checks 20,000 Engine, 20,000 Playback and 20,000 streamed calls without ordinary or aligned allocation/reclamation.
The stress smoke requires source/capture integrity while reporting virtual timing separately.
That two-second CI smoke does not establish sustained Windows latency.

Both native hosts decode the representative VBR MP3, but their sample domains differ.
macOS selects CoreAudio with 352,800 frames at 44.1 kHz.
Windows selects JUCE's MP3 reader with 354,816 frames at 44.1 kHz.
Schema 3 rejects a different saved domain explicitly, rather than shifting source-frame meaning silently.
This build does not establish universal MP3 project interchange between those decoders.

## Shared fixture and remaining gates

Source `b256cc0` adds [one fixed project](../../tests/fixtures/portable-v3.json) for every native host to read and resave.
It has stable project, clip and asset identities and references the independently defined WAV recipe through a Unicode relative path.
It strengthens the check beyond each host reopening its own writer output.
Exact final source `b256cc09b45144facc057e01012e62ced991190a` passes [run 42011432683](https://github.com/coder13/Fader/actions/runs/42011432683).
Linux passes 5/5 and both native hosts pass 7/7.
Downloaded build metadata, JUnit and logs match that source and JUCE pin, with fixtures OFF and zero failed or skipped cases.
The [final aggregate](data/2026-11-05-windows-final.json) also records the common fixture hash and final executable hash.

#41 passes its early software build, Unicode, codec and project-fixture gates at that exact source.
Actual Windows audio, ASIO/WASAPI routing, installation and S4 behavior remain in [#27](https://github.com/coder13/Fader/issues/27).
Native Windows UI startup, visual layout and distribution remain unqualified here.
The separate Mac [sustained run](2026-11-05-software-stress.md) completed on source `566b0ca` and failed deadline, dense-save and memory targets.
Those #40 findings remain open and do not become passes through this portability result.
