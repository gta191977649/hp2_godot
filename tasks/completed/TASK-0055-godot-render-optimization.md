# TASK-0055 - Godot render optimization with original visual rules

- Status: completed
- Scope: implement accepted plan in Compatibility; common TrackViewer/Test drive/race renderer. Keep native frame packet and recovered logic Godot-independent.
- Preserve prior work and immutable TASK0054 approval. Proprietary assets/ELFs external; Ghidra read-only. No original Verified claim.

## Acceptance

- [x] Freeze baseline, deterministic visual timestep/route/seeds and trustworthy active-view measurements; Release1080p warm120/sample600/repeat3 all30 tracks.
- [x] Extend shared resources and dirty submission; persistent static GPU resources and particle mesh capacity; correct ordinary bounds and layers.
- [x] Conservative contiguous MultiMesh eligibility and actual asset coverage; original instance/LOD/order/view/section respected; exact color/depth proxy integration.
- [x] Independent occlusion/opaque experiments with strict visual/state gates; unsupported or unproven classes remain baseline, no guessed visual changes.
- [x] Public host configuration and counters, baseline/optimized shared implementation, load-time CLI settings in Release with no ImGui dependency.
- [x] Native logical packet identity and GPU intermediate/color comparisons; lifecycle/failure/all30 resources, Debug/Release/window regressions.
- [x] Default gates: target median CPU or wall improvement>=5%, no persistent>5% all-route wall median/P95/P99 regression; no quality/distance reduction. Failed experiment stays off with reason.
- [x] Strict successor audit, documentation, passing build delivery with reproducible commands/limitations.60FPS observation only, not completion gate.

## Ownership

Root: shared host configuration/track submission/postprocess integration, builds/GPU queue/performance runs and approval. Particle worker: particle mesh reuse only. Rendering worker: audit55 framework and deterministic performance script. Instancing worker: conservative MultiMesh helper and asset eligibility coverage. No worker shared builds or original core edits.

## Initial state

TASK0054 frozen binaries copied to ignored local/task0055/baseline/{Debug,Release}.
Current host already has contiguous equal-state mesh batches, Shader cache, some
material/matrix dirty tracking and depth proxy cache. Existing script sums stale
viewport timers and uses variable visual dt; those measurements are not acceptance.
No Forward+ migration, automatic LOD, shorter far clip, effect disable or native
physics edits are authorized as an optimization substitute.


## Implementation and validation, 2026-10-19

- Host-only successor source audit approved:16 before/62 after identities;
  predecessor54 immutable; no native/physics deltas. Audit log local/task0055/audit55.log.
- Debug and isolated Release build PASS: cmake --build build/godot-debug -j8,
  cmake --build build/godot-release -j8. GCC13.2, CMake3.29.2, Godot4.4.1.
- Debug core/audit91/91 PASS: ctest --test-dir build/godot-debug -I 1,91
  --output-on-failure -j4. Full Release/headless and Debug GPU checks in progress.
- Actual ELISE reflection3poses PASS: exact final/color/rawdepth/atlas and
  native digest/per-view counts. Original environment matrix equal. Log
  local/task0055/reflection-parity-fixed.log. Colinear offset-8 fixture error
  affected both paths; fixed test pose-4, no production camera math change.
- MultiMesh implementation/asset coverage and static GPU fixture PASS, but
  all30 visual trial failed Track24 pose50: exact rawdepth, different color.
  Reuse-only isolation PASS; instancing excluded from optimized/default.
  Retain experimental explicit opt-in and evidence, no tolerance/asset guessing.
- Resource/particle reuse all30 exact comparison and exclusive Release
  1080p120/600/3 performance pending. Default remains baseline until gate.
- Occlusion/opaque independent experiments remain off pending proof. No quality,
  far-distance, effect or LOD reduction. TASK0052 native effects gaps unchanged.

## Exact continuation

First finish local/task0055/parity-reuse-all30.log and Release tests; then run
independent occlusion/opaque parity, remaining Debug engine tests, exclusive
frozen54 baseline/current optimized all30 deterministic Release benchmark.
Compare with reverse/scripts/compare_task0055_performance.py. Promote project
profile only on successful gates; copy final binaries/launchers/hashes and record
all failures/limitations. No PC test promotes original Verified status.


TASK0055 all30 reuse GPU completion: TRACK_OPTIMIZATION_GPU_OK tracks=30,
90 actual static poses exact final/intermediate-color/rawdepth and logical packet
parity. Post-sequence forest-fire pair516active/91visible equal, exact pixels;
clear/reload/invalid-track cleanup PASS. Log local/task0055/parity-reuse-all30.log.
Counts differ from the initial single-track fixture because the renderer preserves
its original RNG across prior track loads; each pair receives the same sequence.
Benchmark creates fresh renderer/session each repeat, resetting this caller seed.


Full current regression: Debug118/118 (91 core/audit + new performance gate1
+ host12 + GPU14), Release98/98 (97 full + new gate1). Actual-window GPU14
includes moving Sun/sky, particles/buffer fixture, camera+vehicle-basis reflection,
postprocess lifecycle, masks/fusion, pipeline, actual driving and car materials.
No skipped checks. Logs debug-{core,host,gpu}-tests.log/release-tests.log.
Opaque experiment Track24 three poses and forestfire exact parity, but zero
opaque_queue_draws: no eligible class was exercised, not an effectiveness proof.
It remains off. Occlusion and optimized Release driving checks pending.


Performance checkpoint: initial Track11 paired120/600/3 diagnostic showed
wall-14.9239%, prepare-29.0389%, submit-35.0658%, no persistent tail regression,
but is not all30 acceptance. Follow-up full30 run was intentionally stopped
after detecting9-18 differing pixels in independent-process1080p final captures.
Old baseline's own repeated images also differed (9-10 pixels); controlled
same-process90pose/vehicle-reflection pixel tests remain exact. Do not assume
an optimization regression or waive pixels: investigation ongoing.

Benchmark helper now allows only Loading auto-process (delta0); no real-time
Running tick may slip between load completion and fixed loop. End-of-loop
untimed native frame summary and exact serialized camera/car pose bits added.
Running three diagnostic groups baseline-fixed/current-baseline-fixed/optimized-fixed
under local/task0055; compare inputs/digests and images after completion.
Full30 gate/default promotion remains pending; preserve aborted baseline-run log.
Both independent opaque/occlusion probes have zero eligible work; not valid
effectiveness proof, remain off. Optimized Release actual driving smoke PASS.


Final cache correction: independent-process1080p probes isolated differences to
car reflection: raw native-depth and reflection-disabled images exact. Shared
materials yielded107 differing atlas pixels in all3 repeats; distinct materials
removed atlas differences in all3. Remove material sharing entirely, preserve
material identity. Native input/frame digests and camera/car pose bits match.
Residual tiny independent-process car-reflection raster differences also occur
within baseline repeats (1-18pixels); cause is unresolved, not waived as a zero-
difference controlled test. GLES3 source already declares invariant gl_Position;
no speculative precision/shader/depth patch was made.

Strengthened controlled fixture now1920x1080, real Ps2DrivingRuntime first native
snapshot, actual chase pose, real car capture binding, plus3 rotated/moving views.
Final cache passes exact native packet/view counts, atlas/final/color/rawdepth in
all4 poses (reflection-native-drive-1080p.log). All30 final-cache rerun in progress.
Public read-only render-target getter available Release for untimed diagnostics;
no Debug effect override relaxed. Successor review now17before/63after, host only.
Earlier14.9% wall result used unsafe sharing and is diagnostic history, NOT final
performance acceptance. Benchmark loading time is deterministic and final native
digest/pose bits plus color/depth/atlas captures occur outside sampled loops.
Temporary car-reflection-off capture occurs only after all samples/original
captures as isolation evidence, never to improve measured frames.


Final-source regression: Debug non-GPU104/104 and Release98/98 PASS after
material sharing removal/read-only diagnostics. New all30 GPU and full14 GPU
regression running on the final source; do not reuse earlier-sharing FPS numbers.
Next after GPU queue: final optimized Release drive smoke, independent frozen54
vs final-cache Release all30 warm120/sample600/repeat3 benchmark with current
script hashes. No builds/tests during those measured runs. Default baseline still
retained pending all-route gate.


FINAL no-material-sharing candidate: all30/90pose exact final/color/rawdepth and
native digest PASS, dynamic post-sequence particles516/91 equal, failure/clear
cleanup PASS (parity-stable-cache-all30.log). Same final source104 non-GPU Debug
and98 Release checks pass;14 final GPU checks in progress. Snapshot hashes at
local/task0055/final-benchmark-inputs.json. No more production changes planned.

Final timed paths are local/task0055/final-baseline/all30.json and
local/task0055/final-optimized/all30.json; preserve earlier partial/diagnostic
directories. Run frozen54 baseline and final-cache optimized sequentially, no
concurrent builds/tests. Current benchmark optional --performance-isolate-reflection
only generates an extra untimed reflection-off diagnostic after original samples;
default false, not used for final timing. All timed samples retain original effects.
Comparator requires matching serialized pose and final native packet evidence,
in addition to all30/120/600/3, relative gain and no persistent wall tail regression.

## Timed-run checkpoint, 2026-10-19

Final source is frozen. Debug104 non-GPU +14 GPU =118/118 PASS;
Release98/98 PASS. Final optimized Release actual-window drive smoke also PASS
(release-final-cache-race.log), including focus/control/failure/restart/re-entry.
No production changes after final-benchmark-inputs.json. All30 final-cache exact
GPU parity and enhanced1080p actual native driving/reflection4poses PASS.

Exclusive final timed runs are in progress: parent exec session62298 runs
frozen54 baseline then final optimized sequentially. At06:05 local the baseline
completed Track45 repeat2. Inspect final-baseline/run.log and final-optimized/run.log;
do not run builds/tests/other GPU work while these processes are measuring.
Final JSONs are written after every repeat. Earlier performance numbers with
shared materials are superseded and must not be used as final improvement.

After both finish, compare final-baseline/all30.json and final-optimized/all30.json
with compare_task0055_performance.py. Also inspect effective options, original
effect permissions, per-route CPU/tail/draw/memory/loading data and image evidence.
Default remains baseline until gate. Then package final Debug/Release snapshots
and launchers, update docs/task/progress, restore normal GPU-test opt-in OFF and
record exact delivery hashes. Keep TASK0052 effects and TASK0053 cross-process
car-reflection repeatability gaps explicit; no original Verified promotion.


## Completion and delivery

All acceptance criteria have evidence. Final paired gate and manual invariants
PASS:180 complete runs/108000 sampled frames,30 original tracks, fixed1080p
120warm/600samples/3repeats,0 failures. Pose/native packet/effect permissions/draw
statistics match. Wall median-route improvement10.0328%, prepare27.6223%,
submission33.1621%, no persistent>5% wall median/P95/P99 regression. Absolute60FPS
is deferred; loading stalls and original recovery gaps remain explicitly open.

Debug118/118 and Release98/98 PASS; exact all30/90 static views, dynamic particles,
1080p actual native-driving snapshot plus3 camera/basis reflection poses and
all14 GPU regressions PASS. Accepted project default verified in normal/staged
and isolated delivery Debug/Release projects. Actual delivery TrackViewer window
and Release independent race window PASS with no policy CLI override, including
viewer overlays/load/drive/return/clear and native drive controls/focus/failure/
restart/front-end/re-entry. Logs delivery-{viewer,race}-window.log and
{delivery-,}default-{debug,release}.log. Normal GPU-test opt-in restored OFF.

Source is uncommitted; pre-existing changes preserved. Frozen TASK0054 review/
scope unchanged; strict TASK0055 approved17before/63after host-only source audit.
No original/recovered function, native physics, distance/LOD/effect condition or
new grip model changed. No original-console Verified claim.

Delivery: local/task0055/delivery/{Debug,Release}/godot contains isolated DLL/config
snapshots and common workspace resource junctions. BUILD-INFO.json records all
DLL hashes/limits. Launch TrackViewer.cmd, Launch Race Debug.cmd,
Launch Race Release.cmd; Launch Baseline Release.cmd disables new host policy.
Debug DLL1558605e38a9e90681531c2bd472288258df7156eca6ffaa0785b67de640e094;
Releasee7adc55c734c723f9631a564ce8acade1a8d970c3eb930dd0790645cba5e1bc6.

## Reproducible next work

Read TASK0053 and docs/PERFORMANCE_TASK0055.md. Profile remaining initialization/
loading and driver cost using the frozen55 snapshot; preserve default visual
rules. Loading still has median10.17s/worst8.90s frame stalls; investigate before
claiming a loading fix. Retain baseline cross-process car-reflection repeatability
question. TASK0052 original effects/shadow gaps remain independent. Do not enable
MultiMesh/material sharing/occlusion/opaque without fresh correctness and
performance proof. No commit made; no external prerequisites needed for this
completed host optimization scope beyond the documented pinned tools/assets.
