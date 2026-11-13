# TASK-0120 - MVP presentation ownership

- Status: active
- Scope: port-side refactor of the existing GDExtension and its presentation resources.
- Dependencies: TASK-0115, TASK-0116, TASK-0118 and TASK-0119. Original behavior and ELF provenance remain with those records.

| Responsibility removed from the binding | Reused source | Destination and invariant |
| --- | --- | --- |
| Mesh conversion and strip batching | Decoded solids and original texture headers, vehicle model light selection | Shared mesh converter. Preserve winding, material order and cache keys. |
| Car resources, poses, flares and shadows | Model pose and texture adapters, native flare markers/state, `CarRenderInfo::Render` | Vehicle presentation. Preserve handles, interpolation and projected shadow geometry. |
| Track load and section selection | `load_scene`, `GrandSceneryCullInfo::WhatSectionsShouldWeDraw` | Track presentation. Keep source section selection and Godot culling. |
| Scenery overrides and detached objects | `track::Runtime::set_world_objects` | Scenery presentation. Keep staging, visibility and generation cleanup. |
| Particle display | World AcidFX, `ParticleSimulation::import_committed_world` | Particle presentation. Keep the pool and source birth IDs. |
| Atmosphere, post, sun and reflection | `QueryLightDatabase`, weather query, focus/fog palettes, `StuffSun`, environment cameras and warps | Environment presentation and existing reflection adapter. Keep pass order, capture cadence and history invalidation. |
| Camera view masks and visibility inputs | Committed World camera samples and existing Godot camera adapter | Environment and track presentation. Keep race camera script, projection and both clock paths. |
| Selection and debug preview | Selection runtime and original asset catalog | Preview presentation. Keep frontend sequencing and GDExtension entry points. |
| Stage/publish and HUD | `EntityStateBridge`, committed World and race-rule view | Remain with the coordinator. Keep stage validation, commit identity and order. |

## Source audit

`src/game_core/indep` owns original World stepping, scenery state and
collision. `src/game_core/src` includes car shadow projection in `care.cpp`,
sun quad construction in `sune.cpp`, and lighting and palette calculations
in `ecstasy`. `src/game_core/port/track` supplies decoded geometry, scenery
operands, camera-dependent selection inputs and weather and reflection
helpers. `src/port/vehicle/model.cpp` supplies part selection and wheel and
wiper poses. The host retains its existing effect mappings and capture
adapter. Particle presentation retains source emission, motion and lifetime.

Consolidated basis/coordinate conversion, node construction, the recovered
section-selection call site for all views, and the driving-environment load
for race and track preview. Debug car preview can create visuals without
a loaded track. Vehicle staging binds race topology when needed. Replacing
the resident track clears preview nodes attached to the old coordinator state.

## Acceptance

- [x] Build both extensions and pass focused native checks.
- [x] Match the Track 12 tape and validate debug asset preview.
- [x] Record paired performance, resource counts and remaining gaps.
- [x] Verify the live lifecycle on the final DLL.
- [ ] Isolate the intermittent pause-freeze assertion and replay pause revision timeout.
- [ ] Complete a new natural finish and isolate collision, particle and postprocess visual checks.

## File inventory

New under `src/mvp/godot/` are `race_mesh.{hpp,cpp}`, `race_visual_nodes.hpp`,
`track_presentation.{hpp,cpp}`, `scenery_presentation.{hpp,cpp}`,
`vehicle_presentation.{hpp,cpp}`, `particle_presentation.{hpp,cpp}`,
`environment_presentation.{hpp,cpp}` and `preview_presentation.{hpp,cpp}`.
Modified files are `ps2_native_race_presentation.cpp`, `CMakeLists.txt`,
`.gitignore`, `AGENT.md`, `docs/ARCHITECTURE.md`, `docs/DECISIONS.md`,
`docs/PROGRESS_LOG.md` and generated `docs/PROGRESS.md`. Added
`tools/test_mvp_presentation_previews.gd` and this task record. The user's
follow-up modified `godot_mvp/presentation/debug_telemetry.gd` to show only
coordinates and FPS over the scene, with the original HP2 glyph atlas. No files
removed. This task made no changes to game core, World scheduling or shaders.

## Validation

Windows, CMake 3.29.2, Python 3.14.4, Godot 4.7.2 (ed1daf0bf),
Release DLLs, Intel Arc B390,
Vulkan 1.4.359 Forward+. `--debug` enables the in-game overlay.
The two Release builds, the MVP Debug build, seven focused CTest checks,
the headless HUD fixture and
the headless asset preview regression passed. The legacy window passed
Championship startup, physical input and return. The native Track 12
World drove three laps and published natural Results at step 20421.
The authored result-menu walk reached its final action menu, with five
rows and 4,000 player points. The normal-speed Godot window driver ended
at the 240-second budget. Its last periodic sample was step 7197 and
70.86% of the first lap. No windowed settlement or save was reached.

The matched tape compared committed World states for Track 12 TS50 skin 2
across steps 184-379, tape entries 662-1261. Each DLL captured 600 frames
after 120 warm-up frames at 1920x1080, uncapped, VSync off, WASAPI, DOF
and glow off, fidelity profile. The rear view, contact shadows,
reflection and source fog were enabled. Baseline DLL SHA-256 begins
`59867a185157`. Final DLL SHA-256 begins `e7fe4165bdd5`.

| Scope | Baseline control | Final |
| --- | ---: | ---: |
| Average FPS | 156.21 | 152.80 |
| Frame mean / P95 / P99, ms | 6.402 / 10.957 / 12.384 | 6.544 / 11.168 / 11.811 |
| World native advance P95, ms | 1.640 | 1.670 |
| Entity stage / publish P95, ms | 0.149 / 0.467 | 0.180 / 0.468 |
| Section and view update P95, ms | 0.113 | 0.117 |
| Godot viewport CPU P95, ms | 3.196 | 3.769 |
| Root viewport GPU P95, ms | 3.635 | 3.609 |
| Mean draw calls | 2354.86 | 2355.60 |
| Working set / private memory after export, MiB | 1660.45 / 1615.39 | 1662.85 / 1617.73 |
| Godot video memory, MiB | 361.63 | 361.63 |
| Native creation delta, meshes / nodes | 0 / 99 | 0 / 99 |

CPU and delayed GPU viewport timings are independent scopes. The final
build exceeded the 8.33 ms P95 budget. Earlier runs of the same baseline
ranged from 138.32 to 154.45 FPS and 11.021 to 12.014 ms P95. The direct
pair cannot establish a performance trend.

The first matched refactor capture differed from an earlier baseline
at 158 pixels, all inside the FPS readout. The final capture differs
from the immediate baseline at 8,532 pixels out of 2,073,600, including
telemetry and sparse scene pixels. Earlier baseline captures also vary.
Original hardware visual equivalence remains open. The optional
replay-then-pause run timed out on both DLLs at the paused UI revision check.
A separate live intermediate build passed pause, retry and return. The
first live attempt on the final build hit the harness pause-freeze assertion.
A repeat on that same DLL passed all 41 checks, including independent pause
UI revision, freeze, retry cancellation and confirmation, and return.
No PCSX2 was launched.

## Reproduction

```powershell
cmake --build --preset godot-mvp-release --target ps2_port --parallel 8
cmake --build --preset godot-mvp-debug --target ps2_port --parallel 8
cmake --build --preset godot-release --target ps2_port entity_state_bridge_tests track_runtime_tests track_environment_tests track_postprocess_tests event_world_tests --parallel 6
ctest --test-dir build/godot-release -C Release --output-on-failure -R '^(entity_state_bridge|track_runtime|track_runtime_assets|track_environment|track_postprocess|event_world_original|progress_tables)$'
& build/godot-release/event_world_tests.exe --drive-road --tree 0 --frames 25000 --source-menu-walk
& local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe --headless --path build/godot-mvp-release/project --script (Resolve-Path tools/test_mvp_presentation_previews.gd)
& local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe --headless --path build/godot-mvp-release/project --script (Resolve-Path tools/test_mvp_hud_interpolation.gd)
py -3 -B tools/test_event_window.py --debug --entities-project build/godot-mvp-release/project --championship --launch-only --presentation-smoke --result-timeout 70 --output local/task0120/final-live
py -3 -B tools/test_event_window.py --debug --entities-project build/godot-mvp-release/project --championship --launch-only --presentation-smoke --result-timeout 70 --output local/task0120/final-live-repeat
py -3 -B tools/test_event_window.py --debug --race-dll local/task0115/bin/ps2_port.dll --championship --launch-only --result-timeout 70 --output local/task0120/legacy-smoke
py -3 -B tools/test_event_window.py --debug --entities-project build/godot-mvp-release/project --championship-performance --performance-uncapped --performance-engine-breakdown --performance-final-screenshot --race-tape-replay local/task0119/followup/matched.tape --result-timeout 70 --output local/task0120/final-match
py -3 -B tools/test_event_window.py --debug --entities-project local/task0120/baseline-project --championship-performance --performance-uncapped --performance-engine-breakdown --performance-final-screenshot --performance-lifecycle --race-tape-replay local/task0119/followup/matched.tape --result-timeout 70 --output local/task0120/baseline-control
py -3 -B tools/test_event_window.py --debug --entities-project build/godot-mvp-release/project --championship --drive-route --road-course-driver --result-timeout 240 --output local/task0120/final-finish
py -3 -B tools/test_event_window.py --debug --entities-project build/godot-mvp-release/project --championship --race-post on --launch-only --presentation-smoke --result-timeout 70 --output local/task0120/transparent-overlay
py -3 -B reverse/scripts/generate_progress.py
py -3 -B reverse/scripts/generate_progress.py --check
```

The matched benchmarks were captured before the debug overlay text and
background edit. The staged overlay passed the local Godot headless
format probe for available and unavailable coordinates. The windowed
Track 12 HUD capture in `local/task0120/transparent-overlay/original-hud.png`
confirms the original glyph atlas over the scene with no panel fill.

The baseline project is a copy of the staged independent project with the
DLL saved before the refactor. Use fresh output directories for repeats.
If the tape is unavailable, run `--race-tape-record` on the baseline first.
Continue with the windowed Championship finish using a fresh output
directory and a wall-time budget covering all three laps (at least 1200
seconds for the measured external driver pace), then pair effects captures.
