# HP2 Godot MVP

The repository contains the Windows C++ game core from `D:/dev/hp2_ps2_rev`
and a Godot 4.7.2 project for the Track 12 integration.

## Build and launch

Use CMake 3.24+, Ninja, a Windows x64 C++17 toolchain. Place the Godot 4.7.2 console executable under
`local/tools/godot-4.7.2/`.

```powershell
./build.ps1 --run
```

The default launch opens the original title sequence in a Godot
`VideoStreamPlayer`. Set `HP2_FFMPEG` to an FFmpeg build with MV30, Theora
and Vorbis support, or place it under `local/tools/ffmpeg` for the launcher
to discover. On first play in each session FFmpeg prepares the original
AVI as a temporary Ogg stream.

Use the arrow keys and Enter in the menus. World Racing leads to Quick
Race or the Championship event list. Track 12 is the supported race.
Use WASD or the arrow keys to drive, Space for handbrake and Esc to quit.
`./build.ps1 --run --race-track=12` opens the diagnostic race directly.

For race integration, use a complete extracted game directory, a PS2 disc image, or a
`ZZDATA.BIN` archive with its matching directory file. The extracted tree
must include the original sound banks. The asset path can also come from
`config/ps2_assets.local.json` (see `config/ps2_assets.example.json`).
Use `HP2_PROFILE_DIR` to select a profile directory. By default the game
uses the Godot project's `user://profile` directory.

`./build.ps1` stages the project in `build/godot-mvp-release/mvp-project`.
`./build.ps1 --debug` builds and runs the Debug configuration.
See [TASK-0125](tasks/active/TASK-0125-godot-frontend.md) for frontend validation and open coverage.
