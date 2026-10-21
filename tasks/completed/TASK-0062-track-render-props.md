# TASK-0062 - TrackViewer Render props

- Status: completed
- Authority: user-requested port-side viewer control, no recovered behavior change.
- Scope: default-on Visibility checkbox; hide transformed scenery (trees/props)
  before batching for main/reflection/native/HLOD views; restore when re-enabled.
- Preserve all pre-existing user edits. Original world geometry and collision stay.
- Acceptance: native mixed-frame regression, shared settings/action wiring,
  bridge build and source diff review. Runtime visual check if available.
## Validation

- `cmake --build --preset godot-debug -j4`: PASS (GCC13.2 / Godot4.4.1).
- `ctest --preset godot-debug -R "^(track_effects|debug_console)$"`: PASS2/2.
- Godot headless staged-debug `track_controls_smoke.gd`: TRACK_CONTROLS_OK.
- Windowed staged-debug `track_effects_ui_smoke.tscn`: actual ImGui off/on PASS,
  Track11 main commands1186 ->581 ->1186; retained world geometry.
- Paused simulation UI fixture now explicitly redraws before inspecting counts;
  first attempt read stale paused-frame counters and failed; corrected rerun PASS.
- Initial CTest host check used root's older DLL (not the staged build); use the
  direct staged-project command above for this feature. Root DLL untouched.
- `git diff --check`: PASS. No recovered-body edits or PS2 equivalence claim.
- GPU reflection/Show LOD toggle not separately captured; same pre-batch filter
  is used for every view and covered by mixed-frame native cases.
- Run: `local/task0062/Launch TrackViewer.cmd` (built staged-debug DLL).


## Workspace launch correction - 2026-10-19

User reported `./build.ps1 --run` still showed the old UI. Cached
PORT_GODOT_BIN_DIR pointed at task0061 staging while --run launched root godot.
The build entry point now explicitly sets workspace project/bin for Godot presets;
direct CMake staging remains available. `./build.ps1` rebuilt root DLL PASS;
`ctest --preset godot-debug -R '^track_controls_host$'` PASS1/1 (4.68s).
Restart the frontend to load the newly linked root DLL. No process terminated.
