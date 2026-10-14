# TASK-0031 — Native debug console and Debug FPS

- Status: completed
- Objective: integrate the pinned imgui-console through RenderingServer with isolated F8 input.
- Scope: port-side extensions, host adapter, FPS, synthetic tests and documentation only.
- Original PS2 addresses: not applicable; no recovery status changes.
- Dependencies: imgui-console 7840945b5a20ea9f4e83f98d1ad3d9379aede3ed, bundled csys/ImGui, Godot 4.4.1.
- GitHub issue: not created

## Acceptance criteria

- [x] F8 toggles once per press; console captures input, completion/history work; game keeps updating.
- [x] Old TAB/PageUp/PageDown/bracket shortcuts removed; Debug FPS stays top right independently.
- [x] Bounded logs, basic commands, clipboard, settings and RenderingServer triangle clipping implemented.
- [x] Native tests and asset-free smoke; Debug/on, Debug/off, Release and native builds checked.
- [x] Dependencies, architecture, development instructions and generated progress updated.

## Verification procedure

Configure/build native-debug and godot-debug; CTest; separate console-off Debug and Release builds.
Run the synthetic console scene without original assets and inspect a window capture.

## Current findings

The pinned upstream bundles ImGui 1.77 WIP with legacy key IO. Only source and license files are vendored;
upstream example backends are excluded. All new behavior is port-side tooling.

## Unresolved questions

No original-game equivalence claim is made. Visual checks use synthetic input and geometry;
no human physical-key walkthrough or original-game comparison was performed.

## Next steps

Continue with `cmake --preset godot-debug -DHP2_ENABLE_DEBUG_CONSOLE=ON`,
`cmake --build --preset godot-debug`, `ctest --preset godot-debug`.
For a window check, run the documented asset-free debug_console_smoke scene.

## Related Git commits

None.

## Completion evidence — 2026-10-14

Toolchain: CMake 3.29.2, Ninja 1.12.0, GCC 13.2.0 MinGW-w64, Python 3.14.4,
Godot 4.4.1.stable.official.49a5bc7b6, Windows x86-64.

Passed:

- `cmake --preset native-debug`; `cmake --build --preset native-debug`;
  `ctest --preset native-debug`: 29/29, including completion/history input replay.
- `cmake --preset godot-debug -DHP2_ENABLE_DEBUG_CONSOLE=ON`;
  `cmake --build --preset godot-debug`; `ctest --preset godot-debug`: 30/30.
- `cmake --preset godot-debug -DHP2_ENABLE_DEBUG_CONSOLE=OFF`;
  `cmake --build --preset godot-debug --target ps2_port`;
  `ctest --preset godot-debug -R debug_console_host --output-on-failure`: 1/1,
  Debug true, console unavailable. A Compatibility window capture also passed.
- `cmake --preset godot-release -DHP2_ENABLE_DEBUG_CONSOLE=OFF`;
  `cmake --build --preset godot-release --target ps2_port`;
  `ctest --test-dir build/godot-release -R debug_console_host --output-on-failure`: 1/1,
  Debug false, console unavailable. Release full CTest suite was not repeated.
- `local/tools/godot-4.4.1/Godot_v4.4.1-stable_win64_console.exe --path godot
  res://scenes/debug_console_smoke.tscn -- --expect-debug=true --expect-console=true
  --capture=D:/dev/hp2_godot/local/task0031/console-final.png`: passed under default
  D3D12 Forward+. Screenshot inspected: console, clipped log region, focused input,
  top-right FPS. Compatibility/OpenGL window check also passed. Captures are ignored.
- `py -3 reverse/scripts/generate_progress.py`; generated-table and final diff checks.

Resolved validation failures:

- Real history replay initially failed: upstream skipped the newest entry. The documented
  local patch fixes first Up, empty Down, reset and ring wrap.
- An initial console-off smoke expected more frames than its shorter path produced;
  the harness now explicitly waits for updates before asserting continued processing.
- A Debug host check raced a Release link because both presets share godot/bin/ps2_port.dll.
  It failed the configuration assertion and timed out. Sequential configuration/build/smoke
  reruns above passed. Do not build different Godot configurations concurrently into that path.

Final configuration and DLL restored to Godot Debug with the console ON. No proprietary
files, recovery records or Verified flags changed; no Git commit was created.
