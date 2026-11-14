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

## 2026-11-15 GTA SA-inspired player orbit

The primary Godot camera follows raw relative mouse input while the human
player is driving. Mouse input changes world-space yaw and pitch at a gain
scaled by the current field of view. After 1.5 seconds of idle time, the
camera arcs back to the live HP2 view with a linear blend over 0.6 seconds.
Mouse movement during the return resumes orbit from the last safe pose.
The display camera selects the authored HP2 visibility zone during orbit.
Camera switching, looking back, pause and focus loss relinquish orbit.
The behind-car fallback applies when starting from an interior camera.

Reference checkout: `D:/dev/gta-reversed`, commit
`04c5b6643a9059e879f6015ec02e618f141fb27a`. Recovered PC functions inspected:
`CPad::UpdateMouse` (0x53F3C0), `CCamera::ConeCastCollisionResolve`
(0x51A5D0), and `CCollision::CameraConeCastVsWorldCollision` (0x41B000).
`CCam::Process_FollowCar_SA` (0x5245B0) has no recovered body in this
checkout. The [independent SA camera adaptation](https://github.com/erorcun/SACarCam/blob/master/SACarCam/SACarCam.cpp#L662-L692)
provided the world-angle mouse and FOV reference. The available 2014 Steam
PC executable, SHA-256
`15e3cfedba9a841df67d8194e7249afb493b0e10d6138fb8ebab2c136e543efb`,
uses a different address layout. Original-game equivalence remains unverified.

The HP2 adaptation uses the existing two-sided terrain segment query on the
center and four sides of the arm, plus ground clearance at the result.
A blocked arm retracts immediately and extends gradually when clear.
Terrain topology is borrowed during the current presentation frame only.
No changes to `game_core` were needed. Individual moving obstacles and
scenery outside the authored terrain collision are outside this query.

```powershell
cmake --build --preset godot-mvp-release --target stage_mvp --parallel 12
cmake --build --preset godot-mvp-debug --target hp2_track12 --parallel 12
$engine = (Resolve-Path local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe).Path
$project = 'build/godot-mvp-release/mvp-project'
$script = (Resolve-Path tools/test_mvp_orbit_camera.gd).Path
foreach ($case in @(@('vulkan',13), @('d3d12',33), @('vulkan',12))) {
    $driver, $track = $case
    $env:HP2_PROFILE_DIR = Join-Path $env:TEMP ('hp2-orbit-' + [guid]::NewGuid())
    & $engine --path $project --rendering-driver $driver --resolution 1280x720 --script $script -- "--race-track=$track"
    if ($LASTEXITCODE) { throw "Orbit smoke failed on track $track ($driver)" }
}
```

Release staging and the Debug extension build passed. The windowed Release
smoke passed on tracks 13 and 12 with Vulkan and on track 33 with D3D12.
Each run exercised four orbit quadrants, terrain queries during orbit,
mouse interruption during the return, convergence to the current native
camera pose, cancellation with the original camera action and cursor
release on pause. `git -c core.safecrlf=false diff --check` passed. A moving
car along tunnel walls and a side-by-side comparison with GTA SA were
outside the automated coverage.

### Vertical orbit and vehicle center

GTA's controller defaults use non-inverted mouse Y and a 0.0025 mouse
sensitivity. The SA camera adaptation applies positive vertical input to the
forward vector elevation. HP2 stores the opposite elevation on the pivot-to-eye
arm. The Godot orbit now uses that direction, the car pitch response of 0.8,
and the reference car angular limits of about 45 degrees below and 89 degrees
above the target. The HP2 terrain query still limits the ground-level view.
The pivot and look target both use the center of the uploaded main body
mesh bounds transformed by the committed vehicle pose, including pitch and roll.
The existing smoke now checks both vertical directions and compares the
look direction against the rendered vehicle body center.
The Release smoke passed on tracks 13 (Vulkan) and 33 (D3D12) with the
commands above. Release and Debug staging passed.

### Low-angle camera distance

GTA SA's recovered `CCamera::ConeCastCollisionResolve` (0x51A5D0)
shortens the arm along the line from target to eye.
`CCollision::CameraConeCastVsWorldCollision` (0x41B000) tests the
collision volume at progressively shorter distances. The independent
[SA camera adaptation](https://github.com/erorcun/SACarCam/blob/master/SACarCam/SACarCam.cpp#L853-L918)
also retracts toward the target and reduces the near clip plane for nearby
surfaces. The original `CameraColDetAndReact` (0x520190) remains a wrapper
in the supplied GTA checkout.

HP2 keeps the requested pitch during ground contact. It solves for the
shortest necessary retraction with the HP2 terrain queries and uses the
body bounds to stay outside the car. At the body clearance limit, the eye
follows the road near the body along the requested bearing. The orbit
reduces its near plane for clearance and extends the arm when space opens.
The smoke now verifies a shorter low rear radius, an eye outside the body
bounds, aim at the body center, and radius recovery. It passed with the
commands above on Scenic Drive (Vulkan) and Track 33 (D3D12). Debug and
Release staging and `git -c core.safecrlf=false diff --check` passed.

### Smooth mouse takeover and animation sampling

Mouse orbit begins from the current HP2 view and blends position, look
direction and near clip over 240 ms of monotonic time. The transition
checks terrain and resumes from the displayed pose if the player moves
the mouse during the return. Active NIS playback owns the camera.

Race drivers interpolate between the two committed skinning palettes.
The native steering sampler still runs once per world step. The existing
`VisitNisPoses` boundary now accepts fractional world time for the
arrest actors. The core change reuses the decoded clips, inverse binds,
root offsets and hand prop attachments. It was needed because the old
boundary only provided the already rounded native pose. The world
clock, animation phase and playback expiry still use the donor's
PAL1B2C70–1B2C8C and PAL1B13F0–1B1510 paths. The exact PAL ELF version
and SHA-256 remain unresolved. Interpolation uses the authored cadence
of one clip frame per two world frames.

```powershell
$engine = (Resolve-Path local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe).Path
$env:HP2_MVP_TEST_ARREST_NIS = '1'
foreach ($cap in @(60, 120)) {
    $runDir = New-Item -ItemType Directory (Join-Path $env:TEMP ('hp2-arrest-' + [guid]::NewGuid()))
    $env:HP2_PROFILE_DIR = Join-Path $runDir.FullName 'profile'
    $env:HP2_MVP_SCREENSHOT_DIR = $runDir.FullName
    $env:HP2_MVP_TEST_RENDER_FPS = "$cap"
    & $engine --path build/godot-mvp-debug/mvp-project --rendering-driver vulkan --resolution 960x720 --script (Resolve-Path tools/test_frontend_mvp.gd).Path -- --race-track=13
    if ($LASTEXITCODE) { throw "Arrest smoke failed at $cap FPS" }
}
```

Debug and Release staging passed. Scenic Drive Vulkan passed the orbit
entry, terrain and return checks. Track 33 D3D12 passed the steering and
wheel rendering checks. The Debug Vulkan arrest fixture played through
and cleaned up at both render limits. From first visible actors to
cleanup it took 5.133 s at the 60 FPS limit and 4.949 s at the 120 FPS
limit. The smoke observed 185 and 419 changed actor poses, respectively,
between renders of the same committed step. These are wall-clock smoke
measurements of the fixture, including scene publication and cleanup.

### Continuous steering pose sampling

The port previously blended between rendered driver palettes whose source
steering frame was still rounded to an integer. The new port-only
`SampleDriverPresentationPose` samples the two frames around the existing
cubic steering value. It slerps the authored bone rotations and blends root
translation before the existing gesture composition and skinning. Godot
then interpolates consecutive committed poses at render cadence. The native
renderer keeps its rounded sample. The source reference is PAL1AA2F0–1AA6C0
and the existing steering curve and clip 0x2BB7F510. The donor PAL ELF version
and SHA-256 remain unresolved. There is one source update per committed step.

```powershell
$engine = (Resolve-Path local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe).Path
$env:HP2_MVP_TEST_DRIVER_INTERPOLATION = '1'
foreach ($driver in @('vulkan', 'd3d12')) {
    $env:HP2_PROFILE_DIR = Join-Path $env:TEMP ('hp2-driver-' + [guid]::NewGuid())
    & $engine --path build/godot-mvp-release/mvp-project --rendering-driver $driver --resolution 1280x720 --script (Resolve-Path tools/test_mvp_wheel_blur.gd).Path -- --race-track=33
    if ($LASTEXITCODE) { throw "Driver interpolation smoke failed ($driver)" }
}
```

Debug and Release staging passed. In Track 33 Release at the 120 FPS
test limit, Vulkan and D3D12 each recorded two additional hand poses
within a committed step during steering and eleven after input release.
Both runs passed the wheel blur and driver visibility checks.

#### Driver interpolation continuity

`World::frameCounter` is an AI collision search serial as well as a
published world field. `driverCollisionPredictNeighbors` increments it for each
eligible neighbor, so consecutive simulation ticks could have equal or
nonconsecutive values. The former driver history snapped whenever its
serial advanced by more than one. Use `GameTime::worldLoopCounter` for
pose capture, with replay epochs resetting history and bounded catch-up
preserving the last sampled endpoint. This changes only the presentation
clock selection.

Using the Release smoke command above, the same Vulkan fixture recorded
2/11 intermediate hand poses while steering/releasing before the fix and
31/165 afterward. D3D12 recorded 31/151 after the fix. Over 72 sampled
render frames, the largest hand step (translation in metres plus 0.25
times quaternion angle in radians) was 0.267 before the fix, 0.131 after
on Vulkan, and 0.139 after on D3D12. The Vulkan P95 moved from 0.102 to
0.111 as more frames showed motion. The regression checks both motion
continuity and rendered wheel and driver visibility.
