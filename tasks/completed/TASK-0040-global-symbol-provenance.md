# TASK-0040 - Global address-anchored symbol provenance audit

- Status: completed
- Objective: inventory every Sub_<ADDR>/Type_<ADDR> in game_core and determine which original names are supported.
- Scope: read-only PS2 Ghidra symbol checks; existing allocator/vtable/diagnostic evidence; original game data naming leads. No behavior reconstruction or speculative renaming.
- Target: SLUS_203.62, executable revision unknown, SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
- Dependencies: TASK-0019, TASK-0027, TASK-0037; tools/ghidra.py.
- Preserve all pre-existing dirty work. Ghidra database edits are prohibited by AGENT.md R5.

## Acceptance criteria

- [x] Inventory source identifiers globally with reproducible coverage.
- [x] Check live symbols and existing original-name bridges, retaining ambiguity.
- [x] Record adoptable names and unresolved names separately; apply only supported renames.
- [x] Record validation and exact continuation in durable documentation.

## Verification procedure

Run the audit script, generated progress check, class template coverage and diff whitespace check.
No behavioral changes are planned; native behavioral tests are unnecessary unless scope changes.

## Current findings

Startup: original game directory contains ZZDATA; configured ELF is outside GAME at D:/ps2_game/SLUS_203.62. Ghidra program SLUS_203.62 is open (7544 functions). nfshp2.exe is also in the project and is not evidence for PS2 names. Most new Sub_ header files contain port-side projections and free functions, not original class definitions.

## Unresolved questions

Exact method spellings cannot be inferred from virtual membership, allocator labels or physical proximity.

## Next steps

Reproduce with `py -3 reverse/scripts/audit_symbol_provenance.py --live`. A subsequent recovery task must trace a concrete allocator/pool/object or live diagnostic bridge; missing exact names are unresolved findings, not permission to guess.

## Related Git commits

None.


## Outcome and completion evidence

E-0040-N01..N03: public ledger has 562 identifiers/addresses; 392 DEFAULT symbols,
1 USER_DEFINED analyst label, 169 unresolved symbol-object lookups. Existing
class joins attribute 357 entries and 51 constructor associations; zero qualified
label matches. Twelve root driving entry groups were decompiled/xref checked.
The bounded runtime RTTI region and two PhysicsCar method-label xrefs yield no
new adoptable spelling. No source rename or behavioral implementation is justified.
Full observations, counterevidence and limitations are in
reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/task0040-global-symbol-provenance.md.

2026-10-17 validation on Python 3.14.4, CMake/CTest 3.29.2; Ghidra MCP program
SLUS_203.62, MIPS-R5900 little-endian, 7544 functions; Ghidra application version
not exposed by the queried metadata. godot-cpp pin matches e4b7c25e721ce3435a029087e3917a30aa73f06b.

- `py -3 reverse/scripts/audit_symbol_provenance.py --live`: passed; SHA matched;
  expected missing symbol lookups are explicit ledger findings, not transport failures.
- `py -3 reverse/scripts/generate_progress.py`: updated log contents.
- `py -3 reverse/scripts/generate_progress.py --check`: passed.
- `ctest --preset native-debug -R 'class_template_coverage|progress_tables' --output-on-failure`: 2/2 passed, 0.84 seconds.
- `git diff --check`: passed; existing line-ending warnings only.
- No C++/Godot behavior changed, so no new behavior tests or native rebuild required.
- Raw responses remain ignored under local/task0040; no proprietary payload added.
- No commit made; pre-existing work preserved. No Verified promotion.

The scoped global provenance audit is complete. Whole-image call-graph recovery,
asset-payload symbol searches and original method names without naming bridges remain
open and were not claimed as completed.
