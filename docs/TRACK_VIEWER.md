# TrackViewer and real-track driving

TASK0075 browsing uses Camera3D default75 degrees/KEEP_HEIGHT; Test drive and
race retain their own outside projection. Original wheel smoke/tire-spew now
shares track particles; see [effects and limits](DRIVING_EFFECTS.md).

Static draw submission census and default particle/postprocess caches:
[render cost analysis](PERFORMANCE_TASK0063.md), [30-track counts](TRACK_DRAW_COUNTS.md).


Top-left camera coordinates label HP2 and Godot axes. Shared ImGui and host screen
text follow monitor DPI; see [UI_DPI.md](UI_DPI.md).

Track host allocation/eviction, on-demand `get_memory_state()` diagnostics and
Track11 before/after measurements are documented in [TRACK_MEMORY.md](TRACK_MEMORY.md).

TrackViewer loads the player's original Retail track geometry, scenery instances,
textures and road topology directly from the configured ZZDATA tree. Vehicle driving
uses the separate SHA-bound Debug physics implementation. The viewer is a development
tool; loading and PC acceptance tests do not establish original-game equivalence.

Visibility includes **Render props** (checked by default). Uncheck it to hide
placed trees and props in main and reflection views, including Show LOD. This
port-side gate removes transformed scenery submissions; world-identity geometry
(roads and baked world meshes) and collision remain. Recheck to restore them.
The choice is shared across track renderer instances for the session; Restore
Original Defaults enables it again. Geometry baked into world meshes is retained.

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

TASK-0052 adds the original scheduled six-view environment capture and shared
atlas/context2 adapter for the bounded source-alpha branch. Gameplay vehicle
flag4 reflections require destination alpha and remain explicitly unsupported;
the frontend mode3 behavior is kept separate. Per-draw PSMCT16 framebuffer
precision is also an explicit host gap. The tunnel/wet-road paths do not depend
on dynamic capture. Original visual/numeric comparison remains separate; PC
tests never establish Verified status.

Evidence: [scenery](../reverse/task0051/scenery.md),
[blend/depth/state](../reverse/task0051/blend.md),
[runtime entry](../reverse/task0051/runtime-entry.md).


## Effects and visibility (TASK-0052, in progress)

The ImGui `Effects` group lists surface, environment, weather/particles and
postprocess permissions with their actual activation state and unavailable reason.
A checked box allows the original conditions to activate an effect; it does not
force that effect in every track. `Restore Original Defaults` restores session
permissions shared by TrackViewer, Test drive and the independent race scene.
Roadshine controls the high-light contribution while retaining road-mask blending.
Fog and Focus use their original lookup tables; an independent color-grading LUT
has not been established and is explicitly unavailable. Incomplete stages remain
marked unavailable rather than producing a substitute modern effect.

`Visibility` defaults to original near/far selection and pixel-size LOD. Optional
distance override and restore controls are debugging tools; performance acceptance
uses the original automatic settings. Effect settings do not alter wheel grip or
native collision response. Presentation timestep is provided separately from
physics; capture views do not advance WorldEffects again.

Actual-window regressions include the ImGui Roadshine checkbox and visibility
controls, an opaque vehicle in front of the sky, shared viewer/runtime color plus
native-depth coverage, and real Track11 forest-fire particle packets. Full pipeline
recovery and the 1080p/60FPS target are still tracked as incomplete in TASK-0052.


TASK0053 regression build uses layer-aware custom bounds for sky/Sun/particles;
the main compositing viewport must submit zero3D calls. Visibility now shows
visible / considered / loaded instance counts and selected / loaded sections.
The fixed Track11 start uses14/128 sections and102/8407 loaded instances; loading
the complete asset cache does not submit the entire track. Roadshine permission
skips its extra specular sample when disabled, retaining road/water alpha masks.
Debug and Release launchers / build manifest: local/task0053/delivery.


TASK0054 fixes the custom screen-packet render-target Y convention shared by Sun
and world particles. Camera yaw/pitch/translation updates their projection from
original world anchors; the adapter preserves actual GPU projectionY sign.
Original lens-flare offsets still follow their original screen-space layer rules.
Regression tests include off-center Sun positions, frozen-time original particle
packets and an asymmetric host-only production-shader probe. Old53 DLL fails both
strong controls; ordinary sky and native-depth conventions are unchanged.


TASK0055 host optimization selection is shared by TrackViewer, Test drive and
race_scene. CLI --track-optimization-profile=baseline|optimized is applied before
load. The optimized candidate enables resource/particle buffer reuse, preserving
original far distance, LOD, effects and native frame order. get_optimization_state
reports effective policy and reuse counters. Options are immutable while loaded;
clear before changing them. MultiMesh, occlusion and opaque queue are experimental
and excluded from the optimized profile pending their separate gates.


TASK-0055 acceptance completed:30 tracks,120 route-warmup/600 measured frames,
3 repeats in both frozen54/final55 Release1080p runs. Native packet/pose, original
effect permissions and draw-call distributions match. Median-route improvements:
wall10.03%, CPU prepare27.62%, host submission33.16%; no persistent>5% wall median/
P95/P99 regression. GPU timers unavailable; loading stalls remain. The accepted
resource/particle-reuse profile is now the common project default. Distinct
material identity retained; MultiMesh/occlusion/opaque remain experimental/off.
See [performance report](PERFORMANCE_TASK0055.md). Override at launch with
--track-optimization-profile=baseline. No original-console Verified promotion.

Archived TASK0056 HLOD (removed by TASK0058): `--track-lod-backend=godot_hlod` formerly used original authored levels
with Godot engine switching. Original remains default. Local delivery launchers,
API diagnostics, limitations and rollback are in [GODOT_HLOD.md](GODOT_HLOD.md).

### Show LOD (port debug display)

TrackViewer exposes `Show LOD` beside the collision/material controls. Default off;
enabling it displays every available authored model slot simultaneously in the
selected track sections, bypassing normal LOD distance ranges. Section selection
and engine frustum clipping remain. Levels overlap at their original transforms;
z-fighting is expected. No generated low meshes or collision/physics changes.
Production uses only `original`; disabling restores native culling. The previous
`godot_hlod` backend is retired and rejected.
The session choice survives track switches and is suppressed while driving.

Reproduce host coverage:
`local/tools/godot-4.4.1/Godot_v4.4.1-stable_win64_console.exe --path godot --audio-driver Dummy --script res://scripts/runtime/track_show_lod_smoke.gd`.

TASK0058: see [native track culling](NATIVE_TRACK_CULLING.md). `Show LOD` remains
an explicit debug-only bypass; disable it for normal visibility/performance checks.

### Camera movement and particle recovery gaps (TASK-0059)

Fast movement/rotation must not unload scenery when an untranslated particle blend
is encountered. Unsupported blend-bound quads are counted and explained below the
debug controls; supported particles and normal track draws continue. This does not
mean the missing high-alpha compositor is restored. `Reset Camera`/R returns to
current track start/forward. New delivery is local/task0059/delivery; earlier
native launch shortcuts forward to it. Native visibility rules are unchanged.

The coordinate overlay displays `Draw calls (total)` from Godot's `Performance.RENDER_TOTAL_DRAW_CALLS_IN_FRAME`, updated each UI frame from the last rendered frame. This is the engine-wide total across rendered viewports, rather than a native command estimate.

TASK-0065 adds expandable **Render statistics** with last-frame per-viewport visible/shadow/canvas draws and primitive counts, plus a JSON camera/control diagnostic export. TASK-0066 removes the track allowlist: the optimized profile enables matrix batching for every track under common state-based eligibility; car/capture and other generic state fallbacks remain. The known Track24 colour discrepancy is not yet repaired. See [dense-view results and rollback](PERFORMANCE_TASK0065.md).

See [global policy and current validation limits](PERFORMANCE_TASK0066.md). All future optimizations use global rendering-state rules rather than track-ID allowlists.

TASK-0067 displays **Triangles/primitives** beside the global draw-call counter beneath coordinates. Both read Godot Performance monitors for the last rendered frame; the primitive total includes triangle, line and point primitives across rendered viewports. There is no added GPU readback.


## Port-side wireframe display (TASK-0074)

Open **Effects > Debug (Godot) > Wireframe debug** to inspect geometry edges.
The checkbox defaults off and currently supports Vulkan Forward+ only. It uses
Godot viewport debug draw, not replacement GS materials or recovered behavior.
The direct main viewport and compositor source colour viewport switch together;
encoded depth, history and reflection consumers keep their existing modes.
Restoring Original Defaults, unloading the track or exiting the scene disables
the diagnostic and restores the previous main viewport debug mode. New colour
viewports created while enabled receive the diagnostic on the next host update.

This is a script-only adapter through the existing Effects descriptor/action
chain; no DLL rebuild or native selection/physics changes are needed. No helper
node or per-frame polling exists until first enabled; polling stops when disabled.
Forward+ uses the engine's viewport wireframe mode, with no startup wireframe
resource generation enabled. API reference: [Godot RenderingServer](https://docs.godotengine.org/en/stable/classes/class_renderingserver.html#enum-renderingserver-viewportdebugdraw).
