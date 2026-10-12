# TASK-P007 — Pilot 0x001796b0: FUN_001796b0

- Status: backlog
- Objective: measure a bounded recovery/port/comparison experiment for Unknown.
- Scope: this function and explicitly recorded dependency closure only; no full subsystem implementation.
- Original PS2 function address: 0x001796b0
- Binary SHA-256: 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2
- Original identifier: unknown; inherited analysis name FUN_001796b0
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

identify purpose/dependency closure before behavior tests. Record exact emulator/build/settings, inputs/state/capture hashes,
expected/actual outputs and tolerances. Tests alone do not establish original equivalence.

## Current findings

large complex function intentionally retained without speculative subsystem label. Confidence unknown; not verified. Current CFG estimate
148, 2451 observed block-contained instructions;
2 callers / 35 callees. Signature: undefined FUN_001796b0(void) (unverified).
Direct callees: 0x001ee390, 0x00140c68, 0x00140a48, 0x002577c0, 0x001ee3c0, 0x002534a8, 0x001babb8, 0x00144968, 0x00140a20, 0x001454e8, 0x00143d50, 0x001440d8, 0x00140728, 0x0014cbf8, 0x0016eb68, 0x00140ea8, 0x00140978, 0x001411d8, 0x00251398, 0x00178c90, 0x00107c90, 0x001449e8, 0x00144398, 0x00140e20, 0x001767a8, 0x00143dd8, 0x001badc8, 0x0017c888, 0x00182a48, 0x001434a0, 0x0017c7c8, 0x001445f0, 0x00144a28, 0x00206ab8, 0x00181558.
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

