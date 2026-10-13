# TASK-0028 — Evidence-driven C++ class and function templates

- Status: completed
- Objective: materialize confirmed class names and attributed member addresses as C++17 templates, preserving existing implementations.
- Scope: class/member coverage, confirmed identifier corrections, provenance, build integration and template tests. No behavioural reconstruction, inferred inheritance/layout, or Ghidra mutations.
- Original PS2 addresses: `reverse/classes.csv` and confirmed original-method mappings; SLUS_203.62, executable revision unknown, SHA-256 `7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`.
- Dependencies: TASK-0027, TASK-0019, TASK-0020; AGENT.md; read-only Ghidra MCP.
- Relevant files: `src/game_core`, `reverse/classes`, `reverse/scripts`, `reverse/symbols.csv`, `docs/CLASS_TEMPLATES.md`.
- GitHub issue: not created

## Acceptance criteria

- [x] Every valid confirmed class name has a declaration or a preserved existing class.
- [x] Every attributed member is covered by an existing implementation or an address-bearing throwing template.
- [x] Invalid C++ allocation labels are catalogued verbatim, without invented type names.
- [x] Existing behaviour is preserved; confirmed names are used and unknown names remain address-anchored.
- [x] Coverage inventory distinguishes partial implementations, placeholders and unbound names, with evidence and source links.
- [x] All templates compile without Godot; coverage and exception tests pass.
- [x] Relevant native and Godot checks and limitations are recorded; generated progress is current.

## Verification procedure

Run template generator check, native configure/build/CTest, and Godot build if names used by callers change. Record exact commands and versions below. No original comparison is claimed.

## Current findings

- E-0027-C01: 175 allocator labels; 152 are valid C++ identifiers and 23 contain spaces/parentheses. The user selected catalogue-only handling for those 23 labels.
- 117 allocator-labelled records have no bound functions. Do not infer constructors from the first call after allocation.
- Unknown paths use `port/class_templates`; unknown signatures use explicitly port-side `void Sub_<ADDR>()` placeholders.
- E-0019-N03 wins over the conflicting TASK-0027 diagnostic interpretation: 002584B0 is `SlotPoolManager::NewSlotPool`, not its constructor; `bFile::Filename` is a field expression.
- Final inventory: 190 class names, 609 member attributions / 370 distinct addresses, 603 throwing templates, six preserved existing native projections, and 134 unbound class names. Twelve templates also link separate preserved partial projections.
- Added 343 Identified index rows and 342 new function records; one previously unindexed existing record was preserved and indexed. Original index rows, including seven pre-existing duplicate-address rows, were retained rather than silently collapsed. No existing status changed.
- E-0028-C01..C06 and the new record/index/inventory links preserve identity and provenance. `TitleScreen` and `tEventLeaf` replace the independently bound address-named projections; unknown animation-owner names remain unresolved.

## Unresolved questions

Original signatures, inheritance, fields and file attribution remain unknown unless supported by individual records. Attributed virtual targets may be shared/inherited, not exclusive class methods. Native tests do not establish PS2 equivalence.

## Next steps

Implementation task complete. For a new recovery task, select an inventory member, read its
record, inspect the original address read-only, and replace its signature/body in place only
after evidence is recorded. Run the generator with `--write`, its `--check`, and progress
generation. Behavioural tests and original comparisons are separate from template tests.

Default Godot deployment remains subject to the running process releasing `godot/bin/ps2_port.dll`.
No process was terminated and the existing loaded DLL was not replaced. Normal follow-up:
`cmake --build --preset godot-debug --parallel 8` after the DLL is released.

## Related Git commits

None; no commit has been made.

## Completion evidence

2026-10-13 Australia/Sydney. CMake 3.29.2, Ninja 1.12.0, GCC 13.2.0 (Strawberry),
Python 3.14.4; godot-cpp pin `e4b7c25e721ce3435a029087e3917a30aa73f06b` preserved.
`Get-FileHash -Algorithm SHA256 D:/ps2_game/SLUS_203.62` matched the full registered hash.

- Passed: `py -3 reverse/scripts/generate_class_templates.py --write` and `--check`.
- Passed: `py -3 -m unittest discover -s reverse/tests -p test_class_templates.py` (4 tests).
  Tests include idempotence, preservation of edited bodies/unindexed records/legacy rows,
  coverage, and rejection of comment/string text as implementation evidence.
- Passed: `cmake --preset native-debug`; `cmake --build --preset native-debug --parallel 8`.
- Passed: `ctest --preset native-debug`, 28/28 tests. `class_template_gaps` checks all 603
  throwing methods for exception type and exact address/member provenance.
- Passed: read-only body audit against Git HEAD, 501 existing bodies preserved after documented
  identifier substitutions; all 368 original index status rows unchanged.
- Initial failure, resolved: the inventory included changing progress counters for the 23
  catalogue-only labels; after `recover_classes.py --emit` refreshed them, coverage detected
  a stale inventory. Catalogue fields are now attribution-only and final checks pass.
- Default Godot link blocked: `cmake --preset godot-debug` succeeded and bridge sources
  compiled; `cmake --build --preset godot-debug --parallel 8` could not write the loaded
  `godot/bin/ps2_port.dll` (Permission denied). No compiler error or process termination.
- Passed isolated Godot link: configure with
  `cmake --preset godot-debug -DCMAKE_PROJECT_PS2PCPort_INCLUDE=D:/dev/hp2_godot/local/task0028/isolated_output.cmake`,
  then `cmake --build --preset godot-debug --parallel 8`. The ignored include defers output
  properties to `local/task0028/godot-bin`; resulting DLL linked successfully. Restore the
  normal build configuration with `cmake --preset godot-debug -U CMAKE_PROJECT_PS2PCPort_INCLUDE`.
- Passed: `ctest --preset godot-debug -R 'class_template_|police_control|movieplayer|frontend_viewport'`, 6/6 tests.
- Passed: `py -3 reverse/scripts/recover_classes.py --emit`, progress regeneration/check,
  and `git diff --check`. Final source/status audit contains no restricted binary/asset files.

No original hardware/PCSX2 comparison or live replacement of the running frontend was performed.
Templates do not establish behaviour, layouts, inheritance or original ABI. No commit created.
