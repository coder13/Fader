"""Expanded timelines are diagnostics, not a shortcut to performance qualification."""
from pathlib import Path
import subprocess
import sys
import unittest

SCRIPT = Path(__file__).resolve().parents[1] / "run-software-stress.py"


class DriverOptionsTests(unittest.TestCase):
    def test_invalid_expanded_timeline_rejected_before_source_or_output_access(self):
        base = [sys.executable, str(SCRIPT)]
        for option in ["--source-root", "--build-source", "--probe", "--build-info", "--fixtures", "--output"]:
            base += [option, "does-not-exist"]
        cases = [
            ["--seconds", "2", "--timeline-seconds", "4"],
            ["--seconds", "2", "--cpu-diagnostics", "--timeline-seconds", "1"],
            ["--seconds", "2", "--cpu-diagnostics", "--timeline-seconds", "3601"],
            ["--seconds", "2", "--cpu-diagnostics", "--timeline-seconds", "0"],
        ]
        for arguments in cases:
            with self.subTest(arguments=arguments):
                result = subprocess.run(base + arguments, capture_output=True, text=True)
                self.assertEqual(result.returncode, 2)
                self.assertIn("expanded timeline requires", result.stderr)
                self.assertEqual(result.stdout, "")


if __name__ == "__main__":
    unittest.main()
