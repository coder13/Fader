# Synthetic VBR regression fixture

`synthetic-vbr.mp3` contains eight seconds of generated tones and decaying pulses, with different left/right tones. It contains no user music. It is redistributable under the repository license.

Source recipe: `scripts/make-streaming-fixtures.py`, the first eight seconds of `rhythm-44100.wav`; 44.1 kHz stereo signed PCM16. Encoding: FFmpeg 7.1, `libmp3lame -q:a 2`, from the isolated `imageio-ffmpeg==0.6.0` fixture tool. The script also writes `short-vbr.mp3`, which is the checked-in fixture under its descriptive name. FFmpeg is not an application/build/CI dependency.

Size: 74,962 bytes. SHA-256: `a61ddba09391fee3873ec6f03a0fec2618f006a67ae71bf6c9957287e419e80c`.

The native media test compares complete sequential streaming and forward/backward/tail windows with an independent sequential decoder. The separate three-minute CBR/VBR fixtures exercise longer index traversal; see the [streaming report](../../docs/experiments/2026-11-04-bounded-streaming.md).
