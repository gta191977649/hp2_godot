# Need for Speed: Hot Pursuit 2 — Godot PC Port

A native PC port of the US PlayStation 2 release (`SLUS_203.62`). The C++ game core runs the race logic, physics, AI, and menus. Godot handles graphics and input. The build targets Windows x64, Linux x64, and Apple Silicon Macs.

**You need your own US PS2 game ISO. The download contains no original game data.**

## Requirements

- A GPU that supports Vulkan or DirectX 12 on Windows, Vulkan on Linux, or Metal on Apple Silicon. The default is Vulkan on Windows and Linux and Metal on macOS.
- A complete ISO of the US PlayStation 2 release of *Need for Speed: Hot Pursuit 2*.

## Build from source

If you're using a prebuilt release, skip to [Run](#run).

Run `git submodule update --init --recursive` from the repository root. Install CMake 3.24+, Ninja, Python 3, a native C++17 compiler, Godot 4.7.2, and the shared FFmpeg 8 SDK.

| Platform | Build and run | Build a Release package | Output |
| --- | --- | --- | --- |
| Windows x64 | `.\build.ps1 --run` | `.\build.ps1 --mvp` | `out/mvp/windows-release/HP2.exe` |
| Linux x64 | `bash build_linux.sh --run` | `bash build_linux.sh` | `out/mvp/linux-release/HP2` |
| macOS Apple Silicon | `bash build_macos.sh --run` | `bash build_macos.sh` | `out/mvp/macos-release/HP2.app` |

On Windows, put the Godot console executable in `local/tools/godot-4.7.2/` and the FFmpeg SDK in a subfolder of `local/tools/ffmpeg/`. On Linux and macOS, set `GODOT_BIN` to the Godot executable and `HP2_FFMPEG_SDK` to the SDK directory. Install binutils on Linux and the Xcode Command Line Tools on macOS.

Package builds require export templates. See the [development guide](docs/DEVELOPMENT.md) for the Windows template, the Linux and macOS FFmpeg setup, and package checks.

## Run

1. Put the complete package in a folder you can write to and keep its files together.
2. Open `HP2.exe` on Windows, run `./HP2` on Linux, or open `HP2.app` on macOS. Drop your ISO onto the launcher or use the browse button. The launcher remembers your selection.
3. On first launch, type a profile name and press **Enter**. If you already have multiple profiles, choose one from the list.

To choose a different ISO later, start the game with `-- --select-iso`. On macOS, use `open HP2.app --args -- --select-iso` from the package folder.

## Settings and controls

Open **Options → Graphic** for the render API, window mode, resolution, aspect ratio, frame-rate cap, VSync, anti-aliasing, and FPS counter. Restart the game after changing the render API. In Window mode, the resolution follows the window size. **Auto Fit** fills the window by default. **Restore to Default** resets graphics settings and keeps the selected ISO.

Enable **Options → Display → Subtitle** for English police radio captions during races. This setting defaults to Off and applies to all profiles.

| Action | Default keyboard controls |
| --- | --- |
| Navigate menus | Arrow keys |
| Confirm / Go back | Enter / Backspace |
| Accelerate, brake, and steer | WASD |
| Handbrake / Change camera / Recover car | Space / C / Tab |
| Shift gears / Look back / Horn | Q and E / Shift / H |
| Pause a race | Esc |

Menus also support a mouse and gamepad. With the default gamepad layout, use the
left stick to steer, Cross (A) to accelerate, Square (X) to brake, R1 (RB) for the
handbrake, and Start to pause. Press a button or move a stick to use a connected
gamepad at any time. Keyboard controls are available throughout.
Controller settings are available under **Options**.

In Instant Replay, use Enter or Cross (A) to play or pause, Left/Right to
jump between markers, C or Triangle (Y) to change camera, and Esc or Start
to exit.

## Saves and troubleshooting

`profiles/` holds player saves and `launcher.cfg` holds the ISO path and graphics settings. Both are beside the launcher, or beside `HP2.app` on macOS. Keep them when updating. Use **Options → Save/Load** to create or switch profiles. Save and Autosave write to the active profile.

If the game rejects an ISO, check that it is the complete US PS2 release (`SLUS_203.62`). If you report a problem, include the release version, platform, graphics API, and steps to reproduce it. To capture a log, start the launcher with `--log-file hp2.log` and attach the log.

This port is in beta. See [Third-party notices](third_party/THIRD_PARTY_NOTICES.md) for licenses and acknowledgments.
