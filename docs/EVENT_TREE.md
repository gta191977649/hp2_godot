# TASK-0018: `tEventTreeView` — data source, geometry, navigation and UI

> **TASK-0022 continuation (2026-10-13):** Status/Reward text, document17 details,
> runtime title and document names, zoom/alpha transitions, connector rendering,
> selection border, and car/track logo effects now have native implementations.
> See [the integration record](TASK_0022_INTEGRATION.md) for E-0022 evidence and
> remaining gaps. Several earlier interpretations below are superseded: node+0x0C
> does not double drawn size, detail zoom is3.0, the tree uses native interpolators
> rather than tStillCameraMover, and001FAE40 enters reward preview rather than a race.
> TASK-0022 remains active; full3D preview and original equivalence are not recovered.

> **TASK-0021 update (2026-10-13).** The save system was reversed
> ([docs/SAVE_SYSTEM.md](SAVE_SYSTEM.md)), which unblocked five of the six open questions in
> section 7. Section 7 below now records the state of each one. Two claims on this page are
> **superseded**: the `0x34A03` table in section 1 is a *requirement* table, not a link table
> (section 7.4), and the camera constants in section 7.1 are recovered (section 7.1). Section 6's
> file paths were stale from the TASK-0020 move and are corrected. Nothing on this page was
> deleted; superseded text is marked where it stands (AGENT.md R6).

Completed 2026-10-13. Binary `SLUS_203.62`, SHA-256
`7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`; Ghidra 12.0.4, GhidraMCP
**read-only**, no annotations, renames or boundary edits were made. The original source file name
is recovered: `..\indep\src\eventtree.cpp` (ELF string 0x002BA9F8), so the port-side file carries
the same name, `src/game_core/frontend/eventtree.cpp`.

## 1. The data source is in FRONTB.BUN

`FUN_001FFC40` parses one `0x80034A00` container per tree, and **FRONTEND/FRONTB.BUN holds
exactly two of them**, named `Championship` and `Hot Pursuit` by their `0x00000001` child chunk.
`FUN_00185568` chooses between them using `profile+0x1072C`: **8 selects Championship (the World
Racing branch) and 9 selects Hot Pursuit (the branch whose menu entry reads Ultimate Racer)**.
Those are exactly the values the two mode-select screens write into `DAT_0030C77C`.

Sub-chunk to tree-field mapping, all read out of `FUN_001FFC40`:

| Chunk | Stored at | Row size | Championship / Hot Pursuit |
|---|---|---|---|
| `0x00000001` | tree name | string | "Championship" / "Hot Pursuit" |
| `0x00034A01` | tree+0x14 | 4 bytes | 25 / 30 rows |
| `0x00034A02` | tree+0x1C | 4 bytes | 68 / 56 rows |
| `0x00034A03` | tree+0x24 | **8 bytes** | 33 / 38 rows (**the link table**) |
| `0x00034A04` | tree+0x2C | 8 bytes | 21 / 21 rows |
| `0x00034A05` | the event records | **0xBC = 188 bytes** | **32 each** |
| `0x00034A06` | tree+0x34 | 4 bytes | 132 / 59 rows |

`FUN_001FFF60` divides the `0x34A05` payload by 0xBC, and 6016 / 188 = **32 events**, which lines
up with the game's own "30 events" copy (the two extra are the special nodes 31 and 32).

Record fields confirmed so far, every one of them taken from the code that reads it rather than
from staring at bytes:

| Offset | Meaning | Evidence |
|---|---|---|
| +0x08 | 32-byte event name, e.g. "Class 5 Parkland Race" | directly readable |
| +0x28 | **event number**, 1-based | `FUN_001FA840` looks nodes up by it |
| +0x29 | **GoalType** | the switch in `FUN_00200DE8` |
| +0x3D | incoming link count | `FUN_002008B8`'s bound |
| +0x42 | this event's row in the link table | `FUN_002010B8` |
| +0x7C | s16 array, each entry a `0x34A03` row index | `FUN_002008B8` |
| +0xB0 | localised description string id | `FUN_001FA9C8` feeds it to the scrolling text |

Each link-table row is 8 bytes and **+0x02 is the event number at the other end** (the only caller
of `FUN_002008B8` reads just those two bytes). The remaining six bytes are unidentified.

## 2. Node placement: an ASCII map hidden in the ELF

`FUN_001F96B8` places nodes in an unusual way: it walks a **200-character ASCII map**, counting
non-space characters, and when it has counted `record+0x28` of them it stores
`130.0f * (position % 20)` as that event's X. So the map is **10 rows of 20 columns** and the
n-th `x` is event n. The two maps are `DAT_0031B3C0` (Championship) and `DAT_0031B490`
(Hot Pursuit), 32 marks each.

```
Championship                          Hot Pursuit
row0           1                      row0           1
row1       2       3                  row1       2       3
row2   4     5  6  7  8               row2   4   5 6 7   8
row3  9 10   11 12   13   14          row3  9 10    11  12  13  14
row4 15  16 17 18 19    20  21        row4  15    16   17 18 19 20  21
row5 22  23   24 25 26    27          row5 22 23      24   25 26 27
row6       28       29                row6      28           29
row7           30                     row7           30
row8           31                     row8           31
row9           32                     row9           32
```

Y does not come from the map row: `FUN_001F7730` returns `depth * 350.0f`, where `FUN_001F7FA8`
computes depth by following the first incoming link back to the root and counting hops. Column
pitch `gp-0x41B0` = **130.0**, row pitch `gp-0x41B4` = **350.0**.

**This matches the original-hardware screenshot the user supplied**: event 15 is selected with
9 and 10 above it and 16 to the right, which is exactly the Hot Pursuit map's row 3 (9 at col 1,
10 at col 3) against row 4 (15 at col 1, 16 at col 6). So that screenshot is the **Hot Pursuit
tree displayed under the title "Ultimate Racer"**, consistent with the menu entry's localised
label.

## 3. Node state and artwork

`FUN_001F75F0` picks a node's panel texture from the tree type (0 = Championship, 1 = Hot Pursuit)
and the status `FUN_002010B8` returns: **0 → `_LOCKED`, 1/2/3 → `_UNLOCKED`, 4 → `_WON`**. The
texture names are the ELF's own strings, `SPEED_EVENT_PANEL_*` and `PURSUIT_EVENT_PANEL_*`.

`FUN_001FA9C8` writes **1.0f** into the +0x0C of the node it leaves and **2.0f** into the node it
moves to. That is why the selected star in the screenshot is twice the size of its neighbours: it
is node scale, not a second set of textures.

## 4. The profile (save) data source

`FUN_001FE988(tree, block, 0x100, 4)` hands a **256-byte block** out to the 32 events —
`DAT_002FC1D8` for the Championship tree and `DAT_002FC2D8` for Hot Pursuit. How much each event
takes is decided by its GoalType (`FUN_00200DE8`: types 0 and 2 take 1 byte, 1/5/6 take 4, 3/4
take 16).

One detail is worth calling out: it makes **two passes**. The first takes only the events of 4
bytes or more and rounds the cursor up to 4 after each; the second takes the 1-byte events and
packs them **with no rounding at all**. The first implementation here used a single aligned pass,
overflowed the 0x100 block and tripped its own `NotImplemented`; reproducing both passes made it
fit exactly, which is independent confirmation that the two passes are load-bearing. An event
numbered 0x20 (32) reuses the slice cached in the global `DAT_002FC3D8` instead of taking one of
its own.

**The block's contents come from the memory-card profile.** This port does not read a save, so
every slice is zero: only the root event (event 1, which has no incoming links) reports Unlocked
and everything else reports Locked. `Status_002010B8` stops there deliberately — **no save data is
invented**. `FUN_002010B8` also has two special cases that need the profile as well: event 0x1F is
gated on two completion globals, and event 0x20 requires **every event 1..31 of both trees to be
Won**.

## 5. Navigation (`FUN_001FAF60`, state 0)

| Input | Action | Sound |
|---|---|---|
| left / right | `FUN_001FA9C8(-1)` / `(+1)`, walking the node's +0x1C list | `TreeLeftRight` |
| up / down | `(+2)` / `(-2)`, walking the +0x20 and +0x28 lists | `TreeUpDown` |
| confirm | zoom into the event | `TreeZoomInOut` |

When the cursor does not land on a new node the handler writes 0 into the event's +0x0C sound
field, so nothing plays. In state 2 (an event is selected) starting the race first checks the node
record's +0x3E: below 1 it only plays `MenuWrong`.

## 5b. How the player actually gets here (TASK-0021 follow-up)

This page documented the tree but never the route into it, and the route was broken: choosing
Championship or Ultimate Racer did nothing. Three links were missing, and all three are now
recovered.

**The mode-select handlers already set up the tree correctly.** `FUN_002174A0` (Cops On, screen 1)
on its MENUITEM_4 and `FUN_00217AF8` (Cops Off, screen 2) on its MENUITEM_3 do the same four
things before leaving:

| Write | Where it lands | Meaning |
|---|---|---|
| `DAT_0030C77C = 9` / `= 8` | profile+0x1072C | the mode `FUN_00185568` keys on |
| `DAT_0030C784 = 1` | profile+0x10734 | readers are in the race code; not traced |
| `FUN_001F5D50(0x2FC1B4, 1)` / `(…, 0)` | profile+0x164 | the event tree's current tree type |
| `DAT_002FC1C0 = 0` | profile+0x170 | clears the view state, so the tree opens browsing |

Both then request the class **`"View Event Tree"`** (string 0x002BC560). Note that five of the six
mode-select globals are **inside the profile** — writing them is writing the save (E-0021-23).

**Link 1 — the class had no registration row.** `View Event Tree` does not register through
`FUN_0016ADB0` like the other 37 classes; it tail-calls the sibling `FUN_0016AD88` at 0x001FB860.
Its operands, built at 0x001FB7F0..0x001FB830, are the class-name string 0x002BA718, the handler
`FUN_001FB218`, and the document hash **0x147F8A59** — which is exactly screen 39's stored name
hash. Because the port's registry stopped at the 37 `FUN_0016ADB0` rows,
`find_registration("View Event Tree")` returned null and `apply_pending_screen` raised
NotImplemented at 0x0016C8C8. (E-0021-22)

**Link 2 — the transition is deferred behind an animation.** `FUN_00217020` / `FUN_002176D8` do
not switch screens. They build a 0x1C-byte `tExitScreenAnimation` through `FUN_002490D8` — the
same `Type_002490D8` the port already implements for the Options exit — store it at
instance+0x270, and let the switch happen only when it finishes. (E-0021-25)

**Link 3 — the port ignored the chosen mode.** `FrontEndRuntime` held `event_tree_mode_ = 9` as a
hardcoded field, so even with the first two links fixed, Championship would have opened the Hot
Pursuit tree. `FUN_001FB218` message 0x0B shows where it really comes from:
`FUN_00185568(0x2FC050)` — the profile. The field is gone. (E-0021-24)

Both routes are now asserted end to end in `src/tests/frontend_runtime_tests.cpp`
(`check_event_tree_route`), which drives the real joy events and ticks the exit animation out, and
both were confirmed in the running app: screen 1 item 3 and screen 2 item 2 each reach screen 39,
with different modes, different tree types and visibly different panel artwork
(`SPEED_EVENT_PANEL_*` against `PURSUIT_EVENT_PANEL_*`).

## 6. Implementation

> **Corrected by TASK-0021.** The two paths this section originally named no longer exist:
> `src/game_core/frontend/eventtree.{hpp,cpp}` moved to `src/game_core/indep/src/` in TASK-0020,
> and `src/platform/godot_bridge/ps2_event_tree.{hpp,cpp}` with its `Ps2EventTree` class was
> deleted in TASK-0018's third pass along with the GDScript host. The current list is below.

- `src/game_core/indep/src/eventtree.{hpp,cpp}` — parses both trees, the records, the
  **requirement** table, the connected components that become `tEventGroup`s, depth, x/y,
  status-to-texture mapping, GoalType-to-save-size, the two-pass profile slice layout, the full
  `Sub_002010B8` status decode, and the two recovered cursor walks `Sub_001FA880` /
  `Sub_001FA978`.
- `src/game_core/memorycard/{include/memorycard.h,src/memorycard.cpp}` — the profile the statuses
  and the start node come from (TASK-0021).
- `src/port/runtime/event_tree_view.{hpp,cpp}` — draws the connectors and nodes on screen 39 with
  the recovered camera, the selected node at twice the size, and the recovered left/right and
  up/down walks. Also the state machine's 0 to 2 transitions and the recovered sound ids.
- `src/port/runtime/frontend_runtime.{hpp,cpp}` — loads the profile once, scopes the view to
  screen 39, and routes joy events into the tree, playing the recovered sound through the same
  `PlayMenuSound_0016E038` every other screen uses.
- `ps2_texture_loader.*` gained `load_texture_by_name()` and `ps2_frontend_layout.*` gained
  `name_hash()`, so no host code **ever reimplements** the recovered string hash.

### Tests

`src/tests/eventtree_tests.cpp`: the GoalType size table, the status-to-texture table, the shape
of both ASCII maps (200 characters, 32 marks), parsing both trees out of the player's own
FRONTB.BUN, and eight structural assertions (records in event-number order, a single root, one
connected component, event 1 in column 8, events 30/31/32 converging back onto column 8, and the
two tree layouts actually differing), plus profile slices staying inside the block with the two
passes not overlapping. The test skips cleanly when ZZDATA is not configured.

### A build problem fixed along the way

Test executables were resolving `libstdc++-6.dll` / `libgcc_s_seh-1.dll` from PATH, and a
mismatched copy made them exit with `0xC0000139` before `main` ran — that was the cause of the
three long-standing failures (`sound_assets`, `jukebox`, `movie_audio`). `ps2_port` already links
those runtimes statically for the same reason; the test executables now do too. **20/20 pass.**

## 7. Open questions

Each is marked with its state after TASK-0021. Evidence ids are `E-0021-NN`; see
[docs/SAVE_SYSTEM.md](SAVE_SYSTEM.md) and the per-function records under
`reverse/functions/sha256-7335.../`.

1. **The camera — ANSWERED, except the easing.** `FUN_001FAB00(320.0f, 240.0f, view, leaf)` sets
   the mover's target to `centre - leaf_position * zoom`, so the selected node is pinned to the
   centre of the authored 640x480 space. `FUN_001FAB50` clamps a zoom **level** into 0..1 and
   takes that level's scale out of the two-float table at gp-0x4178, `{0.4f, 1.0f}`
   (gp == 0x003358F0). `FUN_001F96B8` calls it with delta 0, so a freshly opened tree sits at
   0.4. The camera object is 0x70 bytes, pool name `tStillCameraMover` (0x002BA898), allocated by
   `FUN_002577C0` and constructed by `FUN_0015E6A0(object, 1)`. The port's measured
   `kPortZoom = 0.6f` and `kPortPanelScale = 0.26f` are **gone**. (E-0021-13, E-0021-21)
   Still open: `FUN_0015E6A0` and the mover's update were not decompiled, so the easing between
   the two levels is unknown and the port snaps; and the authored panel is 256 px wide, which at
   zoom 0.4 does not fit a lattice 52 px apart, so one clearly-labelled port-side fit factor
   remains while the missing transform is looked for.
   *Correction to the original wording: `view+0x3F8..+0x404` is **not** the tree's bounds. It is
   the rectangle of the widget whose name hash is 0xAA5D7D3F, which is screen 39's `BOTTOM_MASK`.*
2. **Real status — ANSWERED.** `FUN_002010B8` is fully decompiled and translated, with five
   distinct status values and four per-GoalType slice decoders; see SAVE_SYSTEM.md section 5. Two
   of the four decoders and both completion gates are still unrecovered and now raise
   `hp2::NotImplemented` naming their ELF address. With a zeroed profile every event but the root
   still reports Locked — but that is now the recovered answer for an untouched save, reached
   through the real decoder, not a shortcut. Showing "Passed - 1st" / "Reward: ..." additionally
   needs `FUN_001F6B70` (question 5). (E-0021-10, E-0021-11)
3. **Runtime widget changes — PARTLY ANSWERED.** The two widgets `FUN_001FAF60` touches are
   `HELP_TEXT` (hash 0x0880AC4C) and `HELP_BUTTON` (0x027F7503), both revealed by
   `FUN_00144968(screen, hash, 1)` when the state-7 zoom transition ends. `FUN_001F96B8` also
   sets `EVENT_TREE`'s localised string id from the selected mode — 0x272A for Championship,
   0x272D for Hot Pursuit — which is the screen title. Nothing was found that hides the authored
   placeholder text, so the tree is still painted over it. (E-0021-15, E-0021-16)
4. **The link row — ANSWERED, and the table was misread.** *Superseded:* section 1 calls the
   `0x34A03` table a link table. It is a **requirement** table: `row+0x02` names a prerequisite
   event and `row+0x04` is the u32 goal that prerequisite must meet, and an event unlocks when
   any one of its `record+0x3D` rows is satisfied. Four of the six "remaining" bytes are
   therefore identified; `row+0x00..0x01` is not. The `0x34A01/02/04/06` tables remain
   unidentified. (E-0021-12)
5. **`FUN_001F5D50` — ANSWERED. `FUN_001F6B70` — still open.** `FUN_001F5D50` is not "tree
   selection": it is two instructions, `sw a1,0x0(a0)`, and it writes the current tree type into
   the profile block at 0x002FC1B4. Its siblings `FUN_001F5D00` (read the start node) and
   `FUN_001F5D28` (write it) are recovered with it, including the quirk that the writer selects
   on the stored type rather than on its own argument. (E-0021-09) `FUN_001F6B70`, the state-2
   event detail sub-menu over document 17, is still not decompiled.
6. **Connector colours — still open.** The `tLineSegment` build inside `FUN_001F96B8` was not
   read, so the port still colours by "is the source node Won" and the code says so.

New questions TASK-0021 opened:

7. The event tree is a **four-document cluster**, not one screen: besides screen 39 it builds
   screen 17 (the event detail sub-menu, `FUN_001F6678` at view+0x13C), screen 18 (the zoom
   transition, `FUN_001F9480` at view+0x654) and screen 19 (state 5, `FUN_001F6328` at
   view+0x5AC). All three were identified by matching the ELF immediates 0xA744BA7B, 0xFD297FD4
   and 0xF1EED4CD against the 41 stored screen name hashes. None of their widget sets is
   translated. (E-0021-17)
8. The view's state word lives at view+0x74 and is **initialised from the profile** at
   profile+0x170, with a stored 2 zooming straight into the selected event. States 5 and 6, and
   `FUN_001F9F10`'s intro path that forces state 7, are not translated. (E-0021-18)
9. `FUN_001F96B8` also writes a 32-float array at `DAT_0031B240`, indexed by event number minus
   one, during layout. Its consumer was not traced.
