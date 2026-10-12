# Project state — 2026-10-12 (Australia/Sydney)

## TASK-0013 — Original front-end actually rendered (not a debug UI)

User rejected the previous deliverable as "the debug screen", asking for the real game front-end
rendered exactly like the PS2. Decoded the game's own screen/widget layout format (chunk
0x80010200: 41 screens of 0x00010210 textured quads and 0x00010212 text widgets, each carrying a
name hash, 24-byte name, layer byte, x/y, scale, texture hash and RGBA with PS2 alpha) and the
bitmap font format (chunk 0x00015010: 32-byte glyph records with atlas UV + advance, plus a
kerning table). Field identification was evidence-driven, not guessed: the texture-hash offset
(+0x50) was found by correlating every 4-byte-aligned offset across all 1549 widget records
against the real TexturePack hash set (507/1015 hits for quads vs 79 for the next best offset),
and the scale semantics were confirmed by TITLE_FULLSCREEN's 512x256 texture at (1.25, 1.88)
resolving to exactly the 640x480 declared in the screen header. Added native Godot-independent
parsers (src/game_core/frontend/fmenus.{hpp,cpp}, named after the original source file
indep\srcmenus.cpp recovered earlier), a layout bridge (Ps2FrontEndLayout), hash-based texture
resolution with caching (Ps2TextureLoader::load_texture_by_hash), and an immediate-mode Godot
renderer (godot/scripts/frontend.gd) that draws quads with draw_texture_rect and text glyph-by-
glyph from the original font atlases. Result: the real Mode Select main menu (Championship /
Speed / Options with the HP2 logo), Options, Controller Configuration (with PlayStation button
glyphs drawn from the BUTTONSOUT icon font) and the Title Screen (HP2 splash + PRESS START) all
render from the player's own ZZDATA at runtime, with nothing pre-extracted. Headless smoke covers
all 41 screens and resolves 1524 widget resources (25 missing). Strong cross-validation with
TASK-0011: widget name hashes in the layout data are the same literal immediates the ELF's
frontend code uses (0xe0105663/4/5 = POPUP_1/2/3, 0xab671c9a = ANIMATING_MENUITEM inside
FUN_0016b478), and TASK-0011's previously-unexplained FUN_00249a40 is now understood as binding
the real GRID / MASTER_BP_OBJECT / FADING_BP_OBJECT background widgets by name. CTest 8/8; the
three pre-existing scene smokes unchanged; standalone out/windows-debug/port.exe rebuilt and
verified. Open and explicitly not faked: no interaction/animation state yet, text is the authored
placeholder strings (LANGUAGES/FE_*.BIN not parsed), kerning parsed but not applied, and the
screen-name <-> screen-index mapping is still unproven (screen 0 == main menu is a content
inference). Full write-up: docs/FRONTEND_LAYOUT.md; task: tasks/active/TASK-0013-frontend-layout.md.
Next: localised strings, then drive selection/highlight state so the real screens are operable.

## TASK-0012 update — Native runtime texture loader; standalone .exe built; original source tree recovered

Three user-directed follow-ups completed in this pass. (1) User required the reversed code to
load everything directly from the original PS2 assets at runtime, not via a manual Python export
step: ported tools/asset_extractor/hp2_tpk_textures.py's decode path (chunk parser + full
Modelulator-derived P8/4bpp RWBuffer decode) verbatim into Godot-independent C++
(src/game_core/assets/elab_chunks.*, hp2_tpk_texture.*), added a runtime ZZDATA-root resolver
(src/platform/ps2_asset_root.*, same env-var/config precedence as tools/ps2_assets.py) and a
Godot bridge (src/platform/godot_bridge/ps2_texture_loader.*, class `Ps2TextureLoader`). Added a
new CTest (`hp2_tpk_texture`) that decodes the same real texture via C++ and Python and asserts
byte-for-byte equality -- passed. frontend_menu_observer.gd now calls
`Ps2TextureLoader.load_texture("FRONTEND/FRONTB.BUN", "LOGO_NFS")` etc. directly at runtime; the
manually-exported PNGs under godot/assets/extracted/ were deleted. (2) User asked to search for
the original PS2 loader code via Ghidra MCP before trusting the ported algorithm: searched for
the TPK chunk tag constants and "TPK"/".tpk"/"eLab" strings as both ELF code and data -- zero
hits, confirming these are pure resource-file data never referenced by name in the ELF; most
likely explanation (not proven) is that real PS2 GS hardware natively samples swizzled/indexed
textures, so the original code probably never does a CPU-side "decode to linear RGBA" at all --
documented as an open item rather than claimed as a recovered original function. (3) User noted
that a `.cpp`/`.hpp` string search would reveal the original source tree: it did, spectacularly --
93 embedded `__FILE__`-style strings recovered the real original directory layout
(`indep\src\fmenus.cpp`, `indep\src\frontendscreens\{selectcar,racesetup,options,
joystickconfiguration}.cpp`, the bWare middleware's own `src\b*.cpp` files) and several confirmed
original class/method names (`World::DoTimestep`, `Race::Update()`, `RaceCoordinator`,
`ReplayManager::HandleSnapshot()`, `TrafficCar::Move()`, `TexturePack`/`TextureAnimPack` --
the last one confirming "TPK" is itself derived from the real original class name, not a
community-invented abbreviation). None of these strings had resolvable code cross-references this
session (likely retail-stripped assert/profiler call sites), so file-to-function attribution
remains unconfirmed per-address; full catalog in new docs/ORIGINAL_SOURCE_TREE.md.
Finally built and ran an actual standalone Windows executable (`out/windows-debug/port.exe` +
`port.pck` + `ps2_port.dll`, via the existing "Windows Debug" export preset, main scene
temporarily pointed at frontend_menu_observer.tscn for the export then reverted) -- launches
independently of the Godot editor/portable runtime, resolves the ZZDATA config, and renders the
same operable, evidence-backed menu closed loop with live-loaded real PS2 textures (NFS logo,
brushed metal panel). Screenshot captured and sent to the user. `out/` is gitignored; nothing
proprietary committed. No Ghidra mutations, no PCSX2 comparison, no Git commit.
Next: locate the actual GAMEFLOW_STATE_IN_FRONTEND sustained update loop (still the top open item
from the prior pass); cross-reference the newly-recovered fmenus.cpp/frontendscreens file names
against specific function addresses if a viable method is found; consider renaming port-side
`HP2_*` labels to align with recovered original class names where confidently attributable.

## TASK-0011 update — Central GAMEFLOW_STATE_* dispatcher found; FUN_0016fa80 hypothesis retired

User pointed at an existing, independently-validated local PS2 texture decoder
(D:\dev\eagl-godot\map_tools_ps2) and asked to combine it with continued Ghidra MCP work to
fully reverse the original menu source, explicitly "don't guess yourself." Fixed TASK-0012's
texture decoder by porting that project's validated decode path verbatim (root cause of the prior
artifact: a missing 0x80-byte alignment on the pixel/palette data-pool base offset); FRONTB.BUN
decode success rate went from 210/365 to 357/365 with zero visible artifacts (see
docs/ASSET_PIPELINE.md §4.4). Returned to the ELF side and obtained a full decompile of
FUN_0016fa80 (previously assumed to be an unaudited ~465KB function and the likely New Main Menu
update/tick routine) -- it is actually a clean 312-byte one-shot cleanup function; the "oversized
envelope" hypothesis from TASK-0006/0007/0011 is retired as almost certainly a Ghidra
function-record bookkeeping artifact, not real code. Tracing its single caller led to a major
structural discovery: the game's central `GAMEFLOW_STATE_*` state-machine dispatcher
(`FUN_001680d8`=request-transition, `FUN_001680e8`=per-frame pump, already known from TASK-0010 as
a main-loop StateDispatch target but previously uninterpreted) and its function-pointer table at
ELF 0x002faea4, whose rows point to ORIGINAL ASCII enum-name strings recovered verbatim from the
binary: GAMEFLOW_STATE_LOADING_FRONTEND, _UNLOADING_FRONTEND, _IN_FRONTEND,
_LOADING_TRACK_COMMON, _LOADING_TRACK_SPECIFIC, _RACING, _UNLOADING_TRACK_COMMON (row order
inferred, not re-verified), _EXIT_DEMO_DISC. This connects to TASK-0011's prior evidence:
GAMEFLOW_STATE_UNLOADING_FRONTEND's exit callback (0x001687a8, decompiled) calls FUN_0016f950
(the confirmed attract-sequence gate leading to "New Main Menu"); GAMEFLOW_STATE_IN_FRONTEND's
enter callback (0x00168830, decompiled) calls FUN_0016fa80 then unconditionally requests
GAMEFLOW_STATE_LOADING_TRACK_SPECIFIC -- an unresolved contradiction with "IN_FRONTEND" sounding
like a sustained browse-the-menu state (its update slot is empty), flagged as the top open
question. 4 new/updated function records (E-0011-17 superseded/corrected, E-0011-18..22); 0x0016fa80's
record rewritten in place rather than left stale. Full write-up: docs/FRONTEND_MENU.md §8. No
Ghidra mutations; no PCSX2 comparison; no Git commit.
Next: resolve the IN_FRONTEND contradiction (decompile 0x00168db8/0x001690a0/etc. to see whether
gameplay ever returns to IN_FRONTEND, or whether the sustained menu-browsing loop lives entirely
inside one state's update callback rather than in this dispatcher), then the remaining open items
in docs/FRONTEND_MENU.md's updated priority list.

## TASK-0012 — PS2 original asset pipeline (active; container + P8/RGBA decode delivered)

User pointed at an authorized local PS2 disc dump (D:\ps2_game\GAME\ZZDATA) and asked for a
non-hardcoded global config entry point, plus reverse engineering that can actually render the
original UI matching real PS2 behavior, with file/function naming reflecting the recovered
structure. Delivered: `tools/ps2_assets.py` as the single global ZZDATA-path resolver (env var
`HP2_PS2_ASSET_ROOT` then gitignored `config/ps2_assets.local.json`; tracked
`config/ps2_assets.example.json` template). Reverse engineered and validated (byte-exact full
file consumption) a generic "eLab" nested-chunk container format shared by FRONTEND/FRONTB.BUN
(14.4MB, 1760 chunks) and GLOBAL/HUDTEX.BIN (1.5MB, 9 chunks) — `tools/asset_extractor/
elab_chunks.py`. Found real original frontend UI text in FRONTB.BUN including developer
placeholder/joke strings ("Phil is a loser, so I have to make this fake menu item") proving
unmodified shipped data, plus real labels (PRESS START, Championship, Options, Credits,
Controller, Language Select, Setup, Race Type) and `UwE*`-prefixed widget type names — not yet
cross-referenced to specific screens. Identified and validated (cross-checked against three
independent open-source community references: xan1242/xnfstpktool, PS2HomeDeveloper/
ps2-tim2-tool, leeao/PS2Textures, all cited in docs/ASSET_PIPELINE.md) the HP2 TPK texture
container's 164-byte `AncientTextureInfo` header and implemented P8 (8-bit indexed, PSMT8
swizzle + CSM1 CLUT reorder + PS2 alpha range) and RGBA32 pixel decode in
`tools/asset_extractor/hp2_tpk_textures.py`. Decoded 89/89 HUDTEX.BIN and 210/365 FRONTB.BUN
texture headers with zero failures (remainder skipped: unsupported 4bpp/DXT, not guessed).
Visually confirmed multiple decoded textures are clearly recognizable real game art (tachometer
gauges, car badges including a legible BMW roundel, a full `DISPLAYSETUP_BG` road scene, and a
`PURSUIT_TREE_BG` event-tree map) — sent to the user. All decoded images and chunk dumps are
gitignored (`tools/asset_extractor/output/`); no proprietary bytes committed. Open: a fine
speckle artifact remains in every P8 decode, most likely authentic PS2-era dithering but not
confirmed; 4bpp/DXT/swizzled-RGBA decode not implemented; the widget/layout record format (which
texture/text belongs to which named screen) is completely unparsed, so no Godot rendering of
real original layout exists yet — explicitly do not treat the decoded images as "the rendered
menu," only as validated raw material toward it. Full report: docs/ASSET_PIPELINE.md. Registered
analyzed asset file hashes in reverse/assets.csv. No Git commit (left to the user, per prior note
in TASK-0011).
Next: parse LANGUAGES/FE_ENGLISH.BIN for authoritative menu text, then reverse the UwEMENUITEM_*
widget record layout, then resolve the P8 artifact (ideally against a real PCSX2 screenshot),
then implement 4bpp/DXT decode — only after that attempt an actual layout-accurate Godot render.

## TASK-0011 — Front-end menu minimal closed loop (active; scoped completion)

User asked for the "original front-end flow minimal closed loop": confirm a main menu and its
adjacent pages, trace NewMainMenu creation/update/input-callback/page-switching, restore
navigation state in the native core, and show an operable Godot UI, menu closed loop first.
Confirmed via read-only GhidraMCP: string "New Main Menu" at ELF 0x002b2f00; CONFIRMED edge
TitleScreen -> New Main Menu (TitleScreen's instance handler FUN_0016eef8, message==1, calling
FUN_0014cc78->FUN_0016c4c0). Manager-level screen switch/find/history push-pop/reset fully
decompiled (FUN_0014cc78/0016c4c0/0016c8c8/0016b418/0016b438/0016b460). MumboJumboScreen
(FUN_0016f810) confirmed as a sibling registered screen class; its edge to Title/MainMenu is
NOT confirmed. The long-flagged oversized FUN_0016fa80 envelope (0x0016fa80..0x001e3dbf, ~465KB,
previously noted in TASK-0006/0007) routes several of its opening state values directly to
"New Main Menu" -- strong candidate for New Main Menu's own update/tick function, still
unaudited (TASK-0003 remains the right next step for it). New Main Menu's own class
constructor/dispatcher was not located; input wiring from raw joypad events
(JOY_EVENT_MENU_SELECT/CANCEL/BACK/HELP strings found, not traced to a call site) to
FUN_0016eef8's message dispatch was not traced. 16 new reverse/functions/.../frontend/ records
(E-0011-01..16) plus a reclassified/updated 0016fa80 record (E-0011-17, moved from unknown/ to
frontend/, Unidentified->Identified, confidence low->medium). reverse/symbols.csv updated.
Native `hp2::frontend::ScreenManager` (src/game_core/frontend/screen_manager.cpp) implements the
confirmed manager-level API as a structural projection (name registry, current screen, 12-slot
history with push/pop, reset); 9 new unit tests. Godot bridge `FrontEndMenuObserver` +
operable scene `godot/scenes/frontend_menu_observer.tscn` (buttons for the confirmed edge, an
explicitly-labelled unconfirmed sibling switch, Back, and Reset; live history/registry display;
evidence footer). Native CTest 7/7 and bridge CTest 7/7 passed (no regressions in the five
pre-existing tests); Godot headless import clean; new `--frontend-menu-smoke` passed
(FRONTEND_MENU_OBSERVER_SMOKE_OK); prior `--observer-smoke` and `--smoke-test` still pass
unchanged. Two real rendered screenshots captured and visually checked
(local/frontend-menu-observer.png, local/frontend-menu-observer-after-press-start.png, both
1380x760, git-ignored). Symlinked the user's existing `reverse-engineer-anything` (REA) skill
from `~/.agents/skills/` into `~/.claude/skills/` at the user's request so Claude Code can load
it; REA's own tool routing has no Ghidra/MIPS-R5900 provider (IDA-only for native targets), so
actual decompilation continued through the existing Ghidra MCP server as in prior sessions.
No Ghidra mutations, no Verified promotion, no PCSX2/original comparison, no Git commit (the
user committed the pre-existing scaffold independently mid-session; this task's new files are
uncommitted). Full report: docs/FRONTEND_MENU.md.
Next: TASK-0003-style boundary/ABI audit of FUN_0016fa80 (highest-value next step -- it is the
best lead for New Main Menu's real update logic and menu-item content); then trace
FUN_0016eef8's raw-input source; then locate New Main Menu's own construction site.

## TASK-0010 — Main-loop structural shell completed

Mainloop 0x0012F378 mapped: 61 callsites / 39 distinct targets, five startup sites,
12 guards and 19 nested World/controller/entity/mover connections. EngineShell
preserves ordered branches and saved s2/f20 operands; strict mode stops on missing
contracts, observation mode labels fixtures. Waiting resumes only the original tail.
Separate Godot observer supports six scenarios, pause/single-step/reset/boundary,
startup/world tabs; old main scene and PortCore APIs preserved.
Native and bridge CTest: 6/6 each. New observer headless smoke and prior demo smoke
passed; actual rendered screenshot inspected (local/main-loop-observer.png).
Full evidence validator checks 51 live function records, metadata/hash stability,
all 19 nested connection PCs and three vtable targets. No Ghidra changes/Verified
promotion or Git commit. Applied REA skill workflow; tools were existing Ghidra MCP.
Report, architecture graphs and extension example: docs/MAIN_LOOP_STRUCTURE.md.
Open: ./tools/run_main_loop_observer.ps1; -Build rebuilds; -Smoke checks headlessly.
Next: resume TASK-0009 with World object/partition/entity ABI and data contracts,
then recover/register a tested module handler. Original execution oracle, clock,
state enum and complete behavior remain unresolved. TASK-0005 unchanged.

## TASK-0009 — Whole-binary recovery active

User requested every function, correct names and classes; only the stripped ELF
is available. Full inventory: reverse/recovery/TASK-0009-functions.csv, 7,544 rows.
First pass exports 7,516 pseudocode bodies and records 28 boundary blockers;
49 class texts / 118 method-text hints await address proof. Two police local bodies
now have native code and partial class storage views. Native CTest 4/4 and five
export safety tests passed. No Verified functions or trustworthy original names.
Move control-flow candidate 0x001A1030–0x001A1BCB produces a full separate
instruction translation; candidate output retains 27 other placeholders. No live
Ghidra names/types/boundaries changed; previous outputs/config remain preserved.
Report and reproduction: docs/FULL_RECOVERY.md. Full restoration remains incomplete.
Next: R5900/pcode/ABI review of Move and tail-call entry 0x001A0E88 before any
journaled live repair; trace class hints and recover external-call contracts.
Existing TASK-0005 remains independently active. No commit made.

## TASK-0008 — Police AI map completed

Read-only MCP inspection links mode 4 to CopDriver and mode 5 to CopMover.
Think candidate 0x001A2110 has 2,432 defined bytes and existing C++ translation.
Move candidate 0x001A1030 has a one-byte body and TODO output; boundary repair
remains required. Eight police function records added with original names unknown;
proposed purpose names were not applied. Report: docs/POLICE_AI_MAP.md.
Private evidence/pseudocode: ps2_decomp/police-ai/. Eight live metadata matches,
unchanged ELF hash and seven non-placeholder C++ syntax probes passed; Move blocked.
No native behavior implemented or Verified status assigned. TASK-0005 untouched.
Next police action: scoped boundary/ABI audit of 0x001A1030 and undefined vtable
entry 0x001A0E88, then typed helper recovery with original behavior comparison.

## TASK-0007 — Function naming completed

Screened all 7,544 functions; reviewed all 20 pilots. Applied 16 evidence-backed
HP2_ purpose names and recorded 11 automatic jump aliases (27 entries total).
No trustworthy original developer names recovered; original identifiers remain unknown.
Ghidra saved; ELF SHA-256, function envelopes, namespaces and calling conventions
unchanged. Names/map/private regenerated C++ agree. Report: docs/FUNCTION_NAMING.md.
Private output: ps2_decomp/recompiled-named/ (7,516 translations + unchanged 28
placeholders). Eleven synthetic safety tests and 20 C++ syntax probes passed.
No full generated link/boot/original comparison; no Verified promotion.
Name rollback mapping/command and source-type limitations are recorded in the report.
Existing TASK-0005 remains independent and active; no implementation changes made there.
Next reverse action: TASK-0003 must audit damaged camera/FlightAreas entries,
the oversized 0x0016FA80 envelope and allocation return ABI before further naming.

## TASK-0006 — PS2Recomp trial completed

User-requested PS2Recomp/Ghidra generation trial completed on 2026-10-10.
See `docs/PS2RECOMP_TRIAL.md` for commands, tool/dependency identities and evidence.
Private outputs: `ps2_decomp/recompiled-triage/`: 7,516 translated functions,
28 explicit unresolved placeholders, registration and headers (~132 MiB).
Baseline full run was terminated near 6 GB memory while generating the oversized
`0x0016FA80` body envelope; its partial output is retained separately.
Tool build, artifact assertions and five representative C++ syntax probes passed.
Full generated build/link and original behavior comparison were not performed;
no equivalence or Verified status is claimed. Existing TASK-0005 remains active
with its own independent scope; this trial did not modify its implementation.
Next action: TASK-0003 should audit the `0x0016FA80` discontiguous body/envelope
alongside its damaged entries before unrestricted generation. Ghidra MCP at
127.0.0.1:8080 and the original authorized ELF are needed to re-export.

## Scope and milestones

M0 is complete for Windows x86-64. TASK-0002 static workload assessment is complete
for the current Ghidra snapshot; no game reconstruction or asset conversion was
performed. M1 is in progress: read-only MCP/R5900 connectivity and ELF identity
are established, but boundary/ABI quality and broad identification are unfinished.
M2–M6 have not been achieved. Current active tasks: TASK-0005 (independent infrastructure),
TASK-0009 (full-binary recovery), TASK-0011 (front-end menu; closed-loop scope delivered,
broader menu-content recovery remains open), TASK-0012 (PS2 asset pipeline; container + P8/RGBA
texture decode delivered, layout/text semantic decode and remaining pixel formats remain open).
Completed tasks: TASK-0000, TASK-0002, TASK-0006, TASK-0007, TASK-0008 and TASK-0010.
Next reverse task: TASK-0003 boundary/ABI audit (now prioritizing the FUN_0016fa80 envelope
surfaced by TASK-0011), then TASK-0004 original comparison setup and the scoped pilot backlog.

## Assessment snapshot — TASK-0002

Report: `docs/assessment/BINARY_REPORT.md`. Binary: D:/ps2_game/SLUS_203.62,
SHA-256 `7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`.
Region USA/NTSC inferred; exact executable revision unknown (boot VER 1.00).
Ghidra project hp2_ps2, R5900/default compiler. Existing annotations preserved.

- 7,544 unique function entries and 17,162 distinct static call relationships.
- 400,582 decoded .text instructions; complete loaded .text matched the disk ELF.
- 663 indirect jalr sites, 190 other indirect jr sites, 26 empty CFGs.
- 21 unique decompile outputs returned, 0 failed probes, 7,523 unprobed;
  global decompile success/failure totals remain unknown. Three probes contain warnings.
- 20 varied pilot selections and individual backlog tasks; none executed or verified.
- Six requested reports plus metadata/evidence exports, a reproducible summarizer,
  a risk register and explicit uncalibrated workload scenarios.

Blocked future work: trustworthy bodies/ABI for damaged entries; emulator/original
oracle setup; timed representative recovery; VU/IOP, layout, asset and adapter scope.
PCSX2 was not located in scoped tool checks; availability elsewhere is unknown.
Planning scenarios are ~12k / 79k / 480k person-hours under stated assumptions,
not a delivery forecast. See WORKLOAD_ESTIMATE.md before interpreting these totals.

Assessment validation: summary script assertions passed for population, uniqueness,
full .text byte coverage/disk agreement and pilot selection. All metadata exports
are separate from ignored local raw listings/decompilation. Final CSV/task/model
consistency checks are recorded in the completed assessment task.

## Delivered infrastructure

- Three-layer C++17 architecture; core and provisional service contracts contain no Godot dependency.
- Optional RefCounted GDExtension wrapper, sample arithmetic, smoke scene, and CTest.
- Session protocols, architecture/decision/roadmap/development documentation.
- Task lifecycle folders, templates, and next environment/provenance task.
- Empty real symbol/binary indexes; function/layout templates and symbolic dummy examples.
- Git initialized; pinned godot-cpp submodule; restricted-input/build/secret ignores.
- Existing `task.txt` and `eval.txt` preserved; no Git commit or remote/Issue created.

## Environment observed at initialization (updates below)

| Tool | Observation |
|---|---|
| CMake | 3.29.2, C:/Strawberry/c/bin/cmake.exe |
| Ninja | 1.12.0 |
| Compiler | MinGW-w64 GCC 13.2.0, x86-64 UCRT |
| Git | 2.54.0.windows.1; repository initialized here |
| Python | 3.14.4 via `py -3`; WindowsApps `python` alias failed |
| Java | Temurin OpenJDK 21.0.11 |
| Ghidra | 12.0.4 installation present |
| R5900 extension | ghidra-emotionengine-reloaded present; host compatibility metadata 12.0.4; operation untested |
| GhidraMCP | Extension description 0.7.0-pre15, compatibility 12.0.4; read-only connectivity confirmed in TASK-0002 |
| Existing Godot | 4.7.2.stable.official.ed1daf0bf in WinGet package directory, outside PATH |
| Pinned Godot | Portable 4.3.stable.official.77dcf97d8 downloaded to ignored local/tools/godot-4.3/ |
| godot-cpp | Submodule fbbf9ec4efd8f1055d00edb8d926eef8ba4c2cce |
| PCSX2 | Not located/validated in this initialization; not a build prerequisite |

The portable engine came from the official `godotengine/godot-builds` release
asset `4.3-stable/Godot_v4.3-stable_win64.exe.zip`. No global installation changed.
Downloaded archive SHA-256:
`8f2c75b734bd956027ae3ca92c41f78b5d5a255dacc0f20e4e3c523c545ad410`.
This is a local artifact identity, not an independently checked upstream checksum.

## Initialization validation ledger — 2026-10-09

| Check | Result | Evidence |
|---|---|---|
| Required directory structure | Passed | PowerShell Test-Path check of all 18 requested directories and 14 requested named files |
| Native configuration | Passed | `cmake --preset native-debug` exited 0 |
| Native compile | Passed | `cmake --build --preset native-debug` exited 0; core static library and test executable linked |
| Native unit tests | Passed | `ctest --preset native-debug`: 1/1 passed; arithmetic normal/zero/negative/int32 boundary sums |
| Bridge configuration | Passed | `cmake --preset godot-debug -DPython3_EXECUTABLE=<py-discovered-path>` exited 0 |
| Bridge compilation | Passed | `cmake --build --preset godot-debug --parallel 4` exited 0; ps2_port.dll linked |
| Bridge-build core tests | Passed | `ctest --preset godot-debug`: 1/1 passed |
| Godot import | Passed after fix | Portable 4.3 `--headless --path godot --editor --import` exited 0 with no errors |
| Godot scene smoke | Passed | Portable 4.3 `--headless --path godot -- --smoke-test` exited 0, printed `PORT_SMOKE_OK: 42` |
| DLL runtime dependencies | Passed | `objdump -p godot/bin/ps2_port.dll` shows Windows system imports; no external MinGW runtime DLLs |
| Ignore checks | Passed | `git check-ignore` matched 11 representative build, private asset, binary, Godot, Ghidra, local-config and secret paths |
| Original behavior comparison | Not performed | Explicitly outside initialization scope |

No unavailable tool blocks native builds. Python is usable with an explicit
interpreter. Assessment established ELF identity and read-only Ghidra/MCP access;
exact release matching and a validated original comparison setup remain needed.

First host import failed with Windows loader error 126 because the initial DLL
still imported `libwinpthread-1.dll`. Godot returned exit 0 despite those errors;
that attempt was treated as failed. Corrected MinGW link options to statically
link its runtimes, relinked, inspected imports, then reran import and scene smoke
successfully. No unresolved initialization validation failures remain.
Only the scaffold call is tested; visual rendering, original gameplay, MSVC,
Linux, and the installed 4.7.2 runtime were not validated.

## Continuation

Front-end menu next step: "Read docs/PROGRESS.md, docs/FRONTEND_MENU.md and
tasks/active/TASK-0011-frontend-menu-closed-loop.md, then audit the FUN_0016fa80 envelope
(0x0016fa80..0x001e3dbf) boundary before deciding whether to decompile it; it is the strongest
lead for New Main Menu's real update/tick logic and eventual menu-item content. Start
read-only, preserve existing annotations and record evidence before any scoped repair."

Original TASK-0003 continuation (still applicable to the broader boundary-audit backlog item,
now informed by the FUN_0016fa80 priority above): "Read AGENTS.md, docs/PROGRESS.md and
docs/assessment/BINARY_REPORT.md, then work on tasks/backlog/TASK-0003-boundary-abi-audit.md.
Audit the one-byte bodies at 0x0019a4b8 and 0x001ebd20 and the shared return/ABI contracts.
Start read-only, preserve existing annotations and record evidence before any scoped repair.
Do not start full game implementation."

Original request-file SHA-256 values after scaffold creation:

- task.txt: 15244A82D71F2D1C85BBF4DC7189937B7F0ADFD63E5E7A45C1D7A63BE0E1F75B
- eval.txt: 5B2E413E9B4153C2DF6E4341417D0943063A04734C0BA0E75A551263C10EC2FA
