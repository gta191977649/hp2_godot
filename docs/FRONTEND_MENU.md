# TASK-0011: a minimal closed loop for the front-end menu (New Main Menu)

Completed 2026-10-12, Australia/Sydney. The scope is **a structural reconstruction of the screen
switch manager plus one confirmed navigation edge** (TitleScreen -> New Main Menu), together with
an operable Godot demonstration loop. Screen-internal rendering, the actual menu item content and
the full input chain are not recovered; they are marked as open items and must not be used as
verified behaviour.

Binary: `SLUS_203.62`, SHA-256
`7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`.
USA/NTSC inferred; the actual executable revision is unknown, the boot file says VER 1.00.
Ghidra 12.0.4, `r5900:LE:32:default` / default, GhidraMCP read-only (127.0.0.1:8080).

## How the user's three requirements were met

1. **"Confirm a main menu and its adjacent pages from original screenshots and recordings."** No
   original screenshots or footage were available this session, so ELF strings and
   cross-references were used as the evidence source instead (below). The string literal
   `"New Main Menu"` (ELF 0x002b2f00) was confirmed, along with its neighbours `"TitleScreen"`
   (0x002b2f68) and `"MumboJumboScreen"` (0x002b2f78). The TitleScreen -> New Main Menu edge is
   confirmed by decompilation (see E-0011-12); edges between MumboJumboScreen and the other two
   are **not confirmed** and it is recorded only as a sibling screen class registered alongside
   them.
2. **"Trace NewMainMenu's creation, update, input callback and page switching."**
   - **Page switching**: fully confirmed. `FUN_0014cc78` (the RequestScreen wrapper) ->
     `FUN_0016c4c0` (the real switch/activate/destroy/history logic) -> `FUN_0016c8c8` (find a
     registered screen by name).
   - **Creation**: the "class dispatch functions" of `TitleScreen` (`FUN_0016f440`) and
     `MumboJumboScreen` (`FUN_0016f810`) were found — message 0xb constructs, 0xc destroys, and
     anything else forwards to the instance handler — which is evidence of the engine's creation
     convention. But **"New Main Menu"'s own class dispatch/constructor was not found this
     session**: all three references to its string are call sites that *request a switch to* it,
     not construction sites. Recorded explicitly as an open item.
   - **Update**: `FUN_0016fa80` (a huge function, entry 0x0016fa80, with Ghidra reporting a
     function envelope extending to 0x001e3dbf, about 465 KB) routes state values to
     "New Main Menu" (states 2-7) or to an empty string (states 8-9) several times within its
     first ~176 bytes, making it a strong candidate for New Main Menu's own update/tick logic.
     TASK-0003/0006/0007 had repeatedly flagged that envelope as a possible Ghidra boundary
     misjudgement, and this session **did not** fully decompile it or audit its boundary; only a
     read-only listing slice was inspected.
   - **Input callback**: `FUN_0016eef8` (the TitleScreen instance message handler) was found; its
     `message==1` branch triggers `RequestScreen("New Main Menu", 1)` when the internal state and
     busy flag conditions are met — a confirmed chain from "message" to "page switch". The
     concrete path from **raw pad input** (this session also found the string constants
     `JOY_EVENT_MENU_SELECT` / `_CANCEL` / `_BACK` / `_HELP`) to the dispatch of "message==1" was
     **not traced**; an explicit open item.
3. **"Restore navigation state in the native core and show an operable UI in Godot."** Done; see
   the implementation and validation records below.

## Evidence overview

Details are in each function's own record (`reverse/functions/<binary-id>/frontend/*.md`, evidence
IDs E-0011-01 to E-0011-17) and in the frontend subsystem rows of `reverse/symbols.csv`. The core
chain:

```mermaid
flowchart LR
    TS[TitleScreen instance\nFUN_0016eef8 message=1] -->|RequestScreen name push=1| REQ[FUN_0014cc78]
    REQ --> SET[FUN_0016c4c0\nSetActiveFrontEndScreen]
    SET --> FIND[FUN_0016c8c8\nFindScreenByName]
    SET --> PUSH[FUN_0016b418\nPushHistory]
    SET --> POP_NOTE["FUN_0016b438 PopHistory\n(used for the Back combination; not confirmed at a single call site)"]
    SET --> RESET[FUN_0016b460\nResetManager]
    FIND -->|strcmp| CMP[FUN_002535f8]
    FA[FUN_0016fa80\n(oversized/unaudited; routes several\nstates to "New Main Menu")] -.-> SET
```

Key strings and addresses (all from read-only `memory search` / `read` calls against the
authorized ELF):

| String | ELF address | Role |
|---|---|---|
| `New Main Menu` | 0x002b2f00 | the only confirmed main menu screen name |
| `TitleScreen` | 0x002b2f68 | the confirmed adjacent/source screen (class FUN_0016f440) |
| `MumboJumboScreen` | 0x002b2f78 | a confirmed sibling screen class (class FUN_0016f810); the edge is unconfirmed |
| `Title Screen` / `Legal MumboJumbo` / `BootAutoload` / `Language Options Pre FE` | 0x002b2f90 / 0x2b2fa0 / 0x2b2fb8 / 0x2b2fc8 | "already shown" markers `FUN_0016f950` pushes onto the history stack — **not** a confirmed navigation order |
| `JOY_EVENT_MENU_SELECT` / `_CANCEL` / `_BACK` / `_HELP` | around 0x002b02xx-0x2b033x | pad menu event name constants exist; no call relationship to `FUN_0016eef8` was traced |
| `GRID` / `MASTER_BP_OBJECT` / `FADING_BP_OBJECT` | 0x00331e60 / 0x002befd0 / 0x002befe8 | the three named behaviour objects `FUN_00249a40` binds (apparently the bWare engine's blueprint system, not per-key input callbacks) |

Structural fields of the manager singleton (the gp-relative global `uGpffffacb4`); all from
decompilation evidence, with the full layout unknown:

| Offset | Meaning | Evidence |
|---|---|---|
| `+0x000..0x180` | history stack, 12 slots of 0x20 bytes (only the name field at the head of a slot is confirmed) | E-0011-05/06 |
| `+0x180` | history count (int32) | E-0011-05/06 |
| `+0x184` | an unresolved flag, adjacent to the history area but not cleared by Reset | E-0011-02/07 |
| `+0x194` | head of the circular intrusive list of registered screens | E-0011-03 |
| `+0x19c` | current screen pointer | E-0011-02 |
| `+0x1c8` | the behaviour binding record (GRID / MASTER_BP_OBJECT / FADING_BP_OBJECT) | E-0011-11 |

## Native implementation (a structural projection, not a byte-for-byte copy)

`src/game_core/frontend/screen_manager.hpp/.cpp` holds `hp2::frontend::ScreenManager`, pure C++17
and independent of Godot. It reproduces the confirmed manager-level API: `register_screen`,
`request_screen(name, push_history)` (matching `FUN_0014cc78` -> `FUN_0016c4c0` ->
`FUN_0016c8c8`), `go_back()` (the combination of PopHistory and `request_screen(name,false)`,
which is a **structural assumption** rather than something confirmed at a single call site) and
`reset()` (matching `FUN_0016b460`, which clears only the history, not the registry). The history
capacity is hardcoded to the 12 slots the evidence supports; a 13th push returns `HistoryFull`
rather than inventing the original's overflow behaviour.

**Deliberately not reproduced** (excluded to avoid invention): screen-internal rendering and the
child widget tree, `FUN_0016eef8`'s instance-level state machine, `FUN_0016fa80`'s full update
logic, `FUN_00249a40`'s behaviour binding, and the mapping from raw pad input to messages.

Unit tests: `src/tests/screen_manager_tests.cpp`, 9 cases covering the empty state, looking up an
unregistered name, history pushes along the evidenced TitleScreen -> MumboJumboScreen -> New Main
Menu chain, Back navigation, Back on an empty history, clearing the current screen, the history
capacity limit, and Reset preserving the registry. `ctest --preset native-debug` and
`--preset godot-debug`: 7/7 (including the 5 pre-existing tests, with no regression).

## The operable Godot UI

A new scene `godot/scenes/frontend_menu_observer.tscn` plus
`godot/scripts/frontend_menu_observer.gd`, backed by the bridge class
`port::godot_bridge::FrontEndMenuObserver` (`src/platform/godot_bridge/
frontend_menu_observer.hpp/.cpp`, registered in `register_types.cpp`). The original `main.tscn`,
`main_loop_observer.tscn` and existing APIs are all untouched.

The page shows the current screen name, history depth and capacity, history contents, the list of
registered screens (with the current one highlighted) and an evidence footnote. Buttons:

- **Press Start (confirmed: TitleScreen -> New Main Menu)** — invokes the confirmed edge. Pressing
  it again while on New Main Menu switches back to TitleScreen (also via push_history=true), which
  exists to demonstrate the loop and does not imply the original has that reverse call site.
- **Construct MumboJumboScreen (registered sibling; edge unconfirmed)** — labelled explicitly so
  it cannot be mistaken for verified game behaviour.
- **Back** — pops the history and requests that screen with push_history=false.
- **Reset** — calls `initialize()` again, returning to TitleScreen with an empty history.

```powershell
# build and open, first time or after a source change
./tools/run_frontend_menu_observer.ps1 -Build
# open with the current DLL (interactive)
./tools/run_frontend_menu_observer.ps1
# headless acceptance
./tools/run_frontend_menu_observer.ps1 -Smoke
# screenshot (needs an absolute path)
./local/tools/godot-4.3/Godot_v4.3-stable_win64_console.exe --path godot res://scenes/frontend_menu_observer.tscn --quit-after 300 -- --frontend-menu-capture=<absolute path.png>
```

## Validation record

- `cmake --preset native-debug` / `--build` / `ctest`: 7/7 (with the new `screen_manager` cases).
- `cmake --preset godot-debug` / `--build --parallel 4` / `ctest`: 7/7.
- Godot headless `--editor --import`: no ERROR.
- `res://scenes/frontend_menu_observer.tscn -- --frontend-menu-smoke`:
  `FRONTEND_MENU_OBSERVER_SMOKE_OK` passes (initial screen, the confirmed edge, Back navigation,
  the placeholder sibling switch, Reset).
- The existing `main_loop_observer.tscn -- --observer-smoke` and the original `--smoke-test` show
  no regression, printing `MAIN_LOOP_OBSERVER_SMOKE_OK` and `PORT_SMOKE_OK: 42` respectively.
- Real rendered screenshots: `local/frontend-menu-observer.png` (the initial TitleScreen state)
  and `local/frontend-menu-observer-after-press-start.png` (after Press Start, with New Main Menu
  current and TitleScreen on the history stack), both 1380x760, manually checked for correct text
  and layout. Both live under `local/`, which `.gitignore` excludes, so they are not distributed
  with the repository.
- Tool versions: Godot 4.3.stable.official.77dcf97d8; GCC 13.2.0; Python 3.14.4; CMake 3.29.2.
- Not done: any PCSX2 / original-behaviour comparison, any Ghidra annotation/name/boundary change,
  and any Git commit (the user manages those).

## 9. Addendum 2026-10-12: the original source file paths (`indep\src\fmenus.cpp` and friends)

A `.cpp` / `.hpp` string search over the ELF (leftover `__FILE__` expansions) recovered the
original source directory structure directly. `indep\src\fmenus.cpp` and
`indep\src\frontendscreens\{selectcar,racesetup,options,joystickconfiguration}.cpp` line up
closely with the functions this document traces (the nearby strings `MENUITEM_%d`, `tHelpMenu` and
`tPopupMenu` match the `UwEMENUITEM_*` strings TASK-0012 found independently in the asset files).
See the new document [docs/ORIGINAL_SOURCE_TREE.md](ORIGINAL_SOURCE_TREE.md); that is
project-level evidence and covers more than the front-end menus. No concrete function address has
been pinned to a specific file or line yet, which is an explicit open item.

## 8. Addendum 2026-10-12: the central GAMEFLOW_STATE_* machine (a major structural finding that corrects an earlier assumption)

The user pointed out that a mature, independently validated HP2 PS2 asset tool project already
exists on this machine (`D:\dev\eagl-godot\map_tools_ps2`) and asked for "no guessing — reverse
the original menu source properly, using Ghidra MCP". That project's texture decode reference
implementation was used first to fix TASK-0012's texture decoding bug (see docs/ASSET_PIPELINE.md
§4.4), after which work returned to the ELF and produced the following:

### 8.1 The earlier assumption about `FUN_0016fa80` was wrong

TASK-0011's first pass read only ~176 bytes of `FUN_0016fa80`'s listing and, together with the
`end_address: 0x001e3dbf` (about 465 KB) that Ghidra's `functions get` reported, **wrongly assumed**
it was New Main Menu's own oversized update/tick function. A full `decompile` this round shows the
body is only **312 bytes**, clean and complete. The oversized end_address `functions get` reports
is most likely a bookkeeping anomaly in Ghidra's function boundary record (it does not reflect
real reachable code), not the "boundary merge error" that had been mentioned repeatedly before.
**This assumption is retired; do not use it again.**

### 8.2 The real finding: the original developers' `GAMEFLOW_STATE_*` enum

Decompiling `FUN_0016fa80`'s only caller, `FUN_00168830` (address 0x00168830), shows it finishes
by calling `FUN_001680d8(4, 0)` — and `0x001680d8` / `0x001680e8` are exactly the "State
Module::State, Phase::StateDispatch" call targets already known from TASK-0010's main loop record
(`0x001680e8` appears in the per-frame dispatch). Fully decompiling both confirms a standard
**state machine dispatcher**:

```c
// 0x001680d8: request a switch to a gameflow state
void HP2_RequestGameFlowState(int state, int param) {
    DAT_00330548 = state;   // target state
    DAT_0033054c = param;
}

// 0x001680e8: the per-frame pump (the known main loop call target)
void HP2_PumpGameFlowStateMachine(void) {
    // if current != target: call current's exit -> switch -> call the new state's enter -> repeat until stable
    // if equal: call the current state's update, once per frame
}
```

The state table sits at ELF `0x002faea4`, 16 bytes per row: `+0x0 = enter function pointer,
+0x4 = exit function pointer, +0x8 = update function pointer, +0xC = pointer to the original ASCII
enum name string`. Reading the string at +0xC yields **the real enum constant names the original
developers wrote** — not guesses, but ASCII strings sitting in the ELF:

| Row | Original name (ELF string, confirmed byte by byte) | enter | exit | update |
|---|---|---|---|---|
| 0 | `GAMEFLOW_STATE_LOADING_FRONTEND` | 0x00168650 (empty, just `jr ra`) | 0 | 0 |
| 1 | `GAMEFLOW_STATE_UNLOADING_FRONTEND` | probed, not confirmed as a real function entry | 0x001687a8 (decompiled) | probed, unconfirmed |
| 2 | `GAMEFLOW_STATE_IN_FRONTEND` | 0x00168830 (decompiled, see below) | 0 | 0 |
| 3 | `GAMEFLOW_STATE_LOADING_TRACK_COMMON` | 0 | 0 | 0 |
| 4 | `GAMEFLOW_STATE_LOADING_TRACK_SPECIFIC` | 0x00168db8 (not decompiled) | 0 | 0 |
| 5 | `GAMEFLOW_STATE_RACING` | 0x001690a0 | 0x00169168 | 0x00169160 (none decompiled) |
| 6 | (string not read) | 0 | 0 | 0 |
| 7 | `GAMEFLOW_STATE_UNLOADING_TRACK_COMMON` (inferred from table order, not individually rechecked) | 0x00169288 | 0 | 0 |
| 8 | `GAMEFLOW_STATE_EXIT_DEMO_DISC` | 0x00168ea0 | 0 | 0 |
| 9 | (+0xC is 0, no name) | 0x00169490 | 0x00169528 | 0x001694a0 |

### 8.3 Connections to TASK-0011's confirmed evidence

- **The startup sequence** (recorded by TASK-0010 but not understood at the time): the main loop's
  startup area at `0x0012f048` calls `HP2_RequestGameFlowState(1, 0)`, i.e. it requests
  `GAMEFLOW_STATE_UNLOADING_FRONTEND` — exactly the call site TASK-0010 noted as "calls
  0x001680D8" in the `gp_a54c==0` branch without interpreting its arguments.
- **`GAMEFLOW_STATE_UNLOADING_FRONTEND`'s exit callback** (`0x001687a8`, fully decompiled)
  finishes by calling `FUN_0016f950()` — which is exactly the function TASK-0011 confirmed as "the
  attract sequence gate that requests a switch to New Main Menu under certain conditions". This is
  the first time the top-level gameflow state machine and TASK-0011's front-end screen manager
  evidence chain are joined up.
- **`GAMEFLOW_STATE_IN_FRONTEND`'s enter callback** (`0x00168830`, fully decompiled) calls
  `FUN_0016fa80()` (see the §8.1 correction) and then **unconditionally** calls
  `HP2_RequestGameFlowState(4, 0)`, i.e. it immediately requests
  `GAMEFLOW_STATE_LOADING_TRACK_SPECIFIC`. In other words, within this dispatcher "entering
  IN_FRONTEND" is not a state the game lingers in and updates per frame (its update slot is
  empty), but a transient one that immediately requests a track load. That is unexpected and is
  recorded as an open hypothesis: perhaps the PS2 main menu's backdrop is a live-rendered attract
  race (rather than a pre-rendered video) and so needs the track loaded first; or perhaps another
  path drives the real "player browsing the menu" update loop, which this round did not find.

### 8.4 An honest assessment

This is a substantial structural finding backed by real Ghidra evidence — especially having the
original enum name strings in hand — but **the goal of "fully reversing the original menu source"
is nowhere near met**. Seven gameflow state handler functions remain undecompiled, `FUN_00168830`
has 7 untraced subsystem calls left and `FUN_0016fa80` has 4, and most importantly **the update
and input loop that actually drives "the player selecting menu items in New Main Menu" is still
not located**. Do not treat this section as a complete reconstruction of the menu system.

## Open questions and next steps

**2026-10-12 update**: item 1 (the boundary and purpose of `FUN_0016fa80`) is resolved and
corrected in §8 — it is not an oversized update function but a 312-byte one-shot gameflow
transition cleanup. The list below is updated to reflect the real priorities after §8:

1. **Highest priority**: the per-frame update and input loop that actually keeps the player in New
   Main Menu browsing and selecting items is still not located. `GAMEFLOW_STATE_IN_FRONTEND`'s
   update slot is empty, and its `enter` (`FUN_00168830`) immediately requests a switch to
   `GAMEFLOW_STATE_LOADING_TRACK_SPECIFIC`, which contradicts the intuition of "lingering in the
   menu" (last paragraph of §8.3). That contradiction has to be resolved before anything else.
2. The 11 located but undecompiled gameflow enter/exit/update functions (0x00168698, 0x00168718,
   0x00168db8, 0x001690a0, 0x00169160, 0x00169168, 0x00169288, 0x00168ea0, 0x00169490,
   0x001694a0, 0x00169528; see the §8.2 table), plus the remaining untraced subsystem calls in
   `FUN_00168830` and `FUN_0016fa80`.
3. "New Main Menu"'s own class construct/destruct dispatch function is not located.
4. The dispatch path from raw pad input (the `JOY_EVENT_MENU_*` constants) to `FUN_0016eef8`'s
   message==1 is not traced.
5. The real navigation order between `MumboJumboScreen`, `FUN_0016f950` (the attract sequence
   gate) and New Main Menu / TitleScreen is unconfirmed; the "blank entry" maintenance semantics
   of `FUN_0016c7b8` in the history stack are not fully resolved.
6. The identity of the GRID / MASTER_BP_OBJECT / FADING_BP_OBJECT behaviour object system that
   `FUN_00249a40` binds is unresolved.
7. The child widget list (`screen+8`) and the child lifetime functions `FUN_0016ad38` /
   `FUN_0016a858` / `FUN_0016a940` are all undecompiled, and are a prerequisite for any work on
   the menu's "content".

Continuation hint: read this document (especially §8) and the `frontend` subsystem rows of
`reverse/symbols.csv`, then resolve the contradiction in item 1 first — decompile 0x00168db8,
0x001690a0 and the other states to see whether RACING / LOADING_TRACK_* ever loop back to
IN_FRONTEND, or whether "browsing the menu" is not driven by this gameflow machine at all but
implemented inside some state's update callback with its own sub-state machine.
