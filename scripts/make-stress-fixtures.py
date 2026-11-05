#!/usr/bin/env python3
"""Create continuous synthetic audio and a 1,000-file library for DJ-040.

No encoder, network, third-party package or user music is required.
The two long WAVs occupy real disk space, about 1.38 GB for the default duration.
"""
import argparse
import array
import hashlib
import json
import math
from pathlib import Path
import shutil
import sys
import wave

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("directory", type=Path, help="New synthetic fixture directory")
parser.add_argument("--seconds", type=int, default=3604)
args = parser.parse_args()
if not 4 <= args.seconds <= 3604:
    parser.error("--seconds must be 4..3604")
args.directory.mkdir(parents=True, exist_ok=False)
rate = 48000
metadata = {"sampleRate": rate, "sourceSeconds": args.seconds, "sources": []}


def write(path, data, seconds):
    with wave.open(str(path), "wb") as output:
        output.setnchannels(2)
        output.setsampwidth(2)
        output.setframerate(rate)
        for _ in range(seconds):
            output.writeframesraw(data)


for index in range(2):
    motif = array.array("h")
    for frame in range(rate):
        t = frame / rate
        envelope = math.exp(-(t % .5) * 16)
        for channel in range(2):
            phase = channel * .3 + index * .7
            value = (.22 * math.sin(2 * math.pi * (70 + index * 10) * t) * envelope
                     + .12 * math.sin(2 * math.pi * (110 + index * 10) * t + phase)
                     + .07 * math.cos(2 * math.pi * (440 + channel * 110) * t + phase)
                     + .02 * math.sin(2 * math.pi * 7001 * t) * envelope)
            motif.append(round(value * 32767))
    if sys.byteorder != "little":
        motif.byteswap()
    data = motif.tobytes()
    path = args.directory / ("AB"[index] + ".wav")
    write(path, data, args.seconds)
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for chunk in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(chunk)
    metadata["sources"].append({"file": path.name, "sha256": digest.hexdigest(),
                                "fileBytes": path.stat().st_size})
    if index == 0:
        write(args.directory / "analysis.wav", data, 1)
        tiny = args.directory / "library-template.wav"
        with wave.open(str(tiny), "wb") as output:
            output.setnchannels(2)
            output.setsampwidth(2)
            output.setframerate(rate)
            output.writeframes(data[:rate // 25 * 4])
        library = args.directory / "library"
        library.mkdir()
        for track in range(1000):
            shutil.copyfile(tiny, library / f"Synthetic {track:04d}.wav")
        tiny.unlink()
(args.directory / "fixture-info.json").write_text(json.dumps(metadata, indent=2) + "\n")
print(json.dumps(metadata, indent=2))
