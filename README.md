# HP2 PS2 Reversed

A native C++ reconstruction of *Need for Speed: Hot Pursuit 2* (PS2), with Godot as the host and rendering backend. The current target is the US PS2 release, `SLUS_203.62`, on Windows x86-64.

## 1. Project Goals

Reconstruct the original game's behavior from binary and asset evidence and run it natively on modern PC hardware. Preserve original logic, names and function addresses where recovered; keep unknown behavior explicit rather than replacing it with guesses or Godot physics.

## 2. Architecture

- **`src/game_core/`** — independent C++17 game logic and class/function templates, without Godot dependencies. Confirmed original directories are preserved; classes with unknown paths remain at this directory's root.
- **`src/port/`** — native frontend runtime, asset handling and render-command generation.
- **`src/platform/` + `godot/`** — platform services, GDExtension adapters and the Godot host. Godot currently handles rendering, windowing, input, audio and frame scheduling; some frontend orchestration also remains in GDScript.
- **`reverse/`, `docs/`, `tasks/`** — evidence, recovery records, progress and scoped work items.

Unimplemented recovered paths throw `hp2::NotImplemented` with their original ELF address.

## 3. Progress

**Early development: this is not yet a playable reconstruction of the full game.**

- Partial frontend recovery includes startup/title flow, menu navigation, several options screens, localization, event-tree logic, media playback and profile/save handling using the player's original data.
- Class scaffolding covers 190 class names and 609 member attributions, including 603 throwing templates. These counts describe structure and missing work, not completed game behavior.
- Racing, vehicle physics, AI and other gameplay systems remain largely untranslated. Native builds and tests pass, but no behavior has been verified against original hardware or PCSX2.

See the generated [progress tracker](docs/PROGRESS.md) for current recovery status, blockers and the next task.

## 4. Build Instructions

On Windows, install a C++17 compiler, CMake 3.24+, Ninja and Python 3. The frontend also requires Godot **4.4.1**, the pinned `godot-cpp` submodule and the pinned FFmpeg **8.1** SDK. Follow the [development guide](docs/DEVELOPMENT.md) for dependency setup and local engine paths.

Provide your own original PS2 `ZZDATA` directory through `HP2_PS2_ASSET_ROOT`, or copy [the configuration example](config/ps2_assets.example.json) to `config/ps2_assets.local.json` and set its `root`. Game binaries and assets are not distributed with this project.

From the repository root in PowerShell:

```powershell
git submodule update --init --recursive
./build.ps1                  # Build the Godot frontend
./build.ps1 --run            # Build and launch
ctest --preset godot-debug   # Run tests
```

Build and test the independent core without Godot:

```powershell
./build.ps1 --preset native-debug
ctest --preset native-debug
```

Close a running frontend before rebuilding its DLL. Some integration tests require the configured original assets; see the development guide for details.

Debug builds show FPS / frame milliseconds at the top right. **F8** toggles the debug
console (when enabled); TAB completes commands and Up/Down recall history. Commands:
`help`, `clear`, `filter <text>`, `status`, `info on/off`. The console captures input while
the game continues updating. TAB no longer toggles information; Page Up/Down and brackets
no longer browse screens. `--info` enables only the detailed information panel.

Debug also includes asset-free **CarViewer** and **TrackViewer** scaffolds. Open them
with `carviewer` / `trackviewer` in the F8 console; `viewer.close` returns to a fresh
frontend. `viewer.load "<ZZDATA-relative-path>"` records a request and reports
`Loader not implemented`; no original car or track is loaded. See the
[viewer guide](docs/DEBUG_VIEWERS.md) for camera controls, commands and direct launch.

## 5. Docs & Help

- [Development](docs/DEVELOPMENT.md) — setup, build, launch and troubleshooting.
- [Architecture](docs/ARCHITECTURE.md) and [decisions](docs/DECISIONS.md) — layer ownership and recorded design choices.
- [Progress](docs/PROGRESS.md) and [session log](docs/PROGRESS_LOG.md) — current state and its evidence/history.
- [Class templates](docs/CLASS_TEMPLATES.md) and [source paths](docs/CLASS_SOURCE_PATHS.md) — existing scaffolding and how to continue recovery.

Before contributing, read [AGENTS.md](AGENTS.md), the binding [AGENT.md](AGENT.md) rules and the selected file under `tasks/`. Record evidence and limitations, preserve existing implementations, and keep proprietary inputs outside Git.
