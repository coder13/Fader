#!/usr/bin/env python3
"""Check serialized stress timing arithmetic without running an audio host."""
import argparse
import json
from pathlib import Path
import sys


class InvalidObservation(ValueError):
    """A record contradicts the declared stress workload or its own timestamps."""


def require(condition, description):
    if not condition:
        raise InvalidObservation(description)


def equal(actual, expected, description):
    require(actual == expected, f"{description}: expected {expected!r}, got {actual!r}")


def integers(record, names):
    for name in names:
        require(type(record[name]) is int and record[name] >= 0,
                f"{name} must be a nonnegative integer")


def retained_rows(trace, index):
    integers(trace, ('tailLimit', 'tailCount', 'omittedTails'))
    tails = trace['tails']
    require(isinstance(tails, list), 'tails must be an array')
    equal(len(tails) + trace['omittedTails'], trace['tailCount'], 'tail accounting')
    require(len(tails) <= trace['tailLimit'], 'tail capacity exceeded')
    indices = [row[index] for row in tails]
    equal(indices, sorted(set(indices)), f'unique ordered {index} values')
    # A global maximum can repeat a retained tail. Do not deduplicate it away.
    return [(f'tail-{i}', row) for i, row in enumerate(tails)] + [
        (name, row) for name, row in trace.items()
        if name.startswith('maximum') and isinstance(row, dict)
    ]


def check_observations(data):
    integers(data, ('sampleRate', 'blockFrames', 'callbacks', 'arrangementPaints'))
    rate, block, callbacks = data['sampleRate'], data['blockFrames'], data['callbacks']
    require(rate == 48000 and block in (128, 256) and callbacks > 0,
            'expected the declared 48 kHz software workload at 128 or 256 frames')
    diagnostic = data.get('cpuDiagnosticsEnabled', False)
    require(type(diagnostic) is bool, 'cpuDiagnosticsEnabled must be boolean')
    report = {
        'scope': 'Serialized observations only. Maxima can repeat tails. Passing '
                 'these checks does not qualify performance, CPU precision or source/binary identity.',
        'hostRowsChecked': 0, 'paintRowsChecked': 0, 'cpuRowsChecked': 0,
        'cpuDeltasAboveEnclosingWall': 0, 'zeroCpuDeltas': 0,
        'largestRetainedQueryBracketNanoseconds': 0,
    }
    trace = data['timingTrace']
    for field in ('callbackSamples', 'hostSamples'):
        equal(trace[field], callbacks, field)
    equal(data['callbackExecution']['samples'], callbacks, 'callback distribution coverage')
    for field in ('invalidOrDuplicateCallbackMetrics', 'invalidOrDuplicateHostMetrics'):
        equal(trace[field], 0, field)
    require(trace['allSamplesPresent'] is True, 'timing histories are incomplete')
    equal(trace['traceComplete'], trace['omittedTails'] == 0, 'trace completeness flag')
    if diagnostic:
        require(data['performanceQualificationEligible'] is False and data['hourTargetPassed'] is False,
                'instrumented observations cannot qualify performance')
        cpu = data['cpuDiagnostics']
        require(cpu['complete'] is True, 'requested CPU observation is incomplete')
        equal(cpu['callbackCpuSamples'], callbacks, 'callback CPU coverage')
        equal(cpu['enclosingCallbackThreadCpu']['samples'], callbacks, 'callback CPU distribution coverage')
        equal(cpu['enclosingPaintThreadCpu']['samples'], data['arrangementPaints'], 'painter CPU coverage')
        for field in ('callbackCpuErrors', 'paintCpuErrors'):
            equal(cpu[field], 0, field)
        report['cpuClock'] = {key: cpu[key] for key in (
            'name', 'unitNanoseconds', 'reportedResolutionNanoseconds', 'resolutionError')}

    def cpu_check(observation, lower, upper, returned):
        first, last = observation['first'], observation['last']
        for reading in (first, last):
            integers(reading, ('beforeNanosecondsFromStart', 'afterNanosecondsFromStart',
                               'threadCpuNanoseconds', 'error'))
            require(reading['valid'] is True, 'native CPU reading is invalid')
            equal(reading['error'], 0, 'native CPU error')
        integers(observation, ('threadCpuDeltaNanoseconds', 'minimumWallWindowNanoseconds',
                               'maximumWallWindowNanoseconds'))
        require(observation['valid'] is True, 'CPU observation is invalid')
        a0, a1 = first['beforeNanosecondsFromStart'], first['afterNanosecondsFromStart']
        b0, b1 = last['beforeNanosecondsFromStart'], last['afterNanosecondsFromStart']
        require(lower <= a0 <= a1 <= returned <= b0 <= b1 <= upper, 'unordered CPU query brackets')
        delta = last['threadCpuNanoseconds'] - first['threadCpuNanoseconds']
        require(delta >= 0, 'thread CPU time went backwards')
        equal(observation['threadCpuDeltaNanoseconds'], delta, 'CPU delta')
        equal(observation['minimumWallWindowNanoseconds'], b0-a1, 'minimum CPU wall window')
        equal(observation['maximumWallWindowNanoseconds'], b1-a0, 'maximum CPU wall window')
        report['cpuRowsChecked'] += 1
        report['zeroCpuDeltas'] += delta == 0
        # Coarse accounting (observed on Windows) can exceed a short wall interval.
        # Count it explicitly; do not reject it or turn zero into zero CPU work.
        report['cpuDeltasAboveEnclosingWall'] += delta > b1-a0
        report['largestRetainedQueryBracketNanoseconds'] = max(
            report['largestRetainedQueryBracketNanoseconds'], a1-a0, b1-b0)
        return {'cpuNanoseconds': delta, 'wallMinimumNanoseconds': b0-a1,
                'wallMaximumNanoseconds': b1-a0, 'firstQueryNanoseconds': a1-a0,
                'lastQueryNanoseconds': b1-b0}

    def frame_time(frame):
        return (frame * 1_000_000_000 + rate // 2) // rate

    host_rows = []
    for label, row in retained_rows(trace, 'projectFrame'):
        fields = ('projectFrame', 'targetNanosecondsFromStart', 'wakeNanosecondsFromStart',
                  'processReturnedNanosecondsFromStart', 'sinkFinishedNanosecondsFromStart',
                  'deadlineNanosecondsFromStart', 'wakeLatenessNanoseconds', 'processCallNanoseconds',
                  'sinkNanoseconds', 'completionLatenessNanoseconds', 'internalCallbackNanoseconds')
        integers(row, fields)
        frame, target, wake, returned, finished, deadline = (row[key] for key in fields[:6])
        require(frame % block == 0 and frame // block < callbacks, 'invalid callback frame')
        equal(target, frame_time(frame), 'absolute callback target')
        equal(deadline, frame_time(frame+block), 'absolute callback deadline')
        require(target <= wake <= returned <= finished, 'unordered host markers')
        for field, expected in (
            ('wakeLatenessNanoseconds', wake-target), ('processCallNanoseconds', returned-wake),
            ('sinkNanoseconds', finished-returned), ('completionLatenessNanoseconds', max(0, finished-deadline))):
            equal(row[field], expected, field)
        require(row['callbackMetricMatched'] is True, 'internal callback metric is missing')
        require(row['internalCallbackNanoseconds'] <= returned-wake, 'internal timer exceeds process call')
        if label.startswith('tail-'):
            half = (deadline-target)//2
            require(wake-target >= half or returned-wake >= half or finished > deadline,
                    'retained callback does not meet tail selection')
        if 'projectFrameModulo4096' in row:
            equal(row['projectFrameModulo4096'], frame % 4096, 'callback frame remainder')
        summary = {'label': label, 'frame': frame, 'endsAt4096Boundary': (frame+block) % 4096 == 0,
                   'wakeNanoseconds': wake-target, 'internalNanoseconds': row['internalCallbackNanoseconds'],
                   'callNanoseconds': returned-wake, 'deadlineLatenessNanoseconds': max(0, finished-deadline)}
        if diagnostic:
            summary.update(cpu_check(row['cpuObservation'], wake, finished, returned))
        host_rows.append(summary)
        report['hostRowsChecked'] += 1
    report['hostRows'] = host_rows
    if 'blocksEndingAt4096FrameBoundary' in trace:
        boundary = callbacks // (4096 // block)
        equal(trace['blocksEndingAt4096FrameBoundary']['samples'], boundary, 'boundary population')
        equal(trace['otherBlocks']['samples'], callbacks-boundary, 'other population')
        report['populations'] = {key: trace[key] for key in ('blocksEndingAt4096FrameBoundary', 'otherBlocks')}
    if 'paintTrace' in data:
        paint = data['paintTrace']
        equal(paint['samples'], data['arrangementPaints'], 'painter coverage')
        equal(paint['invalidOrDuplicateMetrics'], 0, 'invalid painter metrics')
        paint_rows = []
        for label, row in retained_rows(paint, 'paintIndex'):
            fields = ('paintIndex', 'scheduledNanosecondsFromStart', 'beginNanosecondsFromStart',
                      'endNanosecondsFromStart', 'updateAndPaintNanoseconds', 'startLatenessNanoseconds',
                      'parameter', 'movedClipFrames', 'viewportX', 'playheadFrame')
            integers(row, fields)
            index, target, began, finished = (row[key] for key in fields[:4])
            require(index < paint['samples'] and target <= began <= finished, 'invalid painter markers')
            equal(target, index*20_000_000, 'absolute painter target')
            equal(row['updateAndPaintNanoseconds'], finished-began, 'painter duration')
            equal(row['startLatenessNanoseconds'], began-target, 'painter start lateness')
            equal(row['parameter'], index % 3, 'selected painter parameter')
            equal(row['movedClipFrames'], rate if index % 2 else 0, 'editor clip displacement')
            if label.startswith('tail-'):
                require(finished-began >= 20_000_000 or began-target > 20_000_000,
                        'retained painter does not meet tail selection')
            summary = {'label': label, 'index': index, 'wallNanoseconds': finished-began,
                       'startLatenessNanoseconds': began-target, 'viewportX': row['viewportX'],
                       'parameter': row['parameter']}
            if diagnostic:
                upper = row['cpuObservation']['last']['afterNanosecondsFromStart']
                summary.update(cpu_check(row['cpuObservation'], began, upper, finished))
            paint_rows.append(summary)
            report['paintRowsChecked'] += 1
        report['paintRows'] = paint_rows
    report['arithmeticChecksPassed'] = True
    return report


def reject_nonfinite(value):
    raise InvalidObservation(f'non-finite JSON value: {value}')


def read_data(path, ctest_log=False, key=None):
    text = path.read_text(encoding='utf-8-sig')
    decoder = json.JSONDecoder(parse_constant=reject_nonfinite)
    if ctest_log:
        text = text[text.index('{', text.index('software_stress_smoke')):]
        data, _ = decoder.raw_decode(text)
    else:
        data = decoder.decode(text)
    return data[key] if key else data


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('input', type=Path, help='Probe JSON, aggregate JSON, or full CTest LastTest.log')
    parser.add_argument('--key', help='Select timed, timedSmoke, or cpuDiagnosticSmoke from its enclosing object')
    parser.add_argument('--ctest-log', action='store_true', help='Read the software_stress_smoke JSON in a CTest log')
    parser.add_argument('--output', type=Path, help='Write detailed checked rows; stdout keeps only counts')
    args = parser.parse_args()
    result = check_observations(read_data(args.input, args.ctest_log, args.key))
    if args.output:
        args.output.write_text(json.dumps(result, indent=2)+'\n', encoding='utf-8')
    print(json.dumps({key: value for key, value in result.items()
                      if key not in ('hostRows', 'paintRows')}, indent=2))


if __name__ == '__main__':
    try:
        main()
    except (InvalidObservation, KeyError, TypeError, ValueError, OSError) as error:
        print(f'Invalid stress observations: {error}', file=sys.stderr)
        sys.exit(2)
