# TrackViewer and real-track driving

TrackViewer loads the player's original Retail track geometry, scenery instances,
textures and road topology directly from the configured ZZDATA tree. Vehicle driving
uses the separate SHA-bound Debug physics implementation. The viewer is a development
tool; loading and PC acceptance tests do not establish original-game equivalence.

## Open and use

Build the Godot debug preset. Configure original assets through `HP2_PS2_ASSET_ROOT`
or the ignored `config/ps2_assets.local.json`, following [DEVELOPMENT.md](DEVELOPMENT.md).
Rendering (recovered angle tables) and driving require the separately configured Debug ELF described in
[DEBUG_DRIVING_AUDIT.md](DEBUG_DRIVING_AUDIT.md). No converted asset export is required.

1. Open the F8 console and enter `trackviewer`. The debug scene can also be launched
   directly as `res://scenes/debug/track_viewer.tscn`.
2. Choose a track from the catalogue. The selected route resolves original resource
   reuse and time-of-day data; it is not inferred from the display name. Entries with
   missing resources show `[missing]` and a concrete diagnostic.
3. Wait for reading/decompression and upload to finish. The panel shows loading state,
   triangle/texture totals and recovery details. A failed load is reported visibly.
4. Hold RMB to look and use WASD/QE to move; Shift moves faster, the mouse wheel changes
   speed, F frames the track and R resets the camera.
5. Choose a vehicle and select **Drive on track**. The native simulation starts at the
   original forward racer0 marker with recovered path direction and flat-on-track
   placement. Missing marker, path or road contact blocks entry with a diagnostic.
6. Use W/S for throttle/brake-reverse, A/D for steering, Q/E for manual shifting and
   Space for handbrake. Escape releases the cursor; click the world to recapture it.
   **Return to TrackViewer** restores browsing. See [PLAYER_DRIVING.md](PLAYER_DRIVING.md)
   for transmission and input-ownership details.

The real-track drive displays the track identity and read-only four-wheel contact,
terrain and original grip values. It publishes all 33 original Debug terrain rows.
The synthetic five-surface selector belongs to the separate CarViewer test scenario.

## Native boundaries

`port::track` owns asset bytes and topology without Godot types. Original track,
polygon, path, spline and placement operations live in their corresponding recovered
classes. The Godot bridge converts display resources and input; it does not compute
vehicle grip or substitute Godot physics.

Four independent wheel queries supply real road elevation, plane normal and terrain
identity to the existing PhysicsCar/Suspension/Wheel chain. Skid elevation queries use
the same provider. The original tire tables continue to compute load, friction, slip
and road noise. Rendering texture bindings and physics terrain bindings are independent.

Track switches/reset clear the current native pose and recording. A replay retains
the selected immutable track; switching tracks clears its input/time/surface tape.
Candidate simulation steps publish atomically. An unsupported path keeps the last
completed snapshot and reports a diagnostic.

## Version and recovery limits

Retail assets use Retail layouts. Vehicle algorithms use Debug NFSHP2.ELF under the
existing TASK-0044 exception. The 33 terrain IDs are explicitly mapped using independent
ELF table Type/Name reads; the five noise/friction scalar bit patterns also agree for
all 33 rows. No Retail object is reinterpreted as a Debug object. Evidence is recorded in
[driving-integration.md](../reverse/task0048/driving-integration.md).

Road contact and a bounded single-car/static-track-polygon collision chain are implemented.
The original car collision volume, topology broad phase, wafer/Voronoi feature witnesses,
contact-time backup, crowbar displacement and original contact impulse feed the native
vehicle state. This replaces the temporary center-segment diagnostic guard. Arbitrary
static scenery-instance volumes and non-track object lifecycle are not claimed. Water/puddle
special behavior, CAR terrain ownership, destructible/dynamic objects, AI and race orchestration
are excluded. Damage, animated reactions, player impact effects and police impact monitoring
require their original dependencies; the current playerless fixture uses their exact null branches.
Encountering an unsupported physics branch produces a specific failure instead of an
invented response.

The recovery-details panel records current rendering limitations. Original world
visibility, camera-dependent scenery LOD and every original lighting/render branch are
not established by drawing the decoded geometry. Source records distinguish bounded
implemented projections from supporting decompilation. No path here is marked Verified
without a scoped original-game comparison.

## Validation

Native coverage includes track catalogue/resource parsing, topology queries, material
publication, starting positions, native wheel contact, replay and clear/re-entry. Run
the registered `track_assets`, `track_topology` and `driving_tracks` tests with the native
debug preset; the TASK-0048 task records exact commands and results. Tests using
original resources return skip77 when prerequisites are unavailable.

Native driving passed all 30 shipped starts, 120 settling steps with four-wheel contact,
three real material samples, 15-step real-track replay and return to the synthetic scenario.
A complete NativeBackend drive reached a wall 20 native units away and applied original
static contact impulses. Corner contact and rejected-step collision-state rollback also
passed. Contact math/time/order and host rendering checks also passed;
the task records the latest integrated and actual-window runs. These are port-side
checks, not original numerical certification.

Final integrated validation: Godot debug build passed; all84 registered tests passed
(124.21 seconds), and the external all30-track geometry/texture/mip/light sweep passed.
See [TASK-0048](../tasks/completed/TASK-0048-track-viewer-physics.md) for exact commands,
actual-window captures and scoped recovery limits.


## Topology debug display (TASK-0049)

The TrackViewer browsing panel has two independent, default-off checkboxes:

- **Render Collision** draws translucent blue wheel-contact polygons and their original
  perimeter edges. It includes slopes, curbs and separate road layers; it excludes
  wall-only flags and vertical planes, not a heuristic maximum slope. It does not draw
  vehicle volumes or arbitrary scenery object collisions.
- **Render Material** colors these contact polygons by physical terrain ID and displays
  a scrollable ID/name/color/polygon-count legend. ROAD, DIRT, GRASS, SAND and SNOW share
  the existing Test drive tool palette. Other colors are stable tool choices; unknown
  IDs are magenta. This never selects friction from rendered texture names.

Both may be enabled together: one material-colored fill plus collision outlines. Depth
occlusion remains enabled. A small display-only upward offset avoids coplanar flicker.
The original tire query uses each polygon's stored plane; debug faces therefore project
its XY footprint onto that plane, including a noncoplanar fourth corner. No physics data
is changed. Areas hidden by original geometry stay depth-occluded.

Controls are unavailable while loading or without valid debug topology. Preferences
survive track switches, clear and failures within the viewer session; geometry and
legend resources are released with each scene. Driving hides these browsing overlays
and returning restores their settings. Restarting the viewer resets both to off.

Indexed track textures now retain original T=0 source row order, consistently with
linear32 textures, at every original mip. This corrects vertically inverted foliage
without changing the shared decoder, vehicle materials, alpha tests or blend rules.
Evidence: [texture direction](../reverse/task0049/texture-direction.md) and
[debug geometry](../reverse/task0049/debug-geometry.md).

For reproducible window inspection, add `--track-render-collision` and/or
`--track-render-material` after the engine's `--` argument separator together with
`--track-load=11` and `--viewer-capture=<absolute PNG path>`.

TASK-0049 validation: all87 registered tests passed, both all30-track asset and contact
geometry sweeps passed, and the four TrackViewer modes plus CarViewer were inspected in
actual Compatibility windows. See [TASK-0049](../tasks/completed/TASK-0049-track-debug-rendering.md).


## World units and outside-camera projection (TASK-0050)

Track and vehicle geometry retain original native units. The native-to-Godot axis
permutation preserves lengths; the track root and driving vehicle root use unit scale.
Elise body dimensions3.83825 by1.76197 agree with its original collision volume3.88961
by1.80214 and original wheel spacing. Across30 tracks,1388 ordinary curve-sign instances
fit independently stored original scene bounds under the recovered transforms. A tall
sign by itself does not establish an asset scaling error.

The prior tool outside camera used60 degrees vertically. TASK0050 replaces this shared
projection policy with the original Debug DriveCameraMover OUTSIDE angle0x32DC
(71.52099609375 degrees horizontally), using Godot KEEP_WIDTH across window aspects.
Leaving driving restores both prior FOV and aspect policy. Tests measure actual engine
projection matrices for4:3,16:9 and portrait windows.

Only the projection is corrected here. Follow distance, target, orbit, pitch, damping
and recentering remain documented tool behavior; original DriveCameraMover follow,
wall constraints and noise are not restored. There is no matched original frame proving
that this projection difference explains every reported proportion issue. No empirical
scale multiplier is applied to road, scenery, car, collision or physics data. Evidence:
[scale investigation](../reverse/task0050/render-scale.md),
[vehicle units](../reverse/task0050/vehicle-scale.md) and
[original projection](../reverse/task0050/topology-units.md).


## Shared track runtime and independent single-car scene (TASK-0051)

`port::track::Runtime` owns immutable decoded resources and the native
`load / poll_load / unload / prepare_frame(view) / diagnostics` boundary. The
frame follows `eDisplayFrame -> GrandSceneryCullInfo -> StuffScenery ->
eModel::Render -> RenderStrips`, including visible-zone/section/tree culling,
pixel-size LOD selection, local light contexts, solid texture chains and alpha
ordering. These are bounded recovered slices, not the complete original frame
controller. Retail is the track authority; existing Debug driving remains separate.

`Ps2TrackRenderer` is the shared non-Debug GPU host. `Ps2TrackViewer` only adds
Debug request policy; it has no second loader, shader or geometry implementation.
Both consumers submit the same native frame packets. Shader sources live under
`src/platform/shaders/track_*` and are embedded into the binary, including debug
fills/edges. Debug topology overlays are independent of the normal draw commands.

Run the independent entry from the repository root:

```powershell
& local/tools/godot-4.4.1/Godot_v4.4.1-stable_win64_console.exe --path godot res://scenes/race_scene.tscn -- --track=11 --car=ELISE
```

This scene directly owns `Ps2TrackRenderer`, `Ps2VehicleRenderer` and
`Ps2DrivingRuntime`; it does not instantiate AssetViewer or DrivingLab. W/S,
A/D, Space and the existing transmission inputs drive the original native core.
R restarts; Esc returns to the frontend. Failed resource loads include their
concrete cause, discard incomplete loads, and allow re-entry. This is a single-car
runtime entry, not recovered StartRace/GameFlow/RaceCoordinator, AI or race rules.

The actual tunnel and wet-road masks use strip40 alpha rewriting plus packet320.
All30-track, all-original-mip alpha bounds prove the implemented copy/additive
fusion gates; native metadata retains both original passes. The shader computes
integer colour terms before normalized output, including alpha above128.
Retail texture+50 controls depth writes, +75 selects depth projection; the normal
Z24 range is explicit. All observed track surface bindings use Bias0/1; Bias2
fails explicitly. Mip filtering and floating host depth are port projections,
not original GS rasterizer equivalence.

Remaining independent effect: FIRETRUCK_00 and XB_GASTRKTANK_L1A_00 need the
original six-view dynamic environment capture/context2 pass. Their base pass is
shown with an explicit diagnostic; no substitute reflection texture is used.
The tunnel/wet-road paths do not depend on that capture. Original visual/numeric
comparison remains separate; PC tests never establish Verified status.

Evidence: [scenery](../reverse/task0051/scenery.md),
[blend/depth/state](../reverse/task0051/blend.md),
[runtime entry](../reverse/task0051/runtime-entry.md).
