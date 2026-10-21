# Development guide

TASK0071 selects Vulkan + Forward+ on Windows. Normal run/editor/profile paths
share the project configuration; automatic D3D12/OpenGL fallback is disabled.
Use explicit `--rendering-method gl_compatibility --rendering-driver opengl3`
only for diagnostic comparisons. See [Track11 measurement commands and current
visual limitations](PERFORMANCE_TASK0071.md). A successful launch does not prove
PS2 equivalence or a performance gain.

## Prerequisites and pinned dependencies

Windows x86-64: Git, CMake >=3.24, Ninja, and a C++17 compiler (MSVC developer
shell or MinGW-w64). For the bridge add Python 3 and Godot 4.7.2.stable standard
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
$portGodot = 'C:\Tools\Godot-4.7.2\Godot_v4.7.2-stable_win64_console.exe'
& $portGodot --version
& $portGodot --headless --path godot --editor --import
& $portGodot --headless --path godot res://scenes/main.tscn -- --smoke-test
& $portGodot --path godot --editor
```

TASK-0069 downloaded the official checksum-checked portable runtime to
`local/tools/godot-4.7.2/` (ignored). On this machine use:

```powershell
$portGodot = Join-Path $PWD 'local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe'
& $portGodot --headless --path godot --editor --import
& $portGodot --headless --path godot res://scenes/main.tscn -- --smoke-test
```

The smoke scene must exit 0 and print `PORT_SMOKE_OK: 42`, with no extension or
script errors. Import before smoke so the extension list is registered. Headless
smoke validates loading/calls; it does not prove visual rendering or game behavior.
Running without `--smoke-test` keeps the sample label visible. Building just core
does not make the Godot scene runnable; the bridge DLL must exist.

## Normal workflow

`./build.ps1 --run` now selects Godot4.7.2. Use `--profile` for built-in GPU
profiling plus the global live ImGui panel and16 HP2 custom Performance monitors, or `--editor` and F5 for
Debugger > Profiler / Visual Profiler / Monitors. The window title reports the
active API. See [profiling results and limits](PERFORMANCE_TASK0069.md).
The global script is `profile_debug.gd`; see [live graphs and function scopes](PROFILE_DEBUG.md).

Official Windows archive SHA256:
`731980f9608d61333e5baf54a2ef17210acc7a538446c0cb9969f002aca1e953`.
The pinned godot-cpp4.4 minimum ABI remains compatible in the targeted4.7.2
tests; the runtime upgrade does not change the submodule pin. Matching4.7.2
export templates are required for packaging, not ordinary editor/runtime tests.

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
$portGodot = Join-Path $PWD 'local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe'
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

Windows debug export: invoke the pinned Godot 4.7.2 console with `--headless --path
godot --export-debug "Windows Debug" ../out/windows-debug/port.exe`. Matching 4.7.2
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


## TASK-0043 current regeneration

Use the ordered commands in CLASS_MAPPING_COMPLETION.md. New canonical-member and current projection-binding registries extend historical TASK-0042 evidence; do not run one-time migration scripts. Normal generators preserve edited bodies and explicit throwing gaps. CTest now checks complete member coverage, scoped identifier adoption, whole-project function-body preservation and its pinned duplicate-stub removals. Existing TASK-0041/0042 naming catalogues remain historical baselines.


## TASK-0052 rendering validation (in progress)

Use separate `PORT_GODOT_PROJECT_DIR` and `PORT_GODOT_BIN_DIR` staging roots for
Debug and Release to avoid replacing a DLL mapped by a live window. Staged scenes,
scripts, source and numeric test fixtures must resolve to the current checkout.
Run GPU tests sequentially; do not compile or run another renderer during timed
performance acceptance. `track_performance.gd` launches the independent race scene,
requires a real GPU window and records all catalog tracks unless restricted:

```powershell
& <Godot executable> --path <staged project> --script res://scripts/runtime/track_performance.gd -- --performance-track=11 --performance-samples=300 --performance-output=D:/dev/hp2_godot/local/task0052/performance.json
ctest --test-dir build/godot-debug -R 'track_(command_batches|mesh_batch_host|postprocess_gpu|postprocess_lifecycle_gpu|pipeline_gpu)' --output-on-failure -j 1
```

The route holds the car at its recovered start and moves the camera continuously.
It is a reproducible host performance probe, not an original-game comparison.
Report wall-frame P95/P99, loading stalls, CPU submission, GPU timing limitations,
draw calls and memory. Original effect permissions and automatic view distance
must remain enabled during acceptance. Captures and reports stay under ignored
`local/`; no original assets or ELF files belong in Git.


TASK0055 deterministic comparison uses the same script for frozen TASK0054 baseline
and current candidate; keep binaries in isolated projects. Use the real window,
Compatibility,1920x1080,120 warm frames,600 samples,3 repeats and all30 tracks.
No other GPU tests/builds may run during timed acceptance. Visual time is1/60,
physics is paused at the original start and the camera follows a fixed route.
GPU timing is unavailable without fresh scheduled-view metadata/valid timers.

```powershell
& <Godot executable> --audio-driver Dummy --path <project> --rendering-method gl_compatibility --script res://scripts/runtime/track_performance.gd -- --track-optimization-profile=optimized --performance-output=D:/dev/hp2_godot/local/task0055/optimized/all30.json
py -3 reverse/scripts/compare_task0055_performance.py local/task0055/final-baseline/all30.json local/task0055/final-optimized/all30.json --output local/task0055/comparison.json
```

Runtime policy can be selected with --track-optimization-profile=baseline|optimized.
Individual --track-opt-resource_reuse=true|false and the other dictionary keys
particle_mesh_reuse/instancing/occlusion/opaque_queue are experimental overrides.
Unknown names/non-Boolean values fail explicitly. Call configure_optimizations
before request_track; loaded reconfiguration is rejected atomically. Inspect
get_optimization_state for effective flags, cache/buffer counters and limitations.
Do not enable MultiMesh by default: Track24 exact color mismatch remains unresolved.


TASK-0055 acceptance completed:30 tracks,120 route-warmup/600 measured frames,
3 repeats in both frozen54/final55 Release1080p runs. Native packet/pose, original
effect permissions and draw-call distributions match. Median-route improvements:
wall10.03%, CPU prepare27.62%, host submission33.16%; no persistent>5% wall median/
P95/P99 regression. GPU timers unavailable; loading stalls remain. The accepted
resource/particle-reuse profile is now the common project default. Distinct
material identity retained; MultiMesh/occlusion/opaque remain experimental/off.
See [performance report](PERFORMANCE_TASK0055.md). Override at launch with
--track-optimization-profile=baseline. No original-console Verified promotion.

TASK0056 validation: `ctest --test-dir build/godot-debug -R '^static_lods' --output-on-failure`
and `py -3 reverse/scripts/audit_task0056_review.py`. Enable PORT_ENABLE_GPU_TESTS to
register track_hlod_gpu/race_hlod_gpu. GPU checks must run sequentially. Performance
script `track_hlod_performance.gd` additionally supports `--performance-driving`
(native constant-throttle movement including full skid/render presentation), and
records actual engine primitives and selected backend. Keep all original effects;
compare the same final binary's original/godot_hlod modes in exclusive GPU runs.

## TASK-0058 supersedes host HLOD

User revoked Godot-managed LOD. Only original production section/tree/instance
selection is accepted; main and independent capture views share the native runtime.
Show LOD remains debug-only. Prior TASK0056 sections are historical. No generated
low meshes or physics changes. See [native visibility integration](NATIVE_TRACK_CULLING.md)
and E0058R01 for evidence, tests, counters and verification limits.

Rendering optimization policy (TASK-0066, explicit user requirement): implement global rendering-state rules, not hardcoded track-ID allowlists. Track IDs are appropriate in test fixtures and measurements. Report unresolved image differences without silently disabling an individual track.


TASK-0072 standalone threading experiments: ./build.ps1 --run --profile
--render-prepare-workers=2 --render-thread=safe (or4/separate). Defaults0/Safe.
The editor rejects Separate; no default promotion is implied. See
[thread integration](RENDER_PREPARATION_TASK0072.md).


TASK0072 CPU submission/depth experiments are explicit build entry-point flags:
`./build.ps1 --run --profile --render-prepare-workers=2 --render-thread=safe
--track-submit-math=true --track-vehicle-depth-staging=true
--track-depth-discovery-cache=true` (one command line). All three candidates default
false. This is an experimental configuration, not an accepted60FPS preset.
For isolated visual checks use `tools/run_task0072_worker_parity.ps1` with
`-SubmitRepair -PrepareWorkers 2 -RenderThread safe`. Preserve the workspace DLL while captures run.

## Automatic progress tracking

TASK-0073: run `./tools/track_progress.ps1` once to refresh generated progress after
stable saves. Builds refresh once automatically. `./tools/track_progress.ps1 -Check`
checks both the Markdown tables and `reverse/progress.json` offline. MCP refresh is
read-only and optional for local statistics; see [usage and limits](AUTO_PROGRESS.md).


TASK-0075 wheel particle tests and native/host/GPU reproduction commands are in
[DRIVING_EFFECTS.md](DRIVING_EFFECTS.md). Original resources and the SHA-bound
Debug ELF remain required for integration tests; no generated assets are supplied.


TASK-0085 GPU particles remain opt-in: --particle-backend=godot_gpu (or
hp2/rendering/particle_backend="godot_gpu"). Default is native; extended
performance testing was stopped by the user. TASK-0086 original light flare
attachments are independent of this switch. For targeted regression commands see
PERFORMANCE_TASK0085.md and VEHICLE_LIGHT_FLARES.md. Do not infer PS2 equivalence
from PC/GPU smoke results.
