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
- Draws requiring transparent GS framebuffer feedback still use the existing transparent material route. Explicit framebuffer alpha for those draws, independent context tests and write masks, distance-dependent scenery LOD in the face captures, and exact paint parity across damage and wheel variants remain open. Original PS2 equivalence requires matched original runtime captures.

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

## Commands

```powershell
cmake --build --preset godot-mvp-debug --target stage_mvp --parallel 7
cmake --build --preset godot-mvp-release --target stage_mvp --parallel 7
$engine = "$PWD/local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe"
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
