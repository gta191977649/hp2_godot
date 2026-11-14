# TASK-0146 - PS2 car paint and Godot shader resources

- Status: active
- Scope: vehicle paint, environment capture, external Godot shader resources and Forward+ race postprocessing.
- Acceptance: native-depth and GS-history pixel checks on Vulkan and D3D12, diagnostic race lifecycle, frontend preview, and matched PS2 paint comparisons.

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
