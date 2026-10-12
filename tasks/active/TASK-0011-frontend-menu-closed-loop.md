# TASK-0011 — Front-end menu (NewMainMenu) minimal closed loop

- Status: active (scoped closed-loop deliverable complete; broader menu-content recovery open)
- Objective: Confirm a main menu screen and an adjacent page from the original binary, trace
  NewMainMenu creation/update/input-callback/page-switching as far as evidence supports, restore
  the confirmed navigation state in the native core, and show an operable Godot UI driven by it.
- Scope: Included — manager-level screen-switch/find/history API (RequestScreen, FindByName,
  PushHistory, PopHistory, Reset), the confirmed TitleScreen -> New Main Menu edge, a native
  structural-projection `ScreenManager`, a Godot bridge + operable scene, unit/smoke tests, and
  evidence records for every function touched.
  Excluded — menu-internal rendering/widget trees, New Main Menu's own content/items,
  FUN_0016fa80's full body (oversized, flagged for TASK-0003), raw-input-to-message wiring,
  PCSX2/original behavior comparison, any Ghidra mutation.
- Original PS2 addresses (binary sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2):
  0x0014cc78, 0x0016c4c0, 0x0016c8c8, 0x0016c7b8, 0x0016b418, 0x0016b438, 0x0016b460, 0x0016afc8,
  0x0016b068, 0x0016b0b0, 0x00249a40, 0x0016eef8, 0x0016f440, 0x0016f810, 0x0016f950, 0x0016fa80,
  0x002535f8. String literal "New Main Menu" at 0x002b2f00.
- Dependencies: Ghidra 12.0.4 + GhidraMCP (127.0.0.1:8080, read-only), existing `tools/ghidra.py`
  client, existing EngineShell/ModuleRegistry infra pattern (TASK-0010) mirrored for the bridge
  class shape. TASK-0003 boundary/ABI audit is a dependency for any further FUN_0016fa80 work.
- Relevant files: src/game_core/frontend/screen_manager.{hpp,cpp}, src/tests/screen_manager_tests.cpp,
  src/platform/godot_bridge/frontend_menu_observer.{hpp,cpp}, src/platform/godot_bridge/register_types.cpp,
  godot/scenes/frontend_menu_observer.tscn, godot/scripts/frontend_menu_observer.gd,
  tools/run_frontend_menu_observer.ps1, CMakeLists.txt, docs/FRONTEND_MENU.md,
  reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/*.md,
  reverse/symbols.csv.
- GitHub issue: not created.

## Acceptance criteria

- [x] At least one confirmed main-menu-adjacent page edge, traced from decompiled code (not
      guessed from strings alone): TitleScreen -> New Main Menu via FUN_0016eef8 message==1.
- [x] Manager-level page-switching mechanism (creation-lookup, update-of-current, input-triggered
      switch, and the switch call itself) traced with decompiled evidence for each link in the
      chain, with honest gaps recorded where the chain is not fully closed (New Main Menu's own
      constructor; raw input to message dispatch; FUN_0016fa80's full body).
- [x] Native core models the confirmed navigation state (current screen + bounded history stack)
      as a Godot-independent C++ class with unit tests.
- [x] Godot shows an operable UI driven by the native core: current screen, history, registered
      screens, and buttons that exercise the confirmed edge plus back/reset navigation, clearly
      labelling which edges are confirmed vs. structural placeholders.
- [x] Native and bridge CTest suites pass with no regressions to pre-existing tests.
- [x] Godot headless import and a dedicated smoke test pass; at least one real rendered screenshot
      captured and visually checked.
- [x] Every function touched has a reverse/functions record with Confirmed/Hypotheses/Unresolved
      sections and an evidence ID; reverse/symbols.csv updated.
- [ ] Original PCSX2/behavior comparison (explicitly out of scope for this pass; tracked as open).
- [ ] New Main Menu's own class constructor located (open; next-session work).
- [ ] FUN_0016fa80 boundary audited and, if confirmed as one function, decompiled (open; belongs
      to TASK-0003's remit, highest-priority next step for this area).

## Verification procedure

```powershell
cmake --preset native-debug
cmake --build --preset native-debug
ctest --preset native-debug          # expect 7/7, including new `screen_manager`
cmake --preset godot-debug -DPython3_EXECUTABLE=<py>
cmake --build --preset godot-debug --parallel 4
ctest --preset godot-debug           # expect 7/7
./tools/run_frontend_menu_observer.ps1 -Smoke   # FRONTEND_MENU_OBSERVER_SMOKE_OK
./tools/run_main_loop_observer.ps1 -Smoke       # unchanged, still MAIN_LOOP_OBSERVER_SMOKE_OK
```

Original comparison scope: none performed; this task only claims structural fidelity to the
decompiled call graph, not runtime/visual equivalence with the original game.

## Current findings

See docs/FRONTEND_MENU.md for the full evidenced writeup (E-0011-01..17). Summary: the
manager-level screen-switch/history API is fully decompiled and well-evidenced (high confidence
on RequestScreen/SetActiveScreen/FindByName/PushHistory/PopHistory/Reset). TitleScreen -> New
Main Menu is a confirmed edge from a fully-decompiled instance message handler. MumboJumboScreen
is a confirmed sibling registered class with an unconfirmed edge. FUN_0016fa80 (previously
flagged oversized in TASK-0006/0007) routes several state values directly to "New Main Menu" in
its opening instructions — the strongest available lead for New Main Menu's own update logic —
but remains unaudited as a whole.

## Unresolved questions

- Is FUN_0016fa80 genuinely one ~465KB function, or a Ghidra boundary-merge error? (TASK-0003)
- Where is New Main Menu's own construct/destruct class dispatcher (the FUN_0016fXXX-style
  counterpart to FUN_0016f440/FUN_0016f810)?
- How does raw joypad input (JOY_EVENT_MENU_SELECT/CANCEL/BACK/HELP strings exist, call sites not
  traced) reach FUN_0016eef8's message==1 dispatch?
- Does FUN_0016f950's history-push ordering (Title Screen/Legal MumboJumbo/BootAutoload/Language
  Options Pre FE) reflect the actual navigation order, or only "already shown" bookkeeping?
- What do FUN_0016c7b8, FUN_00249a40, and the children-list lifecycle functions
  (FUN_0016ad38/FUN_0016a858/FUN_0016a940) actually do?

## Next steps

1. TASK-0003-style boundary/ABI audit of FUN_0016fa80 (highest value: likely unlocks New Main
   Menu's real update logic and menu-item content).
2. Trace FUN_0016eef8's raw-input source (follow callers of FUN_0016f440/the class dispatcher,
   and/or the JOY_EVENT_MENU_* string cross-references) to close the input-callback gap.
3. Locate New Main Menu's own class dispatcher/constructor.
4. Only after 1-3: consider porting more of New Main Menu's actual content/behavior, and look at
   PCSX2 for a first visual/behavioral comparison baseline.

## Related Git commits

None yet. The user committed the pre-existing project scaffold independently during this session
(commit 9e42866, "add"); this task's new files were not included and remain uncommitted as of
this task's last update.

## Completion evidence

2026-10-12 (Australia/Sydney): native CTest 7/7 (`cmake --preset native-debug`, `--build`,
`ctest`), bridge CTest 7/7 (`cmake --preset godot-debug -DPython3_EXECUTABLE=...`,
`--build --parallel 4`, `ctest`), Godot 4.3.stable.official.77dcf97d8 headless
`--editor --import` clean, `res://scenes/frontend_menu_observer.tscn -- --frontend-menu-smoke`
printed `FRONTEND_MENU_OBSERVER_SMOKE_OK`, prior `--observer-smoke` and `--smoke-test` unchanged.
Two real screenshots captured and visually inspected (local/frontend-menu-observer.png,
local/frontend-menu-observer-after-press-start.png; git-ignored). Tool versions: GCC 13.2.0,
Python 3.14.4, CMake 3.29.2, Godot 4.3.stable.official.77dcf97d8, Ghidra 12.0.4 + GhidraMCP.
Remaining limits: see Acceptance criteria unchecked items and Unresolved questions above. This
task stays in `active/`, not `completed/`, because the broader "trace NewMainMenu creation and
full update/input chain" request is only partially closed.
