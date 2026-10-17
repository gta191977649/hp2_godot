# Debug asset viewers

TASK-0032 adds **CarViewer** and **TrackViewer**, entirely port-side tools. Both run
without original assets. Their colored geometry is a **Synthetic fixture**: it establishes
no original model, unit, coordinate convention, road semantics or physics behavior.
TASK-0033 adds original vehicle BIN decoding and static exterior assembly to CarViewer.
The default display uses the recovered VU base material and initialized frontend light
slice. Full GS material behavior remains incomplete; raw texture inspection is a labeled
debug override. Track loading remains unimplemented. See
[vehicle rendering](VEHICLE_RENDERING.md) for evidence and remaining work.

## Launch and commands

Build Godot Debug with `cmake --preset godot-debug -DHP2_ENABLE_DEBUG_CONSOLE=ON`
and `cmake --build --preset godot-debug`. In the frontend press **F8**, then:

| Command | Result |
|---|---|
| `carviewer` | Open CarViewer in the current window |
| `trackviewer` | Open TrackViewer in the current window |
| `viewer.load "CARS/911TURBO/GEOMETRY.BIN"` | Load original car geometry, bound shared textures and static parts |
| `viewer.clear` | Clear visible test content and the resource request |
| `viewer.fixture` | Restore the synthetic geometry and clear the request |
| `viewer.reset` | Reset the camera and movement speed |
| `viewer.status` | Print viewer, request, load status, fixture, camera and rendering statistics |
| `viewer.close` | Return to a newly initialized frontend |

Use quotes around paths containing spaces. Paths are relative to the configured ZZDATA;
absolute paths and parent traversal are rejected. CarViewer accepts exactly
`CARS/<model>/GEOMETRY.BIN`; `.LZC`, dashboards and track resources are unsupported.
Successful loading removes the fixture and frames original mesh bounds. A failed car load
clears old car content and the fixture, retains the error and does not report success.
All viewer windows, lists, inputs, buttons, status and FPS are native ImGui in the same
frame as the console. Requests execute after that frame. TrackViewer still retains its labeled fixture and reports an unavailable loader. Buttons use the
same request queue as commands. Viewer-specific actions outside a viewer report that it must
be opened first. `viewer.status` can also describe the frontend.

F8 toggles the persistent console. TAB completes command names; Up/Down recall command
history. Logs, console window state and history remain alive when switching scenes. Debug
FPS stays at the top right. Closing a viewer resets the frontend rather than restoring its
old menu/movie state, so the normal frontend asset prerequisites apply on return.

For direct asset-free launch, set the local engine path as in DEVELOPMENT.md:

```powershell
& $portGodot --path godot res://scenes/debug/car_viewer.tscn
& $portGodot --path godot res://scenes/debug/track_viewer.tscn
```

Debug with `HP2_ENABLE_DEBUG_CONSOLE=OFF` supports direct launch and all panel buttons.
Release disables the viewer request interface; direct viewer scenes print a clear diagnostic
and exit with status 1. The normal application entry point remains the frontend.

## Camera controls

| Viewer | Controls |
|---|---|
| CarViewer | Left drag: orbit; right drag: pan; wheel: zoom |
| TrackViewer | Hold right button: look and enable WASD movement, Q down / E up; Shift: faster; wheel: adjust speed |
| Both | F: frame current target; R or Reset Camera: reset; F8: console |

Camera mouse operations start outside the panel. Editing the path field does not move the
camera. Opening the console or losing application focus releases the captured mouse and
held camera keys. Console input does not reach the camera; the scene keeps updating.

For loaded cars, F and Reset Camera frame the original bounds. The default view fixes the highest-detail model. `Show LODs` lazily loads the other
levels and arranges complete vehicles side by side with LOD/triangle labels. This comparison
layout is a port tool; attachment membership still uses the recovered per-LOD rules. Part checkboxes
control individual instances/LOD variants. Wireframe and Bounds are host debug overlays. The
status includes triangle/texture/part counts, selected LOD and projected size. The scrollable
ImGui panel keeps controls reachable on smaller windows. Its searchable original catalog
retains all 64 car IDs and 235 skin records, including missing resources. Colors use their
original ID/name and material binding. Click a paint tile to change skin; hover for its
original name/ID and preview status. Tiles are computed from actual skin texels and the
recovered material calculation, not guessed name-to-RGB values or a custom color picker. HP versions use catalog
resource text rather than suffix guessing. The first available car and first valid skin
are selected on normal entry; explicit path loading remains supported. Lights off, zero
steering and zero wheel spin are explicit viewing choices. General GS source-over alpha above 128
produces a clear material-gap error (current static exterior data stays within range); use the labeled inspection override only
for debugging. This branch is not counted as restored.

Examples using the configured original asset root:

```text
carviewer
viewer.load "CARS/911TURBO/GEOMETRY.BIN"
viewer.load "CARS/BMWM5COP/GEOMETRY.BIN"
viewer.load "CARS/4DR_SEDAN/GEOMETRY.BIN"
viewer.status
```

## Extension points and tests

`src/game_core/extensions/viewer_session.hpp` defines the Godot-free state, action queue
and command registration entry. `viewer_commands.cpp` integrates csys; it does not load files.
The existing Ps2RenderHost exposes those operations to the persistent debug overlay.
`DebugUiSession` owns one context/font/input/render batch stream; `ViewerUi` draws native
controls and queues actions. `godot/scripts/debug/asset_viewer.gd` owns fixture and camera,
submits state snapshots, and executes queued actions after ImGui rendering.
`Ps2CarViewer` owns Godot meshes, images and overlays. Godot-free `port::vehicle::Model`
reads original bytes, calls the recovered CarRenderInfo slices and supplies static instances.
Clear, Fixture and scene destruction release original content; the debug session persists.

```powershell
ctest --preset native-debug -R 'viewer_session|debug_console'
ctest --preset godot-debug -R 'debug_viewers_host|debug_console_host'
ctest --preset godot-debug -R 'vehicle_|car_render_'
& $portGodot --path godot res://scenes/debug/car_viewer.tscn -- --viewer-capture=D:/temp/car-viewer.png
& $portGodot --path godot res://scenes/debug/car_viewer.tscn -- --car-load=CARS/911TURBO/GEOMETRY.BIN --viewer-capture=D:/temp/car-original.png
```

The integration smoke uses an asset-free frontend stand-in solely for its return check.
Real frontend initialization is separately checked by the existing frontend render smoke.
Captures belong outside Git. PC tests and these tools establish no original-game equivalence.
The car integration smoke loads all three original models, switches LOD/parts, reloads,
clears, handles missing resources and checks scene cleanup. It skips only when original assets
are unavailable; native checks reject an explicitly configured but invalid root.


For a real GPU numeric material check (original assets and windowed Compatibility required):

```powershell
cmake --preset godot-debug -DHP2_ENABLE_DEBUG_CONSOLE=ON -DPORT_ENABLE_GPU_TESTS=ON
cmake --build --preset godot-debug
ctest --test-dir build/godot-debug -R '^car_material_gpu$' --output-on-failure
```

This samples final framebuffer RGB for original VU fixture inputs using the actual
vehicle shader, including MODULATE, copy/source-over and strict alpha-test boundaries.
It is optional because headless rendering cannot establish GPU pixel output. Reset
PORT_ENABLE_GPU_TESTS=OFF for the default headless CTest configuration.
The full catalog resource probe is `vehicle_catalog_tests.exe --all-skins`.

## Compact controls and asynchronous loading (TASK-0033 follow-up)

The searchable car selector, clickable paint tiles, Show LODs, Reset Camera and short state
are on the main panel. Advanced starts collapsed and contains resource paths, Clear/Fixture,
wireframe, bounds, raw inspection, parts and diagnostics. All ImGui root windows are contained
inside the viewport when dragging, restoring settings or resizing. A new compact window ID
avoids inheriting the old tall panel layout. Collapsed saved sizes are capped at420x320;
Advanced permits viewport-height resizing. Controls scroll internally on small windows.

Selecting a car/skin queues native work after the ImGui frame. A single Godot-free worker
reads/decodes/binds assets; the render thread pumps uploads with a 2 ms cooperative budget
between surfaces/images. A single upload or driver shader compilation can exceed that target;
it is not a hard real-time guarantee. The old car stays visible with an explicit loading state
until the replacement commits. The latest request wins. Clear, Fixture and scene exit cancel
pending work; a failure clears the display. Extension termination joins the worker before DLL
unload (joining from static destruction under the Windows loader lock is unsafe).

Geometry caches use original resource path plus near/all mode; skin changes reuse meshes,
textures and shader variants, and preserve existing part nodes when geometry/UVs match.
Geometry and decoded-pixel LRU budgets are 256 MiB and 128 MiB; uploaded mesh/image cache
budget is 128 MiB. Resident archives/global data and live scene/model references are additional
and remain valid even when their cache entry is evicted. Root changes invalidate caches.
Clear and scene exit release GPU ownership; the native session cache survives scene switches.
Advanced diagnostics show CPU/read/geometry decode/pixel decode/binding/upload times, maximum
upload slice and cache counters. Cold loads also compute/cache the selected car's paint tiles.
The tile uses upper-facing triangle-centroid paint samples, recovered interpolated VU color
and MODULATE, then averages the brightest quartile to avoid atlas borders dominating it.
This is a documented port-side representative preview, not recovered original menu RGB.
Missing previews show their reason on hover; no original color field is guessed.

`--car-lods` starts a direct CarViewer launch in comparison mode. Capture waits for pending
loads before saving. The normal interactive path is async; legacy synchronous bridge methods
remain available for standalone numerical/smoke tools, with near-only default rendering.

## Car preview mode and shared materials (TASK-0034)

CarViewer no longer shows TrackViewer or Back to Frontend buttons. Console navigation
commands remain available. Toggle **Preview Mode** above the car selector for a low-angle,
narrower-FOV presentation with no debug floor/grid/axes. It continuously orbits at a port-side
12 degrees/second; mouse orbit, pan and wheel zoom still work and never stop the auto orbit.
Opening the console also leaves the presentation moving. Use `--preview` for direct launch.
The small panel moves to the top left on entry; it can subsequently be repositioned normally.

The original static material/light pipeline is shared through Ps2VehicleRenderer and native
CarRenderInfo slices, not implemented in an extension. All shaders are authored under
src/platform/shaders and embedded during CMake configuration. The environment reflections
and floor mirror visible in PS2 reference images are not yet recovered; Preview Mode does
not invent them. See VEHICLE_RENDERING.md for exact supported rendering limits.

CarViewer's **Wheel Blur (high speed)** enables tire rotation and the original `WHEEL_BLUR`
overlay. The **Wheel speed (rad/s)** slider underneath adjusts preview angular velocity
from -150 to150: negative reverses, zero stops. Default80 rad/s is a port-side demonstration
input, not an original vehicle physics constant. Original per-wheel strength is
clamp(abs(velocity)*0.02,0,1), with alpha scale90; the blur layer has its own original
non-spinning matrix. Changes update existing resources and continue while F8 is open.
Disabling resets the static pose; selected speed and switch survive car changes.
`--wheel-blur` enables it on direct launch. Original contact/damage gates and physics remain
untranslated. Missing resources fail explicitly; no synthetic blur texture is generated.

## CarViewer Test drive (TASK-0038)

Click **Test drive** to drive the selected car/skin immediately from a rear follow camera.
W/S accelerate/brake, A/D steer, mouse movement orbits; the camera returns behind after
idle. Esc releases the cursor, clicking the world recaptures. F8/focus temporarily suspends
and resumes automatically. The only mode button is **Return to CarViewer**; no Run/Pause
or simulation function keys. Original native driving stays independent of the tool camera.
The flat plane uses original material1; original numerical equivalence remains unverified.
See [DRIVING_LAB.md](DRIVING_LAB.md) for configuration, scope and input/window tests.
