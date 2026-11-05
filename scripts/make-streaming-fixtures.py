#!/usr/bin/env python3
"""Generate synthetic sparse long WAVs and optional music-like WAV/MP3 fixtures.

The encoder is an explicitly supplied FFmpeg executable, used only by this
fixture script. Fader does not acquire an FFmpeg runtime dependency.
"""
import argparse
import array
import math
from pathlib import Path
import struct
import subprocess
import sys
import wave

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("directory", type=Path, help="New output directory")
parser.add_argument("--mp3-encoder", type=Path)
parser.add_argument("--long-mp3-minutes", type=int, default=0,
                    help="Also encode long seek fixtures from the three-minute rhythms")
args = parser.parse_args()
if args.long_mp3_minutes and (not args.mp3_encoder or not 1 <= args.long_mp3_minutes <= 120):
    parser.error("--long-mp3-minutes requires --mp3-encoder and a value from 1 to 120")
args.directory.mkdir(parents=True, exist_ok=False)


def samples(rate, seconds, phase=0):
    result = array.array("h")
    for frame in range(int(rate * seconds)):
        t = frame / rate
        pulse = math.exp(-(t % 0.5) * 16)
        for channel in range(2):
            value = (0.27 * math.sin(2 * math.pi * 80 * t) * pulse
                     + 0.08 * math.sin(2 * math.pi * (311 + channel * 107) * t + phase)
                     + 0.02 * math.sin(2 * math.pi * 7001 * t) * pulse)
            result.append(round(value * 32767))
    if sys.byteorder != "little":
        result.byteswap()
    return result.tobytes()


# Each source is 2 h 15 m, 1.56 GB logically, but only short bursts occupy
# physical blocks. RIFF length is truthful; the remaining PCM is digital silence.
rate, seconds = 48000, 2 * 3600 + 15 * 60
data_bytes = rate * seconds * 4
for index in range(2):
    path = args.directory / f"long-{'AB'[index]}.wav"
    with path.open("wb") as output:
        output.write(struct.pack("<4sI4s4sIHHIIHH4sI", b"RIFF", 36 + data_bytes,
                                 b"WAVE", b"fmt ", 16, 1, 2, rate, rate * 4,
                                 4, 16, b"data", data_bytes))
        output.truncate(44 + data_bytes)
        burst = samples(rate, 0.1, index * 0.3)
        for second in range(0, seconds, 60):
            output.seek(44 + second * rate * 4)
            output.write(burst)
    print(path)

if args.mp3_encoder:
    for rate in (44100, 48000):
        path = args.directory / f"rhythm-{rate}.wav"
        with wave.open(str(path), "wb") as output:
            output.setnchannels(2)
            output.setsampwidth(2)
            output.setframerate(rate)
            for segment in range(6):
                second = samples(rate, 1, segment * 0.23)
                for _ in range(30):
                    output.writeframes(second)
        for mode, encoding in (("cbr", ["-b:a", "320k"]), ("vbr", ["-q:a", "2"])):
            destination = path.with_name(f"rhythm-{rate}-{mode}.mp3")
            subprocess.run([str(args.mp3_encoder), "-nostdin", "-v", "error",
                            "-i", str(path), "-c:a", "libmp3lame", *encoding,
                            str(destination)], check=True)
            print(destination)
    subprocess.run([str(args.mp3_encoder), "-nostdin", "-v", "error", "-i",
                    str(args.directory / "rhythm-44100.wav"), "-t", "8",
                    "-c:a", "libmp3lame", "-q:a", "2",
                    str(args.directory / "short-vbr.mp3")], check=True)

    if args.long_mp3_minutes:
        for rate, mode, encoding in ((48000, "cbr", ["-b:a", "320k"]),
                                      (44100, "vbr", ["-q:a", "2"])):
            source = args.directory / f"rhythm-{rate}.wav"
            destination = args.directory / f"long-{mode}-{rate}.mp3"
            subprocess.run([str(args.mp3_encoder), "-nostdin", "-v", "error",
                            "-stream_loop", str(math.ceil(args.long_mp3_minutes / 3) - 1),
                            "-i", str(source), "-t", str(args.long_mp3_minutes * 60),
                            "-c:a", "libmp3lame", *encoding, str(destination)], check=True)
            print(destination)
