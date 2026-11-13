# Track 12 native renderer

The opt-in renderer runs Class 5 Parkland Race, the first authored Championship
event, with one player, four AI opponents and the original three-lap rules.
The production frontend still selects the event, car, skin, transmission and
handling. `Ps2DrivingRuntime` opens the same Debug `WorldSession` in either
render mode. Its source World owns input, AI, movement, collision, checkpoints,
timing, race results and audio.

## Build and play

Use the existing Windows development prerequisites, pinned Godot 4.7.2,
configured original ZZDATA and configured Debug ELF. From the repository root:

```powershell
./build.ps1 --run --mvp --preset godot-release
```

For a direct engine launch:

```powershell
$portBin = Join-Path $PWD 'godot/bin'
cmake --preset godot-release "-DPORT_GODOT_BIN_DIR=$portBin"
cmake --build --preset godot-release --target ps2_port --parallel 8
$godot = Join-Path $PWD 'local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe'
& $godot --headless --path godot --import
& $godot --path godot res://scenes/frontend_native.tscn -- --race-renderer=native
```

Select **Championship**, **View Event Tree**, then the first race. Use W/S for
throttle and brake or reverse, A/D for steering, Space for handbrake, and Q/E
for manual shifts. Use the authored pause menu to retry or quit.

The title and event selection menus use the original frontend in both modes.
When the qualifying race opens, the terminal prints
`HP2_RACE_RENDERER native (Entity State Bridge)`. The race HUD shows
`Position`, `Lap`, and speed in km/h. If the MVP was requested and the
selected race uses the legacy renderer, the terminal reports its track and
event IDs.

The default renderer is `legacy`. The native branch activates only for tree
0, authored event number 1, TrackID 12. Append `--mvp-dof` or `--mvp-glow` for
independent Forward+ effects. `--race-post=on` enables both and
`--race-post=off` disables both. For the legacy renderer, an explicit
`--race-post` also requires `--track-performance-analysis` in Release. It
controls the legacy DOF, screen filter, and radial blur options.

## State and ownership

`EntityStateBridge` takes a typed native `WorldFrame`, validates vehicle and
camera operands, and stages a reusable batch. Only the matching committed
generation, source step, and UI revision can publish it. Monotonic handles
separate a respawned or replaced actor from a prior actor at the same address.
The render adapter owns node mappings and reuses immutable meshes and textures
across frames and same-event retries. Native wheel and available part poses are
applied to those nodes. Godot holds the last committed visual pose between
source steps.

The track loader builds persistent meshes from the decoded original scene.
The existing section selector supplies front and rear view section membership.
Godot culls individual meshes and draws them with its own depth, lighting,
shadows and tone mapping. Live HUD values come from the committed native
`RaceHudView` and player speed, and Godot Labels and a TextureRect draw the
HUD and rear view. Original frontend, loading, pause, result and authored
cinematic presentation use their existing adapters.

The native material mapping uses the decoded base textures and a bounded
projection of alpha state. Scenery uses the first available authored model
level, with Godot frustum culling. Full original per-object LOD, billboard
facades, GS material and reflection passes, and the original sun effect are
outside the current visual implementation. The native HUD has the essential
race state and rear view. The legacy HUD also includes its minimap, tachometer,
and standings list. Native effects use committed particle positions and
authored base textures, with simplified Godot billboard presentation. The two
render paths have visible differences and different GPU workloads.

## Benchmark

Build an isolated Release DLL and run the original race harness against it:

```powershell
$benchBin = Join-Path $PWD 'local/task0115/bin'
cmake --preset godot-release "-DPORT_GODOT_BIN_DIR=$benchBin"
cmake --build build/godot-release --target ps2_port entity_state_bridge_tests --parallel 8
py -3 -B src/mvp/benchmark/run_matrix.py --dll local/task0115/bin/ps2_port.dll --output local/task0115/matrix
```

The matrix runs one process at a time and uses the same DLL, authored roster,
initial RNG, normal controls and World elapsed inputs. It records a tape on
the first run and checks source race state, camera and actor poses on each
replayed tick. A valid run has 120 warm-up frames and 600 measured frames at
1920×1080 on Vulkan Forward+, uncapped with VSync off. The script checks that
the same tape indices, source steps and host camera poses were measured. It
writes raw profiles, a screenshot, identity checks, and a summary per run.
Treat GPU measurements as delayed viewport timing and engine draw counters
as whole-frame counters. Process memory is an after-export snapshot.
Effects-on is a comparison of the two shipped effect profiles. The effects-off
matrix is the primary performance comparison. Both have the content and
material differences above.

The migration gate requires a natural playable finish, comparable scene
content and P95 at or below 16.67 ms in every native effects-off repeat.

## Verification

```powershell
ctest --test-dir build/godot-release -R '^(entity_state_bridge|driving_schedule|event_world_original|race_finish_original|frontend_finish_original|authored_championship_clock_original|track_runtime|mesh_prepare)$' --output-on-failure
py -3 -B tools/test_event_window.py --championship --race-renderer native --race-post on --race-dll local/task0115/bin/ps2_port.dll --presentation-smoke --drive-route --road-course-driver --output local/task0115/native-race --result-timeout 1500
```

Use a fresh output directory. The window harness sends ordinary physical
input to its own process and uses read-only source road observations. It
uses an isolated profile and exercises authored retry, settlement, and return.
See the task record for observed results.
