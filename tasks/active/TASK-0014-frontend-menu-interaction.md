# TASK-0014 — Front-end menu interaction, selection and sound events

- Status: active (interaction recovered and working; animation, pop-ups and localisation remain
  explicitly unimplemented and throw)
- Objective: Make the real front-end screens rendered by TASK-0013 operable the way the PS2 is,
  by translating the original menu runtime out of the ELF rather than designing a UI: item list
  construction, selection movement, highlight colouring, activation callbacks and menu sound
  event selection. Add a keyboard mapping onto the original joypad event ids so the result is
  debuggable on a Windows PC.
- Scope: Included -- FUN_00143D50/FUN_00140CA0 widget lookup and colour write, FUN_00169740
  menu items, FUN_00169F78 list construction, FUN_0016A0F0/0016A130/0016A160 cursor handling,
  FUN_0016A5E0/0016A6C8 navigation, FUN_0016A458 and FUN_00169D98 event handling,
  FUN_0016AA00/FUN_0016A858 menu groups, FUN_0016E038 sound selection, the JOY_EVENT table at
  0x002FA2D0, the string hash FUN_002534A8, and the screen name hashes in the 0x10203 header.
  Excluded -- pop-up/help menus (FUN_0016BAC0), the "FMenus Highlight Animation" object's
  per-frame motion (FUN_0016FBB0), LANGUAGES/FE_*.BIN localisation, kerning application, and the
  raw joypad-to-JOY_EVENT generation path.
- Original PS2 addresses: 0x00140CA0, 0x00143D50, 0x00143F88, 0x00169740, 0x00169818,
  0x00169858, 0x00169968, 0x001699F8, 0x00169BD0, 0x00169CB0, 0x00169D98, 0x00169F78,
  0x0016A0F0, 0x0016A130, 0x0016A160, 0x0016A330, 0x0016A458, 0x0016A5E0, 0x0016A6C8,
  0x0016A858, 0x0016AA00, 0x0016B140, 0x0016B478, 0x0016B9C0, 0x0016BAC0, 0x0016C218,
  0x0016CAB0, 0x0016E038, 0x0016FBB0, 0x002534A8.
- Dependencies: TASK-0011 (screen manager evidence), TASK-0013 (layout + font parsers, renderer).
- Relevant files: AGENT.md, src/game_core/frontend/fmenus.{hpp,cpp},
  src/game_core/input/joyevents.{hpp,cpp}, src/game_core/not_implemented.hpp,
  src/tests/fmenus_menu_tests.cpp, src/platform/godot_bridge/ps2_frontend_layout.{hpp,cpp},
  godot/scripts/frontend.gd, tools/run_frontend.ps1, docs/FRONTEND_MENU_INTERACTION.md,
  reverse/functions/<binary-id>/frontend/ (30 new records), reverse/symbols.csv.
- GitHub issue: not created.

## Acceptance criteria

- [x] The string hash the front end keys everything on is recovered and confirmed against the
      literal immediates in FUN_0016B478 and against 1531/1549 stored widget name hashes
      (E-0014-07).
- [x] Menu items are built the way FUN_00169F78 builds them -- the screen's own MENUITEM_0..15
      type-1 widgets -- not from a port-side list (E-0014-04).
- [x] JOY_EVENT_* ids and names come from the ELF table at 0x002FA2D0 verbatim, cross-checked by
      the 3/0x14, 4/0x15, 5/0x12 and 6/0x13 groupings in the recovered handlers (E-0014-01/02).
- [x] Navigation reproduces the original's wrap-around and disabled-item skipping, including the
      gp-0x533C branch and the "give up when you return to the start" exit (E-0014-10).
- [x] Highlight and disabled colours are the values stored in the ELF, reached through the
      recovered (state & 3) selection and written with the original alpha-preserving masking
      (E-0014-09, E-0014-05).
- [x] Menu sound ids resolve to the original bSound event names from the 0x002FB0A8 table
      (E-0014-11).
- [x] AGENT.md written and enforced: new identifiers are recovered-verbatim or address-anchored;
      unimplemented recovered paths raise hp2::NotImplemented.
- [x] Keyboard mapping onto the recovered event ids, usable on Windows.
- [x] Port-side tests: `fmenus_menu` (16 groups) plus the headless `-MenuSmoke` scene check.
- [ ] Original-behaviour comparison (PCSX2 capture of the same menu transitions). NOT done; no
      capture was available in this session, so nothing here is Verified.

## Current findings

Full write-up with evidence: [docs/FRONTEND_MENU_INTERACTION.md](../../docs/FRONTEND_MENU_INTERACTION.md).

Headline results:

- The front-end string hash is `h = -1; h = h * 0x21 + byte` (FUN_002534A8), confirmed four ways.
- Menu items are the screen's own `MENUITEM_%d` widgets, indices 0..15, type 1.
- Highlight colour is the stored (0x80, 0x40, 0x00) with alpha forced to 0x80; disabled is
  (0x18, 0x18, 0x18). gp = 0x003358F0, read from the entry point, so these are stored values.
- The 0x10203 screen header's +0x08 is the screen's own name hash. 18 of the 41 names were
  recovered by hash match, including **screen 0 = NEW_MAIN_MENU**, which confirms TASK-0013's
  content-based inference, and TITLE_SCREEN(22) / OPTIONS(24) / JOYSTICK_CONFIG(13), which
  independently confirm the three screens TASK-0013 had identified only by eye.
- `HIGHLIGHT_BAR` is the name behind the fixed hash 0x27C5F4AB that FUN_00169F78 resolves.
- FUN_00143D50 keys on widget +0x08 and +0x28 and FUN_00140CA0 writes widget +0x5C and stamps
  +0x2b, which are exactly the offsets TASK-0013 derived from the file. The on-disk record **is**
  the runtime widget node.

### Corrections to earlier tasks

- TASK-0013 open item 5 ("screen-name to screen-index mapping unresolved, screen 0 = main menu is
  a content inference") is **resolved for 18 screens** and the inference for screen 0 is now
  confirmed. The remaining 23 hashes are listed in the write-up.
- TASK-0013 open item 2 said interaction and animation were not driven. Interaction is now
  driven; animation is still not, and now fails loudly instead of being silently absent.

## Next steps

1. Trace the joypad-to-JOY_EVENT generation (`GenerateJoyEvents()`, `EventNodeSlotPool`), which is
   the last unrecovered link between hardware input and the menu runtime.
2. Parse LANGUAGES/FE_*.BIN so screens show shipped text instead of author-time placeholders.
3. Locate where menu groups are attached to a screen (the screen +0x08 list's construction site);
   the port currently builds one group per screen itself, which is port-side scaffolding.
4. Continue the screen-name hash search for the remaining 23 screens.
5. Translate FUN_0016BAC0 once the pop-up object's construction and handler table are recovered.
6. Find what drives the "FMenus Highlight Animation" object per frame before implementing any
   motion; do not approximate it.
7. Capture PCSX2 reference footage of the main menu so any of this can move past Decompiled.

---

## TASK-0015 (started, INCOMPLETE — session ended on a usage limit, 2026-10-12)

Reversing continued and the findings below are confirmed, but only the animation/geometry half
landed in code. **Menu activation still does nothing in the running build** — the navigation
wiring described in "What is still missing" was not written.

### Confirmed findings (Ghidra, read-only; evidence ids E-0015-01..10)

**Screen registration table.** `FUN_0016ADB0(descriptor, class_name, handler, document_name_hash,
group_array)` is called once per screen class during static init, and the document name hash is a
literal immediate, so decompiling its 37 call sites recovers the whole table — class name,
handler address and FRONTB.BUN screen index together. Registration functions: FUN_0016E8C0,
FUN_001B98E8, FUN_001F5660, FUN_00218038, FUN_002411F0, FUN_00247DE8, FUN_0024DC80.

| screen | class name(s) | handler |
|---|---|---|
| 0 | New Main Menu | 0x00216A10 |
| 1 | Cops On Mode Select | 0x002174A0 |
| 2 | Cops Off Mode Select | 0x00217AF8 |
| 3 | ChallengeBeACop | 0x001B6F18 |
| 7 / 8 / 9 | Select Car Dialog / P1 / P2 | none (no handler registered) |
| 11 | Challenge | 0x001B6BD0 |
| 12 | Single Race Setup, Free Run Setup, Lap Knockout Setup, Knockout Setup, Be The Cop Setup | 0x001B8AE8 / 8C98 / 8F30 / 9210 / 93D8 |
| 14 | Popup | none |
| 16 | Credits | 0x001F21B0 |
| 22 | (name unresolved, 4 classes) | 0x001F2470 |
| 24 | Options | 0x001EEF00 (instance handler 0x001EECA8) |
| 25 | Customize P1 HUD / Customize P2 HUD | 0x001F4960 / 0x001F49C8 |
| 26 | Display Options | 0x001F3EC0 |
| 28 | Audio Options | 0x001F0278 |
| 29 | Username Options | 0x001F19E8 |
| 30 | Language Options, Language Options Pre FE | 0x001F1DD8 |
| 31 | Joystick PreConfig | 0x00240060 |
| 32 | Calibrate Display | 0x001F4A30 |
| 33 | Calibrate display offset | 0x001F4B20 |
| 34 | JukeBox Music | 0x001F1220 |
| 36 | BootAutoload, Autosave | 0x00247AB0 / 0x00247D00 |
| 37 | Phils Load Save | 0x00247830 |
| 40 | (name unresolved, 2 classes) | 0x001F3320 |

Note: a class does NOT always use the document of the same name. "Language Options Pre FE"
registers against document hash 0x632B068E = screen 30 (LANGUAGE_OPTIONS), even though screen 23's
own document is named LANGUAGE_OPTIONS_PRE_FE. The registration table is authoritative; the
"class name uppercased with spaces to underscores" rule is only a naming convention, confirmed for
16 screens by hash match but not a law.

**New Main Menu item actions** (FUN_00216638, message 1, item name hash in event field +0x00):

| item | hash | action |
|---|---|---|
| MENUITEM_1 Championship | 0x52E039F3 | DAT_002FC070 = 1; RequestScreen("Cops On Mode Select") -> screen 1 |
| MENUITEM_2 Speed | 0x52E039F4 | DAT_002FC070 = 0; RequestScreen("Cops Off Mode Select") -> screen 2 |
| MENUITEM_3 Options | 0x52E039F5 | RequestScreen("Options") -> screen 24 |

Message 4 (highlight) calls FUN_00216880 with MI_1/2/3_MASK and string ids 0x2720 / 0x271F /
0x2719. Message 0x12 is the per-frame update: it ticks the scrolling text (instance+0x04), the
highlight animation (instance+0x1A8) and the entry list (instance+0x1C8), and uses
StringHash("NEW_MAIN_MENU") = 0xAE0F8AA1 as its document handle, which confirms screen 0 from the
code side.

**Options item actions** (FUN_001EECA8, message 1): Audio -> "Audio Options" (28);
Controller -> "Joystick PreConfig" (31); Display -> "Display Options" (26);
Memory Card -> "Phils Load Save" (37); User Names -> "NameEntry" (unresolved);
Credits -> "Credits" (16); Language Select -> "Language Options" (30). Items 1, 2, 3 and 5 do not
call RequestScreen directly: they build a 0x1C-byte "tExitScreenAnimation" object
(pool name 0x002B9BB0) and animate MI_UNDERSCORE first. Message 4 sets the scrolling-text string
id, 0x186B4..0x186BA.

**Highlight animation** (fully recovered, implemented). The highlighted item emits a ghost: the
ANIMATING_MENUITEM template widget is copied onto it (FUN_001410A0), then scaled from the item's
own scale to twice it and faded from alpha 0x80 to 0 over 0.5 s, easing with
`sin(elapsed * (pi/2) / 0.5)`, repeating every 2 s. Constants read with gp = 0x003358F0:
duration 0.5 (gp-0x3ADC), scales 1.0/2.0 (gp-0x3AD8/-0x3AD4), alpha 128/0 (gp-0x3AD0/-0x3ACC),
default period 4.0 (gp-0x3AC8) overridden to 2.0 by FUN_0016FBB0, ghost RGB 0x000080 (gp-0x3F08),
frame counter scale 0.00025 s (gp-0x6790, i.e. a 4000 Hz counter). FUN_0025A5E0 is a sine: it
scales radians by 10430.4 == 65536 / (2*pi) and indexes a table.

**Selection bars** (recovered, implemented). FUN_00216880 puts MI_UNDERSCORE at the highlighted
item's MI_<n>_MASK bottom edge - 3 and MI_OVERSCORE at its top edge - 6 (gp-0x3EF0 = -3,
gp-0x3EF4 = 6). No interpolation: the original snaps them.

**Widget fields newly confirmed**: +0x60 is a localised string id (FUN_00140D30 measures the
widget with FUN_0021E560(id) when it is non-zero, falling back to the +0x64 placeholder);
screen 0's MENUITEM_1 carries 0x2727 and OPTIONS MENUITEM_1 carries 0x186A1. +0x58 is an anchor
bitfield, not an enum (FUN_00140FA8 tests bits 1, 2 and 4). Text width is the sum of glyph
advances times +0x38; text height is the font line height times +0x3C.

**Not recovered**: the scrolling text and background motion run through the engine's generic path
animator (FUN_00248568 -> FUN_001DAEC8 -> FUN_001DA7D0/FUN_001DA800) whose curve data was not
decoded. `Sub_00249348` therefore throws.

### What landed in code

`src/game_core/frontend/fmenus.{hpp,cpp}`: `WidgetRect_00144280`, `GetWidgetRect_00144280`,
`SetWidgetY_00144458`, `SetWidgetAlpha_00140ED0`, `CopyWidgetTransform_001410A0`,
`SetWidgetScale_00140FA8`, `MenuHighlightAnim_00248870` (Construct/Bind/Update),
`PlaceSelectionBars_00216880`, `Sub_00249348` (throws), the animation constants, and
`FeWidget::string_id_0x60`. Builds clean; `ctest --preset native-debug` and `--preset godot-debug`
both 9/9.

### What is still missing (next session starts here)

1. Nothing calls the new animation code yet: no per-frame tick exists on the Godot side, so the
   ghost pulse and the selection bars are implemented but not driven. Needs a `menu_tick(delta)`
   on `Ps2FrontEndLayout`, a `get_widget_dynamic(index)` that returns x/y/scale/colour per widget
   so `_draw()` can animate, and a `_process` call in `frontend.gd`.
2. The screen registration table and the two recovered per-screen handlers are NOT in code.
   Planned layout, following the original tree: `src/game_core/frontend/frontendscreens/`
   with `options.cpp` (the ELF confirms `indep\src\frontendscreens\options.cpp`),
   `newmainmenu.cpp` (named after the recovered class-name string "NewMainMenu"; the original
   file name is NOT recovered) and a screen-table translation unit (original TU unknown).
3. Activation must be wired: item activated -> screen handler -> class name -> table lookup ->
   screen index -> rebuild the menu, with MENU_BACK popping history. Screens whose handler is not
   translated must raise `hp2::NotImplemented` naming the handler address, per AGENT.md R3.
4. The exit animation that precedes four of the Options transitions is not recovered; implementing
   the transition without it is a deviation that must be stated where it is made.


### Continuation update (2026-10-12)

The incomplete TASK-0015 handoff above is historical. Current scope/results and remaining
Display/original-comparison blockers are maintained in TASK-0015-frontend-activation-animation.md.
Main actions and per-frame pulse are now wired; Options exit motion and Back recovered.
