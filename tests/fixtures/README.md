# Synthetic VBR regression fixture

## Common portable project

`portable-v3.json` is a fixed schema 3 project with explicit project, clip and asset identities.
It uses the independent integer PCM16 recipe in `PortabilityTests.cpp`, with 8,192 stereo frames at 48 kHz.
Its eight sample levels are −32768, −16384, −8192, 0, 8192, 16384, 24576 and 32767.
The left channel selects `frame % 8`, and the right selects `(frame * 3 + 1) % 8`.
The source has a 44-byte PCM WAV header and no optional metadata chunks.
The test creates its media under the relative Unicode path in the JSON.

The encoded WAV SHA-256 is `9274aec9d580f2434ac82f1f602a052a4c48c6f5a09165fbb38b5a47ce841977`.
The decoded-domain hash is `3f3cd41159de8c77db90c35d1fee239429edac6b0b75458878da3ac277828c96` under the version 3 block-chain rule.
The asset ID uses the hash/domain/rate/frame/channel rule in [the format contract](../../docs/project-format-v3.md).
Its decoder label records independent provenance and does not select a platform decoder.
Every host reads this same project, compares exact master PCM with the integer recipe, and checks IDs after resave.
This fixture contains no user music or private paths.

## VBR source

`synthetic-vbr.mp3` contains eight seconds of generated tones and decaying pulses, with different left/right tones. It contains no user music. It is redistributable under the repository license.

Source recipe: `scripts/make-streaming-fixtures.py`, the first eight seconds of `rhythm-44100.wav`; 44.1 kHz stereo signed PCM16. Encoding: FFmpeg 7.1, `libmp3lame -q:a 2`, from the isolated `imageio-ffmpeg==0.6.0` fixture tool. The script also writes `short-vbr.mp3`, which is the checked-in fixture under its descriptive name. FFmpeg is not an application/build/CI dependency.

Size: 74,962 bytes. SHA-256: `a61ddba09391fee3873ec6f03a0fec2618f006a67ae71bf6c9957287e419e80c`.

The native media test compares complete sequential streaming and forward/backward/tail windows with an independent sequential decoder. The separate three-minute CBR/VBR fixtures exercise longer index traversal; see the [streaming report](../../docs/experiments/2026-11-04-bounded-streaming.md).
