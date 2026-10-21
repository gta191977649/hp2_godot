# C++ platform portability review (TASK-0090)

Reviewed 2026-10-21. This is a source/build review of the current dirty checkout,
not certification that macOS or Linux builds pass. No recovered code was changed.

## Confirmed

- `game_core` and `port_runtime` use C++17; library targets enable position-independent code. Threads use `Threads::Threads`. Compiler warning options distinguish MSVC and other compilers.
- Searches of `src/game_core` and `src/port` found no `godot_cpp` includes or `windows.h` includes. The media translation unit uses FFmpeg, so Godot independence does not mean every source is standard-library-only.
- The two first-party Windows API users found are `src/platform/not_implemented_dialog.cpp` (MessageBox/debug break) and `src/platform/godot_bridge/ps2_render_host.cpp` (per-window DPI). Their Windows headers and calls are guarded by `_WIN32`. Other hosts retain the exception mechanism and Godot screen-scale fallback, respectively; the optional native dialog has no non-Windows UI implementation.
- `CMakeLists.txt:132` already supports non-Windows FFmpeg discovery via pkg-config (`libavcodec>=62`, `libavutil>=60`, `libswscale>=9`). Full Godot builds require this media target. A platform-compatible SDK with the required MV30 decoder must actually be supplied and tested.
- `godot/ps2_port.gdextension` declares Windows x86-64, Linux x86-64 and macOS libraries. CMake suppresses the library prefix and writes to the matching bin directory. Linux ARM64 has no manifest entry.
- The default FFmpeg SDK directory and Godot test executable path in CMake are Windows-specific. `PORT_GODOT_EXECUTABLE` can be overridden; if it does not exist, host tests are not registered. A non-Windows configure can therefore omit host tests without proving they pass.
- Extension dependency copying/manifest declarations cover Windows DLLs only. Linux/macOS dynamic dependency resolution, distribution and macOS signing are not established by this configuration; locally installed libraries may suffice for development but do not establish relocatable packaging.
- The documented normal launcher is `build.ps1`; the direct CMake native presets do not require that wrapper. No `.github/workflows` platform build matrix was present at review time.
- Strict floating-point options (`-ffp-contract=off`, `-fno-fast-math`) are attached to selected UI/render/test sources, not globally to recovered driving logic. Cross-compiler/CPU arithmetic and replay equivalence therefore remain unestablished. This is a validation risk, not a demonstrated numerical mismatch.

## Assessment and unresolved work

The core is structured for portability and has no direct Windows API dependency
in the searched code. The complete application is currently Windows-oriented;
macOS/Linux support is scaffolded but unvalidated. Support for arbitrary platforms,
architectures, endian models or identical floating-point results is not established.
Searches are not a substitute for compiling all translation units on each target.

Next action: on Linux x86-64 and macOS ARM64, provide the pinned godot-cpp checkout,
Ninja/C++17/Python and suitable FFmpeg development libraries, then run:

```sh
cmake --preset native-debug
cmake --build --preset native-debug
ctest --preset native-debug --output-on-failure
cmake --preset godot-debug -DPORT_GODOT_EXECUTABLE=/absolute/path/to/godot
cmake --build --preset godot-debug
ctest --preset godot-debug --output-on-failure
```

Check that host tests were actually registered, run extension/media/render smoke
checks, compare deterministic driving fixtures, and test the packaged application
without developer SDK paths. Add Linux ARM64 manifest entries only when that target
is built and tested. No new behavior tests were written for this documentation-only
review; foreign-host builds and execution are blocked by unavailable hosts.

## Local validation

CMake 3.29.2; Python 3.14.4; pinned godot-cpp checkout
`e4b7c25e721ce3435a029087e3917a30aa73f06b` matches DEVELOPMENT.md.
`ctest --preset native-debug -N` lists 133 existing tests; this enumerates cached
Windows tests and does not execute them or validate other hosts.
