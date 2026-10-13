# TASK-0018: `tEventTreeView` — data source, geometry, navigation and UI

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

## 6. Implementation

- `src/game_core/frontend/eventtree.{hpp,cpp}` — parses both trees, the records, the link table,
  the connected components that become `tEventGroup`s, depth, x/y, status-to-texture mapping,
  GoalType-to-save-size, and the two-pass profile slice layout.
- `src/platform/godot_bridge/ps2_event_tree.{hpp,cpp}` — the `Ps2EventTree` bridge class.
- `ps2_texture_loader.*` gained `load_texture_by_name()` and `ps2_frontend_layout.*` gained
  `name_hash()`, so GDScript **never reimplements** the recovered string hash.
- `src/port/runtime/event_tree_view.cpp` — draws the connectors and nodes on screen 39, the selected node at
  twice the size, with left/right/up/down walking the recovered ±1/±2 links.

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

1. **The camera is not recovered.** `FUN_001F96B8` ends by attaching a `StillCameraMover` (pool
   name 0x002BA898) to the current node, but its zoom, easing and bounds were not decompiled. The
   port's `EVENT_TREE_PORT_ZOOM` and `EVENT_TREE_PORT_PANEL_SCALE` are **port-side values
   calibrated against the user's original-hardware screenshot, not recovered values**; the code
   says so, and they must be replaced once the camera is reversed.
2. **No profile means no real status.** Showing "Passed - 1st" / "Reward: Alpine Trail" as in the
   screenshot requires parsing the two 256-byte blocks in the memory-card save, plus the part of
   `FUN_002010B8` that decodes a slice by GoalType (only its opening was read this session).
3. Screen 39's placeholder widgets ("This Will Change", "Event 1245", "Status Status…") are
   changed or hidden at runtime by the original; which widgets change and how was not reversed.
   The tree is currently painted over them.
4. The remaining six bytes of each link row, and the meaning of the `0x34A01/02/04/06` tables, are
   unidentified.
5. `FUN_001F6B70` (the event detail sub-menu) and `FUN_001F5D50` (tree selection) are not
   decompiled.
6. The real rule behind the connector colours (green and red in the screenshot) is not recovered;
   the port currently colours by "is the source node Won", and the code says so.
