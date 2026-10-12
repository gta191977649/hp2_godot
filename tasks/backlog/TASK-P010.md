# TASK-P010 — Pilot 0x00250340: FUN_00250340

- Status: backlog
- Objective: measure a bounded recovery/port/comparison experiment for Memory and platform services.
- Scope: this function and explicitly recorded dependency closure only; no full subsystem implementation.
- Original PS2 function address: 0x00250340
- Binary SHA-256: 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2
- Original identifier: unknown; inherited analysis name FUN_00250340
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

guarded buffers, zero length, all byte values, aligned/unaligned pointers. Record exact emulator/build/settings, inputs/state/capture hashes,
expected/actual outputs and tolerances. Tests alone do not establish original equivalence.

## Current findings

byte fill with aligned word loop; memset-like semantics observed. Confidence high; not verified. Current CFG estimate
7, 45 observed block-contained instructions;
90 callers / 0 callees. Signature: undefined FUN_00250340(void) (unverified).
Direct callees: none recovered.
Data uncertainty low (byte-buffer contract); verification difficulty low.

## Unresolved questions

Signature/ABI, data layout, dependency semantics and original oracle remain unverified. No PCSX2 comparison or reconstructed implementation exists.

## Next steps

Read assessment findings and establish a dependency closure. Use synthetic fixtures
and private original captures. Stop with an explicit blocker if original invocation
is not reproducible; do not substitute fabricated behavior.

## Planning assumptions and actual time

Optimistic / baseline / pessimistic phase budgets RE + C++ + verification:
2 + 2 + 4 / 6 + 4 + 12 / 16 + 8 + 32 person-hours, uncalibrated and excluding shared
setup. Actual engineering time: not measured. See WORKLOAD_ESTIMATE.md for limits.

## Related Git commits

None. No pilot implementation was executed during TASK-0002.

