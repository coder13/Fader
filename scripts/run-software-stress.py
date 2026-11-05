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
from stress_archive import (ArchiveError, digest, preserve_capture, preserve_faults,
                            preserve_probe, verify_project_archive)

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
parser.add_argument("--cpu-diagnostics", action="store_true", help="Instrument thread CPU time; cannot qualify hour performance")
args = parser.parse_args()
if not 1 <= args.seconds <= 3600:
    parser.error("--seconds must be 1..3600")


def git(*command):
    return subprocess.check_output(["git", "-C", str(args.source_root), *command])


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
    "cpuDiagnostics": args.cpu_diagnostics,
    "scope": "Software sink and offscreen production painter. No audio/MIDI device or native UI interaction.",
}
(args.output / "metadata.json").write_text(json.dumps(metadata, indent=2) + "\n")


def run(label, command):
    metadata[f"{label}StartedUtc"] = datetime.now(timezone.utc).isoformat()
    (args.output / "metadata.json").write_text(json.dumps(metadata, indent=2) + "\n")
    with (args.output / f"{label}.json").open("w") as output, (args.output / f"{label}.log").open("w") as log:
        print(f"Starting {label}", file=sys.stderr, flush=True)
        result = subprocess.run([str(preserved_probe), *map(str, command)], stdout=output, stderr=log)
    metadata[f"{label}ExitCode"] = result.returncode
    metadata[f"{label}FinishedUtc"] = datetime.now(timezone.utc).isoformat()
    try:
        data = json.loads((args.output / f"{label}.json").read_text())
    except json.JSONDecodeError:
        metadata["runError"] = f"{label} exited {result.returncode} without a complete JSON report"
        raise SystemExit(metadata["runError"] + ". Read the retained log.")
    return data


summary = None
try:
    # Execute the retained copy, so later build replacement cannot destroy the
    # exact executable evidence. Copy/hash work precedes the timed process.
    metadata["preservedProbe"] = preserve_probe(args.probe, args.output, metadata["probeSha256"])
    preserved_probe = (args.output / metadata["preservedProbe"]["file"]).resolve()
    faults = run("faults", ["--stress-faults", args.fixtures])
    if metadata["faultsExitCode"]:
        sys.exit("Fault qualification failed. Read faults.log.")
    metadata["preservedFaultFiles"] = preserve_faults(args.fixtures, args.output)
    command = ["--stress-software", args.fixtures, args.seconds, args.block]
    if args.scheduler == "realtime":
        command.append("--realtime")
    if args.cpu_diagnostics:
        command.append("--cpu-diagnostics")
    timed = run("timed", command)
    metadata["timedPhaseFinishedUtc"] = datetime.now(timezone.utc).isoformat()
    captured = args.fixtures / f"stress-captured-{args.block}.faderset"
    raw_capture = preserve_capture(timed, captured, args.output)
    if raw_capture is not None:
        metadata["preservedOriginalCapture"] = raw_capture
        metadata["capturedProjectSha256"] = raw_capture["sha256"]
        metadata["capturedProjectBytes"] = raw_capture["bytes"]
        # Archive through the production loader/writer, so relative media paths
        # remain valid at the evidence destination. Fixtures remain local.
        resave = subprocess.run([str(preserved_probe), "--resave-project", str(captured),
                                 str(args.output / "captured.faderset")], capture_output=True, text=True)
        metadata["archiveProjectExitCode"] = resave.returncode
        (args.output / "archive-project.log").write_text(resave.stdout + resave.stderr)
        if resave.returncode == 0:
            metadata["archivedProjectSha256"] = digest(args.output / "captured.faderset")
            metadata["archiveVerification"] = verify_project_archive(
                args.output / raw_capture["file"], args.output / "captured.faderset", captured.parent)
    summary = {"metadata": metadata, "faults": faults, "timed": timed}
    print(json.dumps({key: timed.get(key) for key in (
        "requestedSeconds", "blockFrames", "softwareIntegrityPassed", "hourTargetPassed",
        "callbackExecution", "virtualDeadlineMisses", "sourceStarvationFrames",
        "capturedInputs", "peakResidentBytes")}, indent=2))
    # Archive failure is an evidence failure even if timing also failed.
    sys.exit(metadata["timedExitCode"] if "archiveVerification" in metadata else 1)
except (ArchiveError, OSError, UnicodeError, json.JSONDecodeError) as error:
    metadata["evidenceArchiveError"] = {"type": type(error).__name__, "details": "archive-error.log"}
    (args.output / "archive-error.log").write_text(str(error) + "\n", encoding="utf-8")
    sys.exit("Evidence preservation failed. Read metadata.json; retained files are not a complete archive.")
finally:
    metadata["finishedUtc"] = datetime.now(timezone.utc).isoformat()
    (args.output / "metadata.json").write_text(json.dumps(metadata, indent=2) + "\n")
    if summary is not None:
        (args.output / "aggregate.json").write_text(json.dumps(summary, indent=2) + "\n")
