# TASK-0029 — Class directories recovered through Ghidra MCP

Target US PS2 `SLUS_203.62`, executable revision unknown (SYSTEM.CNF VER 1.00),
SHA-256 `7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`.
All Ghidra operations were read-only: memory search/read, references, listing and decompile.
No behavioural recovery, original ABI or original equivalence is established here.

## User-selected layout

The user superseded TASK-0028's temporary `port/class_templates` directory. All **179 class
pairs / 358 files** were moved out of it without overwriting existing files. **170 pairs**
remain directly under `src/game_core`; **nine pairs** use confirmed original directories.
The eleven classes already housed in established modules remain in those modules.
Unknown root-level destinations are explicitly user-selected port placement, not provenance.

Per-class basenames and `.hpp` companions are preserved as port-side file partitions. The
original translation-unit filenames below are recorded verbatim; a generated `BeltMover.cpp`
does not claim that the original file was called that instead of `beltmover.cpp`. The user
requested recovered **directories**, so independent class files are not merged into modules.

## Confirmed directories

| Class | Original translation unit | Path string address | Binding entry | Destination directory | Evidence |
|---|---|---|---|---|---|
| BeltMover | `..\indep\src\beltmover.cpp` | 002BB048 | 00204098 | `src/game_core/indep/src` | E-0029-P01 |
| CubicCameraMover | `..\indep\src\cameramover.cpp` | 002B1668 | 0015E7B0 | `src/game_core/indep/src` | E-0029-P02 |
| PlayerCarSound | `..\indep\src\carsounds.cpp` | 002B4810 | 0018D9A0 | `src/game_core/indep/src` | E-0029-P03 |
| ZoneFreezeCameraMover | `..\indep\src\zonecameras.cpp` | 002B1F88 | 00160298 | `src/game_core/indep/src` | E-0029-P04 |
| ZonePreviewCameraMover | `..\indep\src\zonecameras.cpp` | 002B1F88 | 00161F98 | `src/game_core/indep/src` | E-0029-P05 |
| VisibleDrivingLine | `..\indep\src\player.cpp` | 002B3998 | 00174EC0 | `src/game_core/indep/src` | E-0029-P06 |
| AverageInRealController | `..\indep\src\table.cpp` | 002B4A08 | 0018FBF0 | `src/game_core/indep/src` | E-0029-P07 |
| QuickSplineInRealController | `..\indep\src\quickspline.cpp` | 002BE288 | 0023D048 | `src/game_core/indep/src` | E-0029-P08 |
| bFile | `src\bFile.cpp` | 002BF970 | 00253828 | `src/game_core/src` | E-0029-P09 |

For the first eight classes, a named allocation result is passed directly into the object
initialization entry. Fresh decompile shows source-path and source-line arguments on internal
allocations there; fresh MIPS listing confirms the allocation-to-entry object argument,
including the delay slot. This establishes a local source-unit association independently of
nearby string ordering. Existing vtable attribution supplies five of the entry associations;
the other three are separately checked direct initialization-call leads. Their original
signatures remain unknown and this directory task does not widen the template API catalogue.

For `bFile`, 0025387C loads the explicitly class-qualified `bFile::Filename` expression,
00253880 loads `src\bFile.cpp`, and 00253884 calls the assertion function; its delay slot
supplies source line 0x2B. The field expression is not adopted as a method name. The root before
`src\` and original declaration header remain unknown, so no `bWare` directory is invented.

## Sweep and rejected associations

E-0029-S01: a paginated regex search over canonical `002AB000..003358F0` found **298 path
occurrences / 54 distinct paths**. Each occurrence was byte-read and its references exhausted.
Exact instruction ownership in the private listing corpus connected references to class
entries; oversized inventory end-address envelopes were not used as ownership evidence.
Candidates needed class-specific direct entry evidence and focused Ghidra corroboration.

The qualified-name branch covered all **38 additional diagnostic classes**, **100 distinct
diagnostic addresses / 103 class-label entries**. Only six diagnostic addresses had references:
five already bound original methods and `bFile`'s field expression. The other 94 were
unreferenced in this bounded scan. Fresh decompile gives no path arguments for DemoDiscManager,
SlotPool or SlotPoolManager's five methods; nearby `inc/bWare.hpp` at 002C0410 has no references
and is proximity only. Those classes remain at the root.

PsychoDriver and tCustomizeHUDScreen reference the generic `bFragmentedVector2.hpp` header.
An inline utility's file string is not the enclosing class's translation unit. Shared virtual
targets do not identify which derived class owns a file. These candidates remain unresolved,
as do all other classes without accepted evidence. This does not prove their original paths
are absent from the entire binary or unrecoverable by another method.

## Durable records and continuation

- [Source-path scan](../reverse/classes/source-path-scan.json): derived strings, addresses,
  NUL checks and complete reference results; raw code/byte responses stay ignored.
- [Reviewed decisions](../reverse/classes/source-path-decisions.json): exact original paths,
  allocation/call/reference instructions, binding observations and stable evidence IDs.
- [Per-class results](../reverse/classes/source-path-bindings.json): all 179 scoped classes,
  confirmed placements and bounded unknown/candidate states.
- [Template inventory](../reverse/classes/template-inventory.json) and placement policy contain
  the actual build paths. The generator cannot regenerate `port/class_templates`.

```powershell
py -3 reverse/scripts/collect_class_source_paths.py --collect --report
py -3 reverse/scripts/generate_class_templates.py --write
py -3 reverse/scripts/generate_class_templates.py --check
py -3 reverse/scripts/generate_progress.py
cmake --preset native-debug
cmake --build --preset native-debug --parallel 8
ctest --preset native-debug
```

`--collect` requires read-only Ghidra MCP. `--report` reuses the public derived scan and the
private instruction corpus to reproduce candidates; confirmed decisions still require their
documented focused review. If the corpus is missing, retain the ledger and collect new focused
evidence rather than treating missing matches as evidence of absence. A future relocation
requires a new accepted decision, collision checks, include/policy updates and regeneration.
Unknown classes stay at the root until such a decision exists.
