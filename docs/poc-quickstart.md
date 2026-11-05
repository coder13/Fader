# Native prototype quickstart

This is the first software experiment, not a completed Mac/S4 qualification. It runs without a controller.

## Build on macOS

Install the Xcode Command Line Tools and CMake 3.22 or newer. A first configure downloads the pinned JUCE source; subsequent builds use the cached checkout.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
open build/Fader_artefacts/Release/Fader.app
```

For an existing JUCE checkout, add `-DFADER_JUCE_PATH=/path/to/JUCE` to configure. The pinned revision is JUCE 9.0.5, `d60b7a78030661574a615b586d9325ba7a7fa7dd`. The application currently builds as the host architecture; Intel, Windows, and Linux builds are unqualified.

The pure C++ audio-core tests need no JUCE or network:

```sh
cmake -S . -B build-core -DFADER_BUILD_APP=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build-core
ctest --test-dir build-core --output-on-failure
```

## Try the workflow

1. Generate quiet synthetic tracks with `python3 tools/make_demo.py`. Open `build/demo/demo.faderset` with **Open set**. These contain no user music.
2. Alternatively, use **Load A/B**, double-click in the folder browser, or drag files from Finder onto the two waveform lanes. The folder browser uses the selected automation lane to choose A or B; crossfader selection targets B.
3. Drag a waveform to move its clip. Set start, length, or static speed in the text boxes and press Return. Speed changes pitch; there is no key lock.
4. Choose A gain, B gain, or crossfader below the timeline. Click to add automation points, drag to adjust, and right-click to remove. Points cannot cross their neighbors while dragging. Step targets are smoothed over 5 ms; Linear interpolates between points before smoothing.
5. Press **Play**. Audio requests 48 kHz with 128-frame buffers, trying 256 if the device refuses 128. This setting is not a latency measurement. **Audio settings** selects the output device. Master uses the first stereo pair; cue needs a second stereo pair on that same device. Physical routing is not yet verified.
6. To record fader moves, keep the lanes in **Step targets**, enter retake start/end seconds (end 0 means the set end), and press **Record take**. Move the A/B gain or crossfader sliders. Press **Finish take** to commit. Only lanes touched in the interval are replaced. **Undo** restores the previous edit/take.
7. **Save set** writes a `.faderset` JSON project with relative source references, source hashes, and automation. **Open set** verifies those hashes and restores the curves. Keep the project and its source files together when moving it.
8. **Export WAV** renders the saved timeline to stereo 24-bit PCM at 48 kHz using the same engine. An over-range mix fails with a message to lower gains instead of silently clipping the export. Output replaces its destination only on success.

Opening/replacing source media clears the edit undo history to avoid retaining large decoded files. Other edits keep up to 24 undo states. This prototype has no autosave or unsaved-changes prompt: save before closing or opening another set.

## Command-line probes

```sh
build/fader_probe_artefacts/Release/fader_probe --devices
build/fader_probe_artefacts/Release/fader_probe --audio-smoke
build/fader_probe_artefacts/Release/fader_probe --inspect /path/to/audio.mp3
build/fader_probe_artefacts/Release/fader_probe --render build/demo/demo.faderset build/demo/mix.wav
```

`--audio-smoke` opens the default output for two seconds with silent two-source rendering and reports actual rate/buffer size, callback timing, and the device's output-latency estimate. An xrun value of -1 means the driver does not report it. This is not an acoustic latency or reliability test.

`--inspect` decodes the whole file within the memory limit and prints aggregate format information. `--devices` reads audio-output names and matching S4 HID interface properties; it does not open or control the S4.

## Deliberate limits

- Two clips, at most two hours of project time. A source must fit within 33,554,432 decoded stereo frames (256 MiB per deck; about 12.7 minutes at 44.1 kHz). Sources are decoded on a worker before playback; streaming long tracks is a later step. Importing replacement media can temporarily retain old and new buffers.
- WAV and MP3 are the initial compatibility targets. Other JUCE-provided decoders may work, but are unqualified. Mono is duplicated to stereo; multichannel sources and incomplete downloads are rejected.
- Linear interpolation is the prototype resampler and aliases at high playback rates. It is not a production-quality DJ resampler. Exact seeking reconstructs automation history and can delay the UI on long, dense sets.
- Gain/crossfader capture only. No S4 controls/feedback, generic MIDI routing, raw-audio recording, editable beat grid, transport-action capture, plug-ins, MP3 export, or source relinking dialog yet.
- Retakes restore the original target at punch-out through the normal smoother and preserve all later stored points. Live/replay PCM matches; baseline PCM outside the selection may differ during smoothing. The stricter finite return-region contract in the design remains open.
- Cue switches are monitoring state, not saved arrangement automation. No capture during device reconfiguration; playback stops on a device stop notification.
- No long-run deadline/underrun qualification, acoustic latency measurement, hardware cue-routing test, or cross-platform support claim yet. Read [the complete POC gates](poc-plan.md) and the experiment report before treating this as performance software.

The experimental source uses AGPL-3.0-or-later; see the root LICENSE. The long-term stack/reuse decision remains provisional.
