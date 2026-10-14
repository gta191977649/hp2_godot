# Debug asset viewer scaffolds

TASK-0032 adds **CarViewer** and **TrackViewer**, entirely port-side tools. Both run
without original assets. Their colored geometry is a **Synthetic fixture**: it establishes
no original model, unit, coordinate convention, road semantics or physics behavior.
The original vehicle and track loaders are **not implemented**. This task performs no
reverse engineering and does not change recovery confidence.

## Launch and commands

Build Godot Debug with `cmake --preset godot-debug -DHP2_ENABLE_DEBUG_CONSOLE=ON`
and `cmake --build --preset godot-debug`. In the frontend press **F8**, then:

| Command | Result |
|---|---|
| `carviewer` | Open CarViewer in the current window |
| `trackviewer` | Open TrackViewer in the current window |
| `viewer.load "<relative-path>"` | Record a ZZDATA-relative resource request; report `Loader not implemented` |
| `viewer.clear` | Clear visible test content and the resource request |
| `viewer.fixture` | Restore the synthetic geometry and clear the request |
| `viewer.reset` | Reset the camera and movement speed |
| `viewer.status` | Print viewer, request, load status, fixture and camera state |
| `viewer.close` | Return to a newly initialized frontend |

Use quotes around paths containing spaces. Paths are relative to the configured ZZDATA;
absolute paths and parent traversal are rejected. This version never reads a requested file,
enumerates assets or guesses a resource format. A load attempt retains any visible synthetic
fixture, still explicitly labeled, and never reports a loaded original asset. Buttons use the
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
| Both | F: frame synthetic target; R or Reset Camera: reset; F8: console |

Camera mouse operations start outside the panel. Editing the path field does not move the
camera. Opening the console or losing application focus releases the captured mouse and
held camera keys. Console input does not reach the camera; the scene keeps updating.

## Extension points and tests

`src/game_core/extensions/viewer_session.hpp` defines the Godot-free state, action queue
and command registration entry. `viewer_commands.cpp` integrates csys; it does not load files.
The existing Ps2RenderHost exposes those operations to the persistent debug overlay.
`godot/scripts/debug/asset_viewer.gd` owns synthetic geometry and cameras. A future recovered
loader adapter replaces the explicit loading gap and supplies actual rendering content;
it must resolve assets through the existing platform asset-root entry point, retain original
addresses/evidence and keep game logic free of Godot. No speculative mesh schema is introduced.

```powershell
ctest --preset native-debug -R 'viewer_session|debug_console'
ctest --preset godot-debug -R 'debug_viewers_host|debug_console_host'
& $portGodot --path godot res://scenes/debug/car_viewer.tscn -- --viewer-capture=D:/temp/car-viewer.png
```

The integration smoke uses an asset-free frontend stand-in solely for its return check.
Real frontend initialization is separately checked by the existing frontend render smoke.
Captures belong outside Git. PC tests and these tools establish no original-game equivalence.
