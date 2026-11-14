# TASK-0146 - PS2 car paint and Godot shader resources

- Status: active
- Scope: vehicle paint, environment capture, external Godot shader resources and Forward+ race postprocessing.
- Acceptance: native-depth and GS-history pixel checks on Vulkan and D3D12, diagnostic race lifecycle, frontend preview, and matched PS2 paint comparisons.

## Native fullscreen Starting Grid

Vulkan Release on Arc B390, fullscreen window and main viewport both
2880×1800 at 120 Hz. The benchmark starts Track 13 directly through the
frontend and records the background flyby before confirmation. Tick 90
shows the forest warning sign. Tick 60–120 is the slow camera interval.
Image readbacks were performed in separate runs.

Closing just weather fog changed whole-flyby P95 from 12.498 to 12.455 ms.
The matched Visual Profiler capture with original mip filtering measured
14.885 ms GPU P95 in the slow interval, including a 10.656 ms main 3D
scope, 2.981 ms transparent draw, and 1.228 ms screen copy. Suppressing
sun glare reduced GPU P95 to 10.658 ms and removed the screen-copy scope.
Suppressing AcidFX reduced GPU P95 to 12.186 ms and its transparent scope
to 1.327 ms. Suppressing authored light shafts alone measured 13.969 ms
GPU P95. The production path keeps all three effects enabled.

Sun glare now submits a disjoint cover of the visible texture alpha bounds.
The draw preserves the existing depth and sky sampling, screen copy, and
ordered blend. Geometry is allocated once, with pixel-aligned rectangles
and a conservative margin for linear sampling. A frozen tick-90 comparison
against the old rectangle produced identical RGB pixels. The existing sun
alignment and blend fixture passed on Vulkan and D3D12.

The first 4× anisotropic versus old-filter comparison measured 14.235
versus 14.818 ms P95 in the slow interval. A later 4× run spiked to
21.664 ms P95 and 22.644 ms P99 in that window. This variation does not
isolate a causal cost from anisotropic filtering. The old mipmap and GS
filter paths are restored for the selected native configuration. Road-only
2× was left out while native 120 Hz pacing remained above budget.

Opt-in FXAA disables 3D MSAA, copies native depth into a Godot-owned R32
buffer for fog and focus, and antialiases the composed scene below the HUD.
The FXAA path passed native-depth, GS history, resize and reset fixtures on
both Vulkan and D3D12. The runtime launcher flag is `--fxaa`; the normal
launch uses 2× MSAA.

| Fullscreen configuration | Average FPS | Whole P95 / P99, ms | Tick 60–120 P95 / P99, ms |
| --- | ---: | ---: | ---: |
| Old filter, original sun rectangle, 2× MSAA | 108.3 | 13.273 / 14.314 | 14.392 / 14.565 |
| Old filter, sun regions, 2× MSAA | 108.9 | 10.305 / 14.282 | 14.586 / 14.904 |
| Old filter, original sun rectangle, FXAA | 147.7 | 7.908 / 10.193 | 10.847 / 11.549 |
| Old filter, sun regions, FXAA | 173.3 | 7.483 / 8.555 | 8.015 / 8.644 |
| Old filter, sun regions, FXAA, 120 Hz VSync | 119.7 | 8.727 / 9.157 | 8.570 / 8.808 |

The staged launcher run with VSync disabled and the 120 FPS limit averaged
119.4 FPS. Whole-flyby P95/P99 were 8.512/8.908 ms and the slow interval
was 8.501/8.745 ms. The uncapped launcher run had 7.931/8.010 ms in
that interval with all 71 sampled frames in budget. VSync averaged
119.6 FPS with whole-flyby P95/P99 of 8.701/8.987 ms. The limiter gave the
closest pacing to 120 Hz among tested configurations, with a remaining
0.17 ms gap to the strict P95 target in the slow interval.

```powershell
./build.ps1 --fullscreen --fxaa
./build.ps1 --fullscreen --fxaa --fps120
$iso = 'D:/ps2_game/Need for Speed - Hot Pursuit 2 (USA).iso'
py -3 tools/profile_mvp_reflections.py --assets $iso --out local/task0146/native-repro --grid --fog baseline --fullscreen --resolution 2880x1800 --fxaa --quiet
py -3 tools/profile_mvp_reflections.py --assets $iso --out local/task0146/native-sync-repro --grid --fog baseline --fullscreen --resolution 2880x1800 --fxaa --quiet --vsync
py -3 tools/profile_mvp_reflections.py --assets $iso --out local/task0146/native-sun-probe --grid --fog baseline --fullscreen --resolution 2880x1800 --grid-effect no-sun
py -3 tools/profile_mvp_reflections.py --assets $iso --out local/task0146/native-sun-pixels --grid --fog baseline --fullscreen --resolution 2880x1800 --fxaa --quiet --grid-effect compare-sun-bounds --capture-tick 90
```

## Starting Grid fog and AcidFX particles

The fixed-seed Track 13 benchmark enters Quick Race through the original
frontend callback and samples 720 source ticks of the Starting Grid
background flyby before confirming the grid. Every sampled window had
weather fog strength 0.15, fog start slot 125, two focus taps, and history
available with radial blur inactive. The captures show no change in the
authored weather values across this window.

The combined postprocess defaults to the analytic fog shader, which had
the highest median run-average FPS. Fog-only composition uses a cached
256-slot table and draws directly into the output canvas. The table and
encoded-color path in the combined pass remain available for isolated
profiling. Godot's resolved scene depth is still sampled after transparent
draws with 2× MSAA.

The common AcidFX shaders collapse quads rejected by the current camera or
known to fail the authored alpha test. Particle step history is retained in
Godot's layered texture and only dirty pages are uploaded. Each page is
50 KiB, compared to 400 KiB for the old full upload. Source emission, motion,
blend modes, material priority and atlas sampling are preserved. Mipmapped
3D surface materials use 4× anisotropic filtering by default. The opaque
GS material passes directional gradients bounded by its authored mip range.
The Debug console's `ver` command reads the running Godot Engine version.
This follow-up has no game_core changes or new reverse-engineering claims.

### Performance and checks

Windows Arc B390, Godot 4.7.2, Vulkan Forward+, 1080p Release, VSync off.
The six runs used the same build, both with the shared particle changes
and anisotropic filtering. The table reports the median of each metric
across three runs.

| Combined postprocess | Run-average FPS | Frame P50 / P95 / P99, ms | Frames over 8.33 ms |
| --- | ---: | ---: | ---: |
| Analytic | 203.8 | 4.408 / 6.980 / 7.652 | 0.49% |
| LUT and encoded color | 197.1 | 5.171 / 6.851 / 7.790 | 0.41% |

With the Visual Profiler enabled, the analytic pass had render CPU
P50/P95/P99 of 1.751/2.610/2.983 ms and GPU of 3.392/4.346/4.664 ms.
The experimental pass had CPU of 1.865/2.719/3.200 ms and GPU of
3.431/4.712/5.249 ms. The longest CPU-selected transparent scope had
GPU P95 of 1.415/1.424 ms respectively. The similarly selected
CanvasItems scope had GPU P95 of 0.076/0.065 ms. An earlier 120 Hz
diagnostic run had P95 8.621 ms. The final VSync rerun was halted at
the user's request, so final synchronized pacing remains unverified.

The Track 13 postprocessing fixture passed on Vulkan and D3D12. It
compared fog-only output to the analytic pass and checked reverse-Z,
GS history, resize and reset. Debug Vulkan pixel probes confirmed Track 21
sandstorm and debris, Track 12 forest smoke, and Track 61 waterfall mist.
In a single 100-frame Track 21 diagnostic A/B with paged uploads in both
variants, enabling vertex rejection changed frame P50/P95/P99 from
4.668/5.497/5.610 ms to 4.100/4.923/5.394 ms. The D3D12 fire-truck shader
probe passed its visible-pixel and priority checks. The D3D12 sandstorm
launch failed to load a track shader in the shared Debug staging
directory. A fresh isolated rerun, the new Track 33 lifecycle runs and the
console screenshot check were deferred when the user stopped testing.
The exhaust probe rendered pixels, then timed out before the nitro check.

```powershell
$iso = 'D:/ps2_game/Need for Speed - Hot Pursuit 2 (USA).iso'
$env:HP2_PS2_ASSET_ROOT = $iso
$engine = "$PWD/local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe"
cmake --build --preset godot-mvp-release --target stage_mvp --parallel 8
cmake --build --preset godot-mvp-debug --target stage_mvp --parallel 6
py -3 tools/profile_mvp_reflections.py --assets $iso --out local/task0146/weather-grid2-ab --grid --fog ab --quiet
py -3 tools/profile_mvp_reflections.py --assets $iso --out local/task0146/weather-grid2-visual --grid --fog ab --pairs 1
py -3 tools/profile_mvp_reflections.py --assets $iso --out local/task0146/weather-vsync-repro --grid --pairs 0 --quiet --vsync
$env:HP2_MVP_TEST_POST = '1'
$env:HP2_PROFILE_DIR = "$PWD/local/task0146/weather-post-repro"
& $engine --path build/godot-mvp-release/mvp-project --rendering-driver vulkan --resolution 960x720 --disable-vsync --script "$PWD/tools/test_mvp_race_post.gd" -- --race-track=13
Remove-Item Env:HP2_MVP_TEST_POST
$env:HP2_PROFILE_DIR = "$PWD/local/task0146/weather-sand-repro"
$env:HP2_MVP_TEST_TRACK_EFFECTS = '1'
$env:HP2_MVP_PROFILE_PARTICLES = 'baseline'
& $engine --path build/godot-mvp-debug/mvp-project --rendering-driver vulkan --resolution 1920x1080 --disable-vsync --script "$PWD/tools/test_mvp_particle_routes.gd" -- --race-track=21
```

## Evidence and core boundary

- PAL donor annotations retain their existing provenance. ELF version and SHA-256 remain unresolved. PAL1211EC supplies body lighting, PAL123BA0 selects the environment focus, PAL1175C8–1178E8 supplies the face schedule, and PAL117A38–118880 supplies the raster grid and remap ordering. PAL10B08C–10B0F8 selects the second reflection ALPHA context.
- Debug ELF SHA-256 `815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b` was checked against the local file. TASK-0119 records Debug 001210F0 and the prior Ghidra checks. Its distinct US retail evidence uses SHA-256 `7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`, VU 002A7F90, 002A8308 and 002A8AD8, and E0052D08/D13 for packets 318 and 31C.
- The port-side runtime boundary borrows the active camera's followed-car origin and the existing resource catalogue. The frame previously exposed only the hidden car and camera pose, and the decoded light registry and special textures had no host accessor. The donor camera scheduling, lighting queries and asset decoding remain the source of those operands. Debug and Release builds and loaded races exercise the boundary.

## Implementation and limits

- Godot loads project-owned `.gdshader` resources staged from `src/mvp/shader`. The post-transparent callback only publishes color and depth textures. Forward+ 2× MSAA supplies a resolved R32 reverse-Z image. Godot canvas shaders handle fog, focus, filter, monochrome and alternating history.
- Body paint uses raw normals and authored RGB and material coefficients. The current shader folds base, specular and reflection at the fragment for eligible opaque and cutout surfaces. The race atlas uses the donor camera schedule and sample grids, and the frontend uses the authored CarSelectEnvMap.
- Draws requiring transparent GS framebuffer feedback still use the existing transparent material route. Explicit framebuffer alpha for those draws, independent context tests and write masks, and exact paint parity across damage and wheel variants remain open. Original PS2 equivalence requires matched original runtime captures.

## PC results

| Check | Result |
| --- | --- |
| Godot 4.7.2.stable.official.ed1daf0bf, Intel Arc B390, Vulkan and D3D12 Forward+ | Track 12 passed authored weather and forced post effects, reverse-Z visualization, both GS history fixtures, odd-size resize and explicit reset. The race atlas covered 815 sampled points on each driver. The smoke verified 230 vehicle paint surfaces on Vulkan and 234 on D3D12. |
| Track 33 D3D12 and Track 13 Vulkan, Release | Diagnostic Quick Race passed scene publication, driving, pause, restart and exit. |
| Track 33 Vulkan, Release | The diagnostic lifecycle script entered the original replay and reported replay epoch 1. |
| Ultimate Racer event 5, Release Vulkan | The direct police event entered Track 11 with the player as pursuing officer and the authored police HUD. An earlier run also exercised both support requests. |
| Track Viewer, Debug Vulkan | The first smoke loaded Track 11 and timed out during scripted UI interaction. Full viewer control and retirement checks remain open. |
| Frontend, Release Vulkan | The test exercised original menu video and reached ordinary Quick Race. Dedicated Select Car paint and resize acceptance remains open. |
| Builds | Debug and Release staging passed. A running game initially held the standard Debug DLL open, so the final Debug pixel runs used a separate staged project. |
| GPU profile, Debug Vulkan at 960×720 | Godot's `--gpu-profile` reported 2.96–3.29 ms with post disabled and 4.67–5.10 ms with motion blur in the migrated build. The archived compositor build reported 1.19–2.94 ms and 2.59–10.16 ms respectively. These short runs used different randomly selected vehicle grids, so they do not isolate a migration cost. |
| PS2 comparison | Pending. PC smoke results do not establish visual equivalence. |

## Vehicle shadows and transparent effects

Ghidra MCP rechecked Debug `NFSHP2.ELF` (SHA-256
`815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`).
`CarRenderInfo::Render` 001210F0 uses the terrain-projected hull and
`SHADOWRAMP` separately from the car-specific ground texture.
`AcidEffectSystem::RenderEffects` 00128EF8 submits direct texture packets.
`eAttachDirectData` 00110768 admits a packet into the alpha list based on
the texture and retains its sort Z. `RadixSortAlphaKeys` 00104FE0 reads
the authored rendering order. `AttachAlphaPolys` 0010BBF8 chooses the draw
context. `CreateTextureContext` 00108AE0 reads the authored GS texture state.
`StuffSun` 00125430 and `StuffSunGlare` 00125A70 are the separate screen
sun path. The released donor flushes world alpha before fog at PAL1137CC.

The Track 12/13 asset records bind `XE_FireTruck_WaterA` through `D`
to `DUSTA_CS`, priority 7. `SMOKE_CS` and `SMOKEB_CS` use priority 6.
The forest light shaft `GRADIENTLIGHT` and car `WINDOW` use priority 5.
The sunlight ramp and ground shadow textures use priorities 4 and 3.
These use source-over, full alpha range and no depth writes. The forest
shaft, spray and shadows use no mip levels, linear magnification and
nearest minification. Authored glass debris (`XETT_GLASSB`) uses the source
color, an alpha test and depth writes.

The port-side `RaceEffectsFrame` now transports the two original shadow
strip producers from `care.cpp`. The state-only path needed a consumer
because Godot never invokes the full Ecstasy car draw. The donor terrain,
hull, light and visibility calculations supply the strips, while Godot
owns the batched meshes per view and texture. Shadow draws test scene
depth and avoid writing it. Godot uses the authored alpha priorities for
particle materials, glass, shadows, skid marks and light materials.
Particle shaders use raw texture alpha through the authored modulation
and test, removing the extra soft-depth texture sample for AcidFX.
Cutout debris with authoring state 0A and Z writes uses an opaque draw
with a fragment discard. The scene compositor still applies depth fog
after world transparency.

The particle nodes still batch by definition. Equal-priority cross-group
sorting, transparent GS destination math on all surfaces, and the exact
projected GS shadow depth bias remain approximated by Forward+.
The screen-sun composite retains its early alpha pass over the opaque
scene copy. Exact PS2 pixel parity needs matched original captures.

### Validation

Arc B390, Godot 4.7.2, 960×720 pixel probes. Values are changed samples.

| Effect | Vulkan | D3D12 |
| --- | ---: | ---: |
| Scenic Drive vehicle shadows, five batches | 3,746 darker | 3,746 darker |
| Forest light shafts | 1,390 | 1,390 |
| Fire truck spray, all four emitters | 16,121 | 16,156 |
| Forest fire and smoke | 57,140 | 60,853 |

Track 33 passed drive, pause, restart, menu and replay epoch 1 with the
rear/reflection world checks on both drivers. Track 12 passed the authored
weather, compositor depth and GS history probes on both drivers.
Release staging and Debug
linking passed. A running editor instance held the Debug stage DLL, so
Debug effect captures used a separate staged copy. Logs are in
`local/task0146/alpha-smoke`.

Scenic Drive Track 13, Starting Grid background flyby, 1080p Release,
Vulkan, fixed seed and camera. These are individual samples, compared
with an isolated copy of the preceding build.

| Measurement | Frame P50/P95/P99, ms | Frames over 8.33 ms |
| --- | --- | ---: |
| Before, no collector or VSync | 5.428 / 7.132 / 7.982 | 13 / 2113 |
| After, no collector or VSync | 5.033 / 6.693 / 7.408 | 14 / 2252 |
| After, Visual Profiler, no VSync | 5.372 / 6.668 / 7.512 | 8 / 2143 |
| After, 120 Hz VSync, no collector | 8.326 / 8.634 / 9.967 | 635 / 1332 |

In the profiler trace, render CPU P50/P95/P99 was 1.893/2.869/3.233 ms
and GPU was 3.005/4.437/5.104 ms. The 120 Hz VSync sample averaged
119.7 FPS. Frame pacing still exceeds 8.33 ms at P95.

```powershell
$iso = 'D:/ps2_game/Need for Speed - Hot Pursuit 2 (USA).iso'
py -3 tools/profile_mvp_reflections.py --assets $iso --out local/task0146/alpha-final --grid --pairs 0 --quiet
py -3 tools/profile_mvp_reflections.py --assets $iso --out local/task0146/alpha-visual --grid --pairs 0
py -3 tools/profile_mvp_reflections.py --assets $iso --out local/task0146/alpha-final-vsync --grid --pairs 0 --quiet --vsync
$env:HP2_PS2_ASSET_ROOT = $iso
$env:HP2_PROFILE_DIR = "$PWD/local/task0146/shadow-repro"
$engine = "$PWD/local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe"
& $engine --path build/godot-mvp-release/mvp-project --rendering-driver vulkan --resolution 960x720 --script "$PWD/tools/test_mvp_car_shadows.gd" -- --race-track=13
$env:HP2_PROFILE_DIR = "$PWD/local/task0146/firetruck-repro"
$env:HP2_MVP_TEST_TRACK_EFFECTS = 'firetruck'
& $engine --path build/godot-mvp-debug/mvp-project --rendering-driver d3d12 --resolution 960x720 --script "$PWD/tools/test_mvp_particle_routes.gd" -- --race-track=12
```

## Wheel blur follow-up

The wheel blur follow-up uses Retail SLUS_203.62 E-0035-W01/W02 and
E-0038-W01. The committed bridge reads the existing mount matrix and signed
angular velocity, applies the original wheel and side visibility tests, and
publishes each view's overlay transform and alpha. Godot consumes authored
model slots 10/11 through the existing asset accessor. Missing optional blur
solids omit the overlay. No donor code changes are needed for this follow-up.
The windowed smoke verifies the non-spinning pose, the 90 alpha cap, a
rendered pixel difference around the rim, and fade under braking. See
`local/task0146/wheel-final-vulkan.log` and `wheel-final-d3d12.log` for the
individual runs. Both Track 33 runs passed on Godot 4.7.2 Forward+ with four
authored overlays, a peak of 90, and 4,366 and 5,396 changed samples on
Vulkan and D3D12 respectively. Debug and Release staging passed. PS2 runtime
comparison remains open.

## Authored scenery section trees

Ghidra MCP checked US retail SLUS_203.62 (SHA-256
`7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`).
`WhatSectionsShouldWeDraw` 00150950 uses the camera XY zone, global sections
90–99 and authored sections including proxies. `TreeCull` 00150378 tests the
24-byte 34104 tree records and `DrawAScenery` 0014FE18 tests the leaf instances.
The Debug ELF corroborates the section selector at 001505A0 and recursive
culling at 00150178 (SHA-256
`815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`).

The port-side `ScenerySection::cullNodes` exposes the decoder's existing
immutable nodes because the old interface published only packed records.
Godot copies the topology by source section and instance index at scene
creation. It merges authored bounds with all submitted LOD and animation
bounds and tests the actual primary camera planes before filling the existing
batches. Incomplete trees fall back to the flat selection in their section.
Primary transparent placements use the source section membership on their
main visibility layer, with the rear and reflection layers independently
available. The shared Debug Effect and Track Viewer Render control
`Track visibility zones` starts enabled. The viewer performs section
selection with its free camera. Both views restore all available sections
when disabled or outside the authored zones.

Three 1080p Release Vulkan A/B pairs on Arc B390 use the fixed Track 13
Starting Grid flyby. The baseline includes the existing authored zone and
LOD selection. All six runs traversed 363.63 authored camera units.

| Pair | Baseline frame P50/P95/P99 ms | Tree frame P50/P95/P99 ms | Draw calls P50 | Triangles P50 |
| --- | --- | --- | --- | --- |
| 1 | 5.513 / 7.772 / 9.098 | 5.042 / 6.630 / 7.499 | 1426 → 1068 | 161491 → 121375 |
| 2 | 6.677 / 9.614 / 11.074 | 5.346 / 7.547 / 9.003 | 1446 → 1071 | 161513 → 121583 |
| 3 | 5.625 / 7.883 / 8.918 | 6.295 / 10.310 / 11.701 | 1448 → 1062 | 161424 → 121390 |

The median P95 was 7.883 → 7.547 ms. The typical active section count
was 15, with 179 tree nodes visited and 329 opaque candidates versus 998.
The median frame needed zero batch updates. The staged Visual Profiler
pair measured render CPU P95 4.102 → 2.876 ms and render GPU P95
6.642 → 4.391 ms. Earlier captures showed variable GPU tail timings. The
staged 120 Hz VSync sample averaged 119.9 FPS with frame intervals of
8.329 / 8.847 / 9.754 ms at P50/P95/P99.

Vulkan and D3D12 fixed-camera probes checked forward and reverse headings
from the same position, multiple zone boundaries, full 130-section fallback
and Forest shaft and smoke pixels. The Track Viewer UI restored 7 → 122 → 7
sections on both drivers and returned to the main menu. Track 33 forward
and reverse driving, restart, return to menu and replay passed on both
drivers. The reverse replay used the timeline mouse wheel and advanced
the restore epoch to 2 with visible scenery and a live reflection atlas.

```powershell
$disc = 'D:/ps2_game/Need for Speed - Hot Pursuit 2 (USA).iso'
py -3 tools/profile_mvp_reflections.py --assets $disc --out local/task0146/section-tree-staged-ab --grid --section-tree --pairs 3 --quiet
py -3 tools/profile_mvp_reflections.py --assets $disc --out local/task0146/section-tree-staged-visual --grid --section-tree --pairs 1
py -3 tools/profile_mvp_reflections.py --assets $disc --out local/task0146/section-tree-staged-sync --grid --section-tree --pairs 0 --quiet --vsync
```

Smoke logs are under `local/task0146/section-tree-smoke`. For the fixed
camera checks run `tools/test_mvp_scenery_visibility.gd` with
`-- --race-track=13` and `HP2_MVP_TEST_SCENERY_CAMERA=1`. For the Viewer
run `tools/test_mvp_track_viewer.gd` in Debug with
`HP2_MVP_VIEWER_VISIBILITY=1`, `-- --race-track=12` and 1200×800 at 192 DPI.
Each diagnostic uses an isolated `HP2_PROFILE_DIR`.
For Track 33 reverse use `HP2_MVP_TEST_TRACK_REVERSE=1` and run
`tools/test_mvp_race_lifecycle.gd` with `-- --race-track=33`. For the
timeline check also set `HP2_MVP_CHECK_REPLAY=1` and
`HP2_MVP_CHECK_REPLAY_SEEK=1`.

## Scenic Drive Starting Grid flyby

The benchmark starts World Racing Quick Race on Track 13 through the
existing frontend startup callback, using a fresh profile and fixed seed.
The profile's `track_selection=13` uses the original track selection path.
It samples 720 source real-clock ticks of `RC_STATE_CAMERA_FLYBY` while
Starting Grid is visible, before confirming `Pre Race Stats`. The script
subsequently checks the transition through the countdown to GO.

The main camera reuses the reflection scenery batch logic with its own
authored zone lookup and projected-size LOD. The mirror draws the existing
static placement meshes. Models with sorted or blended surfaces stay on
their existing draw path. If there is no authored zone, Godot uses the full
primary scenery coverage. This is a presentation change with no additional
game core interface. The relevant donor zone and LOD evidence is recorded in
the reflection section below.

1080p Release on Arc B390, Vulkan Forward+, with VSync and Visual Profiler
off. All six runs traversed the same 363.63 units of the authored camera
path. Timing covers the rendered flyby and excludes the scene load.

| Pair | Scenery | Frame P50/P95/P99 (ms) | Above 8.33 ms | Draws P50 | Triangles P50 |
| --- | --- | --- | ---: | ---: | ---: |
| 1 | Legacy | 13.29 / 16.09 / 19.08 | 756/895 | 4,975 | 329,266 |
| 1 | Zone and LOD | 5.69 / 8.16 / 9.58 | 78/1,961 | 1,433 | 161,330 |
| 2 | Legacy | 13.80 / 17.99 / 21.83 | 706/876 | 4,977 | 329,433 |
| 2 | Zone and LOD | 5.62 / 7.93 / 8.80 | 56/1,979 | 1,444 | 161,607 |
| 3 | Legacy | 13.68 / 17.99 / 23.61 | 678/892 | 4,969 | 328,815 |
| 3 | Zone and LOD | 6.05 / 9.96 / 12.88 | 239/1,828 | 1,426 | 161,483 |

Two of the three LOD runs met the 8.33 ms P95 gate. Draw calls fell about
71% and submitted triangles about 51%. The third pair still missed the
frame budget.

The separate Visual Profiler pair measured render CPU P95 at 9.65 → 3.06 ms
and render GPU P95 at 9.45 → 4.46 ms. For the longest CPU scope per frame of
each named stage, depth pre-pass CPU/GPU P95 changed from 3.47/3.32 to
0.86/0.87 ms, and opaque pass from 3.36/2.48 to 0.66/0.58 ms. The capture
includes the auxiliary viewports, which repeat these stage names.
With 120 Hz VSync and the profiler off, the LOD run averaged 118.8 FPS.
Frame intervals at P50/P95/P99 were 8.34/9.01/10.91 ms.

```powershell
cmake --build --preset godot-mvp-release --target stage_mvp --parallel 8
cmake --build --preset godot-mvp-debug --target stage_mvp --parallel 8
$disc = 'D:/ps2_game/Need for Speed - Hot Pursuit 2 (USA).iso'
py -3 tools/profile_mvp_reflections.py --assets $disc --out local/task0146/grid-accept-ab --grid --pairs 3 --quiet
py -3 tools/profile_mvp_reflections.py --assets $disc --out local/task0146/grid-accept-visual --grid --pairs 1
py -3 tools/profile_mvp_reflections.py --assets $disc --out local/task0146/grid-accept-sync --grid --pairs 0 --quiet --vsync
```

Track 33 Release passed the Vulkan and D3D12 mirror world, drive, pause,
restart, retirement, replay atlas and post-pixel checks. The Track 13 flyby
smoke captured six points while Starting Grid was visible and reached GO
on both backends. The captures confirm the route, forest and road in the
sampled views. Matched PS2 image comparison remains open. See
`local/task0146/grid-regression` for logs and captures. The commands use the
Track 33 smoke procedure below and `--grid` for Track 13.

## Scenic Drive reflection performance

The six reflection viewports share a dedicated Godot `World3D`. The port
copies authored zone polygons and adjacency lists at scene publication. It
uses the source point-in-polygon function, PAL150950..150B0C zone order,
PAL14FFC4..150030 LOD thresholds and PAL150E04 projected size. It keeps the
PAL113030..11304C 32-unit cull range and derives the face FOV from the
64-wide target and source 512-wide focal basis. Godot batches opaque and
cutout placements by section and LOD, culls each camera's view, and faces
facades in the vertex shader. The scene shares Godot meshes, materials and
sky textures. The existing donor schedule refreshes due faces and leaves
the other face textures resident. Replay restore refreshes the full atlas.
These changes are port-side. The PAL source ELF identity remains unresolved.

Godot 4.7.2, Windows, Intel Arc B390 driver 32.0.101.9150, Vulkan Forward+.
The isolated diagnostic sets the demo race seed and calls the original
`RequestDemoMode()` when the frontend banks are ready. The comparison holds
the heavy Track 13 camera at step 282 and reissues the same 60 Hz face
sequence at two presentation frames per phase. Each run has 360 measured
frames, including 348 matched Visual Profiler frames. The original shared
world is available only in the explicit profiling run.

| Run | Whole frame ms P50/P95/P99 | Render CPU ms P50/P95/P99 | Render GPU ms P50/P95/P99 | Reflection draws P50 on refresh frames |
| --- | --- | --- | --- | ---: |
| 1 shared | 11.87 / 14.90 / 16.40 | 6.53 / 8.86 / 9.92 | 7.31 / 8.07 / 8.23 | 3,530 |
| 1 dedicated | 8.26 / 11.19 / 12.40 | 4.16 / 6.58 / 7.26 | 7.23 / 7.58 / 7.66 | 348 |
| 2 shared | 12.07 / 15.49 / 17.46 | 6.61 / 9.14 / 10.07 | 7.58 / 10.01 / 11.50 | 3,530 |
| 2 dedicated | 9.58 / 13.47 / 14.67 | 5.18 / 7.89 / 8.52 | 7.02 / 8.49 / 9.66 | 348 |
| 3 shared | 12.30 / 16.75 / 19.31 | 6.89 / 10.14 / 12.11 | 5.45 / 6.67 / 6.86 | 3,530 |
| 3 dedicated | 8.18 / 10.18 / 11.43 | 4.13 / 5.65 / 6.85 | 6.20 / 7.54 / 7.64 | 348 |

The mean face count was 1.5 per frame in all six captures. Reflection draw
calls fell 90%. The dedicated world missed the 8.33 ms whole-frame P95
target. In the first dedicated sample the main viewport alone used 5.51 ms
render CPU and 5.81 ms GPU at P95. The no-profiler fixed view measured
8.67 / 11.38 / 13.49 ms P50/P95/P99 at 1920×1080 and 8.56 / 10.20 / 11.11 ms
at 2880×1800 with 120 Hz VSync. Other desktop activity can account for some
of the run-to-run variation. A repeat on the final staged Release build
without the profiler measured 7.11 / 8.74 / 9.99 ms at 1920×1080. The
main-view cost remains open.

The full Track 13 Vulkan demo returned naturally to Title and retired its
reflection world. The smoke observed 165 atlas content changes and 73 camera
movements while the demo's radial blur remained active. Track 33 Release
passed driving, mirror world binding, pause/resume, restart,
retirement, and replay epoch 1 with a populated atlas on both Vulkan and D3D12.
The post smoke counted 815 covered atlas samples on both backends. Resolved
depth, radial history, resize, and history reset also passed.

```powershell
$disc = 'D:/ps2_game/Need for Speed - Hot Pursuit 2 (USA).iso'
py -3 tools/profile_mvp_reflections.py --assets $disc --out local/task0146/reflection-final-ab --pairs 3
py -3 tools/profile_mvp_reflections.py --assets $disc --out local/task0146/reflection-final-quiet --pairs 0 --quiet
py -3 tools/profile_mvp_reflections.py --assets $disc --out local/task0146/reflection-final-native --pairs 0 --quiet --fullscreen --resolution 2880x1800 --vsync
py -3 tools/profile_mvp_reflections.py --assets $disc --out local/task0146/reflection-final-demo --demo
```

For the Track 33 lifecycle commands below, set
`HP2_MVP_CHECK_REFLECTION_LIFECYCLE=1` and repeat with both drivers, first
with `HP2_MVP_CHECK_REPLAY=0` and then with `HP2_MVP_CHECK_REPLAY=1`.
For post pixels, run `test_mvp_race_post.gd` with `--race-track=33` and
`HP2_MVP_TEST_POST=1`. Use a fresh isolated profile per run. Logs are under
`local/task0146/reflection-final-regression`.

## Commands

```powershell
cmake --build --preset godot-mvp-debug --target stage_mvp --parallel 7
cmake --build --preset godot-mvp-release --target stage_mvp --parallel 7
$engine = "$PWD/local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe"
foreach ($driver in @('vulkan', 'd3d12')) {
    $env:HP2_PROFILE_DIR = "$PWD/local/task0146/wheel-final-$driver-profile"
    & $engine --path build/godot-mvp-release/mvp-project --rendering-driver $driver --resolution 960x720 --script "$PWD/tools/test_mvp_wheel_blur.gd" -- --race-track=33
}
$env:HP2_MVP_TEST_POST = '1'
$env:HP2_PROFILE_DIR = "$PWD/local/task0146/post-vulkan-profile"
& $engine --path build/godot-mvp-debug/mvp-project --rendering-driver vulkan --resolution 960x720 --script "$PWD/tools/test_mvp_race_post.gd" -- --race-track=12
$env:HP2_PROFILE_DIR = "$PWD/local/task0146/post-d3d12-profile"
& $engine --path build/godot-mvp-debug/mvp-project --rendering-driver d3d12 --resolution 960x720 --script "$PWD/tools/test_mvp_race_post.gd" -- --race-track=12
$env:HP2_PROFILE_DIR = "$PWD/local/task0146/lifecycle-33-profile"
& $engine --path build/godot-mvp-release/mvp-project --rendering-driver d3d12 --resolution 960x720 --script "$PWD/tools/test_mvp_race_lifecycle.gd" -- --race-track=33
$env:HP2_PROFILE_DIR = "$PWD/local/task0146/lifecycle-13-profile"
& $engine --path build/godot-mvp-release/mvp-project --rendering-driver vulkan --resolution 960x720 --script "$PWD/tools/test_mvp_race_lifecycle.gd" -- --race-track=13
$env:HP2_PROFILE_DIR = "$PWD/local/task0146/lifecycle33-vk-final"
$env:HP2_MVP_CHECK_REPLAY = '1'
& $engine --path build/godot-mvp-release/mvp-project --rendering-driver vulkan --resolution 960x720 --script "$PWD/tools/test_mvp_race_lifecycle.gd" -- --race-track=33
$env:HP2_MVP_POLICE_HUD_ONLY = '1'
$env:HP2_PROFILE_DIR = "$PWD/local/task0146/cop-hud-profile"
& $engine --path build/godot-mvp-release/mvp-project --rendering-driver vulkan --resolution 800x600 --script "$PWD/tools/test_mvp_be_the_cop.gd" -- --race-event=ultimate-racer:5
$env:HP2_MVP_TEST_POST_PROFILE = '1'
$env:HP2_PROFILE_DIR = "$PWD/local/task0146/profile-run"
& $engine --path "$PWD/local/task0146/debug-project" --rendering-driver vulkan --resolution 960x720 --gpu-profile --script "$PWD/tools/test_mvp_race_post.gd" -- --race-track=12
py -3 reverse/scripts/generate_progress.py
git diff --check
```
