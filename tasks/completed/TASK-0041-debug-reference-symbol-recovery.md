# TASK-0041 - Recover retail names using the authorized PS2 debug reference

- Status: completed
- Objective: extract debug class/function names and establish conservative debug-to-retail mappings, then adopt supported retail names.
- Scope: user-supplied DEBUG_VER/NFSHP2.ELF and NFSHP2.MAP; static symbols, type metadata, instruction/call/data correspondence; no inferred behavior changes.
- Authorization: the user explicitly supplied this debug reference on 2026-10-17. It supplements the retail-only evidence restriction for this task; debug names alone do not establish retail identity.
- Retail SHA-256: 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2; executable revision unknown.
- Debug ELF SHA-256: 815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
- MAP SHA-256: 20baece850349d26f95a5da1c1f6e9eacd6c3f5574e560b8ff2f4e536f9f19d1.
- User describes the reference as SLUS-20362, 2002-07-08 Debug Build; verify date separately.
- Preserve pre-existing dirty work and read-only Ghidra policy. Proprietary input and raw dumps stay outside Git or ignored local/.

## Acceptance criteria

- [x] Inventory symbols, classes and available debug metadata with exact provenance.
- [x] Reproducibly compare retail instructions obtained through Ghidra with reference functions.
- [x] Separate supported mappings, ambiguous matches, changed functions and unknowns.
- [x] Adopt supported names without changing behavior, preserving original addresses and reference evidence.
- [x] Validate affected code and records; document remaining coverage and continuation.

## Current findings

E-0041-I01: reference ELF32 little-endian MIPS includes .symtab, .strtab and a 25,935,148-byte .mdebug section. There are 8715 nonzero-sized STT_FUNC entries at distinct addresses. The MAP includes demangled PhysicsCar and RigidBody methods. This is substantially stronger reference evidence than retail's dead diagnostic strings.

## Next steps

The scoped pass is complete. Review candidate-only source rows and ambiguous table groups using named caller/slot evidence; keep all cross-build differences explicit.

## Related Git commits

None.


## Outcome and evidence

E-0041-I01/T01/M01/V01/R01..R20: 8715 reference functions, 1284 named aggregate
reference types, 4410 unique normalized code candidates, 49 unique complete table
pairs, 20 reviewed principal/class/packet bridges. Adopted 940 static retail names;
699 previously unindexed addresses gained Identified records. Existing behavior
statuses and all retail algorithms remain unchanged; no Verified promotion.

Source changes: 55 same-class member names, 6 principal projection function names,
10 former Type_ declarations, exact cDBFileSys class spelling, and 4 projected packet
methods split between original eStripData/eStripDataE scopes. All 11 former Type_
class/struct declarations have a reviewed original spelling or the explicit split.
Constructors/destructors, shared bases, folded symbols and placeholder overload
conflicts retain address entry names where appropriate; exact reference names are
still recorded. Reference signatures and layouts are not transplanted.

The original 562-identifier audit has 191 adopted static names, 175 candidate-only
and 196 unresolved/ambiguous rows. See reverse/naming/task0041-source-coverage.csv.
Remaining address tokens can also denote explicitly projected/inherited entries.
The scoped reference extraction/matching/adoption task is complete; every retail
name and layout is not claimed recovered. Further candidates need a separate
reviewed naming bridge, not a blanket normalized match acceptance.

## Validation and limitations

2026-10-17: Python 3.14.4; CMake/CTest 3.29.2; GCC 13.2.0 in the configured build;
godot-cpp pin e4b7c25e721ce3435a029087e3917a30aa73f06b. Ghidra MCP application
version not exposed by queried metadata; target language MIPS-R5900 LE32.

- SHA checks and live retail Ghidra capture equality: passed for all 5 captured initialized sections.
- `py -3 reverse/scripts/extract_debug_reference_types.py --self-test`: 4 passed.
- `py -3 -m unittest discover -s reverse/scripts -p test_debug_reference_matching.py -v`: 13 passed.
- `py -3 reverse/scripts/match_debug_vtables.py --self-test`: 3 passed.
- `py -3 -m unittest discover -s reverse/tests -p test_class_templates.py -v`: 8 passed.
- `py -3 reverse/scripts/collect_class_source_paths.py --report`: regenerated after exact class spelling correction; 9 prior confirmed source directories preserved.
- `py -3 reverse/scripts/generate_class_templates.py --check`: passed.
- `py -3 reverse/scripts/generate_progress.py --check`: passed.
- `cmake --build --preset native-debug --parallel 4`: passed.
- `cmake --build --preset godot-debug --parallel 4`: passed, including extension DLL.
- `ctest --preset native-debug --output-on-failure`: final 51/51 passed, 43.10 seconds.
- `ctest --preset godot-debug -R 'class_template|debug_reference|progress_tables|vehicle_geometry|frontend_(exit|scroll|viewport)|eventtree|driving_(mover|drivetrain|body)' --output-on-failure`: 15/15 passed, 3.87 seconds.
- `git diff --check`: passed; existing line-ending warnings only.
- Earlier class_template_generator failed because the source-path ledger still used
  CDBFileSys. Regenerating that evidence report corrected it; final native and Godot
  tests passed. A transient Windows write error while reopening an unchanged header
  was corrected by avoiding unchanged writes; repeated adoption/check is stable.
- No full Godot suite, visual comparison or original gameplay equivalence claim.
- No proprietary binaries, MAP contents, raw STABS or byte dumps added to Git;
  captures remain ignored local/task0041. No Ghidra mutations and no commit made.

## Reproducible continuation

Read docs/DEBUG_REFERENCE_SYMBOLS.md. Run its exact extraction/matching/adoption
commands with the authorized debug ELF/MAP and configured retail ELF; --capture
requires the read-only Ghidra connection. Start a follow-on naming review with
TASK-0041 source-coverage candidate rows and ambiguous table groups. In particular,
keep Move/SetDesiredGear's changed field offsets and LanguageOptionsScreenBase's
added retail logic; never copy debug structures wholesale. No missing prerequisite
blocks reuse on this machine; another machine needs the proprietary inputs and
local configuration, which are deliberately untracked.
