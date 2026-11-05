"""Preserve exact stress evidence and check a same-host project-path rebase.

These checks do not qualify playback timing or prove media decoding/parity.
Private path context lives in separate files, outside the aggregate metadata.
"""
import hashlib
import json
import math
import shutil
import struct


class ArchiveError(ValueError):
    pass


FAULT_FILES = (
    "input-overflow-incomplete-prefix.faderset",
    "capture-overflow-incomplete-prefix.faderset",
    "device-loss-incomplete-prefix.faderset",
    "source-gap-valid-prefix.faderset",
    "missing-media-preserved.faderset",
    "missing-media.wav",
)


def digest(path):
    result = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            result.update(block)
    return result.hexdigest()


def preserve_file(source, destination, expected=None):
    """Never replace an existing evidence file; reject a changed/short copy."""
    expected = digest(source) if expected is None else expected
    with source.open("rb") as incoming, destination.open("xb") as outgoing:
        shutil.copyfileobj(incoming, outgoing, 1024 * 1024)
    shutil.copymode(source, destination)
    actual = digest(destination)
    if actual != expected:
        raise ArchiveError("Preserved evidence differs from its expected SHA-256")
    return {"file": destination.name, "bytes": destination.stat().st_size, "sha256": actual}


def write_context(path, data):
    with path.open("x", encoding="utf-8") as output:
        json.dump(data, output, indent=2)
        output.write("\n")


def preserve_probe(source, output, expected):
    name = "fader_probe.exe" if source.suffix.lower() == ".exe" else "fader_probe"
    return preserve_file(source, output / name, expected)


def preserve_faults(fixtures, output):
    root = output / "fault-prefixes-raw"
    root.mkdir()
    files = [preserve_file(fixtures / "fault-results" / name, root / name) for name in FAULT_FILES]
    write_context(root / "context.json", {
        "scope": "Original fault bytes, not rebased projects. This local file contains private path context.",
        "originalDirectory": str((fixtures / "fault-results").resolve()),
        "files": files,
    })
    return files


def preserve_capture(report, source, output):
    # A prior successful run may have left this path behind. Never claim that
    # older file is the current capture when the current save did not succeed.
    if report.get("capturePersistencePassed") is not True:
        return None
    record = preserve_file(source, output / "captured-original-raw.faderset")
    write_context(output / "original-file-context.json", {
        "scope": "Original capture bytes. This local file contains private path context; media is not copied.",
        "originalDirectory": str(source.parent.resolve()),
        "originalFilename": source.name,
        "preserved": record,
    })
    return record


def _unique_object(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ArchiveError("Duplicate JSON key in project evidence")
        result[key] = value
    return result


def _invalid_constant(value):
    raise ArchiveError("Non-finite JSON value in project evidence")


def verify_project_archive(original, archived, original_directory):
    """Only asset paths may change; both must name the same existing file.

    Compare parsed JSON types and float64 bits, including signed zero. This is
    same-host metadata verification, not a source-content or playback check.
    """
    def read(path):
        return json.loads(path.read_text(encoding="utf-8-sig"),
                          object_pairs_hook=_unique_object, parse_constant=_invalid_constant)

    before, after = read(original), read(archived)
    if not isinstance(before, dict) or not isinstance(after, dict):
        raise ArchiveError("Project evidence must be a JSON object")
    left_assets, right_assets = before.get("assets"), after.get("assets")
    if not isinstance(left_assets, list) or not isinstance(right_assets, list):
        raise ArchiveError("Project evidence has no asset list")
    if len(left_assets) != len(right_assets):
        raise ArchiveError("Archive changed the asset count")
    for left, right in zip(left_assets, right_assets):
        if not isinstance(left, dict) or not isinstance(right, dict):
            raise ArchiveError("Project asset must be a JSON object")
        left_path, right_path = left.get("path"), right.get("path")
        if not isinstance(left_path, str) or not left_path or not isinstance(right_path, str) or not right_path:
            raise ArchiveError("Project asset has no valid path")
        # Path equality can case-fold on Windows even inside a case-sensitive
        # directory. File identity also accepts valid hard-link aliases.
        try:
            same_source = (original_directory / left_path).samefile(archived.parent / right_path)
        except OSError as error:
            raise ArchiveError("Source file unavailable for archive path verification") from error
        if not same_source:
            raise ArchiveError("Archive asset path names a different source file")
        del left["path"], right["path"]

    floats = 0

    def compare(left, right):
        nonlocal floats
        if type(left) is not type(right):
            raise ArchiveError("Archive changed a non-path JSON type")
        if isinstance(left, dict):
            if left.keys() != right.keys():
                raise ArchiveError("Archive changed non-path object fields")
            for key in left:
                compare(left[key], right[key])
        elif isinstance(left, list):
            if len(left) != len(right):
                raise ArchiveError("Archive changed a non-path array length")
            for a, b in zip(left, right):
                compare(a, b)
        elif isinstance(left, float):
            floats += 1
            if not math.isfinite(left) or not math.isfinite(right):
                raise ArchiveError("Non-finite JSON value in project evidence")
            if struct.pack(">d", left) != struct.pack(">d", right):
                raise ArchiveError("Archive changed non-path floating-point bits")
        elif left != right:
            raise ArchiveError("Archive changed a non-path JSON value")

    compare(before, after)
    return {
        "scope": "Same-host file identity and parsed project fields; not media-content, sound or timing qualification.",
        "allNonPathFieldsIdentical": True,
        "assetPathsResolveToSameSources": True,
        "assetsChecked": len(left_assets),
        "float64ValuesChecked": floats,
        "originalSha256": digest(original),
        "archivedSha256": digest(archived),
        "originalBytes": original.stat().st_size,
        "archivedBytes": archived.stat().st_size,
    }
