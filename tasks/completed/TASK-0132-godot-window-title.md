# TASK-0132 - Godot window title

- Status: completed
- Scope: show the active graphics API and measured FPS in the game window title. Append `Debug Mode` for the debug launcher.
- Acceptance: verify titles for the frontend and direct race, and preserve existing launcher arguments and profile paths.
- Authority: port-side Godot and PowerShell integration.

Godot samples the active graphics driver and updates the measured FPS
once per second. The launcher passes the effective build mode without
changing race arguments or profile configuration. The native title setter
avoids the duplicate debug suffix added by Godot's Window property.

Validated with Godot 4.7.2 stable, CMake 3.29.2, Ninja 1.12.0 and GCC 13.2.0:

- `./build.ps1 --run --mvp --debug --jobs 16` reached the title screen
  and reported `hp2 - (Vulkan) 91 FPS | Debug Mode`.
- `./build.ps1 --run --mvp --race-track=12 --jobs 16` published the race
  scene and reported `hp2 - (Vulkan) 105 FPS`.

Both launcher processes exited 0 on window close. The values were live
startup samples. Additional graphics drivers were not exercised.
