"""Filesystem, stale-save and corruption regressions for retained stress evidence."""
import copy
import importlib.util
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

SCRIPT = Path(__file__).resolve().parents[1] / "stress_archive.py"
SPEC = importlib.util.spec_from_file_location("stress_archive", SCRIPT)
ARCHIVE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(ARCHIVE)


class ArchiveTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.fixtures, self.output = self.root / "fixtures", self.root / "evidence"
        self.fixtures.mkdir()
        self.output.mkdir()

    def test_executable_and_fault_bytes_survive_source_replacement(self):
        source = self.root / "probe.exe"
        source.write_bytes(b"original executable")
        source.chmod(0o755)
        record = ARCHIVE.preserve_probe(source, self.output, ARCHIVE.digest(source))
        source.write_bytes(b"new executable")
        self.assertEqual((self.output / record["file"]).read_bytes(), b"original executable")
        self.assertEqual(record["file"], "fader_probe.exe")
        faults = self.fixtures / "fault-results"
        faults.mkdir()
        for name in ARCHIVE.FAULT_FILES:
            (faults / name).write_bytes(name.encode())
        records = ARCHIVE.preserve_faults(self.fixtures, self.output)
        self.assertEqual(len(records), 6)
        for item in records:
            (faults / item["file"]).write_bytes(b"next preflight")
            retained = self.output / "fault-prefixes-raw" / item["file"]
            self.assertEqual(retained.read_bytes(), item["file"].encode())
            self.assertEqual(ARCHIVE.digest(retained), item["sha256"])
        # Only the separate local context file contains source-directory paths.
        self.assertNotIn(str(self.root), json.dumps(records))
        context = json.loads((self.output / "fault-prefixes-raw/context.json").read_text())
        self.assertEqual(context["originalDirectory"], str(faults.resolve()))

    def test_evidence_is_never_overwritten_and_bad_copy_is_not_verified(self):
        source, target = self.root / "source", self.output / "copy"
        source.write_bytes(b"correct")
        ARCHIVE.preserve_file(source, target)
        source.write_bytes(b"replacement")
        with self.assertRaises(FileExistsError):
            ARCHIVE.preserve_file(source, target)
        self.assertEqual(target.read_bytes(), b"correct")
        with self.assertRaises(ARCHIVE.ArchiveError):
            ARCHIVE.preserve_file(source, self.output / "wrong-hash", "0" * 64)
        with patch.object(ARCHIVE.shutil, "copyfileobj", side_effect=lambda a, b, n: b.write(b"short")):
            with self.assertRaises(ARCHIVE.ArchiveError):
                ARCHIVE.preserve_file(source, self.output / "short-copy")

    def test_missing_fault_file_rejects_incomplete_archive(self):
        (self.fixtures / "fault-results").mkdir()
        with self.assertRaises(FileNotFoundError):
            ARCHIVE.preserve_faults(self.fixtures, self.output)
        self.assertFalse((self.output / "fault-prefixes-raw/context.json").exists())

    def test_failed_save_does_not_claim_stale_capture(self):
        stale = self.fixtures / "stress-captured-128.faderset"
        stale.write_bytes(b"prior successful take")
        for flag in [False, None, "true", 1]:
            self.assertIsNone(ARCHIVE.preserve_capture({"capturePersistencePassed": flag}, stale, self.output))
        self.assertEqual(list(self.output.iterdir()), [])
        stale.unlink()
        with self.assertRaises(FileNotFoundError):
            ARCHIVE.preserve_capture({"capturePersistencePassed": True}, stale, self.output)

    def test_original_capture_preserves_exact_bytes_and_local_context(self):
        source = self.fixtures / "stress-captured-128.faderset"
        original = b'{"float": -0.0, "originalSpacing": true}\n'
        source.write_bytes(original)
        record = ARCHIVE.preserve_capture({"capturePersistencePassed": True}, source, self.output)
        source.write_bytes(b"future take")
        self.assertEqual((self.output / record["file"]).read_bytes(), original)
        self.assertNotIn(str(self.root), json.dumps(record))
        context = json.loads((self.output / "original-file-context.json").read_text())
        self.assertEqual(context["originalDirectory"], str(self.fixtures.resolve()))

    def projects(self):
        original, archived = self.output / "raw.faderset", self.output / "rebased.faderset"
        before = {"schemaVersion": 3, "assets": [{"id": "source-1", "path": "é.wav", "rate": 48000}],
                  "clips": [{"id": "clip-1", "length": "576000", "rate": 1.0}],
                  "automation": [{"value": -0.0, "hold": True}], "nested": {"path": "unchanged"}}
        after = copy.deepcopy(before)
        after["assets"][0]["path"] = "../fixtures/é.wav"
        original.write_text(json.dumps(before), encoding="utf-8")
        archived.write_text(json.dumps(after), encoding="utf-8")
        return original, archived, after

    def test_rebase_compares_types_signed_float_bits_and_only_asset_paths(self):
        original, archived, after = self.projects()
        result = ARCHIVE.verify_project_archive(original, archived, self.fixtures)
        self.assertTrue(result["allNonPathFieldsIdentical"])
        self.assertEqual(result["float64ValuesChecked"], 2)
        mutations = [
            (("automation", 0, "value"), 0.0),
            (("clips", 0, "rate"), 1),
            (("clips", 0, "length"), "576001"),
            (("clips", 0, "id"), "new-id"),
            (("automation", 0, "hold"), 1),
            (("assets", 0, "path"), "../fixtures/wrong.wav"),
            (("nested", "path"), "changed"),
        ]
        for path, value in mutations:
            with self.subTest(path=path):
                bad = copy.deepcopy(after)
                parent = bad
                for key in path[:-1]:
                    parent = parent[key]
                parent[path[-1]] = value
                archived.write_text(json.dumps(bad), encoding="utf-8")
                with self.assertRaises(ARCHIVE.ArchiveError):
                    ARCHIVE.verify_project_archive(original, archived, self.fixtures)

    def test_malformed_or_nonfinite_metadata_cannot_verify(self):
        original, archived, after = self.projects()
        invalid = ['{"assets": [], "assets": []}', '[]', '{}',
                   '{"assets": []', json.dumps(after).replace('-0.0', 'NaN'),
                   json.dumps(after).replace('-0.0', '1e999')]
        for text in invalid:
            with self.subTest(text=text):
                archived.write_text(text, encoding="utf-8")
                with self.assertRaises((ARCHIVE.ArchiveError, json.JSONDecodeError)):
                    ARCHIVE.verify_project_archive(original, archived, self.fixtures)


if __name__ == "__main__":
    unittest.main()
