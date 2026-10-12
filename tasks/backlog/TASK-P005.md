# TASK-P005 — Pilot 0x001ebd20: HP2_LoadFlightAreas

- Status: backlog
- Objective: measure a bounded recovery/port/comparison experiment for Resource management.
- Scope: this function and explicitly recorded dependency closure only; no full subsystem implementation.
- Original PS2 function address: 0x001ebd20
- Binary SHA-256: 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2
- Original identifier: unknown; inherited analysis name HP2_LoadFlightAreas
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

repair body; malformed/valid synthetic buffers and relocation results. Record exact emulator/build/settings, inputs/state/capture hashes,
expected/actual outputs and tolerances. Tests alone do not establish original equivalence.

## Current findings

preexisting flight-area name; magic test, offset arithmetic and aligned pointer fixups; one-byte body. Confidence medium; not verified. Current CFG estimate
1, 5 observed block-contained instructions;
0 callers / 0 callees. Signature: undefined HP2_LoadFlightAreas(void) (unverified).
Direct callees: none recovered.
Data uncertainty high (unrecovered object/buffer layouts); verification difficulty high.

## Unresolved questions

Function body is one byte; reconcile decompiler reachability and correct entry/body metadata before trusting CFG/call data. No PCSX2 comparison or reconstructed implementation exists.

## Next steps

Read assessment findings and establish a dependency closure. Use synthetic fixtures
and private original captures. Stop with an explicit blocker if original invocation
is not reproducible; do not substitute fabricated behavior.

## Planning assumptions and actual time

Optimistic / baseline / pessimistic phase budgets RE + C++ + verification:
Unknown until body/ABI audit; current metadata is insufficient for a usable
per-function estimate. Actual engineering time: not measured. See WORKLOAD_ESTIMATE.md for limits.

## Related Git commits

None. No pilot implementation was executed during TASK-0002.

