# TASK-0043 - Complete confirmed class-method templates and identifier adoption

## Confirmed coverage

The complete TASK-0042 ledger has 1,025 supported Retail names. Of those, 822
are class-member references belonging to 250 distinct declaring scopes. This
pass accounts for every row: 650 added whole-entry throwing templates, 169
existing throwing entries and three explicit unrepresentable entries. The
overall generator now represents 372 classes and 1,203 member attributions,
including older address-only attributions whose names remain unknown.

The 4,999 Debug class methods without an adopted Retail correspondence are
listed by declaring owner in `task0043-unmapped-debug-methods.csv`. No Retail
address or compiled game entry is invented for them. This is a bounded Debug
symbol catalogue, not a claim that every possible inlined method has a symbol.

`reverse/classes/task0043-member-registry.json` joins the adopted binary address,
original declaring owner, exact Debug ECOFF definition/TU, source placement,
current callable placeholder and the original reference signature. The member
coverage CSV distinguishes missing templates added by this task from existing
entries. Debug parameter types are evidence comments/catalogue data; they are
not an unverified Retail ABI declaration. Constructors/destructors/operators and
unresolved overload signatures retain address-named callable gaps where needed.

Nested/template entries `speech::state_t::~state_t`,
`speech::state_t::handleEvent` and `fragmentedVector<transition>::operator[]`
retain exact reference spellings in the catalogue with explicit exclusions;
creating native nested/template ABI or invented template arguments would be
required to emit them. They are not silently omitted from coverage.

Whole-entry templates always throw `hp2::NotImplemented`. Four packet methods
now have separate void/no-argument gaps alongside their typed partial projections;
the latter were previously mistaken for complete entries. Original source
placement is exact where recovered; unknown companion headers remain port-side.
Twenty-five sole-throw duplicate derived-class aliases were removed only after
the same address had a correctly owned canonical throwing entry. No inheritance
was inferred, and no native implementation was discarded.

## Functions, structs and fields

E-0043-N01 adopts 42 existing address-suffixed callable identifiers using the
already independently supported reference names, preserving host signatures and
behavior. Distinct original scopes with identical host parameter types use
documented static host carriers. For example, Table::GetValue and Graph::GetValue
remain distinct rather than becoming one global overload with the same signature.
EventRankState::GetStatus and EventTimeState::GetStatus also remain separate, so
integer literals cannot silently select a different original comparator.

E-0043-F01 independently confirms `EventRecord_001FFF60` as `EventInfo` and 11
fields using SHA-bound Debug STABS and live decompilation of independently mapped
Retail/Debug GetCar, GetTrack, GetGoal and GetReward getters, corroborated by
SetEvents' 188-byte record stride:

| Original owner | Host former identifier | Recovered identifier | Original offset |
|---|---|---|---|
| EventInfo | Field_0x3B | nCars | +0x3B |
| EventInfo | Field_0x3C | nTracks | +0x3C |
| EventInfo | link_count_0x3D | nGoals | +0x3D |
| EventInfo | Field_0x3E | nRewards | +0x3E |
| EventInfo | Field_0x44 | aCars | +0x44 |
| EventInfo | Field_0x6C | aTracks | +0x6C |
| EventInfo | link_index_0x7C | aGoals | +0x7C |
| EventInfo | Field_0x8C | aRewards | +0x8C |
| EventTree | Field_0x14 | pCars | +0x14 |
| EventTree | Field_0x1C | pTracks | +0x1C |
| EventTree | Field_0x2C | pRewards | +0x2C |

Only spelling and reviewed member uses change. Host vectors project original
pointers/arrays; neither sizeof nor native layout is copied. Identical offsets
in unrelated classes keep their own names. Other fields, constants, aliases and
types do not gain a name merely from sharing a function address. The entity
inventory covers address/offset tokens and declared class/struct/enum/alias names
across src and Godot scripts, excluding bundled add-ons; its lexical categories
are review aids, not binary identity evidence.

## Why TitleStringId_001F96B8 remains

E-0043-T01 establishes that this host helper is the extracted title-selection
branch within a larger Retail entry. Live Debug constructor decompilation at
001F5060 contains the same 272A/272D/0 block inline, with no separately named
selector call. Its imported name is the tEventTreeView constructor. Even an
eventual whole-constructor correspondence would not supply an original name for
the extracted host branch. Therefore the existing address identifier remains.
The bounded query does not prove no other helper symbol exists anywhere.

## Reproduction and checks

```powershell
py -3 reverse/scripts/adopt_task0043_identifiers.py --write
py -3 reverse/scripts/refresh_task0043_projection_bindings.py --write
py -3 reverse/scripts/complete_class_members.py --write --registry-only
py -3 reverse/scripts/generate_class_templates.py --write
py -3 reverse/scripts/complete_class_members.py --write
py -3 reverse/scripts/collect_class_source_paths.py --report
py -3 reverse/scripts/audit_task0043_identifiers.py --write
py -3 reverse/scripts/audit_task0043_preservation.py --renames reverse/naming/task0043-identifier-renames.json --report reverse/naming/task0043-preservation-audit.json
py -3 reverse/scripts/generate_progress.py
```

The current projection bindings derive expected hashes from fixed pre-change Git
revision 19a8de92af90788959ddcc5bcae1d0c4367fb107 and the approved rename rules,
not from the working code being tested. Historical TASK-0042 bindings remain
unchanged. Preservation compares all recognized original C++ function-body token
multisets, ignoring comments/whitespace but preserving literals. The 25 allowed
stub removals are pinned to original body hashes and canonical throwing targets.
Generated test main changes are reported separately. Compilation and runtime
tests supplement the lexical body parser; they do not establish PS2 equivalence.

Binary evidence reproduces with `audit_title_projection.py` and
`trace_event_type_fields.py` through read-only Ghidra. Raw captures remain ignored
under `local/task0043/title`. Retail SHA-256 is
`7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`;
Debug SHA-256 is
`815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`.
All 1,602 pre-existing symbol rows and their verification statuses remain intact.
Final build/test commands and results are recorded in TASK-0043. No behavior
recovery, original equivalence, Ghidra mutation or commit is implied.

Final validation: Native CTest 61/61 passed (57.82s), Godot CTest 67/67 passed
(73.58s), both builds passed; Python suites 54 + 7 passed. Existing-body
protection, pinned stub removals and generation checks passed. Repeated normal
generation changed none of 2,637 checked source/evidence files. No commit made.
