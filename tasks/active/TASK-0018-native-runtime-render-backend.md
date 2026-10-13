# TASK-0018 — Native runtime and replaceable rendering backend

Status: active (render path landed; runtime orchestration still host-owned).
User explicitly authorizes implementing NativeGameCore + render abstraction +
GodotRenderingServer, with reverse code independent of every platform. This is a
port-side architecture migration, not new PS2 recovery. Existing addresses and
evidence are unchanged; no recovered function was renamed or rewritten.

## Scope

Move the front end's frame production out of Godot: typed native render frames,
native resource interfaces, and a shared C++ RenderingServer backend owning RIDs.
The Godot-facing host translates input, time, resources, audio hardware and
diagnostics without making game decisions.

## What landed (2026-10-13)

Decision **D008** in docs/DECISIONS.md: `port::render` is the project's only
rendering contract; `port::platform::Rendering::present()` is superseded for the
front end and must not grow a second command path. Frames carry original units and
original resource identities. `Frame` holds only textured quads because that is
the only evidence the front end provides -- deliberately not a modern RHI, since
PS2 GS/VU1 command semantics are not recovered.

New Godot-free library `port_runtime` (CMake target, links `game_core` only):

- `src/port/render/frame.{hpp,cpp}` -- previously unreferenced, now built and tested.
- `src/port/assets/catalog.{hpp,cpp}` -- previously unreferenced, now built.
- `src/port/runtime/frame_builder.{hpp,cpp}` -- **new**. `WidgetSizes` measures
  widgets from the original font tables and TexturePack dimensions and supplies the
  `hp2::frontend::WidgetMeasure` seam; `build_frame` emits the layer-ordered quads,
  per-glyph text with the align bit, and the original clip rectangles via
  `GetWidgetClip_00145F30` + `render::crop`. `FrameStats` reports missing textures
  and fonts instead of substituting art.
- Reserved live-surface ids `kIntroMovieTexture` (above the 32-bit entry-hash space)
  and `kBackgroundMovieTexture` (the authored 0xc3f0caef slot no bundle contains).

Godot backend, `src/platform/godot_bridge/ps2_render_host.{hpp,cpp}` -- **new**.
`Ps2RenderHost` owns the `RenderingServer` RIDs (one canvas item, one 2D texture per
drawn entry) and re-uploads only on a bitmap revision change. `LiveResources` maps an
id to the original catalog or to a live movie surface. `Ps2RenderHost.draw()` is the
whole per-frame host entry point.

Supporting changes: `Ps2MoviePlayer` now also keeps each decoded frame as a
backend-independent `render::Bitmap` with a per-frame revision.
`Ps2FrontEndLayout` owns a `port::assets::Catalog`, gained
`set_startup_presentation` so the frame builder (not a script) suppresses title
widgets, and `measure_widget` falls back to the native catalog when no host
measurements exist for a screen.

Host, `godot/scenes/frontend_native.tscn` + `scripts/frontend_native.gd` -- **new**.
Window, key-to-JOY_EVENT translation, frame delta, audio hardware, process lifetime.
It reads no widget, position, colour or texture. The previous
`scenes/frontend.tscn` / `scripts/frontend.gd` are retained unchanged.

## Verification (exact commands and results)

Tools: CMake/CTest 3.29.2, Ninja, GCC (native-debug), pinned Godot 4.4.1-stable
console build, local ZZDATA via config/ps2_assets.local.json, Windows 11, D3D12.

- `cmake --build --preset native-debug` / `ctest --preset native-debug`
  -- **21/21 passed** (was 20/20; `frame_builder` is new). No warnings.
- `cmake --build --preset godot-debug --parallel 4` -- clean, no warnings.
  The build caught one real defect during development: `FrameOptions::exclusive_texture`
  was 32-bit and truncated the 64-bit live-surface id. Fixed.
- `frontend_native.tscn -- --native-render-smoke` (headless) -- **PASSED**:
  `screens=41 quads=927 glyphs=7733 textures=118 empty=[36]`. Every screen builds a
  frame natively and submits it through the backend.
- **Pixel comparison, old host path vs new native path**, all 41 screens captured at
  `--screen=N` and diffed: **32/41 byte-for-byte identical** (screens 3-23, 25, 29,
  30, 32-38, 40). Matching Godot's `clip_uv` default was required to reach this; with
  `clip_uv=false` the paths differed on 0.03-0.08% of pixels at glyph edges.
- The 9 non-identical screens are explained, not unexplained:
  - 0, 1, 2, 24, 26, 27, 28, 31 -- exactly the background-movie screens. Layout is
    identical by inspection; the residue is video-decode phase (mean delta ~10/255
    outside the hint bar) and scrolling-hint scroll phase. Control experiment: two
    runs of the *same* path differ only in the hint-bar band, so the movie is
    deterministic within a path and offset between paths.
  - 39 -- the event-tree overlay, which only `scripts/frontend.gd` draws. Known gap.
- Pre-existing Godot checks on the retained scene, all still **PASSED**:
  `--frontend-smoke` (screens=41 resolved=1524 missing=25), `--runtime-smoke`,
  `--activation-smoke`, `--submenu-smoke` (8 Back routes), `--menu-smoke`, and
  `main.tscn --smoke-test` (PORT_SMOKE_OK: 42).

PC-to-PC agreement only. None of this establishes original-hardware equivalence and
nothing was promoted to Verified.

## Runtime extraction and dead-code removal (2026-10-13, second pass)

`port::runtime::FrontEndRuntime` (`src/port/runtime/frontend_runtime.{hpp,cpp}`) now
owns the front end: parsed screens and fonts, the recovered menu group, localisation,
the jukebox and sample bank, the recovered animations, navigation history, the
mode-select globals, frame production and the asset catalog. It returns plain C++
values (`LoadResult`, `MenuState`, `TickResult`, `ScreenInfo`, `SoundRequest`,
`MusicRequest`) and links `game_core` only.

`Ps2FrontEndLayout` dropped from **846 lines to 314**: it resolves the ZZDATA search
root (ProjectSettings/OS, the one thing the runtime cannot do), converts native
results to Dictionaries for the older scene, and turns decoded PCM into
AudioStreamWAV. It holds no game state. `Ps2RenderHost` reads
`layout->runtime()` directly, with no Godot value in between.

Latent defect fixed on the way: `options.hpp`, `modeselect.hpp` and their .cpp files
referred to `port::frontend::` unqualified from inside `namespace hp2::frontend`.
Once any `hp2::port::*` namespace existed (sound, assets, audio), lookup stopped at
`hp2::port` and the headers failed to compile. Some lines in the same files already
used `::port::frontend::`; all are qualified now. The old bridge only compiled
because of its include order.

Deleted as unused:

- `src/port/runtime/value.hpp` -- a diagnostic snapshot variant with zero includers.
- `src/platform/services.hpp` and the `platform_api` CMake target -- provisional
  scaffold contracts no recovered caller justified; the only `#include` of it
  (core_tests.cpp) used no symbol from it. Recorded in D008 and ARCHITECTURE.md B.
- `Ps2FrontEndLayout::get_screen_count` and `::get_last_error` -- bound methods with
  no caller in any script.

Verification of this pass: `ctest --preset native-debug` 21/21; `--native-render-smoke`
unchanged (`screens=41 quads=927 glyphs=7733 textures=118`); all five passing Godot
checks on the retained scene plus `main.tscn --smoke-test` unchanged. Nine screens
re-captured through both paths: still **0 differing pixels** new-vs-old, and **0
differing pixels** against the captures taken before this refactor, for both paths --
so the extraction changed no output at all.

## Event tree, test migration and old-host removal (2026-10-13, third pass)

**1. The event-tree overlay is native.** `port::runtime::EventTreeView`
(`src/port/runtime/event_tree_view.{hpp,cpp}`) holds the open tree and the cursor;
`FrontEndRuntime` loads it when screen 39 opens and clears it when another screen does,
`event_tree_joy_event` routes the recovered FUN_001FAF60 directions, and
`append_event_tree` emits the panels, the event numbers and the `tLineSegment`
connectors. The connectors are axis-aligned, so `render::Quad` gained one convention --
`texture == 0` is a solid-colour rect -- which is a D008-justified extension because a
recovered caller requires it. The backend draws those through `canvas_item_add_rect`.
The missing-panel placeholder changed shape (circle -> square outline) because a frame
carries no arc primitive; both are port-side placeholders. The camera calibration
(`kPortZoom`, `kPortPanelScale`) is still explicitly not recovered and is labelled so at
its definition. **Screen 39 now renders byte-for-byte identically to the old host.**

**2. The behaviour checks are native.** `src/tests/frontend_runtime_tests.cpp` replaces
four and a half GDScript smokes with seven groups under CTest, driving `FrontEndRuntime`
directly with the player's own assets (and skipping with a message when the ZZDATA root
is not configured):

| group | replaces | result |
|---|---|---|
| asset resolution | `--frontend-smoke` | screens=41 resolved=1524 missing=25, identical to the GDScript counts |
| main menu | `--menu-smoke` | items, scroll, sounds, activation, ELF highlight colour, 3 NotImplemented probes |
| activation | `--activation-smoke` | 3 main routes, mode flag, Back, 7 Options routes, deferred exit, 0x001EE6C0 gap |
| submenus | `--submenu-smoke` | 8 Back routes, locale direct-pop, Audio/Display clip + scroll, selectors, non-wrap ends |
| highlight ghost | most of `--animation-smoke` | start, grow/fade, text copy, retarget, delta guards |
| localisation/audio | most of `--runtime-smoke` | 6 hint screens, stereo menu sample (22050 Hz, 23408 bytes, gain 0.1023), jukebox 5 then 7 |
| event tree | new | 32 events, cursor navigation, screen-scoped lifetime |

Each group builds its own runtime, the way each smoke used to get its own process:
navigation history and the locale deliberately survive `menu_open`, so a shared runtime
leaked one group's stack into the next. That was a real defect in the first draft of
these tests, caught by the suite.

What stayed in Godot is what needs Godot: `--runtime-smoke` on the native scene now
covers MV30 decode into a texture, intro PCM into an `AudioStreamWAV`, the recovered
title time gate, and that the authored PANELMOVIE slot reaches the renderer -- the last
checked through a new `movie_quads` statistic from the backend, so it reads no widget.

**3. The old host is deleted.** Gone: `godot/scripts/frontend.gd` (1170 lines),
`godot/scenes/frontend.tscn`, `src/platform/godot_bridge/ps2_event_tree.{hpp,cpp}`, and
from the shim `get_widgets`, `get_widget_dynamic`, `get_widget_colours`, `get_font`,
`get_screen_info`, `set_widget_measurements`, `joy_event_name`, `probe_not_implemented`,
`name_hash`, `event_tree_cursor_info`. `project.godot` now boots
`scenes/frontend_native.tscn` and `tools/run_frontend.ps1` drives it.

With `set_widget_measurements` gone, `FrontEndRuntime` dropped `widget_base_metrics_`,
`has_host_measurements`, `open_screen_index` and the host-measurement branch of
`measure_widget`, which is now one line over `WidgetSizes`. The one recovered call that
hung off the host path, `PlaceSelectionBars_001F4E68`, was a port-side extra there: the
recovered callers in `options.cpp` still invoke it, and the native submenu checks pass
without the extra.

Audit after the deletion: every bound method and every registered bridge class has at
least one caller. `Ps2TextureLoader` survives because `frontend_menu_observer.gd` uses
it.

Verification of this pass: `ctest --preset native-debug` **22/22**; `--native-render-smoke`
`screens=41 quads=1085 glyphs=7765 textures=120` (up from 927/7733/118 because screen 39
now draws); `--runtime-smoke`, `main.tscn --smoke-test`, `frontend_menu_observer
--frontend-menu-smoke` and `main_loop_observer --observer-smoke` all pass; the default
scene boots clean. Nine screens re-captured after the deletion are **0 differing pixels**
against the native captures taken before it.

## Frame-rate readout and a first throughput measurement

Both scenes now print `<n> FPS  <ms>  [<render path>]` in the window title at all times
(updated 4x a second) and, with Tab or `--info`, on the first overlay line. The path name
is in the string so a screenshot says which renderer produced it.

With vsync on, both paths sit at the refresh rate and the number says nothing. Uncapped:

```
godot --path godot --disable-vsync --max-fps 0 --print-fps res://scenes/frontend_native.tscn -- --screen=0
```

Main menu (screen 0, background movie playing), Windows 11, D3D12, Intel Arc B390,
steady-state `Project FPS` over 8 s:

| path | FPS |
|---|---|
| `frontend_native.tscn` -- native frame + RenderingServer | 1237, 1348, 1447, 1422, 1427, 1382 |
| `frontend.tscn` -- GDScript `Control._draw` | 1059, 1154, 1131, 1160, 1191, 1195, 1030 |

Roughly 20% faster on this screen. One machine, one screen, one run each: an indication,
not a benchmark, and not the reason for the migration. The plausible cause is that the
old path builds a Dictionary per widget per frame and does glyph layout and clipping in
GDScript, while the new path builds the quads natively and the backend re-uploads a
texture only when its revision changes.

## Known open question: the highlight ghost period

The old `--animation-smoke` failed with "ghost did not restart after two second period",
and that failure was confirmed pre-existing: it reproduced on a clean `git stash` of all
TASK-0018 work with the bridge rebuilt from the untouched tree.

The native replacement measures it instead of asserting it: at 2.01 s the ghost scale is
**1.7071** against a target of 1.0000, i.e. no restart, which is what a 4 s period gives.
`open_menu_animation` records screen 0 as keeping the default 4 s (E-0016-08), so the old
check's 2 s expectation contradicts the recorded evidence. Either the check was wrong or
the period is, and nothing here resolves it -- the number is printed, not asserted, and
this needs its own scoped task against the original.

## Deliberate divergence to record

Native measurement measures the live widget; the old host sampled measurements once
at screen build time, so an empty-text widget (notably SCROLLING_TEXT before its
string is bound) measured as zero width there and measures its real string here.
`measure_widget` therefore still prefers host-pushed measurements when present, so
the retained scene keeps its previously recorded behaviour exactly. Removing
`set_widget_measurements` is blocked on retiring that scene.

`SCROLLING_TEXT` carries `text_align = 0x2` with no 0x10 bit, so
`GetWidgetClip_00145F30` gives it no clip and it overflows the hint bar. Verified as
the recovered rule in both paths, not a new defect. Whether the original clipped it
by another route is unresolved.

## Acceptance criteria

- [x] Runtime/core compile without Godot; no Godot or platform-specific header in
      recovered core, render contracts or runtime (`port_runtime` links `game_core` only).
- [x] Fake backend exercises actual frontend frames (`frame_builder_tests`).
- [x] A Godot scene submits native commands via RenderingServer.
- [x] Menu/scroll/clip regressions pass; no regression in the retained scene.
- [x] Document dependency graph, resource lifetime, interfaces and unfinished behaviour.
- [x] Frontend state moved out of the Godot adapter into a platform-independent
      runtime object (`port::runtime::FrontEndRuntime`).
- [ ] Startup/movie/music orchestration and screen selection moved out of GDScript.
      Still driven by `frontend_native.gd`.
- [x] Event-tree overlay native; the old GDScript host and its Dictionary surface deleted.
- [x] Front-end behaviour checks moved from GDScript smokes to CTest.
- [ ] Windows DX12 captures and export pass for the new scene (captures done; export
      preset not re-run against `frontend_native.tscn`).
- [x] No fake 3D/physics/AI implementation.

## Next steps

1. Move startup/movie/music orchestration and screen selection into the runtime, so
   `frontend_native.gd` keeps only window, input, audio hardware and delta. It is the last
   front-end decision-making left in GDScript.
2. Resolve the highlight ghost period above as its own scoped task.
3. Re-run the Windows DX12 export preset against `frontend_native.tscn`.
4. Reverse the event-tree camera so `kPortZoom` / `kPortPanelScale` can stop being
   port-side calibration.

## Related commits

None. All TASK-0018 work is uncommitted in the working tree; no commit was requested.
