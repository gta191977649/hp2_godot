# TASK-0013 — Original front-end layout + bitmap fonts, really rendered

- Status: active (real screens render; interaction/animation and localisation remain open)
- Objective: Stop shipping a debug/inspection UI and instead render the game's own authored
  front-end screens the way the PS2 does -- original positions, scales, colours, textures and
  bitmap-font glyphs -- reading everything from the player's ZZDATA copy at runtime.
- Scope: Included -- the 0x80010200 screen/widget layout format, the 0x00015010 bitmap font
  format, native parsers for both, a Godot renderer drawing quads and glyph runs, and a
  standalone exported build. Excluded -- widget interaction/animation/state, localisation
  strings from LANGUAGES/FE_*.BIN, kerning application, and the screen-name <-> screen-index
  mapping.
- Original PS2 addresses: not applicable directly (asset format RE). Cross-checked against
  TASK-0011's ELF evidence via widget name hashes (see Current findings).
- Dependencies: TASK-0012 (native TexturePack decoder + ZZDATA root resolution).
- Relevant files: src/game_core/frontend/fmenus.{hpp,cpp},
  src/platform/godot_bridge/ps2_frontend_layout.{hpp,cpp},
  src/platform/godot_bridge/ps2_texture_loader.{hpp,cpp} (load_texture_by_hash),
  godot/scenes/frontend.tscn, godot/scripts/frontend.gd, tools/run_frontend.ps1,
  docs/FRONTEND_LAYOUT.md.
- GitHub issue: not created.

## Acceptance criteria

- [x] Widget record fields identified by evidence rather than guesswork (texture-hash offset
      found by whole-corpus correlation; scale semantics confirmed by 512x256 x (1.25,1.88) =
      640x480 against the declared screen size).
- [x] Bitmap font format decoded and validated by exact size arithmetic on four separate fonts.
- [x] Real screens render with original textures and original glyphs: Mode Select main menu,
      Options, Controller Configuration (including PlayStation button glyphs from BUTTONSOUT),
      and the Title Screen.
- [x] Everything is read at runtime from the configured ZZDATA copy; no pre-extracted art or
      layout files are shipped or committed.
- [x] Headless smoke covers all 41 screens and resolves 1524 widget resources.
- [x] No regressions: native/bridge CTest 8/8, plus the three pre-existing scene smokes.
- [x] Standalone Windows executable built and verified rendering the main menu.
- [ ] Widget interaction, highlight/scroll/transition animation driven from the original state.
- [ ] Localised strings from LANGUAGES/FE_*.BIN instead of the authored placeholder strings.
- [ ] Kerning pair semantics resolved and applied.
- [ ] Screen-name to screen-index mapping confirmed (screen 0 == "New Main Menu" is currently a
      content inference, not proven).

## Verification procedure

```powershell
./tools/run_frontend.ps1 -Smoke          # expect FRONTEND_SMOKE_OK: screens=41 resolved=1524
./tools/run_frontend.ps1 -Screen 0       # interactive; arrows change screen, Tab hides overlay
ctest --preset godot-debug               # expect 8/8
```
No PCSX2 / original-hardware comparison was performed; screens were checked by human visual
inspection against knowledge of the retail game, not pixel-diffed against a capture.

## Current findings

See docs/FRONTEND_LAYOUT.md for the full field map and its evidence. Key cross-validation: the
widget name hashes stored at record +0x08 appear verbatim as immediates in the ELF's frontend
code -- 0xe0105663/4/5 (POPUP_1/2/3) and 0xab671c9a (ANIMATING_MENUITEM) are passed to
FUN_00169740/FUN_00248870 inside FUN_0016b478 -- which links TASK-0011's code-side evidence to
this task's data-side format. TASK-0011's unresolved FUN_00249a40 ("binds GRID /
MASTER_BP_OBJECT / FADING_BP_OBJECT, role unclear") is now explained: those are three real
background/blueprint widgets present in the layout data, bound by name.

## Unresolved questions

- How does the engine map a screen name ("New Main Menu", "TitleScreen") to one of these 41
  layout entries? No name string exists in the layout data.
- Kerning pair record semantics (second u16 is frequently 0).
- 25 widget texture hashes are absent from the bundles searched so far.
- Which widgets the runtime hides/moves (several authored positions are off-screen by design,
  e.g. ANIMATING_MENUITEM templates).

## Next steps

1. Parse LANGUAGES/FE_ENGLISH.BIN so screens show shipped localised text.
2. Drive selection/highlight state so the menu is operable the way the PS2 is, reusing the
   ScreenManager navigation evidence from TASK-0011.
3. Resolve the screen-name mapping, which is what would let the TASK-0011 navigation graph drive
   these real screens instead of placeholder names.

## Related Git commits

None yet; uncommitted as of this task's last update.

## Completion evidence

2026-10-12: FRONTEND_SMOKE_OK (screens=41, resolved=1524, missing=25); ctest --preset
godot-debug 8/8; frontend_menu_observer / main_loop_observer / --smoke-test unchanged;
screenshots captured for the main menu, Options, Configuration and Title screens, plus a capture
taken from the standalone out/windows-debug/port.exe. project.godot's main scene was temporarily
repointed for the export and restored afterwards (git diff clean).
