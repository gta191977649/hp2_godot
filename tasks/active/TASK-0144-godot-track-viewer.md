# TASK-0144 - Godot Track Viewer

- Status: active
- Scope: Debug-only free camera, authored track catalogue, read-only entity inspection and independent scene loading.
- Evidence: reused the existing PAL track resource entry and decoded Scenery, route, weather and AcidFX owners. The viewer is port-side tooling and introduces no newly recovered PS2 symbols. The retail ELF identity for the local source is recorded in TASK-0143.
- Acceptance: check the real console command, all authored tracks, camera and entity overlays, environment emitters, profile isolation, return to the main menu, Debug and Release builds, and Visual Profiler A/B.

## Implementation

The initial Free Run handoff was removed after it triggered normal race audio.
The viewer loads the global and shared in-game art, TRACKA/B and both track
texture files through `ApplicationResourceRuntime` and `ResourceLoader`.
It creates no gameplay runtime, car, audio device, soundtrack or profile owner.
The Godot viewport uses the same track meshes, sky, GPU effects and compositor
as the race. Its camera supplies the original environmental emitter interest
point and weather query position. Race-only postprocessing triggers have no
operands in a scene-only session. Source entities are inspected by section and
source index, and Godot renders category markers in batches.

The core changes expose two generic boundaries. The resource loader accepts a
scene clock in place of the race audio bindings and retires that clock with the
track weather. This lets the scene read the same bundles without the gameplay
or SoundScape dependency. The existing startup callback can also be used by
the frontend, so the host can return to the main menu while preserving the
original boot autoload. Viewer code is under `src/mvp/assets/trackviewer`.

## Validation

Windows, Godot 4.7.2.stable.official.ed1daf0bf, Intel Arc B390, 192 DPI.
Debug and Release builds used `cmake --build --preset godot-mvp-debug --target
stage_mvp --parallel 16` and the equivalent Release preset.

- Debug Vulkan at 1300×1250 selected all 30 tracks through the toolbar in one
  process. The 29 switches took 678–1153 ms from scripted click to scene ready
  (mean 882 ms). The test checked the viewport contained only the current track,
  camera and entity layer. The viewer exposed no race state or HUD.
- Debug Vulkan at 1200×800 exercised the console command, race exit confirmation,
  entity and render panels, free movement and Escape, then returned to the
  original main menu. Moving the camera to the source fire and sandstorm
  coordinates activated five and two original effect definitions respectively.
  The existing profile SHA-256 remained the same across the viewer roundtrip.
  The D3D12 Debug 1200×800 Track 12 → Desert Heat → main menu smoke also passed.
- Godot's built-in Visual Profiler and server profiler sampled the same fixed
  Track 12 camera at 1200×800, with 481 frames in each window. Frame P95 was
  8.485 ms with markers hidden and 8.420 ms with markers drawn. Render GPU P95
  was 1.896 → 2.063 ms. Render CPU P95 was 3.574 → 3.591 ms. This is a
  fixed-camera comparison, with no claim about full-track 120 Hz pacing.
- Release headless Track 12 lifecycle smoke passed, including pause, resume,
  restart and return. Local runs and screenshots are under `local/task0144-*`.
  The Debug Vulkan frontend smoke passed at 960×720. From the viewer's
  returned main menu, a normal Quick Race reached the driving state on Track 61.

Repeat the windowed checks with `tools/test_mvp_track_viewer.gd` on the staged
Debug project. Use 1200×800 at 192 DPI for the recorded interaction coordinates,
set `HP2_MVP_VIEWER_EFFECTS=1` to check emitters, and add
`-- --race-track=12` for the race-confirmation and Desert Heat path. Set
`HP2_MVP_VIEWER_MATRIX=1` and use 1300×1250 for the full dropdown run. Use an
isolated `HP2_PROFILE_DIR` for testing. Cross-monitor DPI and loss-of-focus
checks require additional windowed coverage.

The toolbar and Camera tab now show live camera Position and Look At in HP2
XYZ. Look At is one unit along the camera's forward axis. The Camera tab copies
both triples to the clipboard. The Debug build and the viewer smoke with
`HP2_MVP_VIEWER_COORDINATES=1` passed on Godot 4.7.2 Vulkan at 1200×800,
1920×800 and 2560×800. The smoke checks the copied values against the camera pose after
moving and releasing the camera.

Render now offers Wireframe view directly below Entity view. It uses Godot's
viewport debug draw and switches between mesh edges and entity markers. Debug
compilation and the Vulkan toggle smoke passed at 1200×800 using a separate
staging directory because the normal staged DLL was in use. Set
`HP2_MVP_VIEWER_WIREFRAME=1` to repeat the toggle check.

Wireframe uses a shared bright amber debug material and viewport glow on black.
The override applies to the viewer's track geometry only. Normal materials and
the sky return when the mode is disabled. Verified the rendered screenshot and
toggle on Godot 4.7.2 Vulkan at 1200×800 from the isolated staged project.
