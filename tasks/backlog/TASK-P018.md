# TASK-P018 — Pilot 0x0012f378: FUN_0012f378

- Status: backlog
- Objective: measure a bounded recovery/port/comparison experiment for Main loop and engine core.
- Scope: this function and explicitly recorded dependency closure only; no full subsystem implementation.
- Original PS2 function address: 0x0012f378
- Binary SHA-256: 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2
- Original identifier: unknown; inherited analysis name FUN_0012f378
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

frame/input trace and update-order/time-step comparisons. Record exact emulator/build/settings, inputs/state/capture hashes,
expected/actual outputs and tolerances. Tests alone do not establish original equivalence.

## Current findings

registered startup callback; repeated timing, update calls and state-dependent orchestration. Confidence medium; not verified. Current CFG estimate
13, 195 observed block-contained instructions;
0 callers / 39 callees. Signature: undefined FUN_0012f378(void) (unverified).
Direct callees: 0x00250f38, 0x00169538, 0x0015ef28, 0x0014e768, 0x001003c0, 0x0014d678, 0x00156400, 0x00155d40, 0x001a35e0, 0x00112858, 0x001af4c0, 0x00181d88, 0x0012f088, 0x001a35f0, 0x0016d420, 0x00153e48, 0x00250f48, 0x00199bd0, 0x0024bdb8, 0x001a33f0, 0x001680b8, 0x0014dcd8, 0x0013e008, 0x001680e8, 0x0012f190, 0x0014d6e0, 0x00181c28, 0x001ec858, 0x001ac0a0, 0x00149dd8, 0x0013d880, 0x0014d718, 0x0012f090, 0x00149650, 0x001a3500, 0x0013e078, 0x0014e5c8, 0x00199788, 0x00149578.
Data uncertainty medium (unverified global/ABI contracts); verification difficulty very_high.

## Unresolved questions

Signature/ABI, data layout, dependency semantics and original oracle remain unverified. No PCSX2 comparison or reconstructed implementation exists.

## Next steps

Read assessment findings and establish a dependency closure. Use synthetic fixtures
and private original captures. Stop with an explicit blocker if original invocation
is not reproducible; do not substitute fabricated behavior.

## Planning assumptions and actual time

Optimistic / baseline / pessimistic phase budgets RE + C++ + verification:
4 + 3 + 4 / 12 + 10 + 12 / 32 + 24 + 40 person-hours, uncalibrated and excluding shared
setup. Actual engineering time: not measured. See WORKLOAD_ESTIMATE.md for limits.

## Related Git commits

None. No pilot implementation was executed during TASK-0002.

