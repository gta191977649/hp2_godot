# TASK-0054 - World effect projection under camera motion

- Status: completed
- Scope: user reports Sun/particles following camera instead of stable world anchors.
- Authority: Retail7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2; Ghidra read-only. Preserve TASK0053 review and all prior work.
- First hypothesis: custom screen-space shader clip coordinates and offscreen host projection disagree. No original coordinate changes without evidence.

## Acceptance

- [x] Reproduce noncentral Sun/particle projection with yaw, pitch and translation.
- [x] Fix shared host projection using measured host convention; native world anchors and original flare/particle arithmetic unchanged unless evidence requires it.
- [x] Dynamic GPU anchor tests fail before and pass after; native and depth/lifecycle regressions pass.
- [x] Debug/Release build and delivery, source preservation audit and durable evidence updated. No Verified claim.

Next: noncentral actual-window Sun pixel projection, then particle and depth path cross-check.


## Results - 2026-10-18

Root owned shared builds and final source review. Production delta: two host
adapter bodies plus shared string-only screen_projection.hpp. Native Sun,
particle simulation, CPU GS projection, alpha/depth and world anchors unchanged.
E-0054-P01 records before/after actual original Sun pixel positions; E-0054-T01
records yaw/pitch/translation and actual particle packet/production-shader tests.
The old53 DLL fails Sun expectedY316.73/actual223.76 and particle359pixels all
outside the asymmetric expected rectangle. Fixed build passes both controls;
three real particle poses changed33811/15425/40072 pixels, outside0. An earlier
broad packet-union check admitted the old mirror, so the isolated transport probe
was added rather than relying on that weaker test. All before/failed logs retained.

Debug101/101 nonGPU PASS (full-debug.log); Release95/95 PASS(full-release.log).
Debug final actual GPU5/5 PASS: Sun,particles,sky-depth,postprocess lifecycle,
viewer/runtime pipeline. Release actual-window original-default Sun moving-anchor
and road occlusion PASS; actualparticle generation/production-shader probe PASS.
Initial Debug-only toggle fixture is unsuitable for Release (effect overrides
intentionally rejected); those failed attempts are retained, then separate
original-default fixtures were used without production-policy changes.

Source audit PASS exact2before/2after; nineteen negative review tests PASS. Frozen
53 review/scope unchanged. No unrelated physics changes or Verified promotion.
Godot4.4.1 official49a5bc7b6, Compatibility/OpenGL3.3, IntelArcB390 driver32.0.101.9150;
CMake3.29.2/Ninja1.12.0/StrawberryGCC13.2/C++17/Python3.14.4.

Commands (PowerShell, repository root):
```
cmake --build build/godot-debug -j 8
cmake --build build/godot-release -j 8
ctest --test-dir build/godot-debug -E '_gpu$' --output-on-failure -j 4
ctest --test-dir build/godot-release --output-on-failure -j 4
ctest --test-dir build/godot-debug -R '^(track_sun_gpu|track_particles_gpu|track_postprocess_lifecycle_gpu|track_pipeline_gpu|sky_depth_gpu)$' --output-on-failure -j 1
python reverse/scripts/audit_task0054_review.py
```
Release original-default commands use --audio-driver Dummy --path
D:/dev/hp2_godot/local/task0052/staged-release/godot --script
D:/dev/hp2_godot/local/task0054/release_sun_runner.gd (or release_particles_runner.gd).
All logs/captures ignored under local/task0054. Passing binaries and launchers at
local/task0054/delivery, hashes in BUILD-INFO.json. Normal godot/bin updated.
No files staged/committed, original assets/ELFs external, prior work preserved.

Continuation: this projection scope complete. Remaining CPU performance work is
TASK0053; original-stage gaps TASK0052, strict60FPS deferred. GPU regression
registration uses cmake -S . -B build/godot-debug -DPORT_ENABLE_GPU_TESTS=ON.
