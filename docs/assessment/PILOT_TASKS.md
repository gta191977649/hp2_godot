# Representative pilot plan — TASK-0002

Twenty selected functions span all requested subsystem categories provisionally,
including an explicitly Unknown large routine. This is an exploratory stratified
sample, not a statistically representative estimate. All entries reference ELF
SHA-256 `7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`.
Names/signatures are inherited Ghidra metadata; original identifiers are unknown.
No pilot reconstruction or original comparison was executed. Actual engineering
time is **not measured** for every pilot, rather than recorded as zero.

## Candidate summary

CFG instructions/counts describe the current analysis only. Camera/load entries
have defective bodies; their instruction/complexity numbers cannot describe the
full original functions. Caller/callee counts exclude unresolved dynamic targets.

| Pilot | ELF address | Subsystem / confidence | CFG instructions / M | Callers / callees | Complexity: code / dependencies / hardware | RE+C+++verification hours O / B / P |
|---|---|---|---|---|---|---|
| P001 | 0x00100008 | Memory and platform services / high | 44 / 2 | 0 / 4 | low / medium / high | 1+0.5+0.75 / 3+2+2 / 8+6+6 |
| P002 | 0x0012f000 | Main loop and engine core / medium | 33 / 2 | 1 / 5 | low / medium / unknown | 1+0.5+0.75 / 3+2+2 / 8+6+6 |
| P003 | 0x0016fa80 | Game states / low | 78 / 7 | 1 / 8 | medium / medium / unknown | 4+3+4 / 12+10+12 / 32+24+40 |
| P004 | 0x0019a4b8 | World and scene management / low | unknown / unknown | 0 / 0 | unknown / low / unknown | unknown; boundary audit first |
| P005 | 0x001ebd20 | Resource management / medium | 5 / 1 | 0 / 0 | low / low / unknown | unknown; boundary audit first |
| P006 | 0x00120ef8 | Graphics and VU/GS / medium | 2461 / 233 | 2 / 45 | very_high / high / high | 40+32+60 / 120+100+180 / 300+240+500 |
| P007 | 0x001796b0 | Unknown / unknown | 2451 / 148 | 2 / 35 | very_high / high / unknown | 40+32+60 / 120+100+180 / 300+240+500 |
| P008 | 0x002514f0 | Memory and platform services / medium | 1879 / 148 | 3 / 11 | very_high / high / unknown | 40+32+60 / 120+100+180 / 300+240+500 |
| P009 | 0x002577c0 | Memory and platform services / medium | 36 / 2 | 255 / 3 | low / high / unknown | 1+0.5+0.75 / 3+2+2 / 8+6+6 |
| P010 | 0x00250340 | Memory and platform services / high | 45 / 7 | 90 / 0 | medium / high / unknown | 2+2+4 / 6+4+12 / 16+8+32 |
| P011 | 0x00250ac0 | Memory and platform services / low | 12 / 1 | 82 / 2 | low / high / unknown | 1+0.5+0.75 / 3+2+2 / 8+6+6 |
| P012 | 0x00285958 | Memory and platform services / high | 123 / 11 | 89 / 7 | medium / high / unknown | 4+3+4 / 12+10+12 / 32+24+40 |
| P013 | 0x0026a310 | Audio / high | 163 / 8 | 82 / 5 | medium / high / unknown | 4+3+4 / 12+10+12 / 32+24+40 |
| P014 | 0x00282fa0 | Graphics and VU/GS / medium | 4 / 1 | 1 / 0 | low / low / high | 1+0.5+0.75 / 3+2+2 / 8+6+6 |
| P015 | 0x001044c8 | World and scene management / low | 168 / 5 | 0 / 5 | low / medium / high | 1+0.5+0.75 / 3+2+2 / 8+6+6 |
| P016 | 0x00131618 | Physics and collision / medium | 121 / 9 | 17 / 9 | medium / high / unknown | 4+3+4 / 12+10+12 / 32+24+40 |
| P017 | 0x00131da8 | Physics and collision / medium | 542 / 60 | 1 / 16 | high / high / high | 16+12+24 / 48+40+72 / 120+100+200 |
| P018 | 0x0012f378 | Main loop and engine core / medium | 195 / 13 | 0 / 39 | medium / high / unknown | 4+3+4 / 12+10+12 / 32+24+40 |
| P019 | 0x00131410 | AI and gameplay / medium | 121 / 7 | 5 / 8 | medium / high / unknown | 4+3+4 / 12+10+12 / 32+24+40 |
| P020 | 0x00281f58 | Input / high | 121 / 7 | 1 / 3 | medium / medium / medium | 4+3+4 / 12+10+12 / 32+24+40 |

Budgets above reuse the **uncalibrated** CFG-stratum rates from WORKLOAD_ESTIMATE.md,
except P010 uses its explicit first-slice assumptions and P004/P005 remain unknown;
they are phase-separated assumptions, not measured work or commitments. Shared
layout/adapter/reference setup is additional and charged once. High dependency,
hardware or data uncertainty can dominate code size; do not accept these rates
until pilot timing confirms them. P004/P005 require additional boundary repair
before even a preliminary budget is usable. Full-module work is excluded.

## Execution gates and order

1. TASK-0003: audit function boundaries/ABI, especially P004/P005 and shared return contracts.
2. TASK-0004: establish a repeatable original/PCSX2 comparison environment.
3. P010: one bounded byte-buffer behavior to prove the recovery/comparison workflow.
4. P009/P012/P013/P020: shared allocation, RPC, audio and pad contracts; measure adapters separately.
5. P019/P016/P017: AI/controller, movement and collision state closures.
6. P002/P018/P003/P015/P004/P005: bootstrap/loop/state/world/resource cases after dependency review.
7. P006/P007/P008/P014/P011/P001: large/hardware/utility/runtime cases; do not omit difficult cases because the first helper succeeds.

A failed or blocked comparison remains useful pilot evidence and stays unfinished.
Do not replace original physics with Godot physics. Do not promote output-returned
decompilation to Verified. Implementation requires its own scoped pilot execution.

## Per-function findings, uncertainties and checks

### P001 — 0x00100008 (entry)

Finding: runtime startup; clears BSS, syscalls and bootstrap calls. Confidence: high; evidence E-0002-03/05/06.
Data uncertainty: medium (unverified global/ABI contracts). Verification difficulty: high.

Direct callees: 0x002501e8, 0x0028b228, 0x00283620, 0x0012f000. 0 observed
callers; full relationships are in CALL_EDGES.csv. 0 decoded indirect
call sites in returned blocks; targets may be absent from the graph.

Required checks: BSS clear, register state and bootstrap order; exclude host OS internals. Blockers: Signature/ABI, data layout, dependency semantics and original oracle remain unverified.
Next step: execute [TASK-P001](../../tasks/backlog/TASK-P001.md) after the
reference/boundary gates; bound the dependency closure before implementation.
Actual RE/C++/comparison time: not measured. Status: selected, not executed.

### P002 — 0x0012f000 (FUN_0012f000)

Finding: entry calls this; installs callback 0012f378; indirect scheduling semantics unresolved. Confidence: medium; evidence E-0002-03/05/06.
Data uncertainty: medium (unverified global/ABI contracts). Verification difficulty: high.

Direct callees: 0x0012ee98, 0x00100118, 0x001001cc, 0x00184bd8, 0x001680d8. 1 observed
callers; full relationships are in CALL_EDGES.csv. 0 decoded indirect
call sites in returned blocks; targets may be absent from the graph.

Required checks: startup state and callback order. Blockers: Signature/ABI, data layout, dependency semantics and original oracle remain unverified.
Next step: execute [TASK-P002](../../tasks/backlog/TASK-P002.md) after the
reference/boundary gates; bound the dependency closure before implementation.
Actual RE/C++/comparison time: not measured. Status: selected, not executed.

### P003 — 0x0016fa80 (FUN_0016fa80)

Finding: branches on global values 8/9 and invokes coordinated service set; state meaning unknown. Confidence: low; evidence E-0002-03/05/06.
Data uncertainty: medium (unverified global/ABI contracts). Verification difficulty: high.

Direct callees: 0x001aec90, 0x0016bde8, 0x0016c4c0, 0x0016b470, 0x0016c7b8, 0x0016c3a8, 0x0016c768, 0x0016d0a0. 1 observed
callers; full relationships are in CALL_EDGES.csv. 0 decoded indirect
call sites in returned blocks; targets may be absent from the graph.

Required checks: mode branches, service-call ordering, global side effects. Blockers: Signature/ABI, data layout, dependency semantics and original oracle remain unverified.
Next step: execute [TASK-P003](../../tasks/backlog/TASK-P003.md) after the
reference/boundary gates; bound the dependency closure before implementation.
Actual RE/C++/comparison time: not measured. Status: selected, not executed.

### P004 — 0x0019a4b8 (HP2_DemoCameraCoordinator_Update)

Finding: preexisting camera-oriented name; vector-distance loops and virtual calls; body/CFG corrupt. Confidence: low; evidence E-0002-03/05/06.
Data uncertainty: high (unrecovered object/buffer layouts). Verification difficulty: very_high.

Direct callees: none in current graph. 0 observed
callers; full relationships are in CALL_EDGES.csv. 0 decoded indirect
call sites in returned blocks; targets may be absent from the graph.

Required checks: repair body first; camera transitions and vector math state. Blockers: Function body is one byte; reconcile decompiler reachability and correct entry/body metadata before trusting CFG/call data.
Next step: execute [TASK-P004](../../tasks/backlog/TASK-P004.md) after the
reference/boundary gates; bound the dependency closure before implementation.
Actual RE/C++/comparison time: not measured. Status: selected, not executed.

### P005 — 0x001ebd20 (HP2_LoadFlightAreas)

Finding: preexisting flight-area name; magic test, offset arithmetic and aligned pointer fixups; one-byte body. Confidence: medium; evidence E-0002-03/05/06.
Data uncertainty: high (unrecovered object/buffer layouts). Verification difficulty: high.

Direct callees: none in current graph. 0 observed
callers; full relationships are in CALL_EDGES.csv. 0 decoded indirect
call sites in returned blocks; targets may be absent from the graph.

Required checks: repair body; malformed/valid synthetic buffers and relocation results. Blockers: Function body is one byte; reconcile decompiler reachability and correct entry/body metadata before trusting CFG/call data.
Next step: execute [TASK-P005](../../tasks/backlog/TASK-P005.md) after the
reference/boundary gates; bound the dependency closure before implementation.
Actual RE/C++/comparison time: not measured. Status: selected, not executed.

### P006 — 0x00120ef8 (FUN_00120ef8)

Finding: many COP2 operations, matrix transforms and buffer/packet construction; cc=233. Confidence: medium; evidence E-0002-03/05/06.
Data uncertainty: high (unrecovered object/buffer layouts). Verification difficulty: very_high.

Direct callees: 0x00296ec4, 0x001107e0, 0x0011b1b0, 0x00188180, 0x001164f0, 0x00110848, 0x001a9fb8, 0x0025aed0, 0x001107f8, 0x0025ac98, 0x001107d0, 0x00110a68, 0x001a9a30, 0x0025ae18, 0x0011b2c0, 0x001109f8, 0x0011ba18, 0x00107cd8, 0x00152070, 0x00120c00, 0x002596f8, 0x0010dc80, 0x00107600, 0x0011b248, 0x001074b8, 0x00120130, 0x00250288, 0x0025ac68, 0x00120d60, 0x0010ccb0, 0x001b50c8, 0x0011b300, 0x00124d38, 0x00110608, 0x0011d980, 0x0010bd38, 0x0010ca60, 0x00115978, 0x0010c8f0, 0x00110a40, 0x00120ea8, 0x001209f8, 0x00110488, 0x00110878, 0x0010caa0. 2 observed
callers; full relationships are in CALL_EDGES.csv. 0 decoded indirect
call sites in returned blocks; targets may be absent from the graph.

Required checks: buffer writes, VU math and captured frame output. Blockers: Signature/ABI, data layout, dependency semantics and original oracle remain unverified.
Next step: execute [TASK-P006](../../tasks/backlog/TASK-P006.md) after the
reference/boundary gates; bound the dependency closure before implementation.
Actual RE/C++/comparison time: not measured. Status: selected, not executed.

### P007 — 0x001796b0 (FUN_001796b0)

Finding: large complex function intentionally retained without speculative subsystem label. Confidence: unknown; evidence E-0002-03/05/06.
Data uncertainty: high (unrecovered object/buffer layouts). Verification difficulty: very_high.

Direct callees: 0x001ee390, 0x00140c68, 0x00140a48, 0x002577c0, 0x001ee3c0, 0x002534a8, 0x001babb8, 0x00144968, 0x00140a20, 0x001454e8, 0x00143d50, 0x001440d8, 0x00140728, 0x0014cbf8, 0x0016eb68, 0x00140ea8, 0x00140978, 0x001411d8, 0x00251398, 0x00178c90, 0x00107c90, 0x001449e8, 0x00144398, 0x00140e20, 0x001767a8, 0x00143dd8, 0x001badc8, 0x0017c888, 0x00182a48, 0x001434a0, 0x0017c7c8, 0x001445f0, 0x00144a28, 0x00206ab8, 0x00181558. 2 observed
callers; full relationships are in CALL_EDGES.csv. 0 decoded indirect
call sites in returned blocks; targets may be absent from the graph.

Required checks: identify purpose/dependency closure before behavior tests. Blockers: Signature/ABI, data layout, dependency semantics and original oracle remain unverified.
Next step: execute [TASK-P007](../../tasks/backlog/TASK-P007.md) after the
reference/boundary gates; bound the dependency closure before implementation.
Actual RE/C++/comparison time: not measured. Status: selected, not executed.

### P008 — 0x002514f0 (FUN_002514f0)

Finding: format specifier switch, numeric conversion and callback output; possible printf-like library. Confidence: medium; evidence E-0002-03/05/06.
Data uncertainty: medium (unverified global/ABI contracts). Verification difficulty: high.

Direct callees: 0x002a1b68, 0x00251398, 0x002a1ab0, 0x002a14d0, 0x002a1538, 0x002a1478, 0x002a3058, 0x00250b40, 0x00250fc8, 0x002a17e0, 0x002a1a60. 3 observed
callers; full relationships are in CALL_EDGES.csv. 8 decoded indirect
call sites in returned blocks; targets may be absent from the graph.

Required checks: formatting vectors and output callbacks; warnings and ABI review. Blockers: Signature/ABI, data layout, dependency semantics and original oracle remain unverified.
Next step: execute [TASK-P008](../../tasks/backlog/TASK-P008.md) after the
reference/boundary gates; bound the dependency closure before implementation.
Actual RE/C++/comparison time: not measured. Status: selected, not executed.

### P009 — 0x002577c0 (FUN_002577c0)

Finding: 255 callers; size/alignment-like arguments and helper 00257690; allocation wrapper hypothesis. Confidence: medium; evidence E-0002-03/05/06.
Data uncertainty: medium (unverified global/ABI contracts). Verification difficulty: high.

Direct callees: 0x0025d048, 0x00253520, 0x00257690. 255 observed
callers; full relationships are in CALL_EDGES.csv. 0 decoded indirect
call sites in returned blocks; targets may be absent from the graph.

Required checks: allocation/failure behavior and return ABI, logging side effects. Blockers: Signature/ABI, data layout, dependency semantics and original oracle remain unverified.
Next step: execute [TASK-P009](../../tasks/backlog/TASK-P009.md) after the
reference/boundary gates; bound the dependency closure before implementation.
Actual RE/C++/comparison time: not measured. Status: selected, not executed.

### P010 — 0x00250340 (FUN_00250340)

Finding: byte fill with aligned word loop; memset-like semantics observed. Confidence: high; evidence E-0002-03/05/06.
Data uncertainty: low (byte-buffer contract). Verification difficulty: low.

Direct callees: none in current graph. 90 observed
callers; full relationships are in CALL_EDGES.csv. 0 decoded indirect
call sites in returned blocks; targets may be absent from the graph.

Required checks: guarded buffers, zero length, all byte values, aligned/unaligned pointers. Blockers: Signature/ABI, data layout, dependency semantics and original oracle remain unverified.
Next step: execute [TASK-P010](../../tasks/backlog/TASK-P010.md) after the
reference/boundary gates; bound the dependency closure before implementation.
Actual RE/C++/comparison time: not measured. Status: selected, not executed.

### P011 — 0x00250ac0 (FUN_00250ac0)

Finding: shared diagnostic/output wrapper; exact formatting/error policy unresolved. Confidence: low; evidence E-0002-03/05/06.
Data uncertainty: medium (unverified global/ABI contracts). Verification difficulty: medium.

Direct callees: 0x00251150, 0x00250ab0. 82 observed
callers; full relationships are in CALL_EDGES.csv. 0 decoded indirect
call sites in returned blocks; targets may be absent from the graph.

Required checks: delegate-call order, output and failure behavior. Blockers: Signature/ABI, data layout, dependency semantics and original oracle remain unverified.
Next step: execute [TASK-P011](../../tasks/backlog/TASK-P011.md) after the
reference/boundary gates; bound the dependency closure before implementation.
Actual RE/C++/comparison time: not measured. Status: selected, not executed.

### P012 — 0x00285958 (FUN_00285958)

Finding: request structures plus CreateSema/WaitSema/DeleteSema and transport helper; RPC-like service. Confidence: high; evidence E-0002-03/05/06.
Data uncertainty: high (unrecovered object/buffer layouts). Verification difficulty: very_high.

Direct callees: 0x002833e0, 0x002833b0, 0x00284ec0, 0x00285388, 0x002833a0, 0x00285068, 0x002852e0. 89 observed
callers; full relationships are in CALL_EDGES.csv. 0 decoded indirect
call sites in returned blocks; targets may be absent from the graph.

Required checks: sync/async completions, failure codes, callbacks and ordering. Blockers: Signature/ABI, data layout, dependency semantics and original oracle remain unverified.
Next step: execute [TASK-P012](../../tasks/backlog/TASK-P012.md) after the
reference/boundary gates; bound the dependency closure before implementation.
Actual RE/C++/comparison time: not measured. Status: selected, not executed.

### P013 — 0x0026a310 (FUN_0026a310)

Finding: SOUND_EndCommand and audio-command/flush diagnostics reference this function. Confidence: high; evidence E-0002-03/05/06.
Data uncertainty: medium (unverified global/ABI contracts). Verification difficulty: high.

Direct callees: 0x002510a8, 0x00250f48, 0x002a3058, 0x0026ca40, 0x00250f38. 82 observed
callers; full relationships are in CALL_EDGES.csv. 0 decoded indirect
call sites in returned blocks; targets may be absent from the graph.

Required checks: command buffer framing, overflow/flush and timing; then audible comparison. Blockers: Signature/ABI, data layout, dependency semantics and original oracle remain unverified.
Next step: execute [TASK-P013](../../tasks/backlog/TASK-P013.md) after the
reference/boundary gates; bound the dependency closure before implementation.
Actual RE/C++/comparison time: not measured. Status: selected, not executed.

### P014 — 0x00282fa0 (SetGsCrt)

Finding: existing SetGsCrt symbol; syscall stub; external system semantics unresolved. Confidence: medium; evidence E-0002-03/05/06.
Data uncertainty: medium (unverified global/ABI contracts). Verification difficulty: high.

Direct callees: none in current graph. 1 observed
callers; full relationships are in CALL_EDGES.csv. 0 decoded indirect
call sites in returned blocks; targets may be absent from the graph.

Required checks: identify syscall/register contract and define host replacement observable effect. Blockers: Signature/ABI, data layout, dependency semantics and original oracle remain unverified.
Next step: execute [TASK-P014](../../tasks/backlog/TASK-P014.md) after the
reference/boundary gates; bound the dependency closure before implementation.
Actual RE/C++/comparison time: not measured. Status: selected, not executed.

### P015 — 0x001044c8 (FUN_001044c8)

Finding: matrix/vector operations over corner-like data used by camera-named entry; culling hypothesis. Confidence: low; evidence E-0002-03/05/06.
Data uncertainty: medium (unverified global/ABI contracts). Verification difficulty: high.

Direct callees: 0x00135170, 0x0025a610, 0x0025a540, 0x0011b300, 0x0011b248. 0 observed
callers; full relationships are in CALL_EDGES.csv. 0 decoded indirect
call sites in returned blocks; targets may be absent from the graph.

Required checks: geometric boundary vectors, float tolerances and side effects. Blockers: Signature/ABI, data layout, dependency semantics and original oracle remain unverified.
Next step: execute [TASK-P015](../../tasks/backlog/TASK-P015.md) after the
reference/boundary gates; bound the dependency closure before implementation.
Actual RE/C++/comparison time: not measured. Status: selected, not executed.

### P016 — 0x00131618 (FUN_00131618)

Finding: PhysicsCar parameter string and movement-mode dispatch; inherited comments are unverified. Confidence: medium; evidence E-0002-03/05/06.
Data uncertainty: high (unrecovered object/buffer layouts). Verification difficulty: high.

Direct callees: 0x001d8138, 0x00167a08, 0x00219628, 0x002577c0, 0x00202f60, 0x00137100, 0x001a0dd8, 0x00204098, 0x00257c80. 17 observed
callers; full relationships are in CALL_EDGES.csv. 1 decoded indirect
call sites in returned blocks; targets may be absent from the graph.

Required checks: mode constructors/destruction, movement-object layouts and state transitions. Blockers: Signature/ABI, data layout, dependency semantics and original oracle remain unverified.
Next step: execute [TASK-P016](../../tasks/backlog/TASK-P016.md) after the
reference/boundary gates; bound the dependency closure before implementation.
Actual RE/C++/comparison time: not measured. Status: selected, not executed.

### P017 — 0x00131da8 (FUN_00131da8)

Finding: CarCollision parameter string, delta-time vector calculations and side effects. Confidence: medium; evidence E-0002-03/05/06.
Data uncertainty: high (unrecovered object/buffer layouts). Verification difficulty: very_high.

Direct callees: 0x0013e270, 0x00152558, 0x002087f8, 0x0021eb20, 0x00135b30, 0x001769e8, 0x00176ab0, 0x00177c88, 0x00134198, 0x0021eb48, 0x0021ebd8, 0x001d6f38, 0x00208838, 0x001c0888, 0x0021eb90, 0x00178358. 1 observed
callers; full relationships are in CALL_EDGES.csv. 0 decoded indirect
call sites in returned blocks; targets may be absent from the graph.

Required checks: timestep/collision traces, contact edges, zero dt and COP2 precision. Blockers: Signature/ABI, data layout, dependency semantics and original oracle remain unverified.
Next step: execute [TASK-P017](../../tasks/backlog/TASK-P017.md) after the
reference/boundary gates; bound the dependency closure before implementation.
Actual RE/C++/comparison time: not measured. Status: selected, not executed.

### P018 — 0x0012f378 (FUN_0012f378)

Finding: registered startup callback; repeated timing, update calls and state-dependent orchestration. Confidence: medium; evidence E-0002-03/05/06.
Data uncertainty: medium (unverified global/ABI contracts). Verification difficulty: very_high.

Direct callees: 0x00250f38, 0x00169538, 0x0015ef28, 0x0014e768, 0x001003c0, 0x0014d678, 0x00156400, 0x00155d40, 0x001a35e0, 0x00112858, 0x001af4c0, 0x00181d88, 0x0012f088, 0x001a35f0, 0x0016d420, 0x00153e48, 0x00250f48, 0x00199bd0, 0x0024bdb8, 0x001a33f0, 0x001680b8, 0x0014dcd8, 0x0013e008, 0x001680e8, 0x0012f190, 0x0014d6e0, 0x00181c28, 0x001ec858, 0x001ac0a0, 0x00149dd8, 0x0013d880, 0x0014d718, 0x0012f090, 0x00149650, 0x001a3500, 0x0013e078, 0x0014e5c8, 0x00199788, 0x00149578. 0 observed
callers; full relationships are in CALL_EDGES.csv. 0 decoded indirect
call sites in returned blocks; targets may be absent from the graph.

Required checks: frame/input trace and update-order/time-step comparisons. Blockers: Signature/ABI, data layout, dependency semantics and original oracle remain unverified.
Next step: execute [TASK-P018](../../tasks/backlog/TASK-P018.md) after the
reference/boundary gates; bound the dependency closure before implementation.
Actual RE/C++/comparison time: not measured. Status: selected, not executed.

### P019 — 0x00131410 (FUN_00131410)

Finding: AIDriver allocation label and controller-mode switch/virtual destruction. Confidence: medium; evidence E-0002-03/05/06.
Data uncertainty: high (unrecovered object/buffer layouts). Verification difficulty: high.

Direct callees: 0x0023a3d8, 0x001c0090, 0x001a1ce8, 0x002577c0, 0x0012ffc8, 0x0021b918, 0x0018c380, 0x0021bbb0. 5 observed
callers; full relationships are in CALL_EDGES.csv. 1 decoded indirect
call sites in returned blocks; targets may be absent from the graph.

Required checks: controller modes, AI object layout, allocation and virtual-call traces. Blockers: Signature/ABI, data layout, dependency semantics and original oracle remain unverified.
Next step: execute [TASK-P019](../../tasks/backlog/TASK-P019.md) after the
reference/boundary gates; bound the dependency closure before implementation.
Actual RE/C++/comparison time: not measured. Status: selected, not executed.

### P020 — 0x00281f58 (FUN_00281f58)

Finding: libpad port/alignment diagnostics and RPC-backed pad-port initialization. Confidence: high; evidence E-0002-03/05/06.
Data uncertainty: high (unrecovered object/buffer layouts). Verification difficulty: high.

Direct callees: 0x00285958, 0x00284970, 0x00296ec4. 1 observed
callers; full relationships are in CALL_EDGES.csv. 0 decoded indirect
call sites in returned blocks; targets may be absent from the graph.

Required checks: aligned buffers, port/slot/error responses, request layout and input traces. Blockers: Signature/ABI, data layout, dependency semantics and original oracle remain unverified.
Next step: execute [TASK-P020](../../tasks/backlog/TASK-P020.md) after the
reference/boundary gates; bound the dependency closure before implementation.
Actual RE/C++/comparison time: not measured. Status: selected, not executed.


