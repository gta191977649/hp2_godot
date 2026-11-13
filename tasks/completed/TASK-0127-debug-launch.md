# TASK-0127 - Debug launch and ImGui tools

- Status: completed
- Scope: restore local Debug frontend launch with the complete game source and show a persistent ImGui Console and frame-time graph through the Godot MVP host.
- Evidence: port tooling. Original PS2 addresses do not apply.

## Acceptance

- [x] Run `./build.ps1 --run --mvp --debug --jobs 16` into the original title sequence.
- [x] Display and update the ImGui overlay in frontend and race, with keyboard capture isolated from game input.
- [x] Render with Windows DPI scaling and show recoverable MVP assertion messages in the Debug Console.
- [x] Build Release without the ImGui overlay.

## Findings and validation

- The exact command initially reproduced `Source common sound bank failed,
  audio result=1`. The loose source was missing both `SOUND/common.msh` and
  `SOUND/common.msb`. The ignored config now selects the complete US retail
  ISO documented in E-0126-A01 for MVP launches. Its existing `root` remains
  available to the extraction tools.
- The Godot MVP owns the overlay in `src/mvp/assets/imgui_overlay.cpp`.
  Debug links the previously pinned vendored ImGui Console sources.
  A persistent CanvasLayer keeps the 240-sample real-time frame graph
  visible through screen changes. The Console owns keyboard input while
  open and F8 hands it back to the game. DPI changes rebuild the atlas
  and rescale widgets using the Windows game window's DPI.
- `./build.ps1 --run --mvp --debug --jobs 16` reached Title Screen and
  started original AVI playback. Logs are under `local/task0127-*.log`.
  A Forward+ Track 12 probe drove after F8 and confirmed that W had no
  effect while the console was open. Screenshots show both panels at
  192 DPI (200%). Multi-monitor DPI changes were not exercised.
- Forcing the incomplete loose source via `HP2_PS2_ASSET_ROOT` reproduced
  the explicit missing-bank assertion. `local/task0127-shots/assert.png`
  shows its red console entry. Esc exited with code 1. Process-aborting
  native assertions and errors before GDExtension load cannot reach the overlay.
- `cmake --build --preset godot-mvp-release --target stage_mvp --parallel 16`
  built Release. CMake 3.29.2, Ninja 1.12.0, MinGW GCC 13.2.0, and Godot
  4.7.2 Forward+ were used.

To repeat TASK-0127, run the exact Debug command above with the local
`mvp_root` pointing to a complete asset source. Press F8 before menu or
race input. The local `task0127-overlay-probe.gd` captures the race overlay
and missing-bank assertion. No PS2 behavior or verification status changed.
