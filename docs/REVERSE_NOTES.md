# Reverse engineering knowledge base

## TASK-0039 - Frontend font/digit/quad corrections (2026-10-17)

Read-only Ghidra MCP confirms that font header28 is the glyph-table offset, not height;
0013FFA8 derives max glyph height (event digits98).001F7BF0 centres digits through00140930.
00140098 uses current-glyph pair slices matched against previous byte;00140D30 measurement
omits kerning while001422F0 draw applies it. Four colours/eight UVs in001428F0 explain shared
gradient/crop loss in the earlier port. E-0039-N01/F01..F06/Q01..Q09/A01 and per-function
records retain details. See FRONTEND_RENDER_AUDIT.md; original pixel equivalence is unresolved.



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



## TASK-0018 event tree (tEventTreeView) — 2026-10-13

The View Event Tree screen's data source is in the player's own FRONTEND/FRONTB.BUN: FUN_001FFC40
parses two `0x80034A00` containers named "Championship" and "Hot Pursuit", each holding 32 event
records of 0xBC bytes (FUN_001FFF60) plus an 8-byte link table. FUN_00185568 picks the tree from
profile+0x1072C, which is the 8/9 the mode-select screens write into DAT_0030C77C. Node X comes
from a 200-character ASCII map in the ELF (DAT_0031B3C0 / DAT_0031B490), 10 rows of 20 columns,
the n-th mark being event n, scaled by gp-0x41B0 = 130.0; node Y is depth * gp-0x41B4 = 350.0,
depth being the hop count back to the root (FUN_001F7FA8). FUN_001F75F0 maps status 0/1-3/4 onto
the SPEED_/PURSUIT_EVENT_PANEL_LOCKED/_UNLOCKED/_WON textures, and FUN_001FA9C8 writes node scale
1.0f/2.0f on the node it leaves and the one it moves to, which is why the selected star is twice
the size. Per-event progress lives in two 256-byte profile blocks (DAT_002FC1D8 / DAT_002FC2D8)
sliced by FUN_001FE988 in TWO passes -- >= 4-byte events first with 4-byte alignment, 1-byte
events packed afterwards -- with sizes from the GoalType switch FUN_00200DE8. The port reads no
memory card, so every event but the root reports Locked and nothing is invented to fill it.
Write-up: docs/EVENT_TREE.md; 3 new reverse/functions records; E-0018-01..12. Also fixed: test
executables now link the MinGW runtimes statically, which clears the three long-standing
0xC0000139 failures. No Ghidra changes; no original-hardware comparison.

## TASK-0016 mode-select screens + front-end movie loop — 2026-10-13

ELF-code RE. The main menu's Hot Pursuit and World Racing entries open "Cops On Mode Select"
(FUN_002174A0, document 0x5B35F533, FRONTB.BUN screen 1) and "Cops Off Mode Select"
(FUN_00217AF8, 0x033E7771, screen 2). Both handlers are fully decompiled: per-item targets, the
globals they set (DAT_002FC06C, DAT_0031AB90, DAT_0030C77C 9 vs 8, DAT_0030C784, DAT_002FC1C0),
the pad gate that refuses Quick Race and the tournament entry from any pad but 0 by zeroing the
event's sound field, the highlight tables (MI_1..5_MASK plus localised string ids), and the
tExitScreenAnimation transition that locks the screen while the items slide out at 650 px/s.
"Ultimate Racer" (Hot Pursuit) and "Championship" (World Racing) are the same tournament entry on
the two branches; both request "View Event Tree", which is NOT reversed -- it is registered
through the other wrapper FUN_0016AD88, whose class names come from runtime gp pointers, and its
document is not one of FRONTB.BUN's 41 screens.

**Background movie loops.** FUN_0016CCA8 -> FUN_0016D028 calls the decoder step FUN_002380A8 every
frame and re-opens the same file with FUN_00237AD8 whenever it does not return exactly 1
(`xori v0,v0,1; beq v0,zero`). FUN_0016CE30 early-outs when asked for the path already cached at
manager+0x1E4, and FUN_0016D0A0 keeps that cache, so re-entering a screen does not restart its
movie -- the re-open path is the loop. No play-once branch exists.

**Correction to TASK-0015**: the 2 s highlight-pulse period belongs only to the manager-level
object FUN_0016FBB0 builds at manager+0x1B8 against GLOBAL_ANIMATING_MENUITEM; every screen-owned
instance keeps FUN_00248870's default 4 s. The driver TASK-0015 could not find is
FUN_0016CCA8 -> FUN_0016FCD8 -> FUN_00248970.

Write-up: docs/FRONTEND_MODE_SELECT.md; 14 new reverse/functions records; E-0016-01..14.
No Ghidra changes; no original-hardware comparison.

## TASK-0014 frontend menu interaction — 2026-10-12



ELF-code RE of the front-end menu runtime. Recovered the front-end string hash FUN_002534A8

(h = -1; h = h*0x21 + byte), confirmed against the POPUP_1/2/3 and ANIMATING_MENUITEM immediates

in FUN_0016B478 and against 1531/1549 stored widget name hashes. Menu items are the screen's own

MENUITEM_0..15 type-1 widgets (FUN_00169F78, format string 0x002B2760). Navigation

(FUN_0016A5E0/FUN_0016A6C8) wraps and skips disabled items under gp-0x533C. Highlight is a colour

write, not a selection box: (state & 3) picks the authored colour, the stored highlight

(0x80,0x40,0x00) with alpha 0x80, or the stored disabled (0x18,0x18,0x18); gp = 0x003358F0 read

from the entry point. JOY_EVENT_* values and names come from the ELF table at 0x002FA2D0 and are

cross-confirmed by the handler groupings. Menu sound event names come from the 0x002FB0A8 table.

FUN_00143D50/FUN_00140CA0 key on widget +0x08/+0x28 and write +0x5C/+0x2b, i.e. the on-disk

FRONTB.BUN record IS the runtime widget node, which closes the gap TASK-0013 could only support

statistically. The 0x10203 screen header's +0x08 is the screen's own name hash; 18 of 41 names

recovered by hash match, including screen 0 = NEW_MAIN_MENU, which confirms TASK-0013's

content-based inference and resolves its open item 5 for those screens. Pop-up handling

(FUN_0016BAC0), the FMenus Highlight Animation motion (FUN_0016FBB0) and FE_*.BIN localisation

are decompiled-or-unknown but NOT translated and raise hp2::NotImplemented per AGENT.md R3.

Write-up: docs/FRONTEND_MENU_INTERACTION.md; 30 new reverse/functions records; E-0014-01..20.

No Ghidra changes; no original-hardware comparison.



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





## TASK-0015 continuation — 2026-10-12



Main activation slice 00216638 is independently anchored by listing: Ghidra decompile at that

interior address emits 00216A10's wrapper, so do not use returned function name as boundary

proof. Full 37-registration operand projection is recorded at 0016adb0.md (E-0015-REG-01),

including NameEntry sharing layout 29. E-0015-21 recovers the Options exit dependency closure

and corrects prior 0016CAB0 history-test interpretation: B3B0 checks pad-filtered groups and

returns zero on a match; C670 asks screen message 0x11 for Back permission. Main clears it;

Options permits it except during exit. Runtime Display snapshot sources remain unknown.


## TASK-0016 final correction

E-0016-N01 fresh path/name audit is in NAMING_AUDIT.md. E-0016-A06/A07 correct
JBC interleave, movie ADPCM/hardware pitch and160-msvideo lead. E-0016-V06
anchors vertical rows to original negative-height/stride. Symbols index retains
combined evidence IDs without claiming Verified.

## TASK-0035 - Vehicle follow-up evidence

Read-only Ghidra E-0035-W01/W02: wheel writer001C0D78 separates tire self-spin from blur
pose;00120EA8 uses float abs/angular velocity times0.02, not decompiler integer casts.
E-0035-R01/R02/R03: original frontend CARSELECTENVMAP source/upload,0011B520 environment
orientation, VU002A8AD8 ST/alpha and context2 additive/depth state are recorded under the
binary SHA vehicle directory. VU LOI latency counterevidence is preserved. Native/GPU
fixtures are not hardware equivalence. E-0035-M01: mirror objects+0x1C0/+0x1D8 require
the same two selected-skin patch pairs as near body, via constructor table+0x528; omitting
them left SKIN1 red mirrors on silver paint. Shared native bindings now cover this.

Continue TASK-0035 original runtime comparison and required live environment inputs; do
not infer whole game physics or six-face capture from the bounded frontend demonstration.

## TASK-0037 - Type_0011BF98 is not an original class symbol

The packet accessor grouping is port-created around address0011BF98. Read-only Ghidra
E-0037-N01/N02 confirms the original six helper names are still DEFAULT-generated FUN
labels; roles and call sites are known, but original class/function spelling and source
path are not.0011BF98 calculates component addresses, not a confirmed constructor.
eStripEntry (002AC298) and DuplicatedStripEntryData allocation labels do not establish
membership/name ownership. Keep Type_/Sub_ addresses until an actual naming bridge exists.
Detailed results: reverse/functions/<binary-sha>/vehicle/task0037-packet-symbol-provenance.md;
compact ledger: reverse/naming/task0037-packet-symbol-provenance.csv. No behavior/Verified
changes; header now explicitly distinguishes the port grouping from original C++ identity.

## TASK-0038 - Driving dependency and original-runtime observations (2026-10-17)

Read-only constructor/mover/force dispatch audit and two bounded finite-float leaves are
recorded under reverse/functions/<binary-sha>/driving and unknown. E-0038-003 retains the
branch-likely/cap/no-lower-clamp behavior of00137C88; E-0038-005 preserves001A4E88's scalar
operation order. PC fixtures establish no PS2 FPU equivalence. The primary three whole-entry
templates remain untranslated.

E-0038-004 captures a live JAGUAR replay-context mode1 mover with table002AEAC8 and target
00137D90 through PCSX2 v2.8.2/PINE. Two repeated paused reads per capture were identical.
A Cross input changed controls+0x214 to1.0 and wheel/motion fields changed during subsequent
wall-clock samples; those samples are non-atomic and are not native trajectory comparisons.
Full pointer/layout and unresolved query information is in the driving structure record.
No Verified promotion. The original SDL input mappings were restored after temporary
keyboard testing, and no emulator save slot was overwritten.

TASK-0038 continuation adds32 bounded address projections and three native test
suites:12 initialization/store/matrix dependencies,8 drivetrain/selector entries
and 12 solver/math/suspension entries. E-0038-I01..I13, D01..D07 and S01..S11 preserve
instruction evidence and open dependencies. Important corrections: constructor
vector W lanes remain untouched;0025AED0 consumes mutable global W;0019CD90 crosses
uncapped input;0018BAA0 multiply grouping follows listing rather than decompiler;
selector bounds preserve ADDIU32 wrap. Whole production driving remains unavailable.

E-0038-007 audits supplied custom PCSX2 build d75a0ad. JSON/TCP21512 supports paused
register captures and owned breakpoints, but step is a PC+4 temporary breakpoint.
Current World/replay pointers are zero; a bounded00137C88 entry attempt timed out.
No original numerical equivalence is claimed. Exact continuation maps are linked
from DRIVING_LAB.md; proprietary captures/listings remain ignored.


## TASK-0038 - minimum native driving closure (2026-10-17)

The supported911TURBO synthetic-plane fixture now uses complete logical-address
constructor/reset/component graph, engine/transmission, steering/tire/suspension,
five-pass force solve, aero/body derivatives, integration, publication and recovered
scheduler. NativeBackend owns one SHA-bound original ELF/data memory image, copies
it transactionally and publishes complete validated poses. Read DRIVING_LAB.md for
scope and ledgers. No Godot physics, invented grip or acceleration is used.

Counterevidence corrected two integration assumptions: original controller+218 is
footbrake, +21C handbrake (JOY_EVENT strings/writers E-0038-E14); Race4+A758zero is
the running context (E-0038-E13), not the separate diagnostic 4+A758nonzero branch.
Mode2 launch/freeze was inappropriate for this tool fixture. The native controller
allocation requires a port bound0xB0 because001A6330 writes+A0; its previous0xA0
allocation overlapped the following list and is guarded by a regression. Resting
wheel torque-ratio0/0 uses a narrow EE signed-finite saturation adapter at original
DIV.S sites (E-0038-E12), rather than host NaN. General PS2 exceptional arithmetic
and hardware equivalence remain unresolved.

Real-data acceptance now exercises sustained acceleration, opposite steering,
footbrake stop/rest, exact reset,132 recorded/replayed complete snapshots and
render pacing. Godot exercises540 production steps and exact pose replay. These
are port usability/regression results, never original equivalence. Current original
World is absent; next work is matching paused original entry/return snapshots under
identical inputs/contact conditions. TASK-0038 remains active only for that deferred
comparison. No Verified promotion or commit.


## TASK-0038 user corrections - steering sign and wheel render stage

E-0038-U01: positive001A5D60 steering maps forward toward source+Y (left); original
0013B5B0 inverts the controller byte axis before filtering. Host keys now mapA-D and
stickX is negated; recovered steering/force code is unchanged.

E-0038-W01: physics001C0D78 publishes raw attachment matrices;00120EF8 subsequently
composes render bases for wheels0/3, with distinct tire/blur versus brake rules. The
DrivingLab renderer path had skipped that known stage. Preview and driving now share
one recovered composition and per-layer group selection. All-four-wheel zero/nonzero
spin fixtures preserve raw physics input, hub translation and nonaccumulating display
updates. Read task0038-wheel-render-basis.md and task0038-steering-direction.md for exact
listing ranges, hashes and counterevidence; not Verified. CarViewer now owns Test drive
as a tool mode, not an independent scene, and new simulation shortcuts were removed.


## TASK-0040 - Global Sub_/Type_ naming audit

E-0040-N01..N03 inventories 562 source identifiers with live PS2 symbol provenance.
357 have existing class attribution (51 constructor associations); exact new method
spellings remain unresolved. 392 symbols are DEFAULT, one USER_DEFINED, and 169
addresses return no symbol object. No speculative rename or status promotion.
Read reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/task0040-global-symbol-provenance.md
and reverse/naming/task0040-global-symbol-provenance.csv. Root Sub_ headers mostly
contain free functions and port projections, not original class declarations.
A bounded RTTI runtime-region check found no game-class spelling; this is not
proof of global absence. Continue with a concrete naming bridge, not semantic guesses.


## TASK-0041 - Authorized reference supersedes prior unknown-name results

E-0041-I01/T01/M01/V01/R01..R20: the supplied PS2 reference ELF retains 8715
function symbols and 1284 named aggregate types.940 static retail name correspondences
are adopted with separate addresses/hashes; full methods and fields are not transplanted.
Read docs/DEBUG_REFERENCE_SYMBOLS.md and reverse/naming/task0041-adopted-retail-names.csv.
This supersedes earlier unknown-name conclusions only where the new bridge is recorded.
The former packet group is TWO original scopes: eStripData and eStripDataE. The 10
other Type_ declarations now use reviewed constructor class spellings. cDBFileSys
corrects an allocator-label spelling. Normalized-only candidates and shared/folded
name conflicts remain unresolved; no behavior or Verified promotion.


## TASK-0042 source ownership and names

Debug-reference call chains confirm additional retail symbol names; exact ECOFF
markers establish CPP ownership and selected declaration headers. See
[DEBUG_CALLCHAIN_RECOVERY.md](DEBUG_CALLCHAIN_RECOVERY.md) and the durable TASK-0042
class/projection registries. Class shells use established declaring scopes;
shared source files merge class implementations without inferring inheritance.
Typed driving projections retain port namespaces and shared port interfaces;
unknown owners remain in port/driving/unresolved_entries.cpp. No Debug ABI or
behavior is transplanted, and whole-entry gaps continue to throw.
