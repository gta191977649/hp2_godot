# HP2 Godot MVP

The repository contains the Windows C++ game core from `D:/dev/hp2_ps2_rev`
and a Godot 4.7.2 project for the race and frontend integration.

## Build and launch

Use CMake 3.24+, Ninja, a Windows x64 C++17 toolchain. Place the Godot 4.7.2 console executable under
`local/tools/godot-4.7.2/`.

```powershell
./build.ps1 --run
```

The default launch opens the original title sequence in Godot.
The build uses the shared FFmpeg 8 SDK under `local/tools/ffmpeg`.
For another installation, configure CMake with `-DHP2_FFMPEG_SDK=<sdk-directory>`.
Godot plays the original AVI directly from the game asset source through
the FFmpeg MV30 decoder. Playback needs no generated media files.

Use the arrow keys and Enter in the menus. World Racing leads to Quick
Race or the Championship event tree (Track 12). Hot Pursuit → Ultimate
Racer leads to the first event, Lotus Elise Delivery (Track 13).
Use WASD or the arrow keys to drive, Space for handbrake and Esc to pause.
In the frontend, Esc exits the application.
The default race minimap rotates with the player's car. Single-player
fixed-map choices are available in Display Options.
`./build.ps1 --run --race-track=12` opens the diagnostic race directly.

For race integration, use a complete extracted game directory, a PS2 disc image, or a
`ZZDATA.BIN` archive with its matching directory file. The extracted tree
must include the original sound banks. The asset path can also come from
`config/ps2_assets.local.json` (see `config/ps2_assets.example.json`).
Use `HP2_PROFILE_DIR` to select a profile directory. By default the game
uses the Godot project's `user://profile` directory.

`./build.ps1` stages the project in `build/godot-mvp-release/mvp-project`.
`./build.ps1 --run --mvp --debug --jobs 16` builds and runs the Debug
configuration. The ImGui Console and live frame-time graph open by
default. The grave/tilde key toggles the console and gives the game its keyboard input
back. The graph stays visible throughout the frontend and race.
The window title shows the active graphics API and live FPS, for example
`hp2 - (Vulkan) 120 FPS`. Debug launches append ` | Debug Mode`.
Godot uses VSync to pace the entire game to the current display refresh.
The donor simulation runs on its own clock.
See [TASK-0125](tasks/active/TASK-0125-godot-frontend.md) for frontend validation and open coverage.
See [TASK-0126](tasks/active/TASK-0126-authored-frontend-and-race-ui.md)
for original font, texture and in-race UI coverage.
