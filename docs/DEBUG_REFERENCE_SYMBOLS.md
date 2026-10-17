# TASK-0041 - PS2 debug reference names and retail correspondence

The user supplied a symbol-rich PS2 reference ELF and linker MAP on 2026-10-17,
labelled SLUS-20362, 2002-07-08 Debug Build. The date is user-provided; no independent
embedded date confirmation was found in the bounded date-string search. The
reference is authorized evidence for this task, supplementing AGENT.md R0's
original retail-only input. It does not replace the retail behavior authority.

## Binary identities and evidence

| Input | SHA-256 | Role |
|---|---|---|
| SLUS_203.62 | 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2 | Retail target; executable revision unknown |
| NFSHP2.ELF | 815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b | User-authorized PS2 reference |
| NFSHP2.MAP | 20baece850349d26f95a5da1c1f6e9eacd6c3f5574e560b8ff2f4e536f9f19d1 | Demangled linker names for the reference |

E-0041-I01: ELF32 little-endian MIPS. The reference contains 8,715 nonzero-sized
STT_FUNC symbols and a 25,935,148-byte .mdebug section. Debug addresses are always
recorded separately from retail addresses. `task0041-debug-functions.csv` retains
the original mangled names, MAP spellings, sizes and addresses.

E-0041-T01: the ECOFF local string table contains STABS aggregate declarations.
The derived reference-only catalogue has 1,284 exact named aggregate types,
1,353 extracted shape variants, 7,357 field rows and 13,273 method token rows.
Fields use original bit offsets. Enums, anonymous aggregates, unresolved type IDs,
inheritance and some nested aggregate fields are outside the parser's coverage.
See `reverse/reference/sha256-815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b/type_summary.json`.
These are reference types, not 1,284 proven retail layouts.

## Matching policy

E-0041-M01: read initialized retail .text/.rodata/.data/.lit4/.sdata through
read-only Ghidra MCP and compare every captured byte with the SHA-bound retail
ELF. The ignored capture manifest retains per-section hashes. Matching then uses
the existing Ghidra function ranges, without creating or editing database objects.
7,517 contiguous, aligned retail ranges are eligible; the whole Ghidra program
has 7,544 functions. The difference is a stated coverage limit, not discarded evidence.

Normalization masks direct jump operands, GP offsets and paired address-shaped
operands, retaining original targets as evidence. It supplies 4,410 unique
instruction-signature pairs, 283 ambiguous signature groups and four conflicting
call-target cases. Most pairs remain candidates. Address-shaped integers can
normalize like pointers; four-byte data equality can collide; calls among
unsupported candidates cannot establish their own identity. These counterexamples
are regression-tested and explicitly prevent general normalized-result adoption.

614 exact leaf pairs qualify independently: at least 32 bytes, raw byte identity,
unique byte sequence anywhere in both complete .text sections, no direct calls or
absolute/GP operand normalization, no non-return indirect transfers, and no
PC-relative exits outside the function range. This supports static code/name
correspondence, not host ABI, object layout or hardware equivalence.

E-0041-V01: 49 unique complete virtual-table pairs match size, both header words,
every this-delta/slot-prefix word and zero-slot shape. Each has at least two distinct
globally unique normalized function anchors and no known forward/reverse target
contradictions. Pair uniqueness is bidirectional across the 283 named reference
tables and 252 previously confirmed retail tables. These independent table
constraints corroborate slot correspondence beyond an isolated normalized body.
Slot names do not imply exclusive class ownership: shared/base implementations
remain separate. Retail 002A0550 has conflicting ActivateSound/DeactivateSound/
__pure_virtual names and is deliberately excluded from single-name adoption.

E-0041-R01..R20: independent code and caller/table review established the following
principal names, with exact instruction differences in
`reverse/naming/task0041-reviewed-bridges.json`:

| Retail | Reference | Name |
|---|---|---|
| 00137D90 | 00137D18 | PhysicsCar::Move(float) |
| 0018BCE0 | 00189E10 | DriveTrain::SetDesiredGear(Gear) |
| 0019D4D0 | 0019ADF8 | RigidBody::GetDerivative(float *) |
| 0018F808 | 0018D810 | Table::Table(float *, int, float, float) |

Move contains four actual field-offset changes (0844/0848 become 0848/084C).
SetDesiredGear changes two offsets (0070 becomes 0074; 004C becomes 0050).
These are direct counterevidence against copying reference layouts or behavior.

## Adoption and source limits

The exact adopted list is `reverse/naming/task0041-adopted-retail-names.csv`;
the source method plan is `task0041-adopted-source-names.csv`. The symbol index and
individual records retain original retail addresses, reference addresses and
evidence IDs. Existing behavior statuses are preserved; new name-only records
are Identified. No entry becomes Verified.

Existing class shells use the exact original method spelling where class scope
agrees and the host placeholder would not collide with another overload.
Constructors/destructors, operators, shared-base attributions and compiler-generated
symbols retain address entry names when the existing port signature cannot safely
express the original member. Exact names remain available in the catalogue.
Native signatures are host projections; reference C++ signatures are documented
without pretending they are validated retail ABIs.

The named primary virtual table establishes `cDBFileSys`, correcting the allocator
label spelling `CDBFileSys`. The separate cDBFilesBase table is not silently renamed
to its derived class. Other allocator-label/table-type differences remain explicit
in the table ledger. The historical classes.csv stays an allocator evidence
catalogue; its label is not always the exact C++ type identifier.

## Reproduction and continuation

Python 3.14.4; CMake/CTest 3.29.2; godot-cpp pin
e4b7c25e721ce3435a029087e3917a30aa73f06b. Ghidra program SLUS_203.62 uses
MIPS-R5900 little-endian; application version was not exposed by the queried MCP
metadata. No import, rename, comment, type or database-boundary operation was used.

```powershell
py -3 reverse/scripts/extract_debug_reference_types.py --elf D:/ps2_game/GAME/DEBUG_VER/NFSHP2.ELF
py -3 reverse/scripts/match_debug_reference.py --debug-elf D:/ps2_game/GAME/DEBUG_VER/NFSHP2.ELF --map D:/ps2_game/GAME/DEBUG_VER/NFSHP2.MAP --capture
py -3 reverse/scripts/match_debug_vtables.py --debug-elf D:/ps2_game/GAME/DEBUG_VER/NFSHP2.ELF --map D:/ps2_game/GAME/DEBUG_VER/NFSHP2.MAP
py -3 reverse/scripts/adopt_debug_reference_names.py --write
py -3 reverse/scripts/generate_class_templates.py --write
py -3 reverse/scripts/collect_class_source_paths.py --report
py -3 reverse/scripts/generate_progress.py
cmake --build --preset native-debug --parallel 4
ctest --preset native-debug --output-on-failure
```

Omit --capture only when the ignored SHA-bound Ghidra cache is already present.
The local ELF configuration and read-only Ghidra connection remain prerequisites.
No original binaries, raw types, byte dumps or full MAP contents enter Git.

Further recovery should review unmatched/changed functions and ambiguous tables,
using named caller/slot relationships and exact differences. Inlined, removed,
folded and reference-only code need explicit treatment. This is not a claim that
every retail class/function has been recovered, nor that reference types fit retail.


## Reviewed address-type classes and packet scopes

E-0041-R05..R12/R19/R20 recover the ten former address-type spellings:

| Former identifier | Original class spelling |
|---|---|
| Type_001F8C10 | tCarLogoAnimation |
| Type_001F90F0 | tTrackLogoAnimation |
| Type_00249DE0 | tScrollingMenuList |
| Type_00249C78 | tMenuAnimElement |
| Type_0024A690 | tLeftRightArrowScroll |
| Type_002495F0 | tGlobalBackgroundAnimation |
| Type_00248568 | AnimationGroup |
| Type_002490D8 | tExitScreenAnimation |
| Type_002491B0 | tAnimatedBlurb |
| Type_001F1A78 | LanguageOptionsScreenBase |

The first eight constructors differ only in reviewed direct call/GP operands.
The retail tAnimatedBlurb constructor adds one store at +0xA8; the remaining
whole instruction stream corresponds. LanguageOptionsScreenBase expands from
52 to 140 bytes: derived-constructor call position, field stores, named base
vtable and destructor corroborate the name, while added logic remains retail-only.
Native fields and interfaces are unchanged, including port-side nested/container
organization; these spelling changes do not assert original C++ nesting or ABI.

E-0041-R13..R18 resolve the old Type_0011BF98 grouping into two original scopes.
The existing projected methods now use eStripData::GetNumVerts (0011C248),
eStripData::GetST0 (0011C268), eStripData::GetXYZ (0011C2B8), and
eStripDataE::GetElementRGBA (0011C180). Address helpers 0011BF98 and 0011C090
are eStripDataE::GetElementXYZ and GetElementST0 respectively; they have no new
invented host entry. Vector/static/value-return interfaces remain port projections.
The legacy source filenames are retained as port partitions because original
source placement has not been established. No type-size or inheritance copy is
implied; debug eStripDataE size1 is specifically not a native packet extent.


## Validation and current coverage

940 retail address/name records are adopted, with 55 same-class member renames,
6 principal native projection function names, 10 former address-type class names,
exact cDBFileSys spelling, and four packet helpers split between two original scopes.
The original 562 source identifiers have 191 supported names, 175 candidates and
196 unresolved/ambiguous entries in `task0041-source-coverage.csv`. Compiler/library
functions and reference-only types are included in catalogues; these counts are not
additional implemented gameplay. All 11 original Type_ class/struct declarations
have been replaced by reviewed spellings or the two-class packet split.

Final native and Godot-debug builds passed. Native CTest: 51/51 passed (43.10s).
Targeted Godot CTest: 15/15 passed (3.87s); exact selection is in TASK-0041.
Matcher/type/table synthetic tests: 13+4+3 passed; class generator tests: 8 passed.
The source-path report was regenerated after the CDBFileSys/cDBFileSys correction.
Generation checks and diff whitespace checks passed. No full Godot suite, visual
comparison, original gameplay comparison or commit is claimed. Repeated adoption
is stable and preserves named entry versus typed projection coverage.
