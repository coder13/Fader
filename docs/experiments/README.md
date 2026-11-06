# Experiment reports

The [read-only library inventory](2026-11-04-library-inventory.md) is complete.
Software engine, streaming, seek, storage, export and portability reports now record their own tested scopes and remaining limits.
Physical controller and acoustic qualification remain open.
The [128/256-frame stress hours](2026-11-05-software-stress-followup.md) and [completed CPU diagnostic](2026-11-05-boundary-hour.md) still fail their timing gates. The [completed activity experiment](2026-11-05-playback-activity-hour.md) records zero virtual misses but still fails the painter-start gate.
The [full-size short CPU diagnostics](2026-11-05-full-timeline-painter-diagnostic.md) isolate native drawing paths without reducing the project or claiming an hour pass.
The [bounded handle drawing candidate](2026-11-05-handle-batching.md) preserves point geometry and requires new timing evidence.
The [native curve clip candidate](2026-11-05-native-curve-clip.md) keeps every curve vertex and tests stroke geometry at Retina scale.
Add a report for each decision spike and qualification run, using its actual date and a descriptive filename.

Copy this outline and replace the prompts with evidence. A failed experiment is a useful result when it identifies the next decision.

## Question and gate

State the specific uncertainty, linked backlog issue and POC criterion, and the pass condition agreed before running. Distinguish feasibility, performance target, and product preference.

## Configuration

Record commit/build type/compiler, dependency versions/flags, OS/chip/RAM, controller model/firmware, audio backend/rate/channels/buffer, USB/power route, DSP mode, and browser version where applicable. Record fixture identity or generation recipe. Omit serial numbers and private media paths from shared reports.

## Method

Describe exact steps, duration, repetitions, warm-up, load, and measurement instrumentation. For timing, name the start/end points and clock domains. Separate requested buffer size from observed buffer size, estimated latency from physical measurement, and real-time PCM from offline render output.

## Results

Include expected versus actual behavior, p50/p95/p99/p99.9 and maximum where relevant, sample count/duration, missed deadlines, underruns, late inputs, queue high-water marks, capture loss, and peak memory. Link raw logs/plots/fixtures. State unmeasured portions instead of estimating them silently.

## Decision and follow-up

Record pass/fail/partial and why. Identify supported configurations and remaining gaps. Link the ADR or design update, follow-up issue, and any scope/threshold change explicitly. Do not turn a one-machine result into a general compatibility claim.
