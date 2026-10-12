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

Windows debug export: invoke the pinned Godot 4.4.1 console with `--headless --path
godot --export-debug "Windows Debug" ../out/windows-debug/port.exe`. Matching 4.4.1
export templates are required. The GDExtension dependency table copies shared codec
DLLs alongside the extension. Keep the FFmpeg license and exact source/build reference
with distributions and retain replaceable DLLs. Original assets are read externally.
macOS/Linux renderer defaults are configured but builds are not validated here.
