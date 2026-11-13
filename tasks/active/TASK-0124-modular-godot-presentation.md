# TASK-0124 - Modular Godot presentation

- Status: active
- Scope: run Track 12 with committed donor World state and Godot-owned
  presentation. Earlier entries describe the retired draw-command prototype.
- Dependencies: TASK-0120 presentation ownership and TASK-0123 donor integration.

## Acceptance

- [x] Map active and historical presentation responsibilities to source owners.
- [x] Separate Godot resource conversion, presentation, and entity lifetimes.
- [x] Build and validate the existing launch and race path.
- [x] Organize MVP files under presentation, bridge, systems and assets.
- [x] Publish the Track 12 race scene from a committed World and drive it in Windows Release Forward+.
- [x] Trace unsupported race entries and integration failures and exit with code 1.
- [ ] Connect live effects and auxiliary passes and validate the full race and restart lifecycle.

## Historical prototype responsibility map

The archived TASK-0120 `Ps2NativeRacePresentation` and
`native_race_materials.*` use the earlier Debug World and asset interfaces.
The active equivalent is `Ps2NativeGame::Host`. The active `game_core/indep`
holds gameplay and scene resource owners. `game_core/src` holds platform
adapters and source presentation producers. `game_core/port` currently holds
the untranslated-path exception type.

| Previous or current responsibility | Active native source | Godot destination and invariant |
| --- | --- | --- |
| Track decode and load, former `request_track` / `pump_load` | `ApplicationRuntime`, GameFlow track stages, ResourceLoader | Godot consumes the ready native draws. Keep load and teardown order. |
| Track visibility and static scenery | `SceneryResources::Render` uses `CollectSections`, also used by `WhatSectionsShouldWeDraw` | `SceneryPresentation` uploads the combined track/scenery draw stream. Keep the single native section traversal. |
| Dynamic scenery | `RenderWorldObjects` in `EcstasyDisplayFrame` | Same world mesh consumer. Keep native transforms and placement visibility. |
| Mesh and material conversion | `EcstasyDrawCommand`, native strip topology, `DecodeTextureMipRgba` | `mesh_converter` and `NativeRaceMaterials` own Godot meshes, textures and shader variants. Preserve the current draw policy. |
| Vehicle models, animation, flares, shadows | `RenderCars`, `carRenderScene`, wheel transforms and shadow producers in `care.cpp` | `VehiclePresentation` owns nodes keyed by source car lifetime. Keep computed parts, visibility and projected shadows. Opaque draws use per-car nodes. Ordered alpha/effects remain in the shared submission order. |
| Particle pooling and display | AcidFX owns simulation and lifetime. `RenderWorldParticles` emits projected quads. | `ParticlePresentation` converts those quads. Preserve their position, depth and source order. |
| Camera | Native movers and `ecstasyViewCalculateMatrices` | `CameraPresentation` owns the host camera and consumes each draw's source projection. |
| Environment, lighting, weather, fog, sun and focus | `EcstasyLitVertexColor` consumes captured light context from `elighte.cpp`. Weather and palettes come from `weatherman.cpp` and `eenveffectse.cpp`. Sun commands come from `sune.cpp`. | Shared mesh conversion consumes the active lighting inputs. Godot owns deferred composition. The active baseline has no Godot depth-readback or postprocess executor for these commands. |
| Environment reflections and mirror | `EnvironmentMap::CreatePrimitives`, reflection strips, target descriptors | Future Godot viewport/capture consumers can read these outputs. Auxiliary passes were outside the active renderer before this refactor. |
| Preview and asset catalog | Native frontend resources and `RenderFrontEndCars`; frontend model requests in `EcstasyDisplayFrame` | Existing frontend preview draw conversion. The archived standalone catalog/preview API is outside the active runtime. |
| UI, stage/publish, performance counters | Native Flailer, HUD producers, sealed `EcstasyFrame`, committed World | `RacePresentation` prepares resources, retires source frame borrows, then publishes visuals and snapshot. The host exposes frame and vehicle counts. |

No game-core files are changed by this task. Adding a second track-loading
or culling service would run the same source work twice. A split between
track and scenery needs explicit source identities beyond the current combined
draw stream. Environment and reflection modules need a real Godot pass consumer
and separate visual acceptance.

## Validation and continuation

Windows Release, CMake 3.29.2, Ninja 1.12.0, GCC 13.2.0, Python 3.14.4,
Godot 4.7.2 (ed1daf0bf), godot-cpp e4b7c25e721ce3435a029087e3917a30aa73f06b.
The window used Forward+ on Intel Arc B390 with Vulkan 1.4.359.

```powershell
./build.ps1 --jobs 8
# Set HP2_PS2_ASSET_ROOT to a complete local disc or resource archive.
$engine = 'local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe'
$env:HP2_PROFILE_DIR = Join-Path (Resolve-Path local).Path 'task0124-smoke'
$smoke = (Resolve-Path tools/test_native_mvp.gd).Path
& $engine --headless --path build/godot-mvp-release/mvp-project --script $smoke
& $engine --path build/godot-mvp-release/mvp-project --script $smoke
py -3 reverse/scripts/generate_progress.py
py -3 reverse/scripts/generate_progress.py --check
git -c core.safecrlf=false diff --check
```

Release built successfully. The final DLL passed headless and Forward+
quick-race runs at World step 63 on track 0. Both published 14 vehicles
and observed player movement through keyboard input. The first window
run exited without reaching the smoke assertions. The retry passed and
saved a local screenshot. No source-owned simulation changes were made.

The Forward+ capture shows missing portions of track and vehicle geometry.
The previous renderer's local capture also shows such gaps. The smoke
checks vehicle state, rendered node presence and player movement. It does
not establish pixel parity, race settlement or original equivalence.

Continue TASK-0124 by checking the main-target sky/track/vehicle depth and
material flags against the sealed source draws, then connect a Godot
secondary-target consumer to the donor reflection/remap and mirror draws.
Follow with a Godot composition pass using captured fog/focus palettes.
No related commit has been made.


## Directory and ownership follow-up

Moved the coordinator, bridge snapshot, assets and systems to the requested
MVP directories. The active Godot scene tree owns track, scenery and vehicle
visual nodes. Godot references own mesh, material and texture resources.
Particles and flares are batched Godot meshes. The native World retains its
simulation data and computes the particle lifecycle. Track and flare draw
origin markers supply provenance without adding a simulation traversal.

The environment modules copy focus/fog data and mirror/reflection dependencies
into the committed snapshot. Godot render passes for those inputs remain open,
along with the previously recorded road and vehicle gaps.

The layout change passed the Release build and the real Forward+ quick-race
smoke. The updated harness required a nonempty Godot track mesh, visible
vehicle nodes, and movement from keyboard input. Track 0 published 14
vehicles at World step 62. The run exited successfully. The local screenshot
is local/task0124-layout-race.png. Road and vehicle gaps remain visible.
No related commit has been made.

```powershell
cmake --build --preset godot-mvp-release --target stage_mvp --parallel 8
# Set HP2_PS2_ASSET_ROOT to the complete local disc and HP2_PROFILE_DIR to a test profile.
$engine = 'local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe'
& $engine --path build/godot-mvp-release/mvp-project --script (Resolve-Path tools/test_native_mvp.gd).Path
py -3 reverse/scripts/generate_progress.py --check
git -c core.safecrlf=false diff --check
```

## Godot launch eligibility correction

The project opens a status screen and the native start binding rejects
game launch. The earlier draw-command adapter is a prototype. Use the
TrackID 12 MVP state bridge as the integration reference. The next step is
to connect a single supported race to committed entity, track and camera
state and create its resources in Godot. Enable its explicit entry after
the Godot race passes windowed validation.

## Remove the draw-command prototype

The user selected the TrackID 12 MVP as the reference and requested removal
of native application startup and draw-command conversion from Godot. The
active Godot project stages a status scene. The static Entity State Bridge
reads committed World data directly. The earlier smoke results document the
retired prototype. Keep this task active until an explicit Track 12 entry
uses Godot-owned scene resources and passes a Forward+ window check.

### Launch trace validation

Windows Release built with CMake 3.29.2 and Ninja 1.12.0. Godot 4.7.2
headless opened the status scene with exit code 0. Explicit Track 12 and
Track 0 requests emitted HP2_MVP_ASSERT at race_entry, included a GDScript
backtrace, and exited with code 1. An invalid ID failed at launch with
the same exit code. The staged project contains no old extension or DLL.

```powershell
cmake --build --preset godot-mvp-release --target stage_mvp --parallel 8
$engine = 'local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe'
$project = 'build/godot-mvp-release/mvp-project'
& $engine --headless --path $project --quit-after 2
& $engine --headless --path $project -- --race-track=12
& $engine --headless --path $project -- --race-track=0
& $engine --headless --path $project -- --race-track=invalid
py -3 reverse/scripts/generate_progress.py --check
git -c core.safecrlf=false diff --check
```

Continue TASK-0124 at race_entry for Track 12. Wire its committed World
snapshot to a Godot-owned race scene and add stage-specific failure traces
at each new integration boundary. Enable the entry after a windowed run.

## Track 12 state-only race

The explicit entry now starts the donor Quick Race on Track 12 with the
donor's skip-start-sequence flag. Its native tick returns before draw capture
and frame submission. The original Flailer HUD renderer depended on a car
bearing written by the original draw path. The state-only path retains
player camera, HUD state and scoring updates and uses Godot to display speed.

Entity State Bridge validates and copies the primary camera and World-issued
vehicle handles, visibility, body and wheel matrices, and physical speed.
The Godot scene builds its resources from decoded track, sky and vehicle
assets. Asset pointers are consumed during scene construction. Per-frame
publication uses copied state. The scene frees removed handles and retires
the previous scene on a World generation change.

The first visual probe confirmed the original road triangles existed under
the player in Godot. The black surface came from alpha-blended overlays whose
descriptors did not request depth sorting. The material mapping now uses
the authored 0x44 alpha equation independently of sorting, tests the
authored GREATER 48 cutoff where applicable, and disables depth writes
when the descriptor disables them. The first car texture publication
also runs the donor lamp and damage replacement selection.

### Windows Release validation

CMake 3.29.2, Ninja 1.12.0, MinGW GCC 13.2.0, Godot 4.7.2
(ed1daf0bf), Intel Arc B390, Vulkan 1.4.359, Forward+. The smoke drove
the player from World step 6 to 96 with the normal W input. It measured
49.48 m of displacement and 79.3 km/h. The scene published 8,178 track
placements and 14 vehicles in World generation 1. The run exited with code
0. The screenshot at local/task0124-track12-forward-plus.png shows the
road, vehicle bodies, sky and speed HUD. Its log is local/task0124-drive.log.

The ordinary window launch also published the scene and exited 0 with
`--quit-after 320`. Idle headless launch exited 0. Track 0, a malformed
track ID, and a missing
asset source each exited 1, with stage race_entry, launch, and asset_source
respectively. Unexpected live RaceCoordinator states, missing inputs and
invalid publication data have checked boundaries in the extension.

```powershell
cmake --build --preset godot-mvp-release --target stage_mvp --parallel 12
$engine = (Resolve-Path local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe).Path
$project = 'build/godot-mvp-release/mvp-project'
# Set HP2_PS2_ASSET_ROOT to the complete local disc or extracted source.
$env:HP2_PROFILE_DIR = "$PWD/local/task0124-profile"
$env:HP2_MVP_SCREENSHOT = "$PWD/local/task0124-track12-forward-plus.png"
& $engine --path $project --script (Resolve-Path tools/test_track12_mvp.gd).Path -- --race-track=12
& $engine --path $project --quit-after 320 -- --race-track=12
& $engine --headless --path $project --quit-after 2
& $engine --headless --path $project -- --race-track=0
& $engine --headless --path $project -- --race-track=invalid
$env:HP2_PS2_ASSET_ROOT = "$PWD/local/task0124-missing.iso"
& $engine --headless --path $project -- --race-track=12
py -3 reverse/scripts/generate_progress.py --check
git -c core.safecrlf=false diff --check
```

TASK-0124 remains active for committed dynamic scenery and particle state,
original section visibility and full materials, mirror and reflection targets,
post-processing, and full race completion and restart verification. This
short PC drive does not establish equivalence to the PS2 release.
