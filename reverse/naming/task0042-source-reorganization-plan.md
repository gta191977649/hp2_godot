# TASK-0042 source reorganization evidence and plan

## Confirmed evidence

E-0042-S001: authorized reference ELF SHA-256
815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b,
`.mdebug` HDRR local SYMR table and 335 FDR records. Exact source-file marker
records have packed flags `0x8F384045`. The current marker immediately before
each procedure symbol establishes its Debug definition file. The first source
translation-unit marker in the FDR identifies its containing compilation unit.
Compiler temporary `.ii` FDR filenames are not original source names.
CSV/JSON retain absolute original ELF file offsets for the FDR, SYMR and marker.
The reference MAP independently binds each procedure address to a `.text`
object range, retaining the MAP line number. Object basename spelling is never
substituted for the embedded source path.

The parser returns 10,064 procedure records; all 8,715 nonzero-size ELF STT_FUNC
address/name pairs agree exactly with ECOFF procedure records. All procedure
addresses have one MAP `.text` object owner. Some library FDRs lack source markers;
those remain unknown. All 940 adopted TASK-0041 retail correspondences join
uniquely to ECOFF source ownership. The class ledger covers 153 current shells
with exact class-qualified Debug member names; it does not infer inheritance.

E-0042-S002: live read-only Debug Ghidra export for PhysicsCar::Move at 00137D18,
ignored capture `local/task0042/source/physicscar-move-live.json`, includes
decompile, listing, incoming references and depth-one call graph. SHA-bound
export identifies the authorized reference. The embedded source ownership is
ECOFF evidence; this live export independently anchors the named code entry.

## Concrete owner destinations

| Existing projection | Confirmed original source owner | Scope and limitation |
|---|---|---|
| PhysicsCar Move / ResolveForces | `..\indep\src\physicscar.cpp` | Merge named implementations with PhysicsCar shell in `indep/src/physicscar.cpp`; original declaration header is `../indep/inc/PhysicsCar.hpp` for exact header-defined members. |
| DriveTrain SetDesiredGear and named engine helpers | `..\indep\src\engine.cpp` | Use `indep/src/engine.cpp`; no evidence for an invented drivetrain.cpp. |
| Table constructor and table functions | `..\indep\src\table.cpp` | Merge into established table.cpp; split port-only data views/interfaces explicitly. |
| RigidBody GetDerivative and named body helpers | `..\indep\src\rigidbody.cpp` | Use rigidbody.cpp; retain every independently unknown address function. |
| Wheel ModifyGroundForce and named wheel helpers | `..\indep\src\wheel.cpp` | Use wheel.cpp. Unknown Wheel entry matches need separate correspondence review. |
| eStripData and eStripDataE packet functions | `src\ecstasy\estripdatae.cpp` | Both original scopes belong to same TU; merge Type_0011BF98 projection into this module. Original declaration headers remain unproven here. |
| cDBFileSys shell | `src\dbfiles.cpp`, `inc/dbFiles.hpp` | Exact lower-case initial c is original. Rename CDBFileSys.cpp/hpp; keep cDBFilesBase scope separate. |
| cDBFilesBase methods compiled in dbfiles TU | `../indep/inc/dbFilesBase.hpp` | Original inline definition owner is base header, not derived dbfiles.cpp. |

These original path spellings are exact, including case; stripping the leading
`..\` when nesting under game_core is a repository path mapping, not original text.
Companion host headers for out-of-line projected functions can remain explicitly
port-side where the original declaration header is not independently established.

## Required split rules

The source-file inventories are address *mention* inventories: callback members,
include names and comments refer to other modules. Never move an entire Sub_ file
solely because its mentioned addresses map to one or several source owners.
For each actual definition, identify its entry from the existing address/comment,
require adopted retail correspondence, then use that row's exact definition path.
Retain helper structs, callback services and memory views under port namespaces.
Their names/interfaces do not claim original C++ nesting, type sizes or ABI.

Several PhysicsCar shell entries resolve to ComponentPhysics implementations or
other shared implementations. Preserve exact original reference scope in records;
do not fabricate PhysicsCar-exclusive ownership. Native inheritance may only be
introduced after independent layout/base evidence. Shared target names alone
justify correcting a misleading member shell scope, not copying Debug layout.

Before merging throwing shells with typed projections, preserve distinct APIs:
the whole-entry shell still throws hp2::NotImplemented at its retail address.
A partial typed projection does not implement that missing whole entry. Avoid
constructor/destructor/operator renaming that accidentally invokes host lifetime.

## Reproduction and validation

Python 3.14.4:

```powershell
py -3 reverse/scripts/recover_debug_source_ownership.py --elf D:/ps2_game/GAME/DEBUG_VER/NFSHP2.ELF --map D:/ps2_game/GAME/DEBUG_VER/NFSHP2.MAP
py -3 tools/ghidra.py --program NFSHP2.ELF --address 0x00137D18 --elf D:/ps2_game/GAME/DEBUG_VER/NFSHP2.ELF --output local/task0042/source/physicscar-move-live.json
```

No native source/build/generator mutation was made during this ownership pass.
This plan establishes source placement only; no behavior is marked Verified.
