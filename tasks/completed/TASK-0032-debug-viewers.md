# TASK-0032 — CarViewer and TrackViewer scaffolds

- Status: completed
- Objective: asset-free 3D debug viewers with console routing and a future loader seam.
- Scope: port-side scaffolds only; no reverse engineering, game-resource parsing or recovery changes.
- Original PS2 addresses: not applicable.
- Dependencies: TASK-0031; Godot 4.4.1; existing csys console.
- GitHub issue: not created

## Acceptance criteria

- [x] Both viewers run directly without assets, with synthetic fixtures and camera controls.
- [x] Commands queue safe scene operations; load requests explicitly report an unavailable loader.
- [x] Shared console, logs and FPS persist across scene transitions; input is isolated.
- [x] Debug console-off viewers work; Release rejects viewer entry.
- [x] Native tests, Godot smoke, window captures, documentation and generated progress complete.

## Verification procedure

Build and CTest native-debug and godot-debug. Run asset-free viewer smoke and window
captures. Sequentially check Debug console-off and Release, then restore Debug console-on.

## Current findings

No native vehicle or track loader exists. Fixtures and camera units are explicitly port-side.
TASK-0031 has uncommitted files; preserve them and extend its interfaces without discarding work.

## Unresolved questions

Original mesh formats, coordinate conventions and loader entry addresses remain unknown.

## Next steps

Continue with `cmake --preset godot-debug -DHP2_ENABLE_DEBUG_CONSOLE=ON`,
`cmake --build --preset godot-debug`, then `ctest --preset godot-debug`. Open F8 and run
`carviewer` or `trackviewer`. Follow docs/DEBUG_VIEWERS.md before integrating a future
evidence-backed loader; do not substitute synthetic geometry for successful loading.

## Related Git commits

None.

## Completion evidence — 2026-10-14

All implementation is port-side. The native ViewerSession has no Godot dependency.
The optional csys command layer queues requests, and the persistent DebugTools overlay
processes transitions after drawing. The original frontend remains the default entry.
Console/settings/history/logs and FPS survive scene switches. No resource file is opened
or decoded by viewer loading; requests retain a labeled synthetic fixture and report the gap.

Passed checks (run from repository root):

- `cmake --preset native-debug`; `cmake --build --preset native-debug`;
  `ctest --preset native-debug`: **30/30**. Includes viewer queue/state, parsing/rejection,
  commands with quoted paths, load gap, console completion/history and tiny-viewport drawing.
- `cmake --preset godot-debug -DHP2_ENABLE_DEBUG_CONSOLE=ON`;
  `cmake --build --preset godot-debug`; `ctest --preset godot-debug`: **32/32**.
- `cmake --preset godot-debug -DHP2_ENABLE_DEBUG_CONSOLE=OFF`;
  `cmake --build --preset godot-debug --target ps2_port`;
  `ctest --preset godot-debug -R 'debug_viewers_host|debug_console_host' --output-on-failure`:
  **2/2**, plus the updated queue-after-switch smoke rerun **1/1**.
- `cmake --preset godot-release -DHP2_ENABLE_DEBUG_CONSOLE=OFF`;
  `cmake --build --preset godot-release --target ps2_port`;
  `ctest --test-dir build/godot-release -R 'debug_viewers_host|debug_console_host' --output-on-failure`:
  **2/2**. Direct headless CarViewer launch prints the Release diagnostic and exits **1**, as intended.
  Full Release CTest was not repeated.
- `local/tools/godot-4.4.1/Godot_v4.4.1-stable_win64_console.exe --headless --path godot
  res://scenes/frontend_native.tscn -- --native-render-smoke`: **passed**, 41 screens,
  1076 quads, 7681 glyphs, 120 textures. This separately checks the real asset-backed frontend.
- Same engine with `--path godot res://scenes/debug/viewer_frontend_fixture.tscn --
  --viewer-smoke --expect-debug=true --expect-console=true`: **passed** in a D3D12 Forward+
  window. Camera orbit/free movement/reset, F8 isolation, mouse release, repeated scene
  transitions, queued load-after-switch and continued updates are driven by synthetic input.
- Direct window launches of `res://scenes/debug/car_viewer.tscn` and `track_viewer.tscn`
  with `--viewer-capture=D:/dev/hp2_godot/local/task0032/car.png` / `track.png`: **passed**.
  Both captures inspected: fixtures, grid, axes, lighting, controls, state panel and FPS.
  Captures remain ignored; they contain synthetic content only.
- Engine `--headless --path godot --editor --import --quit`: **passed**; generated script UIDs retained.
- `py -3 reverse/scripts/generate_progress.py`; `--check`; `git diff --check`: **passed**.

Resolved failure: the first viewer smoke exposed the upstream console's conditional
BeginChild/EndChild pairing under restored window settings. It terminated on the first
console opening. The documented one-line vendor pairing patch fixes it; tiny-viewport
native tests and headless/windowed host smoke now pass. No assertion was disabled.

Toolchain: CMake 3.29.2, Ninja 1.12.0, GCC 13.2.0 MinGW-w64, Python 3.14.4,
Godot 4.4.1.stable.official.49a5bc7b6, Windows x86-64. The D3D12 engine emits its existing
PSO-cache-not-implemented warning; no viewer validation failures remain.

Limits: no reverse engineering, real car/track loading or original-game comparison was
performed. Asset-free return-path smoke uses a clearly named frontend stand-in; normal
application return creates the real frontend scene and reruns its existing initialization.
Window input testing is scripted, not a human physical-key walkthrough. Future real resource
loading is deliberately unfinished and is not an acceptance criterion of this scaffold task.

Final build is restored to Godot Debug / console ON. TASK-0031's pre-existing uncommitted
work was preserved. No recovery records, Verified flags or proprietary files changed.
No Git commit was created.
