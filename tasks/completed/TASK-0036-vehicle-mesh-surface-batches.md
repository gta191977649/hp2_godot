# TASK-0036 - Vehicle mesh surface capacity

- Status: completed
- Scope: port-side Godot vehicle upload batching, cache, LOD copies and regressions.
- Preserve all existing uncommitted work; no original behavior recovery/status changes.

## Acceptance

- [x] Keep every drawable original strip/material within Godot per-mesh surface limits.
- [x] Preserve part transform, wheel animation, reflection, visibility, bounds and LOD copies.
- [x] Exercise over-limit original vehicles, warm loads and asynchronous uploads.
- [x] Build/test, document results and regenerate progress; no proprietary files added.

Next: split oversized solids across child MeshInstance3D batches, retain one parent per
native part, and cache complete ordered batches. This adapts the host capacity only.

## Findings

The pinned generated RenderingServer API declares MAX_MESH_SURFACES=256. The original
adapter assigned one original solid to one ArrayMesh, so strips beyond256 caused Godot
rejection and incomplete loading. Configured corpus scan reproduces the relevant oversized
geometry in ELISE/ELISEHP (407 drawable surfaces across their near vehicle parts).

The fix keeps source strips/materials/reflection passes intact and splits at the host limit.
One native part root owns identity child batches; full batch vectors are cached. Draw records
retain native strip identity and host batch/local-surface indices. Bounds/LOD spacing, LOD
copies, wireframe, inherited wheel transforms/visibility and async cleanup include all batches.
No recovered functions, original assets or Verified state change.

Regression initially compared scene-child order to native part order after warm-node reuse;
that assumption was invalid because warm reuse changes child attachment order. Added stable
port-side hp2_part_name metadata and verify each part against native draw_surfaces instead.
The visibility failure was in the test selector, not silently accepted or suppressed.

## Validation and next action

Focused Debug car_surface_batches + car_render_host:2/2 passed. Godot Debug full suite
with GPU tests:41/41 passed45.30s. Release configure/build and full suite:40/40 passed7.17s;
windowed common material/reflection GPU checks also passed. Real Compatibility corpus run
loaded62 available catalog IDs (first skin): two oversized entries ELISE/ELISEHP, zero
failures. The7 missing skin associations remain unrelated existing DIABLO/DIABLOHP gaps.

Tests verify each native part surface count, aggregate triangles, complete union bounds,
exact256-surface chunk boundaries, cached mesh resource identity, sync/async/warm loads,
part visibility, wireframe, LOD material copies and complete cleanup. The expected-child-order
selector failure was corrected as described above; no adapter error was ignored. Native
logic is unchanged; native checks also run inside the full Godot/Release suites.

Actual windowed ELISE capture local/task0036/elise-batches.png inspected: complete vehicle,
Loaded status and no capacity error. Original model data are read through the configured
asset root; no extracted assets/captures are checked in. All shaders remain in platform.
Toolchain CMake3.29.2/Ninja1.12.0/GCC13.2.0/Python3.14.4/Godot4.4.1 stable49a5bc7b6;
Compatibility OpenGL Intel Arc B390. Debug/Release DLL switches force relink.

Final delivery restored godot-debug, HP2_ENABLE_DEBUG_CONSOLE=ON and GPU tests OFF;
forced relink and focused host/generation checks6/6 passed10.58s. git diff --check and
generated progress/template checks passed; restricted untracked binary candidates: none. No commit created; all existing dirty work is preserved. No original recovery or
Verified status changes: this is a host-only capacity fix, so original behavior comparison is
not a completion prerequisite for this task.

Reproduce: cmake --preset godot-debug; cmake --build --preset godot-debug -j6;
ctest --test-dir build/godot-debug -R car_surface_batches --output-on-failure.
Launch car_viewer.tscn with --car-load=CARS/ELISE/GEOMETRY.BIN for a window check.
Continue original wheel/glass comparison separately under active TASK-0035.
