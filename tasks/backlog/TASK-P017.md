# TASK-P017 — Pilot 0x00131da8: FUN_00131da8

- Status: backlog
- Objective: measure a bounded recovery/port/comparison experiment for Physics and collision.
- Scope: this function and explicitly recorded dependency closure only; no full subsystem implementation.
- Original PS2 function address: 0x00131da8
- Binary SHA-256: 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2
- Original identifier: unknown; inherited analysis name FUN_00131da8
- Dependencies: TASK-0003 (boundary/ABI review), TASK-0004 (original oracle); inspect dependencies before widening scope.
- Relevant source files: docs/assessment/PILOT_TASKS.md, FUNCTION_INVENTORY.csv, CALL_EDGES.csv; PC implementation location not assigned yet.
- GitHub issue: not created

## Acceptance criteria

- [ ] Confirm entry/body, ABI and provenance; separate inherited labels from evidence.
- [ ] Record call/data/structure dependencies and unknown indirect targets.
- [ ] Bound the experiment; reconstruct only justified behavior in independent C++ where feasible.
- [ ] Compare original vs PC for a stated normal/edge-case scope; retain failures and limitations.
- [ ] Record actual person-hours separately for analysis, C++, shared layouts/adapters, comparison and rework.
- [ ] Update confidence/status, evidence, assessment estimates and continuation notes; never imply full-game coverage.

## Verification procedure

timestep/collision traces, contact edges, zero dt and COP2 precision. Record exact emulator/build/settings, inputs/state/capture hashes,
expected/actual outputs and tolerances. Tests alone do not establish original equivalence.

## Current findings

CarCollision parameter string, delta-time vector calculations and side effects. Confidence medium; not verified. Current CFG estimate
60, 542 observed block-contained instructions;
1 callers / 16 callees. Signature: undefined FUN_00131da8(void) (unverified).
Direct callees: 0x0013e270, 0x00152558, 0x002087f8, 0x0021eb20, 0x00135b30, 0x001769e8, 0x00176ab0, 0x00177c88, 0x00134198, 0x0021eb48, 0x0021ebd8, 0x001d6f38, 0x00208838, 0x001c0888, 0x0021eb90, 0x00178358.
Data uncertainty high (unrecovered object/buffer layouts); verification difficulty very_high.

## Unresolved questions

Signature/ABI, data layout, dependency semantics and original oracle remain unverified. No PCSX2 comparison or reconstructed implementation exists.

## Next steps

Read assessment findings and establish a dependency closure. Use synthetic fixtures
and private original captures. Stop with an explicit blocker if original invocation
is not reproducible; do not substitute fabricated behavior.

## Planning assumptions and actual time

Optimistic / baseline / pessimistic phase budgets RE + C++ + verification:
16 + 12 + 24 / 48 + 40 + 72 / 120 + 100 + 200 person-hours, uncalibrated and excluding shared
setup. Actual engineering time: not measured. See WORKLOAD_ESTIMATE.md for limits.

## Related Git commits

None. No pilot implementation was executed during TASK-0002.

