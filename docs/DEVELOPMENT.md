# Development guide

## Prerequisites and pinned dependencies

Windows x86-64: Git, CMake >=3.24, Ninja, and a C++17 compiler (MSVC developer
shell or MinGW-w64). For the bridge add Python 3 and Godot 4.4.1.stable standard
edition. godot-cpp is pinned as documented in DECISIONS.md. Ghidra/Java, its R5900
extension, GhidraMCP, and PCSX2 are needed for later analysis/comparison, not builds.
See PROGRESS.md for tools actually detected. Python's WindowsApps alias may not
be a real interpreter; use `py -3` or an explicit Python executable.

Run from repository root in PowerShell:

```powershell
git submodule update --init --recursive
git -C third_party/godot-cpp rev-parse HEAD
cmake --preset native-debug
cmake --build --preset native-debug
ctest --preset native-debug
```

Expected submodule commit: `e4b7c25e721ce3435a029087e3917a30aa73f06b`.
Do not use `git submodule update --remote`. A fresh clone should use
`git clone --recurse-submodules <your-repository-url>`. No remote is set yet.

In a Visual Studio x64 Native Tools shell these Ninja presets use MSVC; otherwise
CMake chooses the compiler on PATH. Do not reuse a build directory across
different compilers; use a separate binary directory or local user preset.

## Optional bridge

```powershell
cmake --preset godot-debug
cmake --build --preset godot-debug --parallel 4
ctest --preset godot-debug
```

If interpreter discovery selects a broken alias, pass the real executable:

```powershell
$portPython = py -3 -c "import sys; print(sys.executable)"
cmake --preset godot-debug "-DPython3_EXECUTABLE=$portPython"
```

The DLL is written to `godot/bin/ps2_port.dll`; all configured library variants
use the same path, so rebuild the intended configuration before launching.
This initial bridge uses godot-cpp's CMake Debug configuration; a production packaging
task must define and validate separate debug/release binaries. With MinGW the
extension statically links GCC/C++/pthread runtimes to avoid host DLL discovery
issues. MSVC builds may require the corresponding Visual C++ runtime.

Download the pinned engine from the official archive linked in DECISIONS.md.
Use an explicit local path, not a guessed system install:

```powershell
$portGodot = 'C:\Tools\Godot-4.4.1\Godot_v4.4.1-stable_win64_console.exe'
& $portGodot --version
& $portGodot --headless --path godot --editor --import
& $portGodot --headless --path godot res://scenes/main.tscn -- --smoke-test
& $portGodot --path godot --editor
```

Initialization downloaded an isolated portable copy to
`local/tools/godot-4.4.1/` (ignored). On this machine use:

```powershell
$portGodot = Join-Path $PWD 'local/tools/godot-4.4.1/Godot_v4.4.1-stable_win64_console.exe'
& $portGodot --headless --path godot --editor --import
& $portGodot --headless --path godot res://scenes/main.tscn -- --smoke-test
```

The smoke scene must exit 0 and print `PORT_SMOKE_OK: 42`, with no extension or
script errors. Import before smoke so the extension list is registered. Headless
smoke validates loading/calls; it does not prove visual rendering or game behavior.
Running without `--smoke-test` keeps the sample label visible. Building just core
does not make the Godot scene runnable; the bridge DLL must exist.

## Normal workflow

### Project-root build entry point (TASK-0024)

```powershell
./build.ps1                         # configure and build godot-debug
./build.ps1 --run                   # build successfully, then start the frontend
./build.ps1 --clean --run            # clean generated outputs, build, then run
./build.ps1 --clean --preset native-debug --jobs 4
./build.ps1 --preset=godot-release --jobs=4
./build.ps1 --help
```

GNU-style options work in PowerShell7 and Windows PowerShell5.1. Run mode requires
the local pinned engine described above; native-debug cannot launch a Godot scene.
`--clean` invokes CMake's generator clean target in the selected preset's build
directory, then rebuilds. It preserves arbitrary user files and the configuration;
it does not recursively delete build/, third_party/, assets, or local/. If a running
frontend holds ps2_port.dll open, close it before rebuilding. Build errors prevent launch.

Follow AGENTS.md startup/completion protocols and a task's acceptance criteria.
Keep implementation, evidence, test vectors, and current status together in a
scoped change. Adding recovered functions follows REVERSE_NOTES.md. Add source
files explicitly to CMake; avoid recursive globbing and huge shared source files.
Use narrow interfaces and headless tests before integrating the host. Do not
introduce rendering/gameplay systems merely to make placeholders look complete.

GitHub Issues can mirror local task IDs after a repository remote and project
policy are supplied. Record the issue URL in the task; preserve repository state
so continuation does not depend on remote service availability.

## Private inputs and safety

Put binaries, Ghidra projects, PCSX2 captures, and proprietary resources under
ignored `private/` or outside the repository. Keep only hashes, analysis records,
synthetic fixtures, and shareable scripts in Git. Inspect `git diff --cached` and
`git status` before any commit: ignore rules cannot recognize arbitrary asset
formats, and previously tracked files are not protected by ignores.

## Main-loop observer (TASK-0010)

Run `./tools/run_main_loop_observer.ps1 -Build` to build the bridge and open the separate scene; `-Smoke` runs its headless acceptance checks. Default demo remains unchanged. `py -3 reverse/scripts/validate_main_loop.py --metadata-only` checks public scheduling catalogs without the original ELF/Ghidra; full validation needs those local prerequisites. See MAIN_LOOP_STRUCTURE.md for module registration and frame-wait contracts.

### TASK-0016 frontend runtime and export

Install the pinned host codec with `py -3 tools/setup_ffmpeg.py`; its shared LGPL
DLLs provide MV30 without preconverting original assets. Configure ZZDATA using the
existing local asset config or HP2_PS2_ASSET_ROOT (never commit that config/assets).
Run `./tools/run_frontend.ps1 -Build` for startup, or `-Screen 0` for direct main-menu
inspection. `-RuntimeSmoke`, `-ActivationSmoke`, `-AnimationSmoke` validate integration.

### TASK-0018 native render path

`scenes/frontend_native.tscn` is the main scene and the only front-end host: the native
runtime plus the RenderingServer backend. It takes `--screen=`, `--screen-name=`,
`--scale=`, `--info`, `--capture=`, `--capture-after=` and `--menu-events=`.

The front end's behaviour checks are native and run under CTest, not Godot:
`frontend_runtime_tests` covers asset resolution, the main menu, activation routing,
submenu history, Audio/Display clipping and scrolling, the highlight ghost, localisation,
menu samples, the jukebox and the event tree. It skips with a message when the ZZDATA root
is not configured. Only two host checks remain in Godot, because only they need it:
`--native-render-smoke` (every screen builds and submits a frame) and `--runtime-smoke`
(MV30 decode into a texture, PCM into AudioStreamWAV, the recovered title time gate).

```powershell
$portGodot = Join-Path $PWD 'local/tools/godot-4.4.1/Godot_v4.4.1-stable_win64_console.exe'
& $portGodot --headless --path godot --editor --import
& $portGodot --headless --path godot res://scenes/frontend_native.tscn -- --native-render-smoke
& $portGodot --path godot res://scenes/frontend_native.tscn -- --screen=0
```

The native frontend shows `FPS / ms` at the top right in C++ Debug builds, independently
of `--info` and the console. Release hides the readout. F8 toggles the debug console;
TAB completes commands and Up/Down recall history. Old TAB, Page Up/Down and bracket
menu shortcuts are removed from the native frontend. The legacy comparison scene
retains its old overlay and is not the supported entry point.

## Port debug console (TASK-0031)

`HP2_ENABLE_DEBUG_CONSOLE` defaults ON for Godot Debug and OFF for Release/native.
CMake cache values persist; pass `-DHP2_ENABLE_DEBUG_CONSOLE=ON` or `OFF` explicitly
when changing an existing build directory. FPS uses `HP2_DEBUG_BUILD`, independently.
Native tests build the same ImGui console library without Godot, even with the feature OFF.
No upstream example or graphics backend is linked.

The console starts hidden; F8 ignores key repeat. While open, keyboard/text/mouse input
is consumed before game dispatch, but frame updates continue. It provides `help`, `clear`,
`filter <text>`, `status`, `info on/off`; info controls only the detailed panel. Frontend
errors and untranslated-path notices are mirrored into the console and still printed.
Logs are collected while hidden and bounded to 2,048 displayed items and 2,048 pending
entries. Command history uses the upstream bounded ring. Settings are stored in
`user://debug/imgui.ini`, not the source directory. Clipboard uses Godot DisplayServer.
Native command/state/frame access is in `src/game_core/extensions/debug_console.hpp`;
only `log()` is thread-safe. All other console operations belong to the host thread.

Asset-free smoke (with the extension built):

```powershell
& $portGodot --headless --path godot res://scenes/debug_console_smoke.tscn
& $portGodot --path godot --rendering-method gl_compatibility res://scenes/debug_console_smoke.tscn -- --capture=D:/temp/console.png
cmake --preset godot-debug -DHP2_ENABLE_DEBUG_CONSOLE=OFF
cmake --build --preset godot-debug --target ps2_port
# Run smoke again: Debug FPS remains visible with the console unavailable.
cmake --preset godot-debug -DHP2_ENABLE_DEBUG_CONSOLE=ON
cmake --build --preset godot-debug --target ps2_port
```

The synthetic scene needs no proprietary data. It checks input routing, F8 repeat,
old shortcuts, commands, continued updates and resize positioning. Native `debug_console`
tests exercise log capacity, completion, history, clipping and vertex/index offsets.


`--native-render-smoke` must print `NATIVE_RENDER_SMOKE_OK` and exit 0; it builds and
submits a frame for every screen. `./tools/run_frontend.ps1` drives the scene: `-Build`,
`-RenderSmoke`, `-RuntimeSmoke`, `-Screen N`, `-Capture path`.

Windows debug export: invoke the pinned Godot 4.4.1 console with `--headless --path
godot --export-debug "Windows Debug" ../out/windows-debug/port.exe`. Matching 4.4.1
export templates are required. The GDExtension dependency table copies shared codec
DLLs alongside the extension. Keep the FFmpeg license and exact source/build reference
with distributions and retain replaceable DLLs. Original assets are read externally.
macOS/Linux renderer defaults are configured but builds are not validated here.

## Debug asset viewers (TASK-0032)

See [DEBUG_VIEWERS.md](DEBUG_VIEWERS.md). The shared DebugTools autoload now owns the
console host, logs and FPS across frontend/viewer scene changes. Native frontend rendering
retains its own Ps2RenderHost, independent of the overlay. The viewer scenes need no
original assets. They are Debug-only; console-off Debug still supports their panels.

```powershell
& $portGodot --path godot res://scenes/debug/car_viewer.tscn
& $portGodot --path godot res://scenes/debug/track_viewer.tscn
ctest --preset godot-debug -R 'viewer|debug_console'
```

`debug_viewers_host` uses a clearly named asset-free frontend fixture for return-path tests;
the application default still returns to frontend_native.tscn and its normal asset setup.
`--viewer-smoke` starts a persistent test driver; `--viewer-capture=<absolute.png>` captures
a directly launched viewer and exits. These flags are test tools, not loader features.
The viewer smoke checks command queueing, repeated transitions, synthetic geometry,
camera motion/reset, unavailable loading, input isolation, continued updates and log/session
persistence. Existing debug_console_host checks TAB/history routing, FPS and resize behavior.

Build configurations sequentially: their DLL output path is shared. Restore
`cmake --preset godot-debug -DHP2_ENABLE_DEBUG_CONSOLE=ON` and rebuild after testing
Release or console-off Debug. An existing process must be closed before replacing its DLL.

### Original vehicle checks

With the normal original-asset configuration, CarViewer supports
`viewer.load "CARS/911TURBO/GEOMETRY.BIN"` and the equivalent BMWM5COP/4DR_SEDAN paths.
See DEBUG_VIEWERS.md for LOD, part, wireframe and bounds controls, and VEHICLE_RENDERING.md
for the recovered base-material scope, explicit inspection mode and remaining GS
destination-access blending work. Viewer UI and FPS use a shared native ImGui frame;
console-off Debug retains viewer input. The default backend is Compatibility to preserve
the encoded GS RGB blend domain.

```powershell
ctest --preset native-debug -R 'vehicle_|car_render_'
ctest --preset godot-debug -R 'vehicle_|car_render_|debug_viewers_host'
& $portGodot --path godot res://scenes/debug/car_viewer.tscn -- --car-load=CARS/911TURBO/GEOMETRY.BIN
```

The native tests always run synthetic malformed-data checks. Configured assets enable
three original geometry/model/texture checks; invalid configured roots fail. The dedicated
car host smoke exercises real loading and resource cleanup, and rejects native loading in
Release. All configurations share the DLL path and must be built/tested sequentially.


TASK-0033 validation adds an optional windowed RGB/alpha/depth oracle. See DEBUG_VIEWERS.md
for PORT_ENABLE_GPU_TESTS; the default remains OFF for headless CTest. Smoke ImGui settings
use a separate profile, preserving normal viewer layout and console history. Default
working configuration is Debug with the console enabled.

## Vehicle shader sources (TASK-0034)

Edit shader text only under src/platform/shaders. CMake reconfigures and embeds the text
into generated/hp2_embedded_shaders.hpp; rebuild ps2_port after shader edits. No shader
source file needs copying into the Godot project or exports. Ps2VehicleRenderer is the
shared runtime backend; Ps2CarViewer inherits it. Preview uses `--preview` or its checkbox.
Run the optional real Compatibility numeric GPU oracle with PORT_ENABLE_GPU_TESTS=ON;
restore OFF for the default headless-friendly test preset after validation.

## TASK-0035 validation and preview controls

Wheel speed is a port-side angular-velocity slider beneath Wheel Blur, in rad/s. It updates
existing tire/blur transforms and shader intensity; it does not reload resources. Native
`vehicle_model` covers zero/reverse speed, phase wrapping, opacity and all McLaren mirror
skins; `debug_console` drags the actual ImGui slider. `car_render_host` checks live updates
and console coexistence. Enable `PORT_ENABLE_GPU_TESTS=ON` for real Compatibility framebuffer
ST/additive/material checks. `car_material_gpu_smoke.tscn` uses Ps2VehicleRenderer and can
also run against Release; Debug-only viewer loading remains disabled there.

Run `cmake --preset godot-debug -DHP2_ENABLE_DEBUG_CONSOLE=ON -DPORT_ENABLE_GPU_TESTS=ON`,
then build and CTest. Repeat console OFF host tests and the Release preset sequentially.
Delete the generated `godot/bin/ps2_port.dll` before switching build trees to force relink;
close test windows first. An existing user window may keep an older mapped DLL alive.
Restore Debug/console ON/GPU tests OFF before delivery. No PS2 equivalence follows from
these checks; active TASK-0035 tracks original comparison and remaining game environment work.

## Vehicle surface capacity regression (TASK-0036)

Run `ctest --test-dir build/godot-debug -R car_surface_batches --output-on-failure`. The
asset-dependent test skips with77 when no original root is configured. It scans all
available catalog IDs and verifies every drawable native surface survives the per-mesh
256-surface capacity split. The same scene can run windowed with Compatibility, or on
Release through the common Ps2VehicleRenderer backend. It also checks async/warm cache,
part visibility, wireframe, LOD copies and complete cleanup. No Godot engine limit is raised.


## TASK-0042 naming/layout regeneration

Use `py -3 reverse/scripts/adopt_task0042_names.py --write`, then
`py -3 reverse/scripts/generate_class_templates.py --write` and
`py -3 reverse/scripts/generate_progress.py`. TASK-0041 adoption is historical;
its write command now refuses to overwrite migrated TASK-0042 layout. One-time
migration scripts are audit tools for the original source layout and must not
be rerun on the current tree. See DEBUG_CALLCHAIN_RECOVERY.md for evidence and
exact binary prerequisites. CTest includes finite naming-proof, adoption-section
preservation and projection-body/overload-address regression checks.
