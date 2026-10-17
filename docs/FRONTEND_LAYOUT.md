# TASK-0013: the original front-end screen layout format (really rendered, not a debug view)

TASK-0039 correction (2026-10-17): the historical completion wording below does not establish
original equivalence. Read [FRONTEND_RENDER_AUDIT.md](FRONTEND_RENDER_AUDIT.md) for the current
Ghidra MCP-backed rules and remaining gaps. Font height, pair lookup, right alignment and
four-corner quad rendering were incomplete or misidentified in this initial recovery.

Completed 2026-10-12, Australia/Sydney.

The user pointed out that what had been delivered was a **debug/inspection view**, and that what
they wanted was the front end **actually rendered the way the PS2 draws it**. This task decodes
the game's own screen layout records and bitmap font tables inside `FRONTEND/FRONTB.BUN` and
draws them in Godot one widget at a time, at the original positions, scales, colours, textures and
glyphs. **All 41 original screens now render for real** — the Mode Select main menu, Options,
Controller Configuration, the Title Screen and the rest — rather than a hand-built mock-up.

## 1. Screen and widget layout format (chunk 0x80010200)

The top-level `0x80010200` container in `FRONTB.BUN` is the entire front end's screen list:

```text
0x80010200  screen list container
├── 0x00010213  20-byte header
└── 0x80010201 x 41   one per screen
    ├── 0x00010203  40-byte screen header (+0x18 u16 width, +0x1a u16 height -> all 640x480)
    ├── 0x00010210  144 bytes  textured quad widget
    └── 0x00010212  104-124 bytes  text widget (trailing variable-length string, hence the range)
```

The part both record types share (offsets are within the record):

| Offset | Type | Meaning | How it was established |
|---|---|---|---|
| 0x00 | u32 x2 | runtime pointer slots, always 0 in the file | observation |
| 0x08 | u32 | **widget name hash** | matches the immediates in the ELF byte for byte (see §4) |
| 0x0c | u32 | class hash, shared by widgets of a kind (e.g. 0x457755c9) | consistent across widgets |
| 0x10 | char[24] | widget name (`MENUITEM_1`, `GRID`, `PRESS_START_1`, …) | directly readable |
| 0x28 | u8 | type (0x01 text / 0x02 quad) | one-to-one with the chunk id |
| 0x29 | u8 | visible/enabled flag | only ever 0 or 1 |
| 0x2a | u8 | **layer** (larger is further back; grid backdrops are 0x08/0x0a, menu text is 0x00) | painting in descending order is the only order that looks right |
| 0x30 | float | X (top-left, in 640x480 space) | see the checks below |
| 0x34 | float | Y | same |
| 0x38 | float | X scale (multiplies the texture's native width) | see the checks below |
| 0x3c | float | Y scale | same |
| 0x50 | u32 | **texture hash** (a TexturePack entry hash; for text widgets, the font atlas) | whole-corpus correlation, see below |
| 0x58 (quad) / 0x5c (text) | u8 x4 | colour R,G,B,A, PS2-style alpha where 0x80 is opaque | values like neutral 0x80808080 and half-transparent black 0xaa000000 fit what is on screen |

Text widgets carry three more fields: `0x54` u32 is the author-set text buffer length, `0x58` u32
is the alignment (**2 = left, 4 = centred**), and the variable-length default string starts at
`0x64`.

### The field map was derived, not guessed

- **The texture hash is at 0x50.** Every 4-byte-aligned offset in every widget of all 41 screens
  was read as a u32 and correlated against the real TexturePack entry hash set. `0x50` hits
  507/1015 in the 0x10210 records and 206/534 in the 0x10212 records; every other offset scores
  far lower (the runner-up, 0x08, manages 79/1015, which is the name hash colliding by chance).
- **Scale multiplies the texture's native size.** `TITLE_FULLSCREEN` references `SPLASH_SCREEN`
  (512x256) at scale (1.25, 1.88), which gives 640 x 481 — exactly the 640x480 the screen header
  declares. `GRID` (512x384) at scale 1.25 gives 640x480 as well. `WHITE16X16` (16x16) at scale
  (32.0, 1.70) gives the 512x27 highlight bar, which matches the menu's line height.
- **Alignment 2 vs 4.** `PRESS_START_1` sits at x = 320 (screen centre) and the right-hand data
  column `LINE_1..8` at x = 494, both with align = 4; the left-hand menu items `MENUITEM_1..8` sit
  at x = 153 with align = 2.
- **Descending layer order.** Painting by `0x2a` descending puts the grid and blueprint backdrops
  correctly behind the menu text; every other order tried (file order, reverse file order) paints
  the backdrop over the text.

## 2. Bitmap font format (chunk 0x00015010)

```text
0x00  u32  font texture hash (FONT_IMPACTSMALL / FONT_IMPACTBIG / BUTTONSOUT / ...)
0x04  u32  glyph count
0x08  u32  kerning pair count
0x0c  u32  glyph-table byte offset including chunk header (28 in observed fonts)
0x10  u32  byte offset of the kerning table (including the chunk's own 8-byte header)
0x14  ..   glyph records, 32 bytes each:
           u16 code, u16 flags, float u, v, w, h, advance (all in atlas pixels)
then       the kerning table, 8 bytes per pair
```

The sizes check out exactly: `FONT_IMPACTSMALL` has 130 glyphs, and 130 x 32 + 20 + 8 = 4188,
which is the kerning offset the header states; the payload is 10412 bytes long and 10412 - 4180 =
6232 = 779 pairs x 8 bytes, to the byte. `FONT_EVENTNUMBERS` (10 glyphs, 50 pairs, 740-byte payload,
pair-table offset348 including the chunk header) is
equally exact. Glyph codes are **sparse** — the Impact fonts have no `$` — so lookups must go by
character code, never by index.

Every font located:

| Font | Bundle | Glyphs | Notes |
|---|---|---|---|
| FONT_IMPACTSMALL / IMPACTBIG / IMPACTNORMAL | FRONTEND/FRONTB.BUN | 130 | menu body and titles |
| FONT_EVENTNUMBERS | FRONTEND/FRONTB.BUN | 10 | digits only |
| FONT_A / FONT_HUD / FONT_STATS | GLOBAL/GLOBALB.BUN | 130 | |
| **BUTTONSOUT** | GLOBAL/GLOBALB.BUN | 29 | **the pad-button icon font** — the circle/cross/triangle/R3/stick icons on the Configuration screen are drawn as characters out of it |
| FONT_GAUGE / FONT_POSITION | GLOBAL/INGAMEB.BUN | 12 / 9 | in-game HUD |

## 3. Implementation

- `src/game_core/frontend/fmenus.hpp/.cpp` — the Godot-free native parsers (`parse_screens`,
  `parse_fonts`). The file is named after the original source path embedded in the ELF,
  `indep\src\fmenus.cpp` (see docs/ORIGINAL_SOURCE_TREE.md).
- `src/platform/godot_bridge/ps2_frontend_layout.{hpp,cpp}` — the `Ps2FrontEndLayout` bridge,
  exposing screens, widgets and fonts to GDScript.
- `src/platform/godot_bridge/ps2_texture_loader.*` — gained `load_texture_by_hash()`, resolving a
  texture by hash across bundles with both an archive and a texture cache.
- (TASK-0018: deleted. The host below was `godot/scenes/frontend.tscn` + `godot/scripts/frontend.gd`;
  frames are now built by `port::runtime::build_frame` and submitted by `Ps2RenderHost`.)
  Historical: a single immediate-mode `_draw()`:
  quads through `draw_texture_rect`, text glyph by glyph through `draw_texture_rect_region` taking
  UVs from the font atlas. Left/right arrows change screen, Tab hides the debug line, Esc quits.

Every byte is read **at runtime** from the locally configured ZZDATA copy (path resolution lives
in `src/platform/ps2_asset_root.hpp`); the repository contains no pre-exported images or layout
files.

## 4. Cross-check against the ELF code

The widget name hashes at record +0x08 appear in the ELF as **literal immediates**: `0xe0105663`,
`0xe0105664` and `0xe0105665` in `FUN_0016b478` are exactly the name hashes of `POPUP_1`,
`POPUP_2` and `POPUP_3` in the layout data, and `0xab671c9a` in the same function is
`ANIMATING_MENUITEM`. That ties TASK-0011's code-side evidence from Ghidra directly to this task's
data-side format — one hash scheme, with the code looking widgets up by name hash and the data
declaring them by name hash. The three names `FUN_00249a40` binds in TASK-0011, `GRID`,
`MASTER_BP_OBJECT` and `FADING_BP_OBJECT`, are real widgets in the layout (see the widget lists of
screens 0, 2 and 40), so what was then recorded as "purpose not fully resolved" can now be
confirmed as **binding, by name, the background grid and blueprint layer widgets on the screen**.

## 5. Validation

```powershell
# interactive (arrow keys change screen)
./tools/run_frontend.ps1
# headless self-check: walk all 41 screens and resolve every widget's texture/font
./tools/run_frontend.ps1 -Smoke
```

- `FRONTEND_SMOKE_OK: screens=41 resolved=1524 missing=25` — 41 screens with 1524 widget assets
  resolved; only 25 across the whole front end are unresolved, scattered over a few widgets that
  reference bundles this session did not cover.
- Visually confirmed and captured: the Mode Select main menu (Championship / Speed / Options with
  the HP2 logo), Options (Audio … Language Select), Configuration (pad button icons over the
  blueprint grid) and the Title Screen (HP2 splash with PRESS START).
- Native CTest 8/8; the three existing scenes `frontend_menu_observer`, `main_loop_observer` and
  the original `--smoke-test` show no regression.
- Standalone build: `out/windows-debug/port.exe` (plus `port.pck` and `ps2_port.dll`). The main
  scene was temporarily pointed at `frontend.tscn` for the export and `project.godot` was restored
  afterwards.

## 6. Open questions

1. **Superseded by TASK-0039:** the previous parser actually discarded the pair table.
   Ghidra00140098 now establishes glyph+02 pair count and glyph+1C first index, previous-byte
   matching and pair float adjustment. Actual draw applies pairs; width measurement deliberately
   does not.0013FFA8 derives line height as maximum glyph height, rather than header value28.
2. ~~**No interaction or animation**~~ — **the interaction half was solved by TASK-0014** (see
   [docs/FRONTEND_MENU_INTERACTION.md](FRONTEND_MENU_INTERACTION.md)): the item list, selection
   movement, highlight colouring, activation callbacks and sound events were all reversed out of
   the ELF and implemented. **Animation is still not implemented**: only the constructor of the
   object the ELF calls "FMenus Highlight Animation" was recovered, and no recovered code states
   what it does per frame, so the port raises `hp2::NotImplemented` there instead of inventing an
   easing curve. The same applies to the scrolling text.
3. **The text is the author's placeholder.** What is drawn is the default string in the layout
   file (`MenuItem 1`, `EACOPYRIGHTSTRING-…`, `This text will scroll sideways…`). The real runtime
   copy comes from the `LANGUAGES/FE_*.BIN` localisation tables, whose format was not parsed here.
   The main menu and Options happen to carry their final text in the layout, which is why they
   look correct.
4. 25 widget texture hashes are not found in the bundle list searched this session.
5. ~~The screen-to-screen-name mapping is undetermined~~ — **TASK-0014 resolved 18 of them.** A
   screen's name is not a string but a **hash**, stored at **+0x08** of the 0x10203 header;
   hashing candidate names with the recovered `StringHash` confirms a match. **Screen 0 is
   `NEW_MAIN_MENU`**, so what this document previously called a content-based inference is now
   **confirmed**; screen 22 = `TITLE_SCREEN`, 24 = `OPTIONS` and 13 = `JOYSTICK_CONFIG` likewise
   confirm the three screens §5 above had only identified by eye. The remaining 23 screen hashes
   are listed in FRONTEND_MENU_INTERACTION.md §8.
