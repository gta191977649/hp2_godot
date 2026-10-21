# TASK-0072 - Original visibility and native-resolution Debug profiler performance

- Status: active
- User authorization: implement the agreed plan; Track11 at2880x1695 Vulkan Forward+, Debug with Profile UI enabled. Preserve original visibility/effects/LOD/physics. Median andP95 <=16.67ms in each valid120warm/600sample/3repeat run across current/nearby/dense route.
- Baseline: current uncommitted TASK0071 and earlier user work preserved under ignored local/task0072/baseline, original DLL/dependencies, SHA256 manifest and source identities. Do not overwrite the user's loaded workspace DLL; all builds and graphical tests use isolated projects.
- Evidence input: user120-frame profile-debug-116582753.json,2880x1695; wall median25ms/P9543.638ms; Sun CPU readback wall median11.5903ms. Repeated identical viewport timestamps are unverified, not additive GPU budget.
- Scope: read-only Ghidra camera/visibility audit, exact GPU-resident Sun counter/alpha transport with reference fallback, profiler timestamp validity/context and compact history, measured host caches and source viewport suppression.

## Acceptance

- [ ] Record original default/effective/override distances and per-view visibility boundaries; unknown dynamic branches explicit.
- [ ] Preserve original GPU/CPU Sun counter and alpha semantics, including first frame, clipping, offscreen/disabled/reload, with zero extra visibility latency.
- [ ] Useful profiler capture with full camera/effect/build/backend context; unverified timestamps unavailable; live panel remains responsive.
- [ ] Each optimization independent, exact native/pixel/depth parity before default enablement.
- [ ] Native-resolution Debug+Profile120/600/3 fixed/current/nearby route tests meet median/P95 <=16.67ms; report P99/noise and blockers honestly.
- [ ] Relevant Debug/Release builds, lifecycle/source checks; update progress/log/evidence and leave reproducible continuation.

## Findings and next action

Planning Ghidra rereads confirm Debug Camera001566E0 sets NearZ0.5 and FarZ from00324F90 bytes00401c46 (10000); CreateViewMatricies consumes camera fields and explicit force overrides. DrawAScenery uses integer projected size cutoff2 plus asset FarClipSize thresholds, not a single distance. Default10000 alone does not imply rendering all loaded geometry.

Implementation in progress on2026-10-19/20:

- Fresh visibility evidence E-0072-V01..V04: default .5/10000; Debug mirror .75/192, sky100/50000; ordinary audited gates match bounded main adapter. No justified global far reduction. Full Track11 key coordinator and rejection-stage counters remain open.
- GPU Sun adapter implemented behind --track-sun-gpu-resident=true, default-off pending final gates. Original recovered core unchanged. E-0072-S01 records count-only alpha dependency, exact byte shader, doublebuffer previous result, initial0, full source-over domain validation and explicit CPU reference fallback. Comparison flag deliberately blocks and is excluded from timed runs.
- GPU16467 synthetic cases and30productionframes/720checks passed exact native digest/colour/depth/final parity at960x540. Producer-frame BA stamping was subsequently added and passed the same30frame parity. Native-resolution parity and actual asynchronous telemetry checks remain to run.
- Profile timestamp queries use RD completed source-frame pairs on render thread; repeated/missing pairs are null. Exports contain numeric full camera/effects/source hashes. Async diagnostic payload validates its own32-bit producer stamp and lifetime generation; never gameplay input. Compact history independent candidate defaults off.
- Race-context diagnostics show Sun fixed pose median31.419/32.247/32.349ms ->23.199/23.446/23.888ms, P9537.386..38.061 ->24.962..25.782ms; this includes car reflection work absent from TrackViewer browsing and does not certify screenshot FPS.
- Actual TrackViewer adapter retains real AssetViewer UI/process, hidden CORVETTE native updates, mover2 and empty visual state. Early fixed-pose runs showed13.191..13.526ms/P9514.340..14.941 but one final camera pose was externally moved: preliminary, not final acceptance. Wrapper now disables external input; v3 matrix uses separate application name/user data and re-runs all three120warm/600sample/3repeat routes. Original screenshot basis remains unavailable; heading180 at supplied position is a port-side test pose. Four direction probes found180densest(813draws vs293/490/523), also preliminary.
- Builds isolated at local/task0072/candidate/godot and release/godot; Debug and Release compiled. Native relevant8/8 tests passed. Workspace DLL untouched. CMake current Debug output points to isolated candidate; build.ps1 resets it to workspace before normal build.
- Old viewer-baseline-fixed has SCRIPT ERROR and was stopped; viewer-gpu-dense incomplete; exclude both. v2 comparison invalid because final pose changed. Keep these raw records as counterevidence.

Next: finish v3 strict paired matrix, native-resolution30frame parity, async GPU lifetime check, lifecycle/source checks, decide defaults only after all gates, update reports and generated progress. TASK0071 COLCLAMP/reflection/PS2 gaps remain; no Verified promotion.

## Related commits

None created.

## Driving and rotating-camera priority

User steering: prioritize actual Drive mode stutter and simultaneous route movement / camera rotation. Add short_spin to the camera harness: existing closed translation path plus two full yaw revolutions per warm/sample traversal. This is a synthetic free-camera stress fixture, not actual vehicle driving or original route evidence. Compare movement-only, rotation-only and combined movement/rotation separately. Actual driving with native physics, reflection capture and deterministic input remains a separate required test. Report spikes and P99 in addition to median/P95; a stationary pass cannot close this requirement.


## Driving implementation checkpoint

Production driving/drive_spin and free-camera rotate/short_spin fixtures added.
One production tick per frame is counted; legitimate original scheduler holds
(step_delta0) are retained. Input/body/camera/native-dt/contact/terrain, source
frame GPU timestamps, pipeline counters and per-view selection/planning/submission
are exported. Native8/8 relevant tests and staged registry lifetime/order tests
pass. Async immutable Profile snapshot worker and error/retry/shutdown contracts
pass headless. Candidates remain independently switchable and default-off pending
final gates; source guards are being updated without promoting recovered status.

Strict v6 real driving rotation: reference median56.006/57.896/57.722ms,
P9573.673/73.173/76.292; residency47.907/48.276/49.591ms,
P9556.147/57.977/57.797. Matching720-frame deterministic trajectories in each
pair; gain8.131ms >baseline repeat range1.890ms, noP95/P99 regression. Still fails
60FPS. Native-resolution rotating transport parity720checks passes for combined
residency/dynamic-planner/depth-discovery candidate. Actual driving/reflection
parity, additional ablations, five-minute stress, final Debug/Release builds,
source protection and final defaults remain outstanding. See performance report.


## WorkerThreadPool integration phase (2026-10-20)

Authorized scope: standard-C++ mesh payload preparation under platform/render;
Godot owns native group scheduling, same-view join and ordered resource upload.
Recovered selection and driving code are unchanged. Compare0/2/4 bridge job limits
with Safe/Separate, actual drive_spin and viewer short_spin, full native-resolution
colour/depth/final/reflection parity before any default change. Defaults0/Safe.
Residency remains off: actual driving reflection parity failed despite measured
CPU gains. Prior free-camera parity does not certify driving reflections.
Current work: split cached/missing sources and batch assembly, preserve singleton
and material creation order, add diagnostics and native/engine contract tests.


## Worker integration checkpoint and testing stop

CPU split and0/2/4 native group scheduling implemented; immutable inputs and
same-view joins keep recovered selection, render order and physics unchanged.
Frozen legacy->split0/Safe,0->2/Safe and0->4/Safe each pass8759 full driving
image/state checks. Separate fails image and shutdown/lifecycle checks and stays
experimental. Initial native28/28, focused settled8/8 and Python21+7 checks pass.
Neighbour-zone/sector exclusion/no-zone fallback tests pass; actual Track11
zone71 selects14/128 sections (first main view235 considered of8407 instances).
Full timing matrix/Profile-off/5minute soak were not run: the user requested
stopping repeated Godot launches. Running/queued graphical tests were stopped.
Do not restart automatic graphical validation without renewed user direction.
Defaults0/Safe and residency-off remain. No60FPS or original Verified claim.
See PERFORMANCE_TASK0072.md and RENDER_PREPARATION_TASK0072.md for commands,
architecture, failed Separate results and exact continuation bounds.


## Worker integration checkpoint and testing stop

Transport-only CPU split and0/2/4 native group scheduling implemented. Immutable
inputs and same-view joins preserve recovered selection, render order and physics.
Legacy->split0/Safe,0->2/Safe and0->4/Safe each pass8759 actual driving image/state
checks. Separate fails image and shutdown/lifecycle gates. Native28/28 initial,
focused8/8 settled and Python21+7 checks pass; isolated Debug/Release builds pass.
Neighbour-zone/sector exclusion/no-zone fallback tests pass. Actual Track11 zone71
selects14/128 sections (first main view235 considered of8407 loaded instances).
Full performance/Profile-off/5minute soak were NOT RUN: user stopped repeated
Godot launches. All owned graphical/queued runs stopped; do not restart them
without renewed user direction. Defaults0/Safe and residency-off remain. No60FPS
or original Verified claim. See PERFORMANCE_TASK0072.md and integration contract.


## CPU submission repair phase

Initial authority: no Godot process, including headless. User subsequently
renewed permission for necessary Godot validation while away. Use existing JSON, isolated
builds and native-only checks. Add versioned bulk vehicle parameter snapshots,
relevant-source depth discovery/dirty synchronization, reused submission handles
and same-view worker submission math. Preserve core/sector/physics/order; new
candidates off,0/Safe unchanged. Report offline CPU work separately from game
FPS/tail acceptance. Baseline/core hashes:local/task0072/cpu-submit-phase/.


## CPU submission repair checkpoint (2026-10-20)

Bulk/versioned staging, source/order/field depth tracking, stable Draw handles
and contiguous submission math tasks are implemented. Debug/Release builds pass;
native Debug5/5,Release2/2 and Python7+7+21 checks pass. Port parity passes8759
checks/723 native ticks/seven native-resolution image checkpoints, including
reflections and reload. No original Verified promotion. All552 core source hashes
are unchanged; source review approves39 prior/102 resulting host identities.

User renewed necessary Godot permission. One incomplete timing reference was
excluded after finding/fixing an all-material eviction cleanup walk. Four settled
sequential timing processes completed: reference versus2-worker math+staging+
discovery for real drive_spin and viewer short_spin, each120 warm/600 x3,
2880x1695 Debug/Profile, Safe. Exact comparison protocols pass; performance fails.
Driving median37.527/37.752/37.515 ->37.417/38.182/38.611ms;
P9548.054/48.030/48.094 ->48.665/48.136/50.753. Moving rotation median
17.433/18.352/18.168 ->19.910/20.442/19.020; P9527.358/28.223/28.525
->36.932/49.875/29.283. All pairs regress tails. Candidates remain off,0/Safe.

Uniform reads drop89.18%, but source proxy churn8185 per599 intervals remains;
discovery scans558->557. Small group dispatch/join and snapshot overhead remain.
Moved math TU is outside existing host-O2 coverage; this is a separate follow-up
compiler candidate, not a silently changed core option. Raw frame719 maxima
include synchronous profile export; sustained/export-separated acceptance remains
pending. GPU whole-frame time N/A. No60FPS claim; TASK0072 stays active.

Next: inspect paired CPU/detail metrics; fix benchmark export boundary, then
consider workload granularity and explicit preparation compiler coverage, and
reduce lifecycle maintenance within existing source lifetimes/order. No further
ad-hoc window run is queued. Profile-off, independent ablations,4-worker game
timing, five-minute soak and PS2/PCSX2 comparison are still outstanding. Exact
commands/results/limits:docs/PERFORMANCE_TASK0072.md. Captures remain ignored local/.
