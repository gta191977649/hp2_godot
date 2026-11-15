# Need for Speed: Hot Pursuit 2 — Godot PC Port

A Windows x64 port of the US PlayStation 2 release (`SLUS_203.62`). The native C++ game core runs the original race logic, physics, AI, and menus. Godot handles graphics, audio playback, and input.

**You need your own US PS2 game ISO. The download contains no original game data.**

## Requirements

- 64-bit Windows and a GPU with current drivers that support Vulkan or DirectX 12. Vulkan is the default.
- A complete ISO of the US PlayStation 2 release of *Need for Speed: Hot Pursuit 2*.

## Build from source

If you're using a prebuilt release, skip to [Run](#run).

1. From the repository root, run `git submodule update --init --recursive`.
2. Install CMake 3.24 or newer, Ninja, Python 3, and a Windows x64 C++17 toolchain. Put `Godot_v4.7.2-stable_win64_console.exe` in `local/tools/godot-4.7.2/` and extract the shared FFmpeg 8 SDK into a subfolder of `local/tools/ffmpeg/`.
3. Run `.\build.ps1 --run` in PowerShell to build and play from the checkout.

Run `.\build.ps1 --mvp` to create a standalone Windows package in `out/mvp/windows-release/`. The default package uses a custom Godot export template. See the [development guide](docs/DEVELOPMENT.md) for its build dependencies and the official-template option.

## Run

1. Extract the entire release into a folder you can write to. Keep `HP2.exe`, `HP2Runtime.exe`, the PCK, and the DLLs together.
2. Open `HP2.exe`. Drop your ISO onto the launcher or use the browse button. The launcher remembers your selection.
3. On first launch, type a profile name and press **Enter**. If you already have multiple profiles, choose one from the list.

To choose a different ISO later, run this from the game folder:

```powershell
.\HP2.exe -- --select-iso
```

## Settings and controls

Open **Options → Graphic** for the render API, window mode, resolution, aspect ratio, frame-rate cap, VSync, anti-aliasing, and FPS counter. Restart the game after changing the render API. In Window mode, the resolution follows the window size. **Auto Fit** fills the window by default. **Restore to Default** resets graphics settings and keeps the selected ISO.

| Action | Default keyboard controls |
| --- | --- |
| Navigate menus | Arrow keys |
| Confirm / Go back | Enter / Backspace |
| Accelerate, brake, and steer | WASD or arrow keys |
| Handbrake / Change camera / Recover car | Space / C / Tab |
| Shift gears / Look back / Horn | Q and E / Shift / H |
| Pause a race | Esc or Backspace |

Menus also support a mouse and gamepad. Controller settings are available under **Options**.

## Saves and troubleshooting

`profiles/` holds player saves and `launcher.cfg` holds the ISO path and graphics settings. Both are beside `HP2.exe`. Keep them when updating. Use **Options → Save/Load** to create or switch profiles. Save and Autosave write to the active profile.

If the game rejects an ISO, check that it is the complete US PS2 release (`SLUS_203.62`). If you report a problem, include the release version, graphics API, and steps to reproduce it. To capture a log, launch with `.\HP2.exe --log-file hp2.log` and attach `hp2.log`.

This port is in beta. See [Third-party notices](third_party/THIRD_PARTY_NOTICES.md) for licenses and acknowledgments.
