# TASK-0009 — Whole-binary recovery, first pass

This task remains **active**. A complete export inventory is available; restoring
every original function, class, name and behavior is not complete. The user
confirmed that only the current ELF is available. Its symbol count is zero and it
has no debug sections; no trustworthy original function names have been recovered.
Surviving class/method text and analysis-purpose names are recorded separately.

Binary: `D:/ps2_game/SLUS_203.62`, SHA-256
`7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`;
USA/NTSC inferred; executable revision unknown; boot VER 1.00.
Ghidra 12.0.4, `r5900:LE:32:default` / default, read-only MCP.

## Exhaustive inventory

`reverse/recovery/TASK-0009-functions.csv` tracks all 7,544 entries, observed
envelopes, actual Ghidra names, unknown original names/signatures, proposed purpose
names, class associations, pseudocode/translation paths and native implementation
status. One private JSON checkpoint per address supports resuming and rejects
changed metadata or binary identity. Five-second decompile timeout failures are
preserved, never relabeled as successful recovery.

Private export location: `ps2_decomp/full-recovery/functions/` and `pseudocode/`.
The first complete pass exports 7,516 pseudocode functions and records 28 boundary
blockers. Pseudocode is unverified C-like text, not compilable restored native C++.
Before/after full function metadata and ELF hashes agree.

`TASK-0009-class-hints.csv` records 118 class/method text hints covering 49 distinct
class texts, e.g. Car, World, CopDriver, CopMover and rendering/audio classes.
Text occurrence is not proof of a function address, inheritance or complete class
definition. Class membership is assigned only for the supported police chain.

## First native bodies and partial classes

`src/game_core/police/cop_control.hpp/.cpp` adds two storage-view classes,
`hp2::recovered::CopDriver` and `CopMover`, independent of Godot. These are analysis
interfaces over existing 0x4C/0x80-byte PS2 images, not recovered original C++
inheritance, constructors, virtual layout or host objects. Original fields and
types outside the documented subset remain unknown. Layout knowledge:
`reverse/structures/CopDriver.md`, `CopMover.md`.

- E-0009-01 / 0x001A0EA0: `CopMover::select_type_coefficient`. Reads car/config/type
  through an explicit 32-bit memory adapter and writes exact coefficient bits.
  Disk switch table 0x002B5760 covers type codes 4–34: 4→11.25, 31→12.75,
  32/33→15, 34→14.25. Other types leave the field unchanged; units unknown.
- E-0009-02 / 0x001A2A90: `CopDriver::check_low_speed_timeout`. Calls opaque external
  0x00231258 first; states 0/1/4 reset the timer, 2/3 accumulate when speed < 2,
  returning true strictly above timer 1. Other states leave it unchanged. States
  remain numeric. External predicate behavior and side effects are adapter
  obligations. LWC1/ADD.S/SWC1 prove that the timer is float bits, correcting
  misleading integer conversions in the Ghidra pseudocode.

Native tests cover 262 coefficient/type/address-wrap cases, 64
state/predicate/speed cases, strict threshold and accumulated float storage;
complete byte-image comparison catches unintended writes, and mocks check memory
read order and external call count. This is Tested local logic only. Original
behavior comparison, exceptional PS2 floats/denormals, adapter implementation and
valid-memory preconditions remain unresolved. Zero Verified functions.

## Move boundary candidate and separate generation

E-0009-04: raw disk branch-topology audit from 0x001A1030 reaches 743 instruction
words with no unresolved branch/jump path inside the candidate window. Entry
`addiu sp,sp,-0x210`; return at 0x001A1BC4; delay slot at 0x001A1BC8 restores 0x210.
0x001A1BCC is unreachable trailing padding in this audit. This supports the
candidate inclusive body end 0x001A1BCB. It does not validate every instruction,
ABI, ownership, exception, float or external-call behavior. No live repair made.

Private `police-flow-audit.json` preserves topology and limitations. Undefined
vtable entry 0x001A0E88 reaches a jump to 0x00136B78; it is not a standalone
returning-body proof and remains blocked pending a tail-call/ownership audit.
The old MCP listing window was capped at 900 code units and includes undecoded
bytes; it is insufficient as a full disassembly. The new audit reads disk words.

A separate candidate map overrides only Move's end and six police purpose names;
manifest records old names/ends and all changes. Connected Ghidra remains unchanged.
`ps2_decomp/full-recovery/recompiled-candidate/` contains 7,517 translations,
27 placeholders, registration and headers. Move now has a 149,170-byte instruction
translation instead of a TODO. This is not a readable native Move implementation.
Prior named/triage outputs and original 28-exclusion config remain untouched.

E-0009-03 identifies disk class/method text extraction. E-0009-05 identifies
checkpoint integrity, generated syntax and native/synthetic tests. Hashes and exact
compiler argument arrays: private `validation.json` and generation manifest.

## Reproduction and checks

```powershell
py -3 reverse/scripts/export_full_recovery.py --workers 4
py -3 reverse/scripts/catalog_full_recovery.py
py -3 reverse/scripts/audit_police_flow.py
py -3 reverse/scripts/generate_police_candidate.py
py -3 reverse/scripts/validate_full_recovery.py
py -3 -m unittest discover -s reverse/scripts/tests -v
cmake --preset native-debug
cmake --build --preset native-debug
ctest --preset native-debug
```

Use `--retry-failed` to retry recorded decompile failures. `--generate` on the
candidate generator invokes the built PS2Recomp binary, refusing to overwrite an
existing candidate output directory. The current output was generated with that
same config/executable; its log ends with recompilation completed successfully.

Passed: eight synthetic tests (five export safety, three raw topology); native configure/build and CTest 4/4;
full export/catalog identities and per-pseudocode SHA-256; seven generated C++
syntax probes (six police entries including full Move plus registration).
Tools: Python 3.14.4, GCC 13.2.0, CMake 3.29.2, existing PS2Recomp 0.4 build.
No full generated link/boot or original/PCSX2 comparison. No Ghidra mutation,
no live-name rollback needed, no Git commit made. Existing TASK-0005 preserved.

## Exact continuation

TASK-0009 remains active: audit Move with the R5900 disassembler/pcode, review
call/return registers and virtual dispatch, snapshot the project, then make a
scoped journaled boundary/name/type update if evidence remains consistent. Recover
its native movement body only after the car/terrain/vector and external-call
contracts are established. Next trace each class-text hint through actual
references, constructors, vtables and callers, recording exceptions and ownership;
process every inventory row through the same evidence gates. SDK/libc matching
and original execution traces are additional prerequisites. An original name that
was erased and has no surviving evidence must remain unknown, even at completion.
