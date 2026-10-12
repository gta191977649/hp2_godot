# TASK-0008 — Police AI and movement map

Read-only Ghidra MCP investigation, 2026-10-11 Australia/Sydney. Binary
`D:/ps2_game/SLUS_203.62`, SHA-256
`7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`.
USA/NTSC inferred; exact executable revision unknown; boot VER 1.00.
Ghidra 12.0.4, `r5900:LE:32:default`, default compiler, MCP at
`http://127.0.0.1:8080/mcp`. No live names, types, boundaries or analysis changed.
All original identifiers remain unknown; no function is Verified.

## Evidence-backed entry points

| Address | Current Ghidra name / proposed purpose | Evidence and confidence |
|---|---|---|
| 0x00131410 | HP2_SetDriverControllerMode | Mode 4 allocates 0x4C bytes with `CopDriver` label 0x002AE4E8 and calls 0x001A1CE8; stores controller at car+0x28. High. E-0008-01 |
| 0x00131618 | HP2_SetCarMovementMode | Mode 5 allocates 0x80 bytes with `CopMover` label 0x002AE560 and calls 0x001A0DD8; stores mover at car+0x30. High. E-0008-01 |
| 0x001A1CE8 | FUN_001a1ce8 / HP2_CopDriverConstruct | Constructor writes vtable 0x002B5998 at this+8, initializes state and routing object. High role confidence. E-0008-02 |
| 0x001A0DD8 | FUN_001a0dd8 / HP2_CopMoverConstruct | Constructor writes vtable 0x002B5A08 at this+4, stores car at this+8, copies initial movement fields. High role confidence. E-0008-02 |
| 0x001A2110 | FUN_001a2110 / HP2_CopDriverThink | CopDriver vtable+0x24 points here. Requires car movement mode 5, reads pursuit record and target car, switches controller state, computes aim/heading and desired speed in mover+0x30. High role confidence; exact prototype unknown. E-0008-03 |
| 0x001A1030 | FUN_001a1030 / HP2_CopMoverMove | CopMover vtable+0x14 points here. Pseudocode reads car, integrates motion, queries terrain and updates orientation/wheels. Strong role candidate, **one-byte defined body blocks reliable translation**. E-0008-04 |
| 0x001A1E90 | FUN_001a1e90 / HP2_CopDriverChooseAimPoint | Called by Think; selects target/route points using pursuit-record mode. Medium purpose confidence. E-0008-05 |
| 0x001A2A90 | FUN_001a2a90 / HP2_CopDriverCheckLowSpeedTimeout | Called by Think; in states 2/3 accumulates time when car+0x3C4 < 2, returns flag above 1; states 0/1/4 reset timer. High observation confidence; interpreting this as stuck recovery remains a hypothesis. E-0008-05 |
| 0x001A2C30 | FUN_001a2c30 / HP2_RouteSelectLookAheadPoint | Shared helper projects onto route segments and selects/interpolates a forward point, with lateral offset/error. Medium purpose confidence; parameter types/preconditions unresolved. E-0008-05 |
| 0x001A0EA0 | FUN_001a0ea0 / HP2_CopMoverSelectTypeCoefficient | Sets mover+0x3C to 11.25, 12.75, 15 or 14.25 according to car type codes 4/31/32/33/34. High observation confidence; physical meaning/units unknown. E-0008-05 |

Proposed HP2_ names in this table are analysis labels, **not applied names or
recovered original symbols**. Constructors, allocation labels and virtual slots
tie the functions to police vehicles. `CopDriver::Think()` at 0x002B58D8 and
`CopMover::Move()` at 0x002B57F0 are surviving diagnostic text, but have no existing
Ghidra references; they corroborate terminology, not an exact symbol-to-address proof.
Virtual-table extent and adjustment conventions still require an ABI audit.

## What can be recovered now

Private pseudocode: `ps2_decomp/police-ai/<eight-digit-address>.ghidra.c` with
matching JSON function metadata, bounded listing and references. Think is 2,432
defined bytes. Existing PS2Recomp instruction translation:
`ps2_decomp/recompiled-named/FUN_001a2110_0x1a2110.cpp`.
Other generated filenames can use `sub_` rather than `FUN_`; exact paths are in
`ps2_decomp/police-ai/validation.json`.

These are useful analysis/translation artifacts, not restored original source.
Readable native C++ needs object layouts, register/float ABI, state enums, route
types and external-call contracts. Numeric states are retained as numbers: this
investigation does not assign patrol/chase/arrest meanings. No gameplay logic was
implemented and no reconstructed-behavior tests were appropriate for this map.

Move's generated file `FUN_001a1030_0x1a1030.cpp` contains an unresolved TODO.
The decompiler nevertheless traverses hundreds of pseudocode lines; that is not
proof of a correct body. `move-linear-listing.json` is a read-only investigation
window 0x001A1030–0x001A1CE0, not a proposed function boundary. The vtable's
0x001A0E88 pointer has no defined function; saved gap listing does not justify
creating one. Both require evidence-led boundary repair in a separate task.

## World orchestration remains unresolved

`World::DoTimestep - CopCarThink` survives at 0x002AFDC8, without Ghidra xrefs.
A coarse LUI/ADDIU scan suggested 0x0012E050, but listing disproves that match:
`a1` is overwritten by `lui a1,0x33` at 0x0012E048, so ADDIU uses 0x33 rather
than an earlier 0x2B. Do not label 0x0012DEE0 as police world update from that hit.
The world dispatch caller and full pursuit-record ownership remain unresolved.
Serialization/debug-field-like functions 0x001A0F20 and 0x001A2B58 repeat class
labels; their exact runtime purpose is not established.

## Evidence and validation

E-0008-01: TASK-0007 saved mode-dispatch probes plus `string-refs.json`.
E-0008-02: constructor probes and disk ELF `vtable-words.json` (32-word windows
include adjacent data; no claimed table extent).
E-0008-03: `001a2110.json`, constructor and vtable chain.
E-0008-04: `001a1030.json`, `move-linear-listing.json`, triage exclusion by address.
E-0008-05: respective helper probes and Think direct calls.
All private evidence is under `ps2_decomp/police-ai/`; validation records hashes.

Reproduce from repository root:

```powershell
py -3 reverse/scripts/inspect_police_ai.py 001a1ce8 001a0dd8 001a2110 001a1030 001a1e90 001a2a90 001a2c30 001a0ea0
py -3 reverse/scripts/validate_police_ai.py
```

Validation: eight captured/live function metadata matches; unchanged ELF hash;
seven representative non-placeholder C++ syntax checks with GCC 13.2.0,
`-std=c++20 -msse4.1 -mavx -fsyntax-only` and PS2Recomp runtime/Kernel headers.
Exact argument arrays, tool versions, exit codes and evidence hashes are private
`validation.json`. Move check is explicitly blocked, not counted as a pass.
No build/link/boot or original/PCSX2 comparison. No rename/rollback changes or
tools introduced, so TASK-0007 naming synthetic tests need not be rerun.

Next: scope a police boundary/ABI audit for 0x001A1030 and 0x001A0E88; compare
branch/return paths and virtual dispatch before repairing. Then recover a small
typed helper with native tests and original execution traces. Broader AI requires
target/route structures and an original behavior oracle; exact developer names
remain unavailable without additional symbol-bearing material.
