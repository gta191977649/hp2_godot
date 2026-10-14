# TASK-0034 — Vehicle UV audit, preview mode and shared PS2 material recovery

- Status: completed
- Target: SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
- Scope: diagnose texture-coordinate corruption, simplify CarViewer navigation, continuous preview orbit, evidence-backed common native material/light pipeline.
- Preserve all existing TASK-0033 and user working-tree changes.
- No original-output equivalence or Verified status without a recorded comparison.

## Acceptance criteria

- [x] UV fault identified from original instructions/data; meaningful regression and original-car window checks.
- [x] CarViewer removes TrackViewer / Back to Frontend buttons.
- [x] Preview Mode continuously orbits while mouse adjusts view; test UI/state/input.
- [x] Audit original lighting/material entries and put recovered shared behavior in original native class slices, with host-only shader adaptation.
- [x] Record supported effects and unresolved PS2 passes; native/Godot checks and generated progress updated.

## Authorized follow-up

The user also requested a debug high-speed wheel blur preview during this task.
Add a force-on control backed by original WHEEL_BLUR resources; do not invent speed thresholds.

## Confirmed findings and implementation

- E-0034-U01: original CPU UV accessors0011C090/0011C268/0011C398 agree with current
  attribute indexing. The legacy indexed texture decoder emits bottom-first rows internally.
  Vehicle decode now uses flip=true to restore original row-zero order, while retaining raw
  GS alpha. Original UV numbers and frontend decode convention are unchanged. A two-row
  indexed test checks texel placement, not just image dimensions.
- E-0034-M01: reread00120EF8,00115978,00116828,00117098,00198418 and relevant VU
  blocks through read-only Ghidra MCP. A bounded matrix-only CarRenderInfo::Sub_00120EF8
  overload delegates the original default frontend light context. Existing recovered native
  binding/pose/LOD/material/VU/GS slices supply the common rendering backend.
- Ps2VehicleRenderer now owns shared upload/material/submission behavior. Ps2CarViewer is
  a thin Debug-only request-policy wrapper. Camera and initialized light directions include
  rigid world placement; the host test checks translated/rotated instances. This is not a
  full live-car renderer, physics implementation or original ABI reconstruction.
- All shader source/specialization text is under src/platform/shaders; CMake tracks and
  embeds it. Runtime/export requires no source-tree shader files. No extension shader,
  PBR approximation or invented original light rig was added.
- CarViewer has no TrackViewer / Back to Frontend navigation buttons. Preview Mode uses
  explicitly port-side12-degree/second continuous camera orbit and35-degree FOV, retaining
  mouse adjustments even during dragging. UI/console capture does not suspend orbit.
- Authorized follow-up E-0034-W01: constructor blur geometry states+0x130/+0x148 use exact
  WHEEL_BLUR and F/R fallbacks. A Wheel Blur (high speed) checkbox / --wheel-blur forces
  four original geometry/texture overlays at original alpha scale90. This is a debug
  sample of the effect, not an invented automatic km/h threshold. Native model/request
  ownership remains outside extensions. Missing resources report errors.

## Validation (2026-10-14)

Tool versions: CMake3.29.2, Ninja1.12.0, GCC13.2.0 (Strawberry MinGW), Python3.14.4,
Godot4.4.1 stable49a5bc7b6; Intel Arc B390 Compatibility renderer.
All commands run from the repository root and stop if configure/build/test fails.

- Native: cmake --build --preset native-debug -j6; ctest --preset native-debug:
 36/36 passed26.40s. Original asset samples are configured through the existing global
 resolver; no private root is hardcoded. Final UI click regression also passes in Debug.
- Debug console off: cmake --preset godot-debug -DHP2_ENABLE_DEBUG_CONSOLE=OFF
 -DPORT_ENABLE_GPU_TESTS=ON; cmake --build --preset godot-debug -j6;
 ctest --test-dir build/godot-debug -R "car_material_gpu|car_render_host|debug_console_host|debug_viewers_host"
 --output-on-failure:4/4 passed7.09s, after forcing relink of the generated DLL.
- Release: cmake --preset godot-release -DHP2_ENABLE_DEBUG_CONSOLE=OFF
 -DPORT_ENABLE_GPU_TESTS=OFF; cmake --build --preset godot-release -j6;
 ctest --test-dir build/godot-release --output-on-failure:39/39 passed8.57s.
 Direct windowed car_material_gpu_smoke.tscn also reports CAR_MATERIAL_GPU_OK using
 Ps2VehicleRenderer in Release; debug viewer entry stays rejected.
- Debug console on: cmake --preset godot-debug -DHP2_ENABLE_DEBUG_CONSOLE=ON
 -DPORT_ENABLE_GPU_TESTS=ON; cmake --build --preset godot-debug -j6;
 ctest --preset godot-debug:40/40 passed33.08s, including the actual GPU oracle.
- Window captures inspected: Corvette after row correction, CL55AMG Preview Mode and
 911TURBO forced wheel blur. Private images remain ignored under local/task0034/.
 Continuous mouse/console orbit, four blur overlays and toggle-off restoration have
 native UI and Godot smoke coverage. Existing original car families still load.
- Final default Debug/on, GPU option OFF restored; focused UI/inventory/progress/three-host
 checks6/6 passed4.94s.
- Class inventory/progress regenerated; UTF-8, shader-location and diff checks performed.
 No assets, binaries, captures or Ghidra databases are added to Git. No commit created.

Rejected intermediate checks: a loaded Windows DLL initially blocked relinking; it was
renamed into ignored local/task0034/previous_in_use.dll without killing the user's window.
Switching build directories can also leave a newer DLL from the other configuration, so
that generated output was removed before relinking. Configuration assertion failures from
stale DLL runs are rejected, not accepted as passes. The car-render smoke now checks its
expected Debug flag too. A test-local source variable compile error and a borrowed-array
snapshot assertion were corrected before the accepted runs. The Release debug-wrapper
policy regression was fixed without disabling its rejection assertion. No feature failures
remain in the accepted matrix.

## Unresolved original behavior / continuation

No PS2/PCSX2 comparison has been made. Full environment/reflection/extra material passes,
the reference image's floor mirror and environment inputs, complete live-car submission,
partial-frustum clipping, and wheel speed/time/per-wheel activation remain unrecovered.
Whole-entry no-argument overloads still throw hp2::NotImplemented; no Verified promotion.
Continue these under TASK-0033 with read-only Ghidra00120EF8 /00120EA8 and callers;
record actual speed inputs before implementing automatic wheel blur switching.

Default delivery is Debug console ON and GPU test option OFF. Reproduce:
cmake --preset godot-debug -DHP2_ENABLE_DEBUG_CONSOLE=ON -DPORT_ENABLE_GPU_TESTS=OFF
cmake --build --preset godot-debug -j6
ctest --preset godot-debug
Launch godot/scenes/debug/car_viewer.tscn, select a vehicle, enable Preview Mode and
Wheel Blur (high speed). Restart an already-running viewer to load the new DLL.
When switching between existing Debug/Release build trees, force relink of godot/bin/ps2_port.dll
before host tests; a successful no-op build alone does not establish DLL configuration.

## Related commits

None.
