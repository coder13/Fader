#!/usr/bin/env python3
"""Generate redistributable audio and a set for --incomplete-take-fixture."""
import argparse
import hashlib
import json
import math
from pathlib import Path
import struct
import wave

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("directory", type=Path, help="New directory for generated fixture files")
root = parser.parse_args().directory
root.mkdir(parents=True, exist_ok=False)
audio = root / "Synthetic rhythm.wav"
with wave.open(str(audio), "wb") as writer:
    writer.setnchannels(2)
    writer.setsampwidth(2)
    writer.setframerate(48000)
    for second in range(30):
        samples = bytearray()
        for frame in range(48000):
            time = second + frame / 48000
            value = (0.3 * math.sin(2 * math.pi * 110 * time) * math.exp(-(time % 0.5) * 10)
                     + 0.09 * math.sin(2 * math.pi * 440 * time))
            sample = int(value * 32767)
            samples.extend(struct.pack("<hh", sample, sample))
        writer.writeframes(samples)


def curve(value, points=None):
    return {"initial": value, "interpolation": "linear", "points": [
        {"frame": str(int(seconds * 48000)), "value": target, "hold": False}
        for seconds, target in (points or [(0, value)])]}


digest = hashlib.sha256(audio.read_bytes()).hexdigest()
clips = []
for index in range(2):
    clips.append({
        "path": audio.name, "sha256": digest, "start": str(index * 12 * 48000),
        "sourceOffset": "0", "length": str(30 * 48000), "rate": 1, "bpm": 120,
        "firstBeatSeconds": 0, "row": index, "name": f"Synthetic rhythm {'AB'[index]}",
        "artist": "Fader test fixture", "key": "", "hasGrid": True,
        "automation": [curve(0.7, [(0, 0.7), (18, 0.7), (29, 0.1)]) if index == 0
                       else curve(0.1, [(0, 0.1), (10, 0.7)]), curve(0), curve(1)]})
project = root / "Synthetic timing.faderset"
project.write_text(json.dumps({
    "schemaVersion": 2, "engineVersion": 2, "sampleRate": 48000, "clips": clips,
    "automation": [curve(0.7), curve(0.7), curve(0)]}, indent=2) + "\n")
print(project)
