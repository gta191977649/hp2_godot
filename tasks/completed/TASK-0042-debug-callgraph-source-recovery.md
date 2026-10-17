# TASK-0042 - Debug call-chain correspondence and source structure recovery

- Status: completed
- Objective: resolve as many as evidence permits of TASK-0041's 175 candidates and 196 unresolved source identifiers; adopt confirmed names and correct CPP/HPP organization.
- Scope: both authorized PS2 ELF images, Debug Ghidra NFSHP2.ELF, named caller/callee and complete vtable evidence, reference source ownership; source names/files/includes/build metadata and durable records. No guessed identifiers or behavior transplantation.
- Retail SHA-256: 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2; executable revision unknown.
- Reference SHA-256: 815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b; user label 2002-07-08 Debug Build, SLUS-20362.
- MAP SHA-256: 20baece850349d26f95a5da1c1f6e9eacd6c3f5574e560b8ff2f4e536f9f19d1.
- Dependencies: TASK-0040/0041 ledgers and ignored SHA-bound Ghidra retail capture.
- Startup: clean worktree at e6f300a; both SLUS_203.62 and NFSHP2.ELF are open in Ghidra. Confirm Debug memory identity before relying on live results. Read-only Ghidra policy remains binding.
- Authorization: user explicitly requests Debug-reference continuation, evidence-backed direct source adoption, CPP/HPP renames and correction/merging of incorrect structure. Preserve all original addresses, native projections and throwing whole-entry gaps.

## Acceptance criteria

- [x] Revisit every one of the 371 scoped rows with evidence or an explicit bounded unresolved disposition.
- [x] Use live Debug Ghidra symbols/decompilation/references and static call/vtable evidence; separate supported mappings from normalized-only candidates and folded ambiguity.
- [x] Adopt confirmed symbols and correct CPP/HPP filenames, original owner grouping and references where established; no guessed original hierarchy, ABI or file paths.
- [x] Keep generators, symbol index, records, source ownership and reproducible continuation coherent.
- [x] Build affected native/Godot targets and run relevant tests; document exact results and limits.

## Current findings

Baseline TASK-0041: 940 adopted retail names; source coverage 191 adopted, 175 candidate-only, 196 unresolved/ambiguous. Some address-named files still group multiple original classes, and current whole-entry shells intentionally coexist with typed host projections. Names alone do not grant native-layout equivalence.

## Next steps

This bounded pass is complete. Continue a separately scoped recovery task from
the 111 candidate and 175 unresolved dispositions, requiring new independent
caller or table context. Regenerate metadata using the commands below.

## Related Git commits

None for TASK-0042.


## Bounded results

- Revisited all 175 candidates/all 196 unresolved with explicit rows in the two
  task0042 disposition CSVs. Supported 64 C + 21 U; unsupported 111 C + 175 U remain
  address identifiers. Eight U rows retain folded/signature ambiguity.
- Adopted ledger 1025 names total; historical 562-source audit now 276 adopted,
 111 candidate-only, 175 unresolved. TASK-0041 ledgers remain unchanged.
- Live Debug full.text matches authorized ELF, SHA 651589b26daef1846eb067d308762583323e3e2c260a5c524de4ca2421580610.
  Bridge JSON retains caller/table roots, actual changed-body counterevidence,
  source offsets and limitations. Raw captures stay ignored.
- 193 class file mappings,203 classes/578 members,19 new exact method-shell names;
  wrong base/derived ownership corrected without invented inheritance.
- 14 address-grouped CPPs partitioned into 19 files; 151 entry/exported functions and
 12 anonymous helper chunks/47 helpers preserved.68 proved token substitutions;
  constructors/destructors/unrepresentable flattened collisions retain addresses.
  Exact source paths use ECOFF; unknown companion headers remain port-side.
- All 1597 existing index rows/statuses retained in order, including existing
  duplicate address 0x00249AE8 rows.5 new rows are Identified only. No behavior promotion.
  Original behavioral code and whole-entry throw gaps remain.

## Validation

Tools: Python 3.14.4, CMake 3.29.2, MinGW GCC 13.2.0; configured pinned Godot
bindings and existing GhidraMCP endpoint. Exact commands:

```powershell
py -3 reverse/scripts/adopt_task0042_names.py --write
py -3 reverse/scripts/generate_class_templates.py --write
py -3 reverse/scripts/collect_class_source_paths.py --report
py -3 reverse/scripts/generate_progress.py
cmake --build --preset native-debug --parallel 4
cmake --build --preset godot-debug --parallel 4
py -3 -m unittest discover -s reverse/tests -v
ctest --preset native-debug --output-on-failure
ctest --preset native-debug --output-on-failure --rerun-failed
ctest --preset godot-debug --output-on-failure
git diff --check
```

Native and Godot builds passed. Python reverse/tests discovery 37/37 passed
(including 4 call-chain, 4 adoption, 8 partition and 10 class tests). Native CTest
initially 52/54 passed in 48.06s; two metadata checks ran before final record/doc
regeneration. After regeneration both failed checks passed 2/2 in 0.78s; no
behavior tests failed. Earlier build failures were stale moved includes and
ambiguous int test literals after restoring bSin overloads; corrected include
paths and explicit original uint16/float argument domains.

Adopter+class generator repeated symbols.csv byte-identically. Source owners
join 1025 adopted rows. Body audit compares full original snapshots to HEAD plus
all relocated body/helper token fingerprints. Diff whitespace and restricted
untracked suffix checks pass; no ELF/MAP/assets/raw listings staged or added.
There is no new reconstructed behavior to compare; existing host behavior
tests are retained. No PS2/PCSX2 equivalence, visual comparison or commit claimed.

## Continuation

Read docs/DEBUG_CALLCHAIN_RECOVERY.md and TASK-0042 source coverage/disposition
ledgers. Next recovery task should independently anchor remaining 111 candidates
and 175 unresolved via another named caller or a reviewed changed-size table;
do not use normalized uniqueness alone. Reproduce normal metadata regeneration
with the first four commands above. Both authorized SHA-bound binaries and
read-only Ghidra captures are prerequisites for rerunning binary analysis.
Do not rerun one-time source migration scripts on the migrated tree.


Final Godot CTest: 60/60 passed in 59.91s, including all native checks and six Godot
host suites. Final generated-class/progress checks passed after completion
records were refreshed. No external prerequisite blocked this bounded pass.
