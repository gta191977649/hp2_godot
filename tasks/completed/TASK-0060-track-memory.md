# TASK-0060 - Bounded track host resource residency

- Status: completed
- Scope: public track renderer memory accounting, bounded inactive draw/mesh cache,
  synchronized postprocess release, lazy debug geometry, Track11 lifecycle measurements.
- Authority: user-authorized host ownership changes only. Preserve native culling,
  pass order, materials, visual updates and physics. No new recovered behavior.
- Acceptance: before/after actual-process private bytes, working set and page-fault
  counters; stationary/moving/rotating/repeated route/clear/reload on Track11;
  classified owned resource statistics; bounded inactive retention and released
  ownership after unload; Debug/Release builds and window checks; source review.
- Verification: PC-only; no PS2 Verified claim, no all-track sweep or FPS target.

## Initial evidence

E0060M01: source inspection finds persistent draw and depth-proxy caches,
mesh cache eviction blocked by hidden-node references, unconditional debug mesh
construction. Earlier Track11 snapshot was 4.61 GiB actual game private bytes;
console launcher memory is not the game process. This is not proof of a leak.

## Implemented / validated

Lazy source GPU upload and direct CPU-to-final merged attributes remove the
whole-track intermediate GPU surface set. Bound inactive draw/source caches;
eviction removes depth proxies/materials/uniform refs together. Lazy debug
geometry and release of consumed raw bundle. On-demand classified memory API.
Native/physics bodies unchanged; strict successor13before/20after PASS.

Godot4.4.1/GCC13.2/CMake3.29.2/Python3.14.4, Intel ArcB390 Compatibility driver
32.0.101.9150. Debug/Release build PASS. Debug targeted CTest8/8 and Release7/7
PASS; mesh transport GPU fixture PASS; postprocess ownership GPU PASS. Actual
Debug TrackViewer motion/rotation PASS (3particle-gap frames remain explicit);
Release race/native driving/main+6capture/restart/failure/re-entry PASS.

Track11,1080p actual-process peak private/working MiB:

| Flavor | TASK0059 baseline | TASK0060 delivery |
| --- | --- | --- |
| Debug | 4237.52 /4230.44 | 1909.84 /1927.56 |
| Release | 4225.77 /4218.57 | 1896.73 /1912.72 |

Delivery fixture additionally covers four overlay modes, worker cancel and
invalid-ID cleanup. Isolated same-route Debug before/after4237.52/1897.39MiB.
Repeated warmed laps:388draws/256inactive,378depth pairs,371source meshes,
641merged entries/20,855,024payload bytes, stable across laps. Unload/cancel/failure
owned source/draw/mesh/batch/depth/material/target categories zero; scene nodes6.
All24native packet hashes and final1920x1080 screenshot bytes match predecessor
for both flavors. No whole-process return-to-startup claim or PS2 Verified.

Process fault deltas (include soft faults): Debug baseline1,554,771 vs delivery
739,000; Release1,534,436 vs729,078. System paging counters sampled, but global
peaks vary with other desktop processes and do not attribute hard faults to Godot.
Remaining private bytes after clear require allocator/driver tracing before
claiming either a leak or a specific retention cause. See docs/TRACK_MEMORY.md.

Exact commands: cmake --build --preset godot-{debug,release} -j4;
reverse/scripts/measure_track_memory.py --engine <actual Godot executable>
--project local/task0060/delivery/{Debug,Release}/godot --output <ignored directory>
--enforce. Artifact identities in delivery/*/BUILD-INFO.json and runtime manifests;
all8DLLs staged. Launchers in local/task0060/delivery. Source scripts/scenes/assets
are workspace junctions, not a portable export. No commit or staging.

Logs local/task0060/{final-baseline-debug,final-baseline-release,delivery-debug,
delivery-release}/measurements.json; debug/release-targeted-tests.log,
mesh-cache-test.log,postprocess-lifecycle.log,viewer-motion.log,race-release.log,
source-audit-pass.log. Initial test-only errors retained: accidental --viewer-smoke
autoload quit, copied audit test predecessor-error labels, initial source format
variable scope compile error. Corrected checks pass; no suppressed runtime errors.

Continuation: launch local/task0060/delivery/Launch TrackViewer Native.cmd.
User's next UI/DPI request is a separate task; do not widen this completed scope.
