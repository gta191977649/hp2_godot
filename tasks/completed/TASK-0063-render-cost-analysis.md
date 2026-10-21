# TASK-0063 - Render cost attribution and parity-preserving optimization

- Status: completed
- Authority: user-approved implementation plan, 2026-10-19.
- Target: Arc B3901080p P95<=16.67ms, total all-pass draw calls<1000.
- Preserve original effects, LOD, view distance, particle RNG and capture timing.
- Scope: scenario benchmark, per-stage counters; particle state/transport caches;
  submission diagnostics; postprocess consumers and depth reuse feasibility;
  Compatibility/Mobile/Forward+ comparison. Original physics untouched.
- Baseline DLLs copied to ignored local/task0063/baseline before changes.
- Acceptance: representative factorial/isolates, native and image parity,
  30-track regression, lifecycle, Debug/Release and workspace build entry point.
- No performance or PS2 Verified claim without evidence. Failed candidates off.
- Root owns builds/GPU runs/renderer/postprocess/docs. Delegated particle state,
  transport and benchmark agents own disjoint files. One GPU run at a time.

## User scope correction - 2026-10-19

User stopped the long performance method: use static code tests and drawcall
statistics instead. Root terminated its own active FPS benchmark. Remaining
full120/600/3 matrix, all30 GPU performance sweeps, backend benchmarks and GPU
capture are no longer required by the current request. Existing guarded partial
data is scoped evidence only, never a full-budget/FPS acceptance claim.

Mutable particle-state, exact stream transport and invariant texture caches are
selected by default based on exact native/byte/image parity and removal of
redundant operations. This is not a measured FPS win. Sun reduction and opaque
queue remain independent off-by-default experiments pending wider evidence.
CPU-only asset census will compare native commands, host batches and estimated
scene passes; actual engine draw counts remain distinctly labeled.

## Revised-scope completion

- Debug/Release builds passed; normal ./build.ps1 rebuilt the root workspace DLL.
- CPU-only all30 asset census passed60Props on/off rows. Native runtime all30
  prepare also passed before the method change. Census camera self-test passed.
- Targeted native/source checks and analyzer tests recorded in root report/logs.
- Candidate-true GPU fixtures previously passed before the user ended runtime
  testing;15 current-run captured PNGs exact, Sun compare zero mismatches.
- Final default caches change only redundant transport/allocation work; no60FPS
  or full-GPU <1000 claim. Source review30before/40after plus exact auxiliary
  file hashes and19successor tests preserve frozen inputs/recovered physics.
- Long matrix/backends/capture cancelled/deferred by user; no engine process left.
- Reports: docs/PERFORMANCE_TASK0063.md and docs/TRACK_DRAW_COUNTS.md.
- Reproduce quick inspection: cmake --build --preset godot-release --target
  track_draw_inventory -j4; build/godot-release/track_draw_inventory.exe --assets.
- Next independent work: validate opaque cutout queue ordering and fix prior
  MultiMesh pixel discrepancy before enabling either. No commit created.
