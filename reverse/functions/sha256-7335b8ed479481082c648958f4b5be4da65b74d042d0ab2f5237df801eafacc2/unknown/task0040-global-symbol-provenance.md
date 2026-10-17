# TASK-0040 - Global Sub_/Type_ name provenance

Target: US PS2 SLUS_203.62; executable revision unknown; SYSTEM.CNF VER 1.00
does not identify a compiler build. SHA-256:
7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
2026-10-17. Ghidra MCP read-only. No original-equivalence claim.

## Confirmed observations

**E-0040-N01**: `reverse/scripts/audit_symbol_provenance.py --live` inventories
every token matching `Sub_<8 hex digits>` or `Type_<8 hex digits>` in game_core
`.cpp/.hpp/.h` files, including references and declarations. The public ledger is
`reverse/naming/task0040-global-symbol-provenance.csv`; each row retains source
locations, live symbol source, constructor/class attribution and evidence IDs.
562 identifiers map to 562 addresses. This is a source identifier inventory,
not a count of original classes or reconstructed functions.

Live `symbols get` returns 392 DEFAULT names and one USER_DEFINED name.
169 requests return `symbol 'address' not found`; these are explicitly unresolved
database lookups, not proof of missing original functions. Follow-up `functions get`
also reports no function at sample 0012CCA8. No database objects were created.
The sole USER_DEFINED name is `HP2_DemoCameraCoordinator_Update` at 0019A4B8;
its text is analyst metadata, not recovered original spelling. Comments in existing
decompiler output are likewise not binary naming evidence.

Joining E-0027-C01/C04 yields 357 identifiers with class attribution, including
51 constructor associations, and zero referenced qualified-label matches for this
source inventory. Attributions include shared base/subobject entries and must not
be treated as exclusive ownership or exact method names. Existing five adopted
method spellings are outside this inventory because they no longer use Sub_ names:
DemoDiscManager::Init (0012E6C8), DemoDiscManager::SetEndReason (0012EB00),
SlotPool::CleanupExpandedSlotPools (00257F28), SlotPoolManager::NewSlotPool
(002584B0), SlotPoolManager::DeleteSlotPool (00258548). E-0019-N03/E-0028-C04
remain authoritative; raw diagnostic reference text is not always method identity.

**E-0040-N02**: Fresh decompilation and incoming references were retained for all
12 root Sub_ entry groups, with raw responses only in ignored `local/task0040/`.
The symbol index links these 12 entries to this record. Exact spelling remains
unknown for the following groups:

| Entry/group | Existing supported class bridge | Exact method name |
|---|---|---|
| 00137100 | PhysicsCar constructor association | not newly recovered |
| 00137D90 | PhysicsCar virtual member | unknown |
| 00138E08 | PhysicsCar virtual member | unknown |
| 00137C88 | no named class bridge in E-0027 table | unknown |
| 00138F38 | no named class bridge in E-0027 table | unknown |
| 00152070 | no named class bridge in E-0027 table | unknown |
| 0018BCE0 | no named class bridge in E-0027 table | unknown |
| 0018F808 | no named class bridge in E-0027 table | unknown |
| 0019D4D0 | no named class bridge in E-0027 table | unknown |
| 0019DA30 | no named class bridge in E-0027 table | unknown |
| 001A4930 | no named class bridge in E-0027 table | unknown |
| 001A4E88 | no named class bridge in E-0027 table | unknown |

These file groups mostly expose free functions and port-side projection structs.
There is no `class Sub_...` declaration in the audited source. Eleven Type_ class/
struct declarations remain, including Type_0011BF98's explicit port-created grouping
(E-0037-N01/N02). A projection's filename does not establish an original class.

**E-0040-N03**: Fresh Ghidra references_to returns no references at full string
starts 002AEA70 (`PhysicsCar::Move()`) and 002AEA88
(`PhysicsCar::ResolveForces()`). This agrees with the prior E-0027-C04 instruction
scan. It does not permit renaming 00137D90 or 00138E08 to these spellings.

Read 558 bytes at 002C7090..002C72BD through Ghidra. Printable type-name strings
in this bounded region name the C++ runtime (`__class_type_info`, `__si_type_info`,
`__user_type_info`, bad_typeid, bad_cast, type_info, bad_alloc, array/ptmd/ptmf/func/
builtin/attr/pointer type_info) and primitive types. No game-class spelling was
found there. 002C7090 has no returned xref; 002C7098 has PARAM xref at 002A448C.
This is a bounded runtime-region observation, not a global RTTI-absence proof.

The supplied GAME directory contains ZZDATA. A recursive filename search for
`.elf/.map/.sym/.pdb/.dbg/.debug/.cpp/.hpp/.h` under GAME found no matches.
This does not inspect arbitrary asset payloads for embedded symbols. The authorized
ELF was located using the existing local config and its full SHA checked. Ghidra's
other program nfshp2.exe was not used as evidence for PS2 names.

## Hypotheses and counterevidence

Semantic resemblance to Move/ResolveForces and physical code proximity are useful
leads but do not supply an address-to-name bridge. USER_DEFINED symbols and imported
decompiler comments cannot repair that gap. No speculative identifier was adopted.

## Unresolved questions and continuation

No newly adoptable exact method spelling was found. Retain current Sub_/Type_ names.
This audit covers all current source address identifiers, existing class/name joins,
12 deeper entry checks and one bounded RTTI region; it is not a whole-image recursive
call-graph recovery or a scan of every asset payload. Nonvirtual object ownership,
other RTTI locations and uncreated Ghidra functions remain open. A further recovery
task should start with a concrete allocator/pool-to-object or live diagnostic bridge;
an authorized matching PS2 map/debug binary, if provided, would support another route.

Reproduce: `py -3 reverse/scripts/audit_symbol_provenance.py --live`.
Offline rerun without --live requires the ignored SHA-bound symbol cache.
Ghidra must expose SLUS_203.62 at the endpoint in tools/ghidra.py and the existing
local ELF configuration must resolve to the stated SHA. No Ghidra edits are needed.
