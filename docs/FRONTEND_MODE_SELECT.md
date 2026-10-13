# TASK-0016: the Hot Pursuit and World Racing mode-select screens, and whether the background movie loops

Completed 2026-10-13, Australia/Sydney. Binary `SLUS_203.62`, SHA-256
`7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`; Ghidra 12.0.4,
`r5900:LE:32:default`, GhidraMCP **read-only**, no annotations, renames or boundary edits.

## 1. What these two screens are

The main menu (`NEW_MAIN_MENU`, FRONTB.BUN screen 0) has three entries, which localise to
**Hot Pursuit / World Racing / Options**:

| Main menu entry | Opens | Class name | Document hash | Layout screen |
|---|---|---|---|---|
| Hot Pursuit | the cops-on mode list | `Cops On Mode Select` | 0x5B35F533 | screen 1 |
| World Racing | the cops-off mode list | `Cops Off Mode Select` | 0x033E7771 | screen 2 |

Their menu entries, after localisation:

```
Hot Pursuit (screen 1)              World Racing (screen 2)
  Quick Race                          Quick Race
  You're The Cop                      Challenge
  Challenge                           Championship   <- the tournament entry
  Ultimate Racer   <- the tournament entry
```

**"Ultimate Racer" and "Championship" are the same thing on the two branches**: each is its
screen's tournament entry, and both request the same screen, `"View Event Tree"`. The only
difference is the event tree id they write (cops on = 9, cops off = 8).

## 2. Per-item behaviour (`FUN_002174A0` / `FUN_00217AF8`, message 1)

The event structure is `{+0x00 widget name hash, +0x04 pad index, +0x08 result, +0x0C sound id}`.

### Hot Pursuit (`FUN_002174A0`)

| Entry | Name hash | Action |
|---|---|---|
| Quick Race | 0x52E039F3 | **pad 0 only**; `FUN_00216C18(instance+0x274)` |
| You're The Cop | 0x52E039F4 | `DAT_002FC06C = 1`; exit animation to `"ChallengeBeACop"` |
| Challenge | 0x52E039F5 | `DAT_0031AB90 = 1`, `DAT_002FC06C = 0`; exit animation to `"Challenge"` |
| Ultimate Racer | 0x52E039F6 | **pad 0 only**; `DAT_0030C77C = 9`, `DAT_0030C784 = 1`, `FUN_001F5D50(0x002FC1B4, 1)`, `DAT_002FC1C0 = 0`; exit animation to `"View Event Tree"` |
| (MENUITEM_5) | 0x52E039F7 | exit animation to `"Options"`. **This layout declares no fifth item, so the branch is unreachable on screen 1** |

### World Racing (`FUN_00217AF8`)

| Entry | Name hash | Action |
|---|---|---|
| Quick Race | 0x52E039F3 | **pad 0 only**; `FUN_00216C18` |
| Challenge | 0x52E039F4 | `DAT_0031AB90 = 1`, `DAT_002FC06C = 0`; exit animation to `"Challenge"` |
| Championship | 0x52E039F5 | **pad 0 only**; `DAT_0030C784 = 1`, `DAT_0030C77C = 8`, `FUN_001F5D50(0x002FC1B4, 0)`, `DAT_002FC1C0 = 0`; exit animation to `"View Event Tree"` |
| (MENUITEM_4) | 0x52E039F6 | exit animation to `"Options"`, unreachable for the same reason |

**The pad gate is the original's behaviour, not something added here.** Quick Race and the
tournament entry check the event's +0x04 and, when it is not 0, write **0** into the event's +0x0C
sound id and return — no screen change, no sound, not even `MenuWrong`. The port reproduces that
verbatim.

## 3. The exit animation (`FUN_00217020` / `FUN_002176D8`)

Neither screen **ever calls RequestScreen directly**. Every activation does this:

1. allocate 0x1C bytes from the pool named `tExitScreenAnimation` (string 0x002BC528);
2. construct it with `FUN_002490D8(object, document, target class name, 1)`;
3. store it at instance+0x270;
4. start `MI_UNDERSCORE` moving through `FUN_00248C60` at `gp-0x6AC8` = **650 px/s**.

The important part is the `if (instance+0x270 == 0)` gate at the top of both dispatchers: while an
exit animation exists, **every message is routed to `FUN_00249128`** instead, so the screen is
locked for the duration and the target screen is only requested once the animation finishes. That
is what the screenshot of the menu entries sliding off to the right shows.

## 4. Highlight (`FUN_00217380` / `FUN_002179F0`, message 4)

Both have the same shape. First `FUN_002488D0(instance+0x23C, widget name hash)` retargets the
ghost — **before the switch**, so the ghost follows even for an item the table below does not
know — and then the item is mapped to a mask widget and a localised string id:

| Entry | Mask | Hot Pursuit string id | World Racing string id |
|---|---|---|---|
| MENUITEM_1 | `MI_1_MASK` 0x6E34E350 | 0x2721 | 0x2716 |
| MENUITEM_2 | `MI_2_MASK` 0x708A0BF1 | 0x271E | 0x2717 |
| MENUITEM_3 | `MI_3_MASK` 0x72DF3492 | 0x271C | 0x2718 |
| MENUITEM_4 | `MI_4_MASK` 0x75345D33 | 0x271D | 0x2719 |
| MENUITEM_5 | `MI_5_MASK` 0x778985D4 | 0x2719 | — |

All five mask hashes were checked by running the recovered `StringHash_002534A8` over
`MI_<n>_MASK`; they match bit for bit. The handler then takes the mask's bounding box with
`FUN_00144280`, puts `MI_UNDERSCORE` at `y1 - 3` and `MI_OVERSCORE` at `y0 - 6` (`gp-0x3EF0` = -3,
`gp-0x3EF4` = 6), hands the string id to the scrolling text, and switches a visibility group with
`FUN_0016E270`.

## 5. Quick Race is not implemented (and does not belong in the front end)

`FUN_00216C18` opens no front-end screen. It sets the event tree id to 2, reads the cops flag the
main menu wrote into `DAT_002FC070`, and from it fills an entire race configuration block
(`DAT_0031A8CC` becomes 7 or 3, `DAT_0031AAF4` 4 or 0, `DAT_0031AB14` 2 or 0, and
`DAT_0031AAF8/AAFC/AB00/AB04/AB08` all take the flag), then registers the entry through
`FUN_00153420` / `FUN_00187A68` and hands off to the loader. That is not front-end work, so the
port raises `hp2::NotImplemented` rather than approximating it.

## 6. The background movie **does loop**

The per-frame chain is `FUN_0016CCA8` (the front-end manager's update) into `FUN_0016D028`:

```c
if (manager->movie_0x1CC != 0) {
    if (gp[-0x53B0] != 0) { StopMovie(manager); return; }
    if (FUN_002380A8(/*player*/ 0x002FB308, /*dt*/ gp[-0x4C8C]) != 1)
        FUN_00237AD8(0x002FB308, manager->path_0x1E4);   // re-open the cached path
}
```

The disassembly is unambiguous: `jal 0x002380a8` followed by `xori v0,v0,1` and
`beq v0,zero,skip` — **only a return value of exactly 1 skips the re-open**. `FUN_00237AD8` is the
"open movie" function, and re-opening restarts from the beginning. **So the front-end background
movie loops by re-opening the same file whenever the decoder step stops returning 1; there is no
play-once branch anywhere.**

Two supporting observations:

- `FUN_0016CE30` (set movie) returns early when asked for the path **already cached**, without
  restarting playback — so re-entering a screen does not restart its movie, and the looping is
  entirely the per-frame path above.
- `FUN_0016D0A0` (stop) clears `+0x1CC..+0x1E0` but **deliberately keeps** the path cache at
  `+0x1E4`.

Which movie each screen asks for (the argument to `FUN_0016CE30`, strings verbatim):

| Screen | Movie |
|---|---|
| New Main Menu | `MOVIES\mainmenu_movie.AVI` |
| Cops On Mode Select | `MOVIES\hotpursuit_movie.AVI` |
| Cops Off Mode Select | `MOVIES\worldracing_ movie.AVI` (the space in that path is **the original's**) |
| Options | `MOVIES\options_movie.AVI` |

**Residual uncertainty, stated plainly**: `FUN_002380A8` has several paths that return false, one
of which tops up the stream and bails for the frame when more than 20 units have passed since the
last read. This session did **not** classify which of them corresponds to "the clip finished".
What is certain is that no play-once branch exists and that the re-open path is the looping
mechanism. Pinning down the exact end-of-clip condition means continuing into
`FUN_002380A8`'s streaming buffer state machine.

## 7. A correction made along the way: the ghost pulse period is 4 s, not 2 s

TASK-0015 set the main menu ghost's loop period to 2.0 s. The evidence says that period belongs to
the **manager-level** object: `FUN_0016FBB0` overwrites +0x1C with 2.0 on `manager+0x1B8`, and that
object binds `GLOBAL_ANIMATING_MENUITEM` against the `LOADING_TRACK` document. The **screen-owned**
ghosts — the main menu's at +0x1A8 in `FUN_00216460`, and the two mode selects' at +0x23C in
`FUN_00216E80` / `FUN_00217538` — only call `FUN_00248870` and **never overwrite +0x1C**, so they
keep the default `gp-0x3AC8` = **4.0 s**. Corrected.

This also closes the item TASK-0015 recorded as "no driver found": the driver is
`FUN_0016CCA8` → `FUN_0016FCD8` → `FUN_00248970`.

## 8. Background animation (`FUN_00249AE8`, partially recovered)

Per frame, `GlobalBackgroundAnimation` advances a phase accumulator at +0x78 by the frame delta
and wraps it at `gp-0x676C` = **2π**. The blueprint object **fades in over the first second**
(alpha 0 to 128), drifts along a path, and **fades out over the last second**, with a whole cycle
of `gp-0x3A94` = **50 seconds**, after which `FUN_00249888` swaps in the next one. The fade
envelope and the 50-second cycle are recovered; the **path curve itself** goes through the
engine's generic path animator (`FUN_001DB000` / `FUN_001DAB38`) whose curve data was not decoded,
so it is not implemented and nothing was invented in its place.

## 9. Implementation and validation

- `src/game_core/frontend/frontendscreens/modeselect.{hpp,cpp}` — message 1 and message 4 for both
  screens. The original source file name is **not recovered** (only the pool names
  `CopsOnModeSelect` / `CopsOffModeSelect` and the class names are), and the file header says so.
- `src/platform/godot_bridge/ps2_frontend_layout.cpp` — activation dispatch gained screens 1 and 2;
  the activation event is now passed by reference so the pad gate can write back the sound field
  the way the original does; both screens enable the screen-level ghost.
- `src/tests/frontend_modeselect_tests.cpp` — 7 groups of assertions: registration-table
  consistency, per-item behaviour and globals on both sides, the pad gate (including "the sound is
  zeroed when refused and kept when accepted"), Quick Race raising NotImplemented, both highlight
  tables, and where the selection bars land.

```powershell
ctest --preset native-debug          # the new frontend_modeselect test passes
./tools/run_frontend.ps1 -Screen 1   # Hot Pursuit, operable with the arrow keys
./tools/run_frontend.ps1 -Screen 2   # World Racing
```

Screenshots are archived under `local/shots/` (not committed): the localised main menu
(Hot Pursuit / World Racing / Options), screen 1 after pressing Hot Pursuit, screen 1 after two
presses of down with Challenge selected (the two selection bars following it), and the menu
entries sliding off to the right after Ultimate Racer is chosen (the exit animation).

**Not done**: any PCSX2 comparison. Every record from this task stops at `Decompiled`; none is
marked `Verified`.

## 10. Addendum (same day, after the user supplied an original-hardware screenshot): `View Event Tree` located

The user provided an original screenshot: the title **Ultimate Racer**, a tree of numbered star
nodes, `Event 15 / Status: Passed - 1st / Reward: Alpine Trail` on the right and
`European Invitational Tournament` along the bottom. That is exactly the screen the previous
section recorded as "not reversed", and it has now been located.

### Screen identity

| Item | Value |
|---|---|
| Class name | `View Event Tree` (string 0x002BA718, **a third copy**; the other two at 0x002B2FE0 and 0x002BC560 are only used for comparisons and requests) |
| Document name hash | **0x147F8A59 = FRONTB.BUN screen 39** |
| Class dispatch | `FUN_001FB218` |
| Instance handler | `FUN_001FAF60` |
| Instance size / pool name | 0x860 bytes, pool name **`tEventTreeView`** (an original literal, 0x002BA8F8) |
| Registration site | 0x001FB860, through the other wrapper `FUN_0016AD88` |

The previous section claimed "the class names there are runtime gp pointers, so they cannot be
resolved". That is **only true of the post-race batch** (`gp-0x44D8..-0x44A4` hold statically
initialised pointers to `Points Rewards` / `Race Results` / `Tourney Standings` and friends). The
event tree's registration uses a **literal address**, `addiu a1,a1,-0x58E8`; it was missed because
only the two known copies of the string were searched for pointers, and the third copy of the
string itself was overlooked.

Screen 39's widget names line up with the screenshot item for item: `EVENT_NUMBER` (the large
number inside the star, placeholder "88"), `SMALL_EVENTNUMBER` ("Event 1245"),
`OVERLAY_STATUS_HEADING` (string id 0x101D0, "Status:"), `OVERLAY_STATUSTITLE`, a `NONAME` with
string id 0x101D7 ("Reward:"), `OVERLAY_REWARDTITLE`, `EVENT_TITLE_BAR` (the bottom banner),
`EVENT_PANEL` (the selection bracket), `HLINE` / `VLINE` (the tree's connectors),
`TRACK_LOGO` / `FADING_LOGO_1`, `EVENT_TREE` and `SPEED_TREE_BG`.

### Interaction (`FUN_001FAF60`, state machine on instance+0x74)

**State 0, browsing the tree:**

| Message | Source | Action | Sound |
|---|---|---|---|
| 1 | select | `FUN_001F5D28` + `FUN_001FAC78` | 0x15 `TreeZoomInOut` |
| 6 / 7 | left/right (`FUN_00169D98`) | `FUN_001FA9C8(-1)` / `(+1)` | 0x14 `TreeLeftRight` |
| 8 / 9 | up/down (`FUN_0016A458`) | `FUN_001FA9C8(+2)` / `(-2)` | 0x13 `TreeUpDown` |
| 0x0F | L1 | refresh every node (`FUN_001F8300`) | — |
| 0x11 | | `FUN_001F5D28`, without the zoom | — |

When the move fails (no new node) the handler writes 0 into the event's +0x0C sound id, so
**nothing plays**.

**State 2, an event is selected:** 0x11 zooms back out (`FUN_001FAD20`, sound 0x15); message 10
starts the race (`FUN_001FAE40`, sound 0x18) but **only after checking the node record's +0x3E
byte — below 1 it just plays `MenuWrong` (8)**, which is why locked events refuse to open. Every
other message goes to the detail sub-menu `FUN_001F6B70`.

### Cursor movement (`FUN_001FA9C8`)

The current node sits at instance+0x18. Direction **-1/+1 walks the node's +0x1C list**, **+2 walks
+0x20** and **-2 walks +0x28** — left/right and up/down travel two different axes of the graph. On
a successful move the old node's +0x0C is set to 1.0f and the new one's to **2.0f**, **which is why
the selected star 15 in the screenshot is larger than 9, 10 and 16** (node scale). The panel is
then updated, the scrolling text is set to the node record's string id at +0xB0, and the Status and
Reward overlays are refreshed.

### What is still missing to make it run

The tree's **node graph comes from the player profile**: the constructor is
`FUN_001F96B8(instance, document, FUN_00185568(0x002FC050))`, and 0x002FC050 is the save/progress
object. So even though the port can already draw screen 39 (see
`local/shots/screen39-eventtree.png`, showing the author's placeholders "This Will Change" /
"Event 1245" / "This is The Title of the Event Phil is still poo poo."), **without profile data
there are no nodes, no connectors and no status text**. That dependency has to be solved before
the screen can be implemented; it is not a rendering problem.

The title "Ultimate Racer" is written at runtime (screen 39's title widget has string id 0).
Branch-wise it would be 0x272D or 0x272A, but that is **not confirmed** and is listed as open.

## 11. Open questions

1. ~~The `View Event Tree` screen itself is not reversed~~ — **located, see §10.** What remains is
   the node graph's profile data source (0x002FC050), the runtime title text, and the
   `FUN_001F6B70` detail sub-menu.
2. The event tree node record is only recovered down to field purposes (+0x14 data pointer,
   +0x1C/+0x20/+0x28 adjacency lists, +0x3E unlock byte, +0xB0 string id); the overall layout is
   undetermined, and `FUN_001F6B70` is not decompiled.
3. `FUN_001F5D50(0x002FC1B4, 0/1)`, which selects the event tree data, is not decompiled.
4. Classifying `FUN_002380A8`'s false paths (see §6).
5. The background animation's path curve data (see §8).
6. The ghost colour comes from `gp-0x3F08`, whose initial value in the image is 0x00000080, i.e.
   RGB (0x80, 0, 0), pure red. That is the **initial** value — the global is read with `lw` and
   this session did not confirm whether anything overwrites it at runtime.
