# TASK-0053 - Track runtime regressions and original visibility

- Status: active
- User request: fix zero-length skid VU exception, missing Sun, severe driving /
  Roadshine stalls, and verify original draw distance/section/LOD actually limit
  GPU submissions. User previously deferred the strict60FPS target; prioritize
  working rendering and removal of abnormal cost.
- Authority: Retail track behavior and assets; existing authorized Debug driving
  chain for SkidSet001A8150. Ghidra read-only. Preserve TASK0052 frozen source
  review and all pre-existing work. No guessed epsilon, synthetic Sun, arbitrary
  short distance or disabled original effects as a claimed fix.

## Acceptance

- [x] Original exact-zero VU carrier at Debug001A8150, finite repeated stationary
  contacts, original merge/end/capacity behavior tested; unrelated physics intact.
- [x] Sun-facing original-asset GPU capture proves disc/flares and occlusion;
  explicit native projection/depth/readback dependencies retained.
- [x] Original zone/section/tree/frustum/pixel cutoff/LOD reach shared GPU submits;
  report loaded vs considered vs visible instances and active view draw budgets.
- [x] Roadshine on/off controlled same-pose GPU/CPU cost isolation; eliminate
  demonstrable extra work without changing base masks or original numeric output.
- [ ] Driving route has no blocking exception and no stale/whole-track host draws;
  Debug/Release build and appropriate native/window regressions pass.
- [x] Explicit TASK0053 source-review successor, evidence/task/progress updated.

## Scope and ownership

Root owns shared host/visibility/profiling/build/integration and successor audit.
Drive worker owns only the evidenced skid numerical corner and regression.
Environment worker owns Sun investigation/native and GPU fix. Shared builds are
root-only. Current TASK0052 implementation remains bounded, not Verified.


## Integrated checkpoint - 2026-10-18

Debug/Release builds pass (build-debug-final.log / build-release.log).
Debug100 nonGPU:99 initial pass and preservation rerun1/1 after explicit successor
review approval. Release94:93 initial pass and preservation rerun1/1. GPU8/9
initial pass; race functional assertions passed but unrelated WASAPI invalidation
triggered the ERROR filter. Dummy audio rerun1/1 passes. Test commands now isolate
render tests from audio hardware. Relevant logs under local/task0053. Native/host
6/6 targeted PASS including new skid and capture-binding lifecycle cases.

E-0053-H01: drawcalls3921 ->1912 identical Track11 start; compositor root3Dcalls
370 ->0. E-0053-H02: loaded8407, selected14/128 sections, considered230,
visible102. No new distance or original culling arithmetic change.
E-0053-S01: actual original Sun disc/flares, clear coverage1024, road-occluded0.
E0053D01: exact signed-zero VU MAX carrier only; nonzero underflow explicit gap.
E0053P03: stable capture binding cache, live matrix validation/update retained.
E-0053-H03 / E0053P04: conditional Roadshine sample; GPU fusion/base mask PASS;
final wet-pose wall medians34.973/33.445/33.791ms on/off/on,715draws each.
This does not prove global driving latency fixed or a60FPS target. Submission
still costs~18ms in that Debug pose. Broad performance acceptance remains open.

TASK0052 frozen review/scope unchanged. TASK0053 source review separately approved
13 replaced /2 new functions, audit PASS,19 negative rules PASS. No original
Verified promotion. All source modifications remain uncommitted. Passing binaries
and shortcuts: local/task0053/delivery/BUILD-INFO.json. Normal godot/bin is Debug;
Release isolated project remains local/task0052/staged-release/godot.

Release actual-window check PASS (release-race-window.log): real Track11/Elise,
projection/control/focus/failure/restart/frontend exit/re-entry.

Next: profile remaining moving-driving
CPU submission over a fixed route before claiming severe stutter resolved. Strict
1080p60FPS remains deferred by user. Reproduce independent runtime via
local/task0053/delivery/Launch Race Debug.cmd; all ELF/assets remain external.


Reproducible validation commands (PowerShell, repository root):
```
cmake --build build/godot-debug -j 8
cmake --build build/godot-release -j 8
ctest --test-dir build/godot-debug -E '_gpu$' --output-on-failure -j 4
ctest --test-dir build/godot-release --output-on-failure -j 4
ctest --test-dir build/godot-debug -R '^existing_body_preservation$' --output-on-failure
ctest --test-dir build/godot-release -R '^existing_body_preservation$' --output-on-failure
ctest --test-dir build/godot-debug -R '^(race_runtime_gpu|track_mask_gpu|track_fusion_gpu|track_effects_ui_gpu|track_particles_gpu|track_sun_gpu|track_postprocess_lifecycle_gpu|track_pipeline_gpu|track_pipeline_parity)$' --output-on-failure -j 1
python reverse/scripts/audit_task0053_review.py
python reverse/scripts/generate_progress.py
```
Tools: CMake3.29.2, Ninja1.12.0, Python3.14.4, StrawberryGCC13.2,
Godot4.4.1 official49a5bc7b6 / Compatibility OpenGL3.3 / IntelArcB390 driver32.0.101.9150.
Final diff whitespace check passes; original assets/ELFs remain external and no
files were staged or committed. User files and prior modifications preserved.


## Follow-up from TASK0055 (record only, not widened implementation)

Independent-process actual-car reflection raster repeatability: baseline itself
varies1-18 car-edge color pixels across repeated fixed1080p routes; root camera/
car pose bits and native frame digest identical. Car-reflection-off images and
encoded depth are exact; residual reflected color variation can remain with
identical atlas pixels. No shader/depth/tolerance guess approved. Reproduce using
local/task0055/{baseline-fixed,current-baseline-fixed,optimized-no-sharing} and
benchmark captures/pose_bits/final_frame. Controlled same-session actual native
driving pose plus3 camera/basis cases passes exact atlas/final/color/depth after
removing unsafe material sharing. Treat separate-process residual as unresolved
host render-order/raster evidence, not original-game Verified equivalence.


TASK0055 performance follow-up: accepted host cache reduces median-route wall
10.03% and submission33.16% at1080p, but loading remains median10.17s with an
observed8.90s loading frame. Profile initialization/driver/upload phases before
changing native behavior; no claim that all driving/loading stalls are solved.
See docs/PERFORMANCE_TASK0055.md and local/task0055/final-* raw samples.

## User-requested MultiMesh trial (2026-10-19)

Scope: TASK0053 manual runtime trial only; user explicitly requested enabling the
existing experimental instancing path after disclosure of TASK0055 pixel differences.
No recovered behavior, source implementation, binary or accepted default changed.
Added ignored delivery launchers `Launch TrackViewer MultiMesh.cmd` and
`Launch Race Release MultiMesh.cmd`, passing `--track-opt-instancing=true`.
Headless Godot4.4.1 Release startup assertion PASS via
`local/task0055/check_instancing.gd`: resource_reuse/particle_mesh_reuse/instancing
true; occlusion/opaque_queue false; unsafe material sharing remains disabled.
Launch Release Track11/ELISE for manual trial. Performance improvement is unmeasured;
Track24 exact-color mismatch remains unresolved. Original Verified status unchanged.
Use original launchers to restore the accepted profile. Next: user visual/FPS trial,
then fixed-route measurement before any default promotion. No rebuild needed for
CLI-only activation; no new native reconstruction tests applicable.
