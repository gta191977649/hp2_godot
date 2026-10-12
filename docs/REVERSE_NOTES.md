# Reverse engineering knowledge base

No game binary has been imported or analyzed for this initialization. No real
addresses, symbols, structures, or comparison results were known at initialization.
The subsequent 2026-10-10 static assessment used the already-open private
`hp2_ps2/SLUS_203.62` program. See `docs/assessment/BINARY_REPORT.md` and
`reverse/binaries.csv` for identity, measured function addresses and limitations.
No structures or original behavior are verified; `reverse/symbols.csv` is
the curated recovery index, while the assessment CSV is the unverified inventory.
TASK-0007 added 16 evidence-backed purpose names and 11 direct-jump aliases with
per-address records. These are analysis names, not recovered original symbols.
See docs/FUNCTION_NAMING.md for evidence, deferred hypotheses and name rollback.

## Identity and storage

TASK-0006 captured read-only Ghidra function bounds and two pseudocode samples,
and generated unverified PS2Recomp diagnostic C++ privately under ignored
`ps2_decomp/`. See `docs/PS2RECOMP_TRIAL.md` for stable evidence IDs and the
oversized `0x0016FA80` envelope counterevidence. No curated symbol status changed.

Register each authorized ELF in `reverse/binaries.csv`: ID, full SHA-256, exact
region/build/version, source provenance, and tool/import configuration reference.
Unknown fields remain `unknown`. Prefer `sha256-<full hash>` as the binary ID.
Keep original input private. Record ELF virtual addresses as fixed-width hex,
separately from file offsets and runtime relocation observations.

`reverse/symbols.csv` is the searchable index with one row per binary/address.
Store records as `reverse/functions/<binary-id>/<subsystem>/<address>.md` and
layouts as `reverse/structures/<binary-id>/<subsystem>/<name>.md`. Use `unknown`
subsystem until evidence supports classification. Record original identifiers
unchanged, even when recovered names improve. Link source comments to record
paths and include binary ID/address. Update the index when implementation moves.

Templates are under `reverse/functions/TEMPLATE.md` and
`reverse/structures/TEMPLATE.md`; examples are explicitly synthetic and kept out
of the real CSV indexes. `<DUMMY_ELF_ADDRESS>` is a symbolic example token, not
an address inferred from a game. Never assign one to a real implementation.

## Status and confidence are independent

| Status | Meaning and required evidence |
|---|---|
| Unidentified | Candidate entry/location, purpose unknown |
| Identified | Evidence supports a name/purpose and boundaries |
| Decompiled | Decompiled output captured with tool/configuration and known limitations |
| Implemented | PC code exists, provenance retained; equivalence is unproven |
| Tested | Explicit PC tests passed; vectors and results recorded |
| Verified | Reproducible original/PCSX2 comparisons support equivalence over a stated scope |

Statuses do not guarantee completeness, and can regress when counterevidence
appears. Record dates and reasons. Verified never means all possible behavior:
state coverage and remaining gaps explicitly. A high-confidence interpretation
can remain Identified. A working PC function can remain low confidence.

Confidence is `unknown`, `low`, `medium`, or `high`, with supporting and opposing
evidence. Do not promote it from intuition or a test pass alone. Keep confirmed
facts, hypotheses, and unresolved questions in separate sections.

## Workflow for a recovered function

1. Verify authorized input/hash and exact binary version; log Ghidra, R5900
   processor/compiler specification, loader settings, extensions, and MCP version.
2. Identify boundaries and original symbol; record disassembly/decompiler exports,
   signature, ABI/register/stack assumptions, callers/callees and indirect calls.
3. Record relevant layouts, sizes, offsets, alignment, byte order, and uncertainty.
   Decompiler C types are hypotheses until supported by evidence.
4. Select one scoped task and its dependency closure. Write portable logic with
   explicit provenance and keep hardware operations behind platform contracts.
5. Create deterministic vectors for normal/boundary cases and observable outputs.
   Record handling of overflow, precision, nondeterminism, and side effects.
6. Compare with original execution using recorded PCSX2 version/settings, input
   trace, initial state/save hash, capture IDs/hashes, expected vs actual outputs,
   tolerances, and differences. Keep private captures outside Git.
7. Update status/confidence independently, links, CSV, task, and progress. Keep
   failed comparisons and limitations; never substitute PC-only tests for evidence.

## Evidence references

Use stable IDs such as `E-<task>-001`, with date, author/tool version, binary ID,
address range, reproducible steps, private artifact hash/path alias, observation,
interpretation, and limitations. Commit shareable metadata; exported proprietary
bytes/source and save data require separate review and normally remain private.
Prefer checked-in scripts for repeatable exports under `reverse/scripts/`.

## TASK-0008 police investigation — 2026-10-11

See docs/POLICE_AI_MAP.md for CopDriver/CopMover constructor, vtable and control evidence. Think 0x001A2110 is translated; Move 0x001A1030 remains boundary-blocked. A coarse LUI/ADDIU World label hit at 0x0012E050 was disproved by register overwrite; do not reuse it as evidence. Original names/types/enums remain unknown.

## TASK-0009 whole-binary recovery

See docs/FULL_RECOVERY.md and the 7,544-row recovery inventory. Native police timer uses float bits confirmed by LWC1/ADD.S/SWC1, correcting Ghidra casts. Move static branch topology supports candidate end 0x001A1BCB; separate generated output only, live Ghidra unchanged. Every original identifier remains unknown; class/method text hints are not address associations.

## TASK-0010 main loop — 2026-10-12

61 loop callsites/39 targets plus five startup sites captured; dynamic startup and frame gates, saved s2/f20 operands, and post-boundary tail retained. World controller path is 00131A30; mover goes via entity virtual 0014D898 -> 001D81F0 -> 00131AC8, with a possible remainder step. 00131A80 is a secondary-object callback, not mover. See docs/MAIN_LOOP_STRUCTURE.md, reverse/main_loop/ catalogs and E-0010 evidence. No Ghidra changes or original verification.

## TASK-0013 frontend layout — 2026-10-12

Asset-format RE. FRONTB.BUN chunk 0x80010200 holds 41 front-end screens (0x80010201), each a list
of 0x00010210 textured quads and 0x00010212 text widgets; chunk 0x00015010 holds bitmap font
glyph tables. Field map, evidence and open items: docs/FRONTEND_LAYOUT.md. Widget name hashes at
record +0x08 are the same literal immediates the ELF frontend code uses, which ties this data
format to TASK-0011's code-side findings and explains FUN_00249a40's GRID/MASTER_BP_OBJECT/
FADING_BP_OBJECT bindings. Real screens (main menu, Options, Configuration, Title) now render in
Godot from the player's ZZDATA at runtime. No Ghidra changes; no original-hardware comparison.

## TASK-0012 asset pipeline — 2026-10-12

Asset-file format RE (not ELF code), separate from the symbols.csv/functions workflow above. See
docs/ASSET_PIPELINE.md for the full write-up. Global ZZDATA path resolution lives only in
tools/ps2_assets.py (env var HP2_PS2_ASSET_ROOT, then gitignored config/ps2_assets.local.json) —
never hardcode a ZZDATA path elsewhere. A generic eLab nested-chunk container format and the HP2
TPK 164-byte texture header are confirmed via byte-exact full-file consumption and cross-checked
against three independent open-source community references (cited in the doc, not silently
trusted). P8/RGBA pixel decode produces recognizable real game art with one unresolved residual
artifact. reverse/assets.csv registers analyzed asset file SHA-256 hashes, parallel to
reverse/binaries.csv for ELF identity. No ELF/Ghidra state touched by this task.

## TASK-0011 frontend menu — 2026-10-12

String literal "New Main Menu" at 0x002b2f00 confirmed; CONFIRMED edge TitleScreen (FUN_0016f440 class, instance handler FUN_0016eef8 message==1) -> New Main Menu via FUN_0014cc78->FUN_0016c4c0. Manager-level RequestScreen/FindByName/PushHistory/PopHistory/Reset fully decompiled (FUN_0014cc78, FUN_0016c4c0, FUN_0016c8c8, FUN_0016b418, FUN_0016b438, FUN_0016b460). MumboJumboScreen (FUN_0016f810) is a confirmed sibling screen class with no confirmed edge to Title/MainMenu. New Main Menu's own class constructor was not located. See docs/FRONTEND_MENU.md and E-0011-01..22 evidence; 16 new reverse/functions/<binary-id>/frontend/ records. No Ghidra changes or original verification.

**Correction (same day, later pass)**: FUN_0016fa80 was initially mischaracterized as an unaudited
~465KB "oversized envelope" and update/tick candidate based on a 176-byte listing slice; a full
decompile shows it is a clean 312-byte one-shot cleanup function (`functions get`'s reported
~465KB end_address does not reflect real/reachable code -- treat as a Ghidra bookkeeping artifact
for this address). Its single caller led to the game's central GAMEFLOW_STATE_* dispatcher
(FUN_001680d8/FUN_001680e8, table at 0x002faea4) with original ASCII enum names recovered
verbatim (GAMEFLOW_STATE_LOADING_FRONTEND/_UNLOADING_FRONTEND/_IN_FRONTEND/
_LOADING_TRACK_COMMON/_SPECIFIC/_RACING/_UNLOADING_TRACK_COMMON/_EXIT_DEMO_DISC), connecting to
the TitleScreen->New Main Menu evidence above via GAMEFLOW_STATE_UNLOADING_FRONTEND's exit
callback. See docs/FRONTEND_MENU.md §8 for the full write-up; 0x0016fa80's function record was
rewritten in place (not left stale) and 4 new records added (E-0011-18..22).
