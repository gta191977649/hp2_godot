# TASK-0128 - Display refresh pacing

- Status: completed
- Scope: use the current display refresh for Godot presentation across menus and races and remove the observed Debug main menu bottleneck.
- Baseline: the 120 Hz host measured about 120 FPS at the title and 29.2 FPS in New Main Menu with VSync enabled and no engine FPS cap.

## Acceptance

- [x] Use Godot window VSync and no software FPS cap throughout the application.
- [x] Measure the Debug main menu and an active race against the current display refresh.
- [x] Preserve original video cadence, menu flow and the fixed donor simulation clock.

## Findings and validation

In the baseline, the external menu movie's update requested a writable
pointer to the shared texture allocation. The external decoder ignored
that pointer, but the allocation recorded the exposure and the Godot
cache decoded and uploaded its textures again each frame. The external
path now passes an empty output buffer. The native decoder retains its
original buffer. The Godot project explicitly enables VSync and sets
the software frame cap to zero.

- Both `cmake --build --preset godot-mvp-debug --target stage_mvp --parallel 16`
  and `cmake --build --preset godot-mvp-release --target stage_mvp --parallel 16`
  passed using CMake 3.29.2, Ninja 1.12.0 and MinGW GCC 13.2.0.
- Godot 4.7.2 Forward+ on the 120 Hz display measured 29.2 FPS before
  the fix and approximately 120 FPS in four three-second main menu
  windows afterward, including with the ImGui Console open. Logs are
  `local/task0128-before.log` and `local/task0128-menu-solo.log`.
- Direct Track 12 reached approximately 120 FPS over four three-second
  windows, including with throttle held. Both original title movies and
  the main menu movie played. Logs are `local/task0128-race-solo.log` and
  `local/task0128-menu-solo.log`. Other refresh rates and monitor
  switching were not exercised. No PS2 behavior was reclassified.

To repeat TASK-0128, run `./build.ps1 --run --mvp --debug --jobs 16`
with the complete game source from TASK-0127. Press F8 to navigate to the
main menu and read the frame graph. Add `--race-track=12` for a separate
race measurement. Local timing probes are under `local/task0128-*-pace.gd`.
