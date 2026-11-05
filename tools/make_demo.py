#!/usr/bin/env python3
"""Create two quiet, synthetic tracks and a portable Fader demonstration set."""
import hashlib
import json
import math
from pathlib import Path
import struct
import sys
import wave


def main():
    directory = Path(sys.argv[1] if len(sys.argv) > 1 else "build/demo")
    directory.mkdir(parents=True, exist_ok=True)
    paths = [directory / name for name in ("pulse-a.wav", "pulse-b.wav", "demo.faderset")]
    if any(path.exists() for path in paths):
        raise SystemExit("Demo files already exist; choose a new output folder.")
    clips = []
    for deck, sample_rate in enumerate((44100, 48000)):
        frames = bytearray()
        for index in range(sample_rate * 12):
            seconds = index / sample_rate
            beat_age = seconds % 0.5
            kick = math.sin(2 * math.pi * (65 + 30 * math.exp(-beat_age * 40)) * beat_age)
            tone = math.sin(2 * math.pi * (220 if deck == 0 else 330) * seconds)
            fade = min(1, seconds * 10, (12 - seconds) * 10)
            sample = int(32767 * fade * (0.16 * kick * math.exp(-beat_age * 16) + 0.04 * tone))
            frames.extend(struct.pack("<hh", sample, sample))
        with wave.open(str(paths[deck]), "wb") as audio:
            audio.setnchannels(2)
            audio.setsampwidth(2)
            audio.setframerate(sample_rate)
            audio.writeframes(frames)
        clips.append({"path": paths[deck].name,
                      "sha256": hashlib.sha256(paths[deck].read_bytes()).hexdigest(),
                      "start": str(deck * 6 * 48000), "sourceOffset": "0", "length": str(12 * 48000),
                      "rate": 1.0, "bpm": 120.0, "firstBeatSeconds": 0.0, "hasGrid": True,
                      "row": deck, "name": "Pulse A" if deck == 0 else "Pulse B", "artist": "Fader demo", "key": "",
                      "automation": [
                          {"initial": 0.7 if deck == 0 else 0.0, "interpolation": "linear", "points":
                           [{"frame": "0", "value": 0.7}, {"frame": str(6*48000), "value": 0.7}, {"frame": str(12*48000), "value": 0.0}]
                           if deck == 0 else [{"frame": "0", "value": 0.0}, {"frame": str(6*48000), "value": 0.7}]},
                          {"initial": 0.0, "interpolation": "linear", "points": [{"frame": "0", "value": 0.0}]},
                          {"initial": 1.0, "interpolation": "linear", "points": [{"frame": "0", "value": 1.0}]}]})
    automation = [{"initial": 0.7, "interpolation": "step", "points": []} for _ in range(2)]
    automation.append({"initial": -1.0, "interpolation": "step", "points": [
        {"frame": str(round((6 + step * 0.15) * 48000)), "value": -1 + step * 0.05}
        for step in range(41)]})
    paths[2].write_text(json.dumps({"schemaVersion": 2, "engineVersion": 2, "sampleRate": 48000,
                                  "clips": clips, "automation": automation}, indent=2) + "\n")
    print(paths[2].resolve())


if __name__ == "__main__":
    main()
