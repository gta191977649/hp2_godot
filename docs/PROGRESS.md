# Project state — 2026-10-12 (Australia/Sydney)

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
M2–M6 have not been achieved. Current active task: TASK-0005 (independent infrastructure).
Completed tasks: TASK-0000, TASK-0002, TASK-0006, TASK-0007 and TASK-0008. Next reverse task: TASK-0003 boundary/ABI audit,
then TASK-0004 original comparison setup and the scoped pilot backlog.

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

Use: "Read AGENTS.md, docs/PROGRESS.md and docs/assessment/BINARY_REPORT.md, then
work on tasks/backlog/TASK-0003-boundary-abi-audit.md. Audit the one-byte bodies at
0x0019a4b8 and 0x001ebd20 and the shared return/ABI contracts. Start read-only,
preserve existing annotations and record evidence before any scoped repair.
Do not start full game implementation."

Original request-file SHA-256 values after scaffold creation:

- task.txt: 15244A82D71F2D1C85BBF4DC7189937B7F0ADFD63E5E7A45C1D7A63BE0E1F75B
- eval.txt: 5B2E413E9B4153C2DF6E4341417D0943063A04734C0BA0E75A551263C10EC2FA
