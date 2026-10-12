# TASK-P006 — Pilot 0x00120ef8: FUN_00120ef8

- Status: backlog
- Objective: measure a bounded recovery/port/comparison experiment for Graphics and VU/GS.
- Scope: this function and explicitly recorded dependency closure only; no full subsystem implementation.
- Original PS2 function address: 0x00120ef8
- Binary SHA-256: 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2
- Original identifier: unknown; inherited analysis name FUN_00120ef8
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

buffer writes, VU math and captured frame output. Record exact emulator/build/settings, inputs/state/capture hashes,
expected/actual outputs and tolerances. Tests alone do not establish original equivalence.

## Current findings

many COP2 operations, matrix transforms and buffer/packet construction; cc=233. Confidence medium; not verified. Current CFG estimate
233, 2461 observed block-contained instructions;
2 callers / 45 callees. Signature: undefined FUN_00120ef8(void) (unverified).
Direct callees: 0x00296ec4, 0x001107e0, 0x0011b1b0, 0x00188180, 0x001164f0, 0x00110848, 0x001a9fb8, 0x0025aed0, 0x001107f8, 0x0025ac98, 0x001107d0, 0x00110a68, 0x001a9a30, 0x0025ae18, 0x0011b2c0, 0x001109f8, 0x0011ba18, 0x00107cd8, 0x00152070, 0x00120c00, 0x002596f8, 0x0010dc80, 0x00107600, 0x0011b248, 0x001074b8, 0x00120130, 0x00250288, 0x0025ac68, 0x00120d60, 0x0010ccb0, 0x001b50c8, 0x0011b300, 0x00124d38, 0x00110608, 0x0011d980, 0x0010bd38, 0x0010ca60, 0x00115978, 0x0010c8f0, 0x00110a40, 0x00120ea8, 0x001209f8, 0x00110488, 0x00110878, 0x0010caa0.
Data uncertainty high (unrecovered object/buffer layouts); verification difficulty very_high.

## Unresolved questions

Signature/ABI, data layout, dependency semantics and original oracle remain unverified. No PCSX2 comparison or reconstructed implementation exists.

## Next steps

Read assessment findings and establish a dependency closure. Use synthetic fixtures
and private original captures. Stop with an explicit blocker if original invocation
is not reproducible; do not substitute fabricated behavior.

## Planning assumptions and actual time

Optimistic / baseline / pessimistic phase budgets RE + C++ + verification:
40 + 32 + 60 / 120 + 100 + 180 / 300 + 240 + 500 person-hours, uncalibrated and excluding shared
setup. Actual engineering time: not measured. See WORKLOAD_ESTIMATE.md for limits.

## Related Git commits

None. No pilot implementation was executed during TASK-0002.

