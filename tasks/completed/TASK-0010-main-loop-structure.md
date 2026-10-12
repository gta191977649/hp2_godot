# TASK-0010 — Main-loop structure and module skeleton

- Status: completed (structural shell scope); date 2026-10-12 Australia/Sydney.
- Scope: evidence-backed startup/frame control flow, ordered call-site/module slots,
  native C++17 resumable EngineShell, strict/observation modes, separate Godot
  observer with synthetic scenarios. No gameplay/module internals or class hierarchy.
- Binary SHA-256: 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
  USA/NTSC inferred, executable revision unknown, boot VER 1.00.
- Acceptance: all 39 static callees and repeated call sites mapped; branch/order
  tests use independently reviewed expectations; nonblocking frame-boundary wait
  resumes tail once; strict missing/failed handlers stop at exact call site;
  native/bridge tests and Godot observer/headless scenario smoke pass; provenance,
  architecture/extension guidance and continuation recorded. No Verified promotion.
- Preserve existing TASK-0005/TASK-0009, demo scene/APIs, staged submodule files,
  previous private output and Ghidra annotations. No Ghidra mutations intended.
- Implementation contracts are port architecture, not original engine APIs.
- Related commits: none.

## Acceptance and validation ledger

- E-0010-01/02: 61 loop callsites, 39 static targets, five startup sites and 12
  guards; known operands and unknown identifiers preserved in reverse/main_loop/.
- E-0010-03: 19 nested world connections, including controller virtual slot,
  entity/mover/remainder-step chain and a tail J; caller instructions and three
  file-backed vtable target words checked. Do not treat 00131A80 as mover.
- Native C++17 EngineShell/ModuleRegistry/FrameContext/FrameTrace, saved s2/f20,
  nonblocking boundary wait, strict terminal failures, bounded per-frame trace,
  callback-driven gates and original ordering implemented as structural projection.
- Manual expected sequences cover all 61 main sites; fixtures do not derive
  expectations from scheduler metadata. Added handler failure/missing return,
  captured operands despite context mutation, startup gate changes, negative/zero/
  positive and wrapping boundary difference tests.
- `cmake --preset native-debug`; `cmake --build --preset native-debug`: passed.
  `ctest --preset native-debug`: 6/6 passed.
- `cmake --preset godot-debug`; `cmake --build --preset godot-debug --parallel 4`:
  passed. `ctest --preset godot-debug`: 6/6 passed.
- Godot `--headless --path godot --editor --import`: passed after registration fix.
  Observer scene `--observer-smoke`: MAIN_LOOP_OBSERVER_SMOKE_OK; six scenarios,
  wait/resume, strict, startup and pause. Original scene `--smoke-test`: PORT_SMOKE_OK: 42.
- `./tools/run_main_loop_observer.ps1 -Smoke`: passed with error/marker checks.
- `py -3 reverse/scripts/validate_main_loop.py`: passed; live 51-record metadata,
  ELF/evidence hashes, public catalogs, nested calls, vtable words, native/bridge
  suites and both smoke runs checked. Exact args/logs/tool versions in private
  ps2_decomp/main-loop/validation.json. No Ghidra mutations.
- Actual renderer screenshot local/main-loop-observer.png (1380x900) captured and
  visually checked; source/runtime values unchanged by rounded UI display.
- Initial failure: new extension class registered outside SCENE conditional,
  producing duplicate-registration ERROR despite exit 0. Added braces; rebuilt,
  imported and reran tests with no ERROR. Failed attempt was not counted as passed.
- REA skill v37 explicitly invoked by user; its staged/parallel evidence workflow
  applied via two read-only reviews. No REA server registered/CLI available;
  evidence producer remained Ghidra MCP, not REA retained Evidence.
- Startup/main/engine/controller records and symbols index updated without
  claiming module internals implemented. No Verified promotion; original comparison
  not performed, outside skeleton acceptance. No game assets/ELF/DB staged.
- Git staged .gitmodules and godot-cpp entries preserved; no new commit/staging.

## Continuation

Read docs/MAIN_LOOP_STRUCTURE.md and the address-keyed catalogs. Resume TASK-0009
with World object/partition and entity ABI; preserve dynamic loops and virtual
adjustments, then bind a tested real handler by target or callsite. Supply actual
clock/base-step/global-state adapters before strict execution can run. Keep original
names/enum values unknown until evidence supports them. Original behavior traces
are required for Verified status. Existing TASK-0005 remains independent.
