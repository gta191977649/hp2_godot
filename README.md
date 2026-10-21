# HP2 PS2 Reversed

TASK-0071 changes the Windows default to **Vulkan + Forward+**, retaining the
global `./build.ps1 --profile` ImGui profiler. Track11 performance/visual
acceptance and original-runtime verification are tracked separately; see
[migration results and limitations](docs/PERFORMANCE_TASK0071.md).

A native C++ reconstruction of *Need for Speed: Hot Pursuit 2* (PS2), with Godot as the host and rendering backend. The current target is the US PS2 release, `SLUS_203.62`, on Windows x86-64.

The CarViewer **Test drive** closure now targets the user-supplied symbol-rich Debug
`NFSHP2.ELF` separately (TASK-0044). Its canonical original-class implementation,
version binding and remaining limits are documented in [DEBUG_DRIVING_AUDIT.md](docs/DEBUG_DRIVING_AUDIT.md).

Player transmission controls, the centralized driving InputMap and original skid
rendering are described in [PLAYER_DRIVING.md](docs/PLAYER_DRIVING.md) (TASK-0046).
Original wheel smoke/tire-spew and remaining physics work are documented in
[DRIVING_EFFECTS.md](docs/DRIVING_EFFECTS.md) and [DRIVING_GAPS.md](docs/DRIVING_GAPS.md).

TrackViewer, Test drive and the independent single-car scene share the track
renderer. See [TRACK_VIEWER.md](docs/TRACK_VIEWER.md) for controls, Effects gates
and current rendering limits. TASK-0052 is still active; complete original effects
and1080p/60FPS performance have not been established.

## 1. Project Goals

Reconstruct the original game's behavior from binary and asset evidence and run it natively on modern PC hardware. Preserve original logic, names and function addresses where recovered; keep unknown behavior explicit rather than replacing it with guesses or Godot physics.

## 2. Architecture

- **`src/game_core/`** â€” independent C++17 game logic and class/function templates, without Godot dependencies. Confirmed original directories are preserved; classes with unknown paths remain at this directory's root.
- **`src/port/`** â€” native frontend runtime, asset handling and render-command generation.
- **`src/platform/` + `godot/`** â€” platform services, GDExtension adapters and the Godot host. Godot currently handles rendering, windowing, input, audio and frame scheduling; some frontend orchestration also remains in GDScript.
- **`reverse/`, `docs/`, `tasks/`** â€” evidence, recovery records, progress and scoped work items.

Unimplemented recovered paths throw `hp2::NotImplemented` with their original ELF address.

## 3. Progress

**Early development: this is not yet a playable reconstruction of the full game.**

- Partial frontend recovery includes startup/title flow, menu navigation, several options screens, localization, event-tree logic, media playback and profile/save handling using the player's original data.
- Class scaffolding covers 190 class names and 611 member attributions, including 605 throwing whole-entry templates. Typed vehicle-rendering slices coexist with untranslated whole entries; these counts describe missing work, not completed game behavior.
- Racing, vehicle physics, AI and other gameplay systems remain largely untranslated. Native builds and tests pass, but no behavior has been verified against original hardware or PCSX2.

See the generated [progress tracker](docs/PROGRESS.md) for current recovery status, blockers and the next task.

## 4. Build Instructions

On Windows, install a C++17 compiler, CMake 3.24+, Ninja and Python 3. The frontend also requires Godot **4.7.2**, the pinned `godot-cpp` submodule and the pinned FFmpeg **8.1** SDK. Follow the [development guide](docs/DEVELOPMENT.md) for dependency setup and local engine paths.

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

Debug also includes **CarViewer** and **TrackViewer**. Open them
with `carviewer` / `trackviewer` in the F8 console; `viewer.close` returns to a fresh
frontend. In CarViewer, `viewer.load "CARS/911TURBO/GEOMETRY.BIN"` loads original
geometry, shared textures and static parts, with recovered LOD selection. Its material
uses the recovered VU base material slice; full original GS materials remain incomplete.
Viewer controls/FPS are native ImGui, with all 64 original cars and 235 skin records.
TrackViewer loads all 30 shipped tracks, original textures/mips and scenery, and can enter
real-track driving with topology-based wheel materials and static track collision. See
[the track guide](docs/TRACK_VIEWER.md) for version boundaries and remaining render limits.
Both retain explicit asset-free fixtures. See the
[viewer guide](docs/DEBUG_VIEWERS.md) for camera controls, commands and direct launch.

## 5. Docs & Help

- [Development](docs/DEVELOPMENT.md) â€” setup, build, launch and troubleshooting.
- [Architecture](docs/ARCHITECTURE.md) and [decisions](docs/DECISIONS.md) â€” layer ownership and recorded design choices.
- [Progress](docs/PROGRESS.md) and [session log](docs/PROGRESS_LOG.md) â€” current state and its evidence/history.
- [Class templates](docs/CLASS_TEMPLATES.md) and [source paths](docs/CLASS_SOURCE_PATHS.md) â€” existing scaffolding and how to continue recovery.

Before contributing, read [AGENTS.md](AGENTS.md), the binding [AGENT.md](AGENT.md) rules and the selected file under `tasks/`. Record evidence and limitations, preserve existing implementations, and keep proprietary inputs outside Git.

CarViewer's **Preview Mode** continuously orbits while retaining mouse control; launch with
`--preview` to enable it immediately. Vehicle shader sources are centralized in
`src/platform/shaders/` and shared by the runtime backend. See [vehicle rendering](docs/VEHICLE_RENDERING.md)
for recovered material/light scope and remaining reflection limitations.

CarViewer's **Wheel Blur (high speed)** enables tire rotation and the original `WHEEL_BLUR`
overlay. The **Wheel speed (rad/s)** slider underneath adjusts preview angular velocity
from -150 to150: negative reverses, zero stops. Default80 rad/s is a port-side demonstration
input, not an original vehicle physics constant. Original per-wheel strength is
clamp(abs(velocity)*0.02,0,1), with alpha scale90; the blur layer has its own original
non-spinning matrix. Changes update existing resources and continue while F8 is open.
Disabling resets the static pose; selected speed and switch survive car changes.
`--wheel-blur` enables it on direct launch. Original contact/damage gates and physics remain
untranslated. Missing resources fail explicitly; no synthetic blur texture is generated.

Grip/material test controls and original-data boundaries: [DRIVING_GRIP.md](docs/DRIVING_GRIP.md).


Independent single-car runtime (TASK-0051): launch `godot/scenes/race_scene.tscn`
with `--track=11 --car=ELISE`. It uses the same native frame packets and GPU host as
TrackViewer in Debug and Release. See [Track rendering](docs/TRACK_VIEWER.md) for
controls, tests and remaining dynamic environment-capture limitations.


### TASK-0055 rendering optimization

The shared TrackViewer/Test drive/race renderer now defaults to accepted host
resource and particle-buffer reuse, preserving native visibility/LOD/effects and
physics. Full30-track Release1080p paired tests reduce median-route frame time
10.03% and host submission33.16%, with no persistent tail regression. Debug118/118
and Release98/98 checks pass. Loading stalls and original effects gaps remain.
See [results and limits](docs/PERFORMANCE_TASK0055.md). Local DLL/config snapshots
and launchers: `local/task0055/delivery/`. Use
`--track-optimization-profile=baseline` to disable the new policy.

Track rendering uses the original zone/section/tree/instance LOD selection.
TASK0058 retires the optional Godot HLOD backend; `godot_hlod` is rejected.
See [native culling integration](docs/NATIVE_TRACK_CULLING.md).
