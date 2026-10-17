# TASK-0042: Debug call-chain naming and source ownership

This continues the historical TASK-0041 audit, not a behavior reconstruction.
All 175 candidate rows and 196 unresolved rows received an explicit disposition.
64 candidates and 21 unresolved rows now have supported names. The remaining
111 candidates and 175 unresolved rows (including eight folded-signature cases)
retain their address identifiers. A failed bounded search is not proof that a
name cannot be recovered later.

## Binary and live-target binding

- Retail SLUS_203.62 SHA-256: `7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`.
- Debug NFSHP2.ELF SHA-256: `815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`.
- Reference MAP SHA-256: `20baece850349d26f95a5da1c1f6e9eacd6c3f5574e560b8ff2f4e536f9f19d1`.

The user labels the reference 2002-07-08 Debug Build; that date remains user
provenance. Ghidra explicitly selected `NFSHP2.ELF`, R5900 little-endian, alongside
the open retail program. Its entire 1,693,772-byte `.text` capture matched the
authorized Debug ELF; section SHA-256 is
`651589b26daef1846eb067d308762583323e3e2c260a5c524de4ca2421580610`.
Live imported symbols, decompilation and references corroborate the bridges.
Ghidra was read-only. Raw captures and binary material remain ignored in
`local/task0042/`.

## Confirmed naming evidence

`task0042-candidate-bridges.json` records all 175 candidate decisions, including
finite caller/callee proof chains and known conflicts. Supported roots require
independent prior correspondence, equal complete parent shape and aligned calls;
the reviewed Move bridge has its exact changed-body hashes pinned. PhysicsCar's
constructor is rooted in primary and secondary complete named vtable stores at
matching code and object offsets. Outgoing-only roots require raw equality of
every non-direct-transfer instruction, no normalized data operands, and proven
callees. Circular candidate support does not establish a name.

`task0042-unresolved-bridges.json` and the companion dispositions CSV document the
196-row review. Several different-sized functions required explicit review of
aligned named callers, retained instructions, changed fields and callees. These
include PhysicsCar::CheckDampingSuitability/SetBoost, DriveTrain::GuessClutchTorque,
Wheel::MaybeMakeSkids, CarState::CalculateWheelMatrices and bATan. Those changes
are counterevidence to copying Debug behavior. The bASin bridge additionally
checks the complete 1,672-byte table against live memory in both versions.

Complete table searches used initialized retail `.data`, `.rodata` and `.sdata`.
They produced no additional safe scoped names under the two-anchor policy.
Folded/shared targets still require individual caller context; no invented
exclusive class ownership follows from one slot.

## Source organization

E-0042-S001 extracts exact local ECOFF file markers preceding procedure symbols;
it preserves FDR/SYMR offsets, marker offsets, compilation TU and MAP object/line.
All 8,715 sized reference ELF functions agree with the extracted procedure names
and addresses. Temporary FDR filenames and library object basenames are not
substituted for source paths. E-0042-S002 corroborates PhysicsCar::Move through
the live imported symbol and Debug decompilation.

The class file map records 193 old-to-current paths, including the final Car
merge into `indep/src/car.cpp`. The class layout registry uses exact declaration header markers only when
unambiguous. Unknown companion headers remain explicitly port-side. Important
corrections include `indep/src/engine.cpp` for DriveTrain, `src/dbfiles.cpp` for
cDBFileSys/cDBFilesBase, `indep/src/componentphysics.cpp` for PhysicsSystem,
and `src/ecstasy/estripdatae.cpp` for both packet classes. Original include roots
are mapped under `src/game_core`; leading relative parent components are removed.
This is repository placement, not evidence of the historical checkout root.

The original address-grouped driving CPPs mixed multiple owners. Their 151
exported/entry definitions now occupy 19 partitions. Confirmed entries use the
exact Debug definition CPP, or its compilation TU for inline definitions.
Unconfirmed entries stay in `port/driving/unresolved_entries.cpp`; host utilities
and byte-image adapters stay in explicit port files. Shared projection declarations
and helper namespaces are port interfaces, not recovered original headers or ABI.
The partition ledger preserves old paths, addresses and pre-migration body hashes.
Only proved identifier substitutions and function-local helper lookup directives
are allowed; arithmetic and error boundaries are retained.

Recovered constructors/destructors retain address-named callable placeholders
where native C++ construction/destruction semantics are unimplemented. Ambiguous
short method spellings in flattened host interfaces also retain address tokens;
the full recovered name remains in the address ledger and exact class shells.
No guessed inheritance, sizes or Debug field layouts enter the port. Whole-entry
gaps still throw `hp2::NotImplemented`; typed partial projections coexist with them.

## Reproduction and continuation

```powershell
py -3 reverse/scripts/trace_debug_candidate_calls.py --debug-elf D:/ps2_game/GAME/DEBUG_VER/NFSHP2.ELF
py -3 reverse/scripts/trace_debug_unresolved.py
py -3 reverse/scripts/recover_debug_source_ownership.py --elf D:/ps2_game/GAME/DEBUG_VER/NFSHP2.ELF --map D:/ps2_game/GAME/DEBUG_VER/NFSHP2.MAP
py -3 reverse/scripts/adopt_task0042_names.py --write
py -3 reverse/scripts/generate_class_templates.py --write
py -3 reverse/scripts/generate_progress.py
```

The one-time class/projection migration scripts require their original input
layout and must not be rerun against the migrated tree. Normal class generation
is idempotent. TASK-0041 CSVs remain historical baselines. TASK-0042's adopted and
coverage ledgers describe this pass. Continue from the unresolved disposition
rows using additional independently named callers or changed-size table context;
normalized uniqueness alone remains insufficient. No original equivalence or
Verified status is claimed. Final commands and results are in the task record.

Final validation: native and Godot builds passed. Godot CTest 60/60 passed
(59.91s); native 52 functional checks passed, and its two initially stale
generated-metadata checks passed after regeneration (2/2, 0.78s). Python reverse
test discovery passed 37/37. Class generation, source-path report, idempotent
adoption, progress generation and whitespace checks passed. See completed
TASK-0042 for exact commands and the initial failures.
