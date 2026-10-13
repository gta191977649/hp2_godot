# TASK-0014: front-end menu interaction, selection and sound (reversed from the ELF, not designed)

Completed 2026-10-12, Australia/Sydney. Binary `SLUS_203.62`, SHA-256
`7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`; Ghidra 12.0.4,
`r5900:LE:32:default`, GhidraMCP read-only (127.0.0.1:8080), **no annotations, renames or boundary
edits were made**.

This task turns the original screens TASK-0013 renders into something **operable**: the item list,
selection movement, highlight colouring, activation callbacks and menu sound events all come from
decompiled original code, not from a redesigned UI. Naming follows [AGENT.md](../AGENT.md):
identifiers recovered verbatim from the binary are used as-is, and everything else carries its ELF
address.

## 1. Headline findings

| Finding | Evidence | Why it matters |
|---|---|---|
| **The string hash** (`FUN_002534A8`): `h=-1; h=h*0x21+c` | E-0014-07 | converts names to hashes and back; nearly everything below depends on it |
| **Menu items are the screen's own `MENUITEM_0..15` type-1 widgets** | E-0014-04 | the menu structure is built from the layout data instead of being guessed |
| **The JOY_EVENT_\* value-to-name table** (ELF 0x002FA2D0) | E-0014-01/02 | input semantics are settled, so the keyboard mapping has a basis |
| **Highlight is orange (0x80,0x40,0x00), disabled is dark grey (0x18,0x18,0x18)** | E-0014-09 | read out of the ELF's initialised data, not eyedroppered |
| **The menu sound event name table** (0x002FB0A8) | E-0014-11 | `MenuUpDown` / `MenuSelect` / `MenuWrong` … verbatim original names |
| **A screen's own name hash lives at +0x08 of the 0x10203 header**; 18 of 41 names cracked | E-0014-16/17 | **screen 0 = `NEW_MAIN_MENU`**, confirming TASK-0013's content-based inference |
| `HIGHLIGHT_BAR` = 0x27C5F4AB | E-0014-06 | an original widget name recovered by reversing the hash |

## 2. The string hash (E-0014-07)

`FUN_002534A8` decompiles to three lines:

```c
int StringHash(const char* s) { int h = -1; while (*s) h = h * 0x21 + *s++; return h; }
```

**This was verified, not eyeballed.** The four immediates hardcoded in `FUN_0016B478` —
`0xE0105663 / 0xE0105664 / 0xE0105665 / 0xAB671C9A` — are bit-for-bit what this function produces
for `POPUP_1` / `POPUP_2` / `POPUP_3` / `ANIMATING_MENUITEM`. It also reproduces **1531 of the
1549** widget name hashes stored in FRONTB.BUN; the 18 that differ are **all** records whose
stored name is exactly 23 characters, i.e. the ones truncated by the 24-byte name field, which in
turn shows the original **hashed the name before truncating it for storage**.

## 3. How a menu is built (E-0014-04)

`FUN_0016A858` (first use of a menu group) calls `FUN_00169F78` (build the list):

```c
// FUN_00169F78(list, group, callback, initial_index, "MENUITEM_%d")
list->next = list->prev = list;                 // circular sentinel
list->screen = group->screen;  list->pad = group->pad;
for (int i = 0; i < 0x10; ++i) {
    sprintf(buf, name_format, i);
    if (FindWidget(list->screen, StringHash(buf), /*type=*/1))   // only if the widget exists
        append(new MenuThing(list, StringHash(buf), i));
}
SetCurrentIndex(list, initial_index);
list->highlight_bar = FindWidget(list->screen, 0x27C5F4AB, /*type=*/2);
```

- The format string `"MENUITEM_%d"` sits at ELF 0x002B2760 and is sprintf'd by both
  `FUN_0016A330` and `FUN_00169F78`. **Verbatim.**
- The bound is **0..15** (both loops are `i < 0x10`).
- `type=1` is a text widget and `type=2` a quad — exactly the values at layout record +0x28, which
  is also the **first direct contact between the code side and the data side** (see §6).
- The name behind `0x27C5F4AB` was **recovered by reversing the hash**: hashing candidates gives
  `HIGHLIGHT_BAR` bit for bit, and no other candidate collides. (None of FRONTB.BUN's 41 screens
  declares that widget, so their highlight-bar pointers are null.)

## 4. Selection movement and highlight colouring

`FUN_0016A5E0` (next) and `FUN_0016A6C8` (previous) walk a circular list forwards and backwards,
wrapping onto the head or tail when they hit the sentinel and **skipping items whose enabled bit is
clear**, giving up once they return to the starting item. Whether they wrap and skip is decided by
the global `gp-0x533C` (= 0x003305B4), and **the retail image stores 1 there**, so the original
menus do wrap (E-0014-10).

The highlight itself is **a colour change on the widget**, not a drawn selection box.
`FUN_00169BD0` / `FUN_00169CB0` select the colour from `state & 3` (state lives at menu item
+0x08; bit 0 = enabled, bit 1 = highlighted):

| state & 3 | Meaning | Colour source |
|---|---|---|
| 1 | enabled, not highlighted | the widget's own authored colour (snapshotted from widget +0x5C by `FUN_00169818`) |
| 3 | enabled, highlighted | `gp-0x5330/-0x532C/-0x5328`, with alpha forced to 0x80 by `lui a1,0x8000` |
| 0 or 2 | disabled | `gp-0x5324/-0x5320/-0x531C` |

The value of gp is not guessed: the entry point at 0x00100008 is
`lui a0,0x33; addiu a0,a0,0x58f0; move gp,a0`, so **gp = 0x003358F0** (Ghidra also puts a `_gp_1`
label there). Substituting and reading:

- highlight = **(0x80, 0x40, 0x00) with alpha 0x80** → orange, which is exactly HP2's menu
  selection colour.
- disabled = **(0x18, 0x18, 0x18)** → dark grey.

`FUN_00140CA0` **preserves the alpha byte** and replaces only the low 24 bits, then stamps the
widget's +0x2b with 7 (a dirty flag). The `state&3 == 0` and `== 2` arms load the same three
globals — that was checked instruction by instruction in the disassembly at 0x00169C1C and
0x00169C38, so it is not the decompiler merging branches.

## 5. Input: the JOY_EVENT_\* enum (E-0014-01/02)

ELF 0x002FA2D0 is a `{ uint32 value, const char* name }` table, 8 bytes per row, whose name
pointers walk the contiguous string block starting at 0x002B0218. **The values and names are read
straight out of the table; no ordering had to be assumed:**

```
0 JOY_EVENT_NULL          6 JOY_EVENT_SCROLL_RIGHT    12 JOY_EVENT_MENU_HELP
1 JOY_EVENT_TYPE_CHANGED  7 JOY_EVENT_START           13..16 MENU_L1/L2/R1/R2
2 JOY_EVENT_EXIT_DEMO_DISC 8 JOY_EVENT_SELECT         17 JOY_EVENT_MENU_SQUARE
3 JOY_EVENT_SCROLL_UP     9 JOY_EVENT_MENU_SELECT     18..21 MENU_ANALOG_LEFT/RIGHT/UP/DOWN
4 JOY_EVENT_SCROLL_DOWN   10 JOY_EVENT_MENU_CANCEL    22 JOY_EVENT_SKIP_DEMO_MODE
5 JOY_EVENT_SCROLL_LEFT   11 JOY_EVENT_MENU_BACK      23 ..._MAIN_MENU_SELECT_TRANSMISSION
```

**Independent cross-check**: `FUN_0016A458` groups 3 with 0x14 and 4 with 0x15; `FUN_00169D98`
groups 5 with 0x12 and 6 with 0x13. Only under the table above do those four groupings read as
"up / analog up", "down / analog down", "left / analog left" and "right / analog right". The two
pieces of evidence confirm each other.

The dispatch chain (E-0014-03):

```
FUN_0016CAB0  manager level (offers the event to the two tPopupMenus, then the current screen)
  └─ FUN_0016B140  walks the menu group list at screen +0x08
       └─ FUN_0016AA00  menu group: pad filter plus the four group-level messages START/SELECT/L1/R1
            └─ FUN_0016A458  menu list: up/down movement (messages 8/9, sounds 3/4)
                 ├─ FUN_0016A5E0 / FUN_0016A6C8
                 └─ FUN_00169D98  current item: left/right adjust (messages 6/7), START/MENU_SELECT
                                   activation (message 1, sound 5 MenuSelect), MENU_SQUARE (message 10)
```

Callback message numbers (the original has only numbers — no names were recovered — so the
enumerators in the code carry only the number): 2 item refresh, 4 highlight gained, 5 highlight
lost, 8 move up, 9 move down, 0x0B menu created (may override the initial index), 0x0D START,
0x0E SELECT, 0x0F L1, 0x10 R1, 1 activation, 6/7 left/right adjust, 0x0A square button, 0x15
pop-up item activated, 0x16/0x17 pop-up previous/next, 0x18 an event the screen did not handle.

The event structure is 16 bytes: `{+0x00 name hash or screen handle, +0x04 pad, +0x08
result/veto, +0x0C sound id}`, and after the callback returns the caller plays +0x0C.

## 6. Menu sounds (E-0014-11)

`FUN_0016E038(id)` indexes a 16-byte-per-row table starting at 0x002FB0A8,
`{u32 id, u32 flag, const char* name, u32 voice}`, whose voice field lands exactly on
`0x002FB0A4 + id*0x10`. The names are the original bSound event names, **verbatim**:

| id | Name | Used for |
|---|---|---|
| 1 / 2 | `MenuLeftRight` | pop-up previous / next |
| 3 / 4 | `MenuUpDown` | menu list up / down |
| 5 | `MenuSelect` | item activation |
| 6 | `MenuCancel` | |
| 7 | `MenuStart` | |
| 8 / 9 | `MenuWrong` | |
| 10..21 | `MenuTrackRightLeft`, `MenuCarRight/Left/Colour`, `MenuCarDisplayOn/Active/Off`, `MenuCarSelect`, `TreeUpDown`, `TreeLeftRight`, `TreeZoomInOut` | car select / event tree |

`FUN_0016E038` also holds a guard that drops a repeat of the same voice within 34.0 units of the
previous play. **The unit of that 34.0 (frames? milliseconds?) was not recovered**, so the port
does **not** reproduce the guard — inventing a unit would be inventing behaviour.

The sound **selection** logic is reversed and implemented; actually **producing** the sound is PS2
bSound hardware work (`FUN_00273618` / `FUN_00274080` plus an unparsed audio bank), which under
AGENTS.md rule 7 stays behind a platform contract: `PlayMenuSound_0016E038` hands the id and the
original event name to an injected sink, and the Godot side displays the name without synthesising
audio.

## 7. The code side and the data side finally meet (E-0014-05)

`FUN_00143D50(screen, name_hash, type)` walks the screen's intrusive widget list comparing
**node +0x08** (name hash) and **node +0x28** (type). Those are exactly the name-hash and type
fields TASK-0013 read out of the FRONTB.BUN records; the **+0x5C** that `FUN_00140CA0` writes is
exactly the text-widget colour TASK-0013 located; and the **+0x2b** it stamps sits one byte after
TASK-0013's type/visible/layer at +0x28/+0x29/+0x2a.

In other words: **the 144-byte record on disk is the runtime widget node itself**, and the
"pointer slots that are always 0 in the file" at +0x00/+0x04 are the list's next/prev. TASK-0013
could only say "statistical correlation supports this field map"; now the code reads and writes
those exact offsets.

## 8. Screen names (E-0014-16/17)

`FUN_0016FBB0` passes `StringHash("LOADING_TRACK")` to `FUN_00143D50` as a "document handle",
which shows screens are addressed **by name hash**. Back in the layout data, each screen's 0x10203
header carries a **u32 at +0x08 that is unique across all screens**, and the values for screens
5/6 and 8/9 differ by exactly 1 — a property of the hash, where names differing only in their last
character produce hashes differing by 1.

Running candidate names through the recovered hash against those 41 values gives **confirmation on
a match** (exact 32-bit equality):

| # | Name | # | Name | # | Name |
|---|---|---|---|---|---|
| 0 | `NEW_MAIN_MENU` | 14 | `POPUP` | 28 | `AUDIO_OPTIONS` |
| 4 | `SELECT_CAR` | 16 | `CREDITS` | 29 | `USERNAME_OPTIONS` |
| 7 | `SELECT_CAR_DIALOG` | 22 | `TITLE_SCREEN` | 30 | `LANGUAGE_OPTIONS` |
| 10 | `SELECT_TRACK` | 24 | `OPTIONS` | 35 | `LOADSAVE` |
| 11 | `CHALLENGE` | 26 | `DISPLAY_OPTIONS` | 36 | `BLANK_SCREEN` |
| 12 | `RACE_SETUP` | 13 | `JOYSTICK_CONFIG` | 40 | `CREDITS_SCREEN` |

**This resolves TASK-0013's fifth open item**: screen 0 really is the main menu, and the original
calls it `NEW_MAIN_MENU` — matching the screen class name string `"New Main Menu"` (0x002B2F00)
that TASK-0011 confirmed in the ELF. The three screens TASK-0013 identified **by eye** — the Title
Screen, Options and Controller Configuration — are now confirmed by hash as `TITLE_SCREEN` (22),
`OPTIONS` (24) and `JOYSTICK_CONFIG` (13), which is a completely independent cross-check.

The remaining 23 screen names have not been cracked; their hashes are recorded here so the search
can be continued:

```
1 5b35f533   2 033e7771   3 e55f1297   5 18dc8e52   6 18dc8e53   8 6344da01
9 6344da02  15 74d2dfa2  17 a744ba7b  18 fd297fd4  19 f1eed4cd  20 492e86e9
21 dd4197c9  23 adb8e59e  25 28891a22  27 a4031ef6  31 8bd5060b  32 ddf09124
33 7ab428c1  34 95b2b627  37 9829bfab  38 bec7cd92  39 147f8a59
```

(The pairs 5/6 and 8/9 differ only in their last character, so they are most likely something like
`..._1`/`..._2` or `..._P1`/`..._P2`.)

## 9. Implementation

- `src/game_core/frontend/fmenus.hpp/.cpp` — the new menu runtime. The file keeps the original
  name `..\indep\src\fmenus.cpp`. Class and function names follow AGENT.md R2:
  `MenuThing_00169740`, `MenuList_00169F78`, `MenuGroup_0016AA00`, `Sub_00169D98`,
  `StringHash_002534A8` and so on — **anything without a recovered original name carries its ELF
  address in the identifier**, with interpretation left to the comments.
- `src/game_core/input/joyevents.hpp/.cpp` — the `JOY_EVENT_*` enum, **verbatim**.
- `src/game_core/not_implemented.hpp` — `hp2::NotImplemented` plus
  `HP2_NOT_IMPLEMENTED(addr, what)`.
- `src/platform/godot_bridge/ps2_frontend_layout.*` — gained `menu_open` / `menu_joy_event` /
  `menu_state` / `get_widget_colours` / `find_screen_by_name` / `joy_event_name` /
  `probe_not_implemented`. The menu mutates the colours in the layout records directly, so the
  renderer only has to re-read them each frame.
- `godot/scripts/frontend.gd` — the keyboard mapping onto `JOY_EVENT_*`, the interaction, the
  status line and the not-implemented notices.

### Keyboard mapping (for PC debugging)

| Key | JOY_EVENT | | Key | JOY_EVENT |
|---|---|---|---|---|
| ↑ ↓ ← → | `SCROLL_UP/DOWN/LEFT/RIGHT` | | Q / E | `MENU_L1` / `MENU_R1` |
| W A S D | `MENU_ANALOG_UP/LEFT/DOWN/RIGHT` | | 1 / 3 | `MENU_L2` / `MENU_R2` |
| Enter | `START` | | V | `MENU_SQUARE` |
| Space / Z | `MENU_SELECT` | | H / F1 | `MENU_HELP` |
| X | `MENU_CANCEL` | | \\ | `SELECT` |
| Backspace / C | `MENU_BACK` | | | |

Non-game keys: PgUp/PgDn (or `[` and `]`) change screen, Tab toggles the info line, F5 rebuilds,
F8/F9/F10 deliberately hit the unimplemented paths (highlight animation / localised text /
pop-up), Esc quits.

## 10. Explicitly not implemented (raises `hp2::NotImplemented`, never degrades quietly)

| Path | Why not |
|---|---|
| `Sub_0016BAC0`, pop-up / help menu input | the logic is decompiled (see the function record) but the pop-up object's construction and its +0x24/+0x28/+0x2C/+0x30 handler table are not recovered, so the table cannot be filled in |
| `Sub_0016FBB0`, "FMenus Highlight Animation" per-frame motion | the constructor is decompiled (it binds `GLOBAL_ANIMATING_MENUITEM` and stores 2.0f) but **no recovered code states what it does per frame** — interpolation, duration, easing and target are all unknown |
| `localised_text` | the `LANGUAGES/FE_*.BIN` format is not parsed; the screens still show the author's placeholder strings from the layout file |

Per AGENT.md R3 these return no default, approximate nothing and never silently no-op. The Godot
side displays the exception message on screen.

## 11. Validation

```powershell
ctest --preset native-debug          # 9/9, including the new fmenus_menu (16 assertion groups)
ctest --preset godot-debug           # 9/9
./tools/run_frontend.ps1 -MenuSmoke  # FRONTEND_MENU_SMOKE_OK
./tools/run_frontend.ps1 -Smoke      # FRONTEND_SMOKE_OK: screens=41 resolved=1524 missing=25
./tools/run_frontend.ps1 -ScreenName NEW_MAIN_MENU   # interactive
```

`-MenuSmoke` runs the whole chain headless: resolve `NEW_MAIN_MENU` by name to 0; build a 3-item
menu from that screen's own `MENUITEM` widgets; move the selection with `JOY_EVENT_SCROLL_DOWN`
and request the `MenuUpDown` sound; move back with `JOY_EVENT_MENU_ANALOG_UP`; activate with
`JOY_EVENT_MENU_SELECT` and request `MenuSelect`; confirm a widget received the recovered
highlight colour; and confirm all three unimplemented paths raise `hp2::NotImplemented`.

Screenshots (under `local/shots/`, not committed): the main menu with Championship highlighted in
orange, and after two presses of down, Options orange with Championship back to grey.

## 12. Open questions

1. How raw pad input becomes `JOY_EVENT_*` (the ELF has the strings `GenerateJoyEvents()`,
   `Joystick`, `EventNode` and `EventNodeSlotPool`, but the generating function was not located).
2. The unit of the 34.0 in `FUN_0016E038`.
3. The pop-up object's construction and handler table (see §10).
4. What the highlight animation does per frame (see §10).
5. The remaining 23 screen names (hashes listed in §8).
6. Which bundle declares the `HIGHLIGHT_BAR` widget (none of FRONTB.BUN's 41 screens does).
7. How menu groups are attached to a screen (the construction site of the screen +0x08 list was
   not located; the port currently builds one group per screen itself).
8. The widget name behind 0x120E827C, which `FUN_0016FBB0` resolves.


## TASK-0015 implementation update — 2026-10-12

Earlier missing-animation claims are superseded for NEW_MAIN_MENU: pulse, bars and live
rendering now run every frame. Global 0016FBB0 bindings remain missing (F8 probes that boundary).
Main actions request registered documents. Options direct actions and exit-before-request
are implemented except Display, whose settings snapshot sources are missing. NameEntry
resolves to screen 29; class identity is retained. Exit is recovered cubic Hermite (E-0015-21).
Main Back is forbidden; Options allows it outside exit animation. No original verification;
clock, sine quantization, localisation and destination internals remain limitations.
See TASK-0015 and PROGRESS for exact checks and continuation.
