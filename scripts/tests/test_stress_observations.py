"""Corruption and accounting checks for the independent observation reader."""
import copy
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

SCRIPT = Path(__file__).resolve().parents[1] / 'verify-stress-observations.py'
SPEC = importlib.util.spec_from_file_location('stress_observations', SCRIPT)
CHECKER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(CHECKER)


def cpu_observation(start, returned, delta):
    return {
        'valid': True,
        'first': {'valid': True, 'error': 0, 'beforeNanosecondsFromStart': start+10,
                  'afterNanosecondsFromStart': start+100, 'threadCpuNanoseconds': 1000},
        'last': {'valid': True, 'error': 0, 'beforeNanosecondsFromStart': returned+10,
                 'afterNanosecondsFromStart': returned+100, 'threadCpuNanoseconds': 1000+delta},
        'threadCpuDeltaNanoseconds': delta,
        'minimumWallWindowNanoseconds': returned+10-(start+100),
        'maximumWallWindowNanoseconds': returned+100-(start+10),
    }


def valid_report():
    # Deliberately small metadata fixture, not a claimed measurement. Only
    # serialized arithmetic is knowable; the reader cannot prove unseen work.
    target, wake, returned = 82_666_667, 82_666_700, 84_066_700
    host = {
        'projectFrame': 3968, 'targetNanosecondsFromStart': target,
        'wakeNanosecondsFromStart': wake, 'processReturnedNanosecondsFromStart': returned,
        'sinkFinishedNanosecondsFromStart': returned+1000,
        'deadlineNanosecondsFromStart': 85_333_333,
        'wakeLatenessNanoseconds': wake-target, 'processCallNanoseconds': returned-wake,
        'sinkNanoseconds': 1000, 'completionLatenessNanoseconds': 0,
        'internalCallbackNanoseconds': 1_399_000, 'callbackMetricMatched': True,
        'projectFrameModulo4096': 3968, 'cpuObservation': cpu_observation(wake, returned, 1_000_000),
    }
    paint = {
        'paintIndex': 0, 'scheduledNanosecondsFromStart': 0, 'beginNanosecondsFromStart': 10,
        'endNanosecondsFromStart': 20_000_010, 'updateAndPaintNanoseconds': 20_000_000,
        'startLatenessNanoseconds': 10, 'parameter': 0, 'movedClipFrames': 0,
        'viewportX': 0, 'playheadFrame': 128, 'cpuObservation': cpu_observation(10, 20_000_010, 5_000_000),
    }
    return {
        'sampleRate': 48000, 'blockFrames': 128, 'callbacks': 32, 'arrangementPaints': 1,
        'cpuDiagnosticsEnabled': True, 'performanceQualificationEligible': False, 'hourTargetPassed': False,
        'callbackExecution': {'samples': 32},
        'cpuDiagnostics': {
            'complete': True, 'callbackCpuSamples': 32, 'callbackCpuErrors': 0, 'paintCpuErrors': 0,
            'enclosingCallbackThreadCpu': {'samples': 32}, 'enclosingPaintThreadCpu': {'samples': 1},
            'name': 'Synthetic clock', 'unitNanoseconds': 1, 'reportedResolutionNanoseconds': 0,
            'resolutionError': 0xffffffff,
        },
        'timingTrace': {
            'callbackSamples': 32, 'hostSamples': 32, 'invalidOrDuplicateCallbackMetrics': 0,
            'invalidOrDuplicateHostMetrics': 0, 'allSamplesPresent': True, 'traceComplete': True,
            'tails': [host], 'tailLimit': 256, 'tailCount': 1, 'omittedTails': 0,
            'maximumProcessCall': copy.deepcopy(host),
            'blocksEndingAt4096FrameBoundary': {'samples': 1}, 'otherBlocks': {'samples': 31},
        },
        'paintTrace': {
            'samples': 1, 'invalidOrDuplicateMetrics': 0, 'tails': [paint],
            'tailLimit': 256, 'tailCount': 1, 'omittedTails': 0,
        },
    }


class ObservationTests(unittest.TestCase):
    def test_expanded_timeline_declarations_and_exclusion(self):
        data = valid_report()
        data.update(requestedSeconds=2, diagnosticTimelineSeconds=4, projectFrames=192000,
                    storedAutomationPoints=102, callbacks=750)
        data['callbackExecution']['samples'] = 750
        data['timingTrace'].update(callbackSamples=750, hostSamples=750)
        data['timingTrace']['blocksEndingAt4096FrameBoundary']['samples'] = 23
        data['timingTrace']['otherBlocks']['samples'] = 727
        data['cpuDiagnostics']['callbackCpuSamples'] = 750
        data['cpuDiagnostics']['enclosingCallbackThreadCpu']['samples'] = 750
        self.assertEqual(CHECKER.check_observations(data)['workloadDeclaration']['projectFrames'], 192000)
        normal = copy.deepcopy(data)
        normal.update(diagnosticTimelineSeconds=0, projectFrames=96000, storedAutomationPoints=54,
                      cpuDiagnosticsEnabled=False, performanceQualificationEligible=True, hourTargetPassed=True)
        with self.assertRaises(CHECKER.InvalidObservation):
            CHECKER.check_observations(normal)
        for key, value in [('storedAutomationPoints', 54), ('projectFrames', 96000),
                           ('diagnosticTimelineSeconds', 1), ('diagnosticTimelineSeconds', 3601),
                           ('cpuDiagnosticsEnabled', False), ('callbacks', 749)]:
            with self.subTest(key=key, value=value):
                bad = copy.deepcopy(data)
                bad[key] = value
                with self.assertRaises(CHECKER.InvalidObservation):
                    CHECKER.check_observations(bad)

    def test_thread_policy_brackets_allow_pre_origin_but_reject_corruption(self):
        data = valid_report()
        names = ['painterBeforeActivity', 'painterDuringActivity', 'callbackAtStart',
                 'callbackAtEnd', 'painterAtEnd']
        data['threadPolicySnapshots'] = {name: {'supported': False,
            'beforeNanosecondsFromStart': i * 100 - 300,
            'afterNanosecondsFromStart': i * 100 - 250, 'queryWallNanoseconds': 50}
            for i, name in enumerate(names)}
        self.assertEqual(CHECKER.check_observations(data)['threadPolicySnapshots']['checked'], 5)
        for name, key, value in [('painterBeforeActivity', 'queryWallNanoseconds', 0),
                                  ('callbackAtEnd', 'beforeNanosecondsFromStart', -500),
                                  ('painterAtEnd', 'afterNanosecondsFromStart', False)]:
            with self.subTest(name=name, key=key):
                bad = copy.deepcopy(data)
                bad['threadPolicySnapshots'][name][key] = value
                with self.assertRaises(CHECKER.InvalidObservation):
                    CHECKER.check_observations(bad)

    def test_valid_rows_and_repeated_global_maximum(self):
        result = CHECKER.check_observations(valid_report())
        self.assertTrue(result['arithmeticChecksPassed'])
        self.assertEqual(result['hostRowsChecked'], 2)
        self.assertEqual(result['paintRowsChecked'], 1)
        self.assertNotIn('hourTargetPassed', result)

    def test_coarse_and_zero_cpu_are_counted_without_attribution(self):
        data = valid_report()
        data['timingTrace']['tails'][0]['cpuObservation'] = cpu_observation(82_666_700, 84_066_700, 0)
        data['paintTrace']['tails'][0]['cpuObservation'] = cpu_observation(10, 20_000_010, 31_250_000)
        result = CHECKER.check_observations(data)
        self.assertEqual(result['zeroCpuDeltas'], 1)
        self.assertEqual(result['cpuDeltasAboveEnclosingWall'], 1)

    def test_visible_truncation_is_not_a_performance_pass(self):
        data = valid_report()
        data['timingTrace'].update(tailCount=9, omittedTails=8, traceComplete=False)
        self.assertTrue(CHECKER.check_observations(data)['arithmeticChecksPassed'])

    def test_rejects_corruption(self):
        changes = [
            (('timingTrace', 'tails', 0, 'projectFrame'), 1),
            (('timingTrace', 'tails', 0, 'projectFrame'), 3968.0),
            (('timingTrace', 'tails', 0, 'projectFrame'), True),
            (('timingTrace', 'tails', 0, 'processCallNanoseconds'), 1),
            (('timingTrace', 'tails', 0, 'cpuObservation', 'threadCpuDeltaNanoseconds'), 0),
            (('timingTrace', 'tails', 0, 'cpuObservation', 'first', 'afterNanosecondsFromStart'), 99_999_999),
            (('timingTrace', 'tails', 0, 'cpuObservation', 'last', 'threadCpuNanoseconds'), 0),
            (('timingTrace', 'blocksEndingAt4096FrameBoundary', 'samples'), 2),
            (('timingTrace', 'hostSamples'), 31),
            (('timingTrace', 'traceComplete'), False),
            (('timingTrace', 'tailCount'), 2),
            (('paintTrace', 'tails', 0, 'parameter'), 2),
            (('paintTrace', 'tails', 0, 'movedClipFrames'), 48000),
            (('cpuDiagnostics', 'callbackCpuSamples'), 31),
            (('cpuDiagnostics', 'complete'), False),
            (('performanceQualificationEligible',), True),
        ]
        for path, value in changes:
            with self.subTest(path=path, value=value):
                data = valid_report()
                parent = data
                for key in path[:-1]:
                    parent = parent[key]
                parent[path[-1]] = value
                with self.assertRaises(CHECKER.InvalidObservation):
                    CHECKER.check_observations(data)

    def test_duplicate_tail_rejected(self):
        data = valid_report()
        data['timingTrace']['tails'] *= 2
        data['timingTrace']['tailCount'] = 2
        with self.assertRaises(CHECKER.InvalidObservation):
            CHECKER.check_observations(data)

    def test_ctest_log_and_utf8_bom(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'LastTest.log'
            text = 'software_stress_smoke\nOutput:\n'+json.dumps({'cpuDiagnosticSmoke': valid_report()})+'\nTest passed.'
            path.write_text(text, encoding='utf-8-sig')
            CHECKER.check_observations(CHECKER.read_data(path, True, 'cpuDiagnosticSmoke'))

    def test_nonfinite_json_rejected(self):
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'bad.json'
            path.write_text('{"sampleRate": NaN}')
            with self.assertRaises(CHECKER.InvalidObservation):
                CHECKER.read_data(path)

    def test_optimized_python_still_rejects_bad_input(self):
        data = valid_report()
        data['performanceQualificationEligible'] = True
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'bad.json'
            path.write_text(json.dumps(data))
            result = subprocess.run([sys.executable, '-O', str(SCRIPT), str(path)],
                                    capture_output=True, text=True)
            self.assertEqual(result.returncode, 2)
            self.assertIn('cannot qualify performance', result.stderr)
            self.assertEqual(result.stdout, '')


if __name__ == '__main__':
    unittest.main()
