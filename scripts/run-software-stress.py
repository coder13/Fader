#!/usr/bin/env python3
"""Run DJ-040 software faults and a real-clock workload with exact-source evidence."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import platform
import subprocess
import sys

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--source-root", type=Path, required=True, help="Clean committed source checkout")
parser.add_argument("--build-source", type=Path, required=True, help="Source folder used for this build")
parser.add_argument("--probe", type=Path, required=True)
parser.add_argument("--build-info", type=Path, required=True)
parser.add_argument("--fixtures", type=Path, required=True)
parser.add_argument("--output", type=Path, required=True, help="New evidence directory")
parser.add_argument("--seconds", type=int, default=3600)
parser.add_argument("--block", type=int, choices=(128, 256), default=128)
parser.add_argument("--scheduler", choices=("normal", "realtime"), default="normal")
args = parser.parse_args()
if not 1 <= args.seconds <= 3600:
    parser.error("--seconds must be 1..3600")


def git(*command):
    return subprocess.check_output(["git", "-C", str(args.source_root), *command])


def digest(path):
    result = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            result.update(block)
    return result.hexdigest()


if git("status", "--porcelain").strip():
    sys.exit("Source checkout is dirty. Commit the measured source before this run.")
revision = git("rev-parse", "HEAD").decode().strip()
manifest = hashlib.sha256()
for name in git("ls-files", "-z").split(b"\0"):
    if not name:
        continue
    relative = name.decode()
    source = args.source_root / relative
    built = args.build_source / relative
    if not source.is_file() or not built.is_file() or digest(source) != digest(built):
        sys.exit(f"Build source differs from committed source: {relative}")
    manifest.update(name + b"\0" + digest(source).encode() + b"\n")
args.output.mkdir(parents=True, exist_ok=False)
metadata = {
    "qualifiedSourceRevision": revision,
    "sourceTreeSha256": manifest.hexdigest(),
    "sourceVerification": "Every tracked file byte-compared between the clean checkout and build source before execution",
    "probeSha256": digest(args.probe),
    "buildInfo": json.loads(args.build_info.read_text()),
    "os": platform.platform(),
    "machine": platform.machine(),
    "python": platform.python_version(),
    "startedUtc": datetime.now(timezone.utc).isoformat(),
    "fixtures": json.loads((args.fixtures / "fixture-info.json").read_text()),
    "seconds": args.seconds,
    "blockFrames": args.block,
    "scheduler": args.scheduler,
    "scope": "Software sink and offscreen production painter. No audio/MIDI device or native UI interaction.",
}
(args.output / "metadata.json").write_text(json.dumps(metadata, indent=2) + "\n")


def run(label, command):
    with (args.output / f"{label}.json").open("w") as output, (args.output / f"{label}.log").open("w") as log:
        print(f"Starting {label}", file=sys.stderr, flush=True)
        result = subprocess.run([str(args.probe), *map(str, command)], stdout=output, stderr=log)
    data = json.loads((args.output / f"{label}.json").read_text())
    metadata[f"{label}ExitCode"] = result.returncode
    return data


try:
    faults = run("faults", ["--stress-faults", args.fixtures])
    if metadata["faultsExitCode"]:
        sys.exit("Fault qualification failed. Read faults.log.")
    command = ["--stress-software", args.fixtures, args.seconds, args.block]
    if args.scheduler == "realtime":
        command.append("--realtime")
    timed = run("timed", command)
    metadata["finishedUtc"] = datetime.now(timezone.utc).isoformat()
    captured = args.fixtures / f"stress-captured-{args.block}.faderset"
    if captured.is_file():
        metadata["capturedProjectSha256"] = digest(captured)
        metadata["capturedProjectBytes"] = captured.stat().st_size
        # Archive through the production loader/writer, so relative media paths
        # remain valid at the evidence destination. Fixtures remain local.
        resave = subprocess.run([str(args.probe), "--resave-project", str(captured),
                                 str(args.output / "captured.faderset")], capture_output=True, text=True)
        metadata["archiveProjectExitCode"] = resave.returncode
        (args.output / "archive-project.log").write_text(resave.stdout + resave.stderr)
        if resave.returncode == 0:
            metadata["archivedProjectSha256"] = digest(args.output / "captured.faderset")
    summary = {"metadata": metadata, "faults": faults, "timed": timed}
    (args.output / "aggregate.json").write_text(json.dumps(summary, indent=2) + "\n")
    print(json.dumps({key: timed.get(key) for key in (
        "requestedSeconds", "blockFrames", "softwareIntegrityPassed", "hourTargetPassed",
        "callbackExecution", "virtualDeadlineMisses", "sourceStarvationFrames",
        "capturedInputs", "peakResidentBytes")}, indent=2))
    sys.exit(metadata["timedExitCode"] or metadata.get("archiveProjectExitCode", 1))
finally:
    (args.output / "metadata.json").write_text(json.dumps(metadata, indent=2) + "\n")
