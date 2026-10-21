# TASK-0072 performance and visibility results

## Outcome and environment

GPU-resident Sun count transport removes the normal synchronous CPU readback and
passes the independent benefit gate on all three accepted Track11 routes. It is
the selected default for the supported domain, with explicit CPU reference
fallback retained. The complete60FPS objective is not achieved: only the fixed
route meets median andP95 <=16.667ms in every repeat. Dense and short routes still
exceed the frame budget. The task remains active; no original function is Verified.

Recorded environment: Windows, Intel(R) Arc(TM) B390 GPU, Vulkan1.4.359,
Forward+, Godot4.7.2.stable.official.ed1daf0bf (full engine hash
ed1daf0bf001b61586d9930840f2f1394092c079), Debug extension, GCC13.2,
CMake3.29.2, godot-cpp e4b7c25e721ce3435a029087e3917a30aa73f06b.
Debug and Release use isolated projects under local/task0072; the user's loaded
workspace DLL was not overwritten during tests. Screenshots, raw captures,
binaries and source identity manifests stay in ignored local/.

## Accepted v3 paired measurements

Every route uses2880x1695,120warmup frames,600samples and three repeats, VSync off
and unlimited FPS. The actual TrackViewer process/UI runs with the selected
CORVETTE hidden, production native car updates retained, mover2 and visual_state{};
there is no driving-mode environment capture origin. Effects, internal rendering
resolution, logical focal H, authored LOD and original visibility are unchanged.
This is a free-browse comparison, not the earlier paused-race workload.

The supplied HP2 camera position is(-238.34,-455.37,-34.04). Its original basis
was not supplied. Heading180 is a synthetic horizontal port-side orientation,
chosen after four exploratory direction probes found it densest (813draws versus
293/490/523). Neither the direction probes nor this pose establish an original
PS2 camera. Fixed holds it; dense adds an in-place25degree yaw sweep; short uses
the deterministic6unit lateral/12unit forward closed route.

The v3 wrapper disables external camera input and uses an isolated application
name/user-data directory. Strict paired checks pass: identical final pose bits,
native packet/selection, requested configuration and one production viewer update
per sample. Only --track-sun-gpu-resident changes false -> true. Compact history,
bulk upload and asynchronous Sun telemetry are off in these Sun-only comparisons.
Numbers below are repeats0/1/2, in milliseconds; samples include the same Profile
UI workload in each paired run.

| Route | CPU reference median | GPU Sun median | GPU Sun P95 | GPU Sun P99 | Every repeat60FPS |
|---|---|---|---|---|---|
| Fixed |21.950 /21.399 /22.339|13.600 /13.322 /13.488|14.899 /14.494 /14.526|18.200 /17.968 /17.652|Yes|
| Dense |24.958 /24.475 /25.231|15.580 /15.836 /15.848|19.413 /19.431 /19.626|21.781 /21.353 /21.619|No|
| Short |24.375 /24.985 /25.591|15.739 /16.283 /16.692|18.985 /19.767 /20.108|21.096 /21.889 /22.391|No|

Fixed GPU median FPS is73.53/75.06/74.14; dense64.18/63.15/63.10;
short63.54/61.41/59.91. P95-equivalent FPS is approximately67.12..68.99,
50.95..51.51 and49.73..52.67 respectively; medians alone do not establish60FPS.

Median paired savings exceed baseline repeat-median ranges: fixed8.350ms versus
0.940ms, dense9.378ms versus0.756ms, short8.702ms versus1.216ms. No pairedP95/P99
regression occurs. All three independent Sun enable gates pass; the stricter
all-repeats60FPS gate passes fixed only. Stored comparisons:
local/task0072/measurements/v3-{fixed,dense,short}-comparison.json; input runs are
v3-viewer-{baseline,gpu}-{fixed,dense,short}/run.json.

Invalid/preliminary v1/v2 records are excluded. viewer-baseline-fixed had a script
error and viewer-gpu-dense was incomplete. A later fixed-pose pair had external
camera movement and failed pose identity; its attractive early numbers are not
accepted evidence. These records remain as counterevidence. Race-context numbers
include a live vehicle/reflection workload and cannot stand in for TrackViewer.

## Independent candidates and remaining frame-time limits

The v4 dense compact-history candidate has valid protocol and no tail regression,
but its paired median saving0.759ms does not exceed baseline repeat range0.940ms.
Therefore its default enable gate fails. Baseline medians15.725/15.984/16.665ms
become14.993/15.225/15.215ms; candidateP95 remains18.371/18.868/19.147ms.
Keep compact history independently switchable and off by default. Evidence:
local/task0072/measurements/v4-compact-comparison.json.

The independent bulk UI upload candidate replaces per-element PackedArray ABI
appends with pre-sized direct writes of the identical vertices/UVs/colors/indices.
Its exact frozen-panel GPU pixel contract passes with compact history both off
and on. v5 independent dense timing is still in progress at this report checkpoint;
no timing benefit, enablement or combined60FPS claim is made for bulk upload here.
The root task will append final results/default decisions after its paired gate.

Profile JSON serialization/export remains on the main thread. Accepted v3 samples
still contain roughly500ms maxima (GPU fixed497.204ms, dense493.620ms,
short501.276ms) around automatic export. These stalls are retained in the raw
samples and are not concealed by the median/P95 target. Fixed-route acceptance
is bounded to those quantiles; it does not mean stutter-free rendering or every
frame <=16.667ms. Dense/short UI, CPU submission and export work remain next
measurement targets. Unverified direct viewport GPU values are unavailable;
raw diagnostic readings are never added into a whole-frame GPU budget.

## Preservation, parity and diagnostics

Read-only Ghidra findings E-0072-V01..V04 and E-0072-S01 retain separate Retail
SLUS_203.62 SHA2567335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2
and Debug NFSHP2.ELF SHA256815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b
provenance. Constructor near/far.5/10000 is not a universal drawing distance.
The existing zone/tree/AABB/flags/projected-size/FarClipSize selection chain is
retained; no evidence justifies shortening global far to gain FPS. Mirror and
sky have different depth ranges. Full Track11 camera-key/coordinator selection
and every exceptional reflection override remain unresolved. See
[visibility audit](VISIBILITY_TASK0072.md) and
[Sun evidence](../reverse/task0072/sun.md).

The Sun adapter preserves original32x32 sampling, depth<2 threshold, alpha byte
calculation and prior-completed-result consumption with initial0. Normal supported
GPU mode exposes no synchronous CPU count and records0readback bytes/0ms;
unsupported original blend domains retain the explicit reference path.

Validation already recorded at this checkpoint:

- 16521 synthetic actual-GPU counter/alpha/clipping/stamp checks pass; this
  supersedes the earlier16467-case fixture total. Counter texture generation and
  producer frame are carried explicitly. Synthetic tests alone do not prove
  production scheduling.
- Production30frame CPU/GPU sequences pass720 exact checks at2880x1695, including
  native packets and raw color/native-depth/final images. They cover initialization,
  movement below the original road, offscreen rotation, disable/resume and clear/
  reload. Earlier960x540 checks also passed. All native-resolution manifests
  report complete=true with no errors under local/task0072/parity/native-{cpu,gpu}.
- Asynchronous diagnostic telemetry passes clear/reload lifetime and blocked-road
  checks. It validates stamped producer/generation identity, rejects stale reused
  textures and never drives gameplay. Current ordinary GPU count remains null;
  delayed diagnostic availability is explicitly labeled rather than fabricated.
- Frozen240-entry Profile graphs have exact append/bulk raw-image equality in
  both compact modes (local/task0072/profile-upload/result.json). This proves the
  bounded UI upload contract, not performance or PS2 rendering equivalence.
- Relevant native tests passed8/8: track_topology, track_effects,
  track_environment, track_postprocess, track_submission_cache,
  track_command_batches, track_runtime and track_texture_orientation.
- Profile native rules and contract tests passed; the history-cache host test
  initially timed out on a fixture RefCounted/Node type mismatch, then passed
  after the fixture correction. GPU regression fixtures initially failed on a
  missing shader helper in test code; corrected encoded transport, sky depth
  and postprocess reruns pass3/3. Native culling, particles and postprocess
  lifecycle had passed the first series. Initial failures are retained in logs.
- Strict comparator rejection tests pass20methods with subcases, covering
  contaminated camera input, duplicate/missing updates, abbreviated samples,
  native/pose/configuration changes, exact optimization flags, noise thresholds
  and per-repeat frame budgets. These are pure Python gate tests, not new GPU data.

Main logs: local/task0072/{native-tests,profile-tests,profile-cache-tests,
gpu-regressions,gpu-regressions-retry,sun-counter,sun-telemetry,profile-upload}.log.
Debug/Release compiled in isolated outputs; final full source/progress validation
and the bulk decision are owned by the root task and remain to be finalized.

## Reproduction and original-reference blocker

Use [benchmark instructions](BENCHMARK_TASK0072.md). One baseline example:

```powershell
.\tools\run_track_costs.ps1 -ProjectPath .\local\task0072\candidate\godot -OutputDirectory .\local\task0072\measurements\reproduce-baseline-fixed -Context viewer -Car CORVETTE -Tracks 11 -Pose user_capture -Headings 180 -Route fixed -Width 2880 -Height 1695 -WarmFrames 120 -Samples 600 -Repeats 3 -ProfileUi -Candidates @('--track-sun-gpu-resident=false','--profile-compact-history=false','--profile-ui-bulk-upload=false')
```

Repeat in a separate output directory with only Sun=true; run dense/short likewise.
Compare each pair with `py -3 tools/task0072_performance.py BASELINE/run.json
CANDIDATE/run.json --output COMPARISON.json`; optional --optimization compact/bulk/
combined requires exactly that mode's false->true flags. The benefit gate is
separate from native/pixel/depth preservation and the60FPS quantile gate.

Matched PS2/PCSX2 validation remains blocked. Read-only probes on2026-10-20 to
PINE28011 and JSON DebugServer21512 each timed out with a0.25second socket timeout
(observed269/259ms); no emulator process was available. Existing local settings
have PINE enabled but3x scaling and widescreen patches, which are not a matched
native reference. No emulator was launched or settings changed during benchmarking.
An identified runnable original Track11 state, recorded settings and matched
camera/game state are still needed. TASK0071 HDR/per-primitive COLCLAMP and
reflection precision gaps remain open; exact port-path parity cannot close them.
No PS2-equivalence or Verified claim is made.


## Actual driving and rotating-camera investigation

The free-camera `short_spin` fixture traverses the synthetic short translation
path and two full yaw revolutions. The original screenshot basis remains unknown.
Unoptimized render-host Debug medians20.122/27.212/27.736ms and
P9537.079/40.317/40.727ms became14.219/14.374/14.216ms and
P9520.085/19.971/19.700ms with the host-only compiler candidate. These are
exploratory older captures, not an all-scenario60FPS acceptance. Core static
archives were byte-identical in that compiler comparison; the import library
changed as expected.30native-resolution lifecycle frames/720exact native,
colour/depth/final comparisons passed for this compiler candidate.

The actual production TrackViewer Test drive wrapper records fixed semantic
throttle0.35/steering0, native scheduler time, body/camera bits, wheel contact,
terrain and each production tick call. Native scheduling can hold a physics
step. A render frame must issue exactly one tick, while native step_delta0or1
is preserved; no forced extra tick or adjusted timestep is permitted. The early
hold==failure assumption was retired after reading Session::tick and recording
a legitimate first-frame hold. Pilot and first reference matrices containing
incomplete rows remain excluded. Startup focus/discard diagnostics are retained;
a cleared discard latch was not evidence of the actual scheduler cause.

Strict v6-drive-reference and v6-drive-residency each completed120warm/600sample/
3repeats at2880x1695 Vulkan Forward+, Debug host compiler candidate, Profile UI.
All paired720-frame body/input/camera/native timestep timelines match. Contact
was known on all four wheels throughout all720frames, terrain1; this does not
establish membership of an original intended racing route.

| Driving + yaw | Repeat0 median/P95/P99 | Repeat1 median/P95/P99 | Repeat2 median/P95/P99 |
|---|---|---|---|
| Reference |56.006/73.673/81.089|57.896/73.173/84.547|57.722/76.292/85.065|
| Residency candidate |47.907/56.147/60.232|48.276/57.977/62.402|49.591/57.797/62.837|

Units arems. Median paired gain8.131ms exceeds baseline median range1.890ms;
no pairedP95/P99 regression. Each run fails the16.67ms target. The first v6
harness omitted the residency CLI field; strict comparison independently requires
the actual typed `configuration.residency_cache` value in all three rows rather
than assuming a default or modifying the raw captures.

E-0072-R01 documents observed proxy churn and residency accounting. The candidate
keeps at most4096inactive draws/128MiB inactive logical mesh payload for1800frames,
plus2048unused source meshes; active original views are never evicted to fit these
host budgets. These are not total process/GPU memory bounds. Default remains off
until image/lifecycle/stress gates complete. See reverse/task0072/residency.md.

Related new candidates remain independent and default-off: exact dynamic planner,
depth discovery watermark, staged vehicle BASE depth parameters, capture-view
world batching, async Profile export, and render-selection/port-UI Debug compiler
options. Staging must consume parameter values only at the old depth polling
point; it must not push later vehicle changes into the current depth pass.
Capture batching retains original selection and conservative contiguous eligibility.

The native-resolution moving/rotating Sun transport fixture passed720exact native/
colour/depth/final checks with residency+dynamic planner+depth discovery versus all
three disabled. Actual Test drive image/reflection parity and candidate performance
ablation are still in progress. No original-runtime Verified promotion.

Build note: an owned isolated GPU fixture held the candidate DLL during one link,
which failed with Permission denied; retry after fixture exit succeeded. Workspace
DLL was untouched. Subsequent links must await all owned fixture processes.


## Worker integration outcome,2026-10-20

Implemented the transport-only architecture documented in
[RENDER_PREPARATION_TASK0072.md](RENDER_PREPARATION_TASK0072.md). New code has no
Godot dependency in the CPU builder and changes no recovered core/physics body.
Worker counts0/2/4 mean bridge concurrency limits; native observed peaks in the
actual parity captures reached1/2/4 respectively, not guaranteed physical cores.
Startup and exports distinguish requested metadata from actual render mode.

| Check | Result |
|---|---|
| Frozen pre-split DLL vs split0/Safe | PASS:723 tick calls, seven native-resolution checkpoints,8759 comparisons |
|0/Safe vs2/Safe | PASS:8759 comparisons including main/depth/final/all submitted reflection faces/atlas |
|0/Safe vs4/Safe | PASS:8759 comparisons including unload/reload |
|0/Safe vs0/2/4 Separate | FAIL:colour/final/reflection hashes differ; original numeric command/driving checks retained |
| Separate shutdown | FAIL:RenderingDevice::finalize reports wrong render thread;2 ObjectDB instances leaked |
| Native tests |28/28 initial relevant tests;8/8 settled focused CPU/configuration/harness checks |
| Pure preparation |200 randomized batches, reversed completion, immutable lifetime, invalid subranges/indices/colour |
| Engine preparation fixture |0/2/4 exact packed attributes/CUSTOM0..3, deduplicated misses/cache hits, joined error recovery |
| Full performance matrix |NOT RUN: stopped before first timed run at user's request to stop repeated Godot launches |
| Profile-off, cold timing and five-minute soak |NOT RUN; no new FPS/P95/P99 result or default promotion |

Private records:local/task0072/workers/worker-baseline (actual preserved baseline
is local/task0072/worker-baseline),parity-legacy-v2,parity-final and build/test logs.
All capture data remains ignored. The current final CPU metadata error handling
was additionally covered by startup-error/mismatch tests; parity rendering logic
is identical. No matching PS2/PCSX2 oracle was available or claimed.

The first matrix was contaminated by inherited focus notifications: overriding
_notification did not suppress the base handler, so native driving could pause.
Those failed/interrupted runs are excluded. An explicit _viewer_notification
virtual hook isolates fixture focus while preserving production focus behavior;
regression tests exercise delivery through the base handler. No tick was skipped,
restarted or replaced with a teleported camera to obtain a valid route.

Separate results are not accepted as equivalent. The colour/reflection mismatch
could involve deferred update/consumption timing, but this is a hypothesis; no
cause was established. The shutdown error is observed in the full project only.
An empty-engine attribution probe was queued but cancelled before launch, so do
not call it a proven upstream engine defect. Do not hide the error or add an
extra frame of latency as a workaround.

The sector mechanism remains active: current driving checkpoints stay in zone71,
select14/128 loaded sections and initially consider235/8407 instances, retaining106.
The worker phase consumes only these selected native commands. New native tests
cover a second sector, neighbouring zone, return transition and original no-zone
fallback. The short driving fixture does not cross an original sector boundary;
those boundaries are covered by synthetic tests only. The original full camera
coordinator and intended route membership remain unresolved.

Defaults stay0/Safe. Residency stays off after its previously observed reflection
regression. Median/P95<=16.67ms is not established; TASK0072 remains active.
Graphical runs and the queued matrix were stopped on user instruction. Do not
resume automated windowed testing without a new request. Next independent work:
review the existing per-view preparation counters and Separate image deltas,
then agree a single consolidated timed run if performance capture is requested.


## Worker integration outcome,2026-10-20

Implemented the port-side architecture in [integration contract](RENDER_PREPARATION_TASK0072.md).
Recovered core/physics bodies are unchanged. Native observed preparation concurrency
peaks were1/2/4 for requested0/2/4; this does not guarantee physical cores.

| Check | Result |
|---|---|
| Frozen pre-split DLL vs split0/Safe |PASS:723 tick calls, seven native-resolution checkpoints,8759 comparisons|
|0/Safe vs2/Safe |PASS:8759 comparisons including main/depth/final/reflection faces/atlas and reload|
|0/Safe vs4/Safe |PASS:8759 comparisons including unload/reload|
|0/Safe vs0/2/4 Separate |FAIL:colour/final/reflection hashes differ; native command/driving checks match|
|Separate shutdown |FAIL:RenderingDevice::finalize wrong-thread error and2 ObjectDB leaks|
|Native checks |28/28 initial relevant tests;8/8 settled focused CPU/configuration/harness checks|
|Pure preparation |200 randomized batches, reversed completion, immutable lifetime, invalid subranges/indices/colour|
|Engine preparation fixture |0/2/4 exact packed attributes/CUSTOM0..3, deduplicated misses/cache hits, joined error recovery|
|Debug/Release |Isolated builds pass; Release CPU preparation test passed|
|Performance/Profile-off/5minute soak |NOT RUN: user stopped repeated Godot launches before the first timed run|

No new FPS/P95/P99 result or default promotion. Source and logs are under ignored
local/task0072/worker-baseline and local/task0072/workers/. Parity-final contains
all six configurations and paired comparisons. Builds used Godot4.7.2.ed1daf0bf,
godot-cpp e4b7c25e721ce3435a029087e3917a30aa73f06b and GCC13.2.0. Exact native
commands: ctest --test-dir build/godot-debug -R '^(mesh_prepare|track_runtime|native_render_workers_rules|native_profile_performance_rules|render_worker_configuration_host|render_worker_invalid_configuration_host|render_thread_mismatch_host|driving_performance_contract_host)$' --output-on-failure.
The settled8/8 run includes startup errors and the last metadata-only boundary
change; rendering logic is unchanged from the full parity matrix. Final Release
configure/build are in release-configure-final.log/release-build-final.log.

Earlier matrix runs were contaminated by inherited focus notifications: overriding
_notification did not suppress the base handler. Those paused/interrupted runs
are excluded. An explicit _viewer_notification hook now isolates fixture focus
while preserving production behavior; regression tests exercise base delivery.
No native tick is skipped, restarted or replaced by a teleported camera.

Separate is not accepted as equivalent. Deferred update/consumption timing is a
hypothesis for the image difference, not an established cause. The shutdown error
was observed in the full project only. An engine-only attribution probe was
cancelled before launch, so an upstream engine defect has NOT been established.
No extra frame of consumption latency or suppressed exit error is introduced.

Native sector selection is active: the sampled driving checkpoints stay in zone71,
select14/128 loaded sections and initially consider235/8407 instances, retaining106.
Workers consume only selected native commands. New tests cover excluded sectors,
a neighbouring zone, return transition and the original no-zone fallback. The
actual short driving fixture does not cross an original sector boundary; those
transitions are covered by synthetic tests only. Original camera coordinator and
intended route membership remain unresolved. No PS2/PCSX2 Verified promotion.

Defaults remain0/Safe. Residency remains off after its prior reflection regression.
Median/P95<=16.67ms is not established; TASK0072 remains active. All owned queued
and running graphical tests were stopped. Do not resume automated windowed runs
without a new user request. Next: inspect existing per-view CPU counters/Separate
image deltas; if requested, perform one consolidated timed capture rather than
repeated ad-hoc Godot launches.

Within Separate,0->2/4 preparation preserves the same image/state hashes; both
pairs still fail lifecycle because of the shared shutdown error. The observed
image regression is Safe versus Separate, not an additional worker-count delta.

Final live source protection audit_task0072_review.py --write passed:35 prior
entries/72 resulting port entries, no missing/changed or unreviewed additions.
The individual performance21/threading7 tests and focused8/8 run passed. An
additional aggregate Python discovery was cancelled before completion; do not
report it as37 passing tests. Generated progress and scoped diff checks passed.
No commit was created; the workspace DLL was not overwritten.

Final source protection: audit_task0072_review.py --write PASS,35 reviewed prior
entries/72 resulting port entries, no missing/changed or unreviewed additions.
All37 TASK0072 Python performance/threading/source-review contract tests pass;
generated progress check passes. No commit was created. Workspace DLL unchanged.


## CPU submission repair phase: existing evidence and implementation

User renewed permission for necessary Godot runs on2026-10-20. Earlier testing
stop statements above describe the prior checkpoint, not the current authority.
Existing v6-drive-reference JSON independently reports reflection CPU median
9.74..9.79ms, post/depth6.0..6.2ms, main submit3.7..3.8ms; inclusive scopes overlap.
Across599 adjacent sample intervals, uniform reads median2789 (total1767926),
proxy creations total8185 and discovery visits1254037. There were558 discovery
scans. These correlate with stutter but do not establish a single causal source.
The residency candidate reduced churn but failed reflection parity and stays off.

`tools/analyze_cpu_submission.py` preserves all frame percentiles, lists>=200ms
extremes separately and computes correlations only on ordinary frames. Counter
resets are excluded rather than treated as savings. Raw output:
`local/task0072/cpu-submit-phase/existing-cpu-analysis.json`. Existing mesh-only
worker checkpoints after cold creation had0..1 tasks; increasing that worker
limit alone cannot address continuing submission/depth work.

Implemented port-side changes: one bulk/versioned vehicle parameter snapshot,
source-structure/order-driven discovery, per-field depth synchronization, stable
Draw handles/StringNames and contiguous per-view CPU arithmetic in the existing
Godot native worker group. Core selection and sector filtering are unchanged.
Initial candidates remain off,0/Safe; this is not a60FPS completion claim.

Debug and Release isolated DLL builds succeeded with official Godot4.7.2,
godot-cpp e4b7c25e721ce3435a029087e3917a30aa73f06b and GCC13.2. Both CMake project
and binary destinations are private. Native tests never launch Godot; the pinned
math test links its static math library only. Five focused Debug tests and two
new Release tests pass; offline analyzer tests3/3 pass.

Actual Vulkan Forward+ checks identified Intel Arc B390, Vulkan1.4.359. Candidate
2/Safe with submit math, vehicle staging and depth discovery enabled passes8759
checks against preserved0/Safe driving reference:723 native tick invocations,
seven2880x1695 colour/native-depth/final/reflection checkpoints and unload/reload.
Runtime error log is clean. Logs/captures are ignored under
`local/task0072/cpu-submit-phase/parity-safe2/`; comparison is
`parity-comparison.json`. This is combined PC parity, not independent ablation
or original PS2/PCSX2 verification. A subsequent diagnostics-only change adds
snapshot accounting and does not change generated numeric/render data.


One preliminary timing reference was intentionally stopped and excluded before
completion: code review found a newly added all-material cleanup walk on mesh
eviction. It was replaced with a reverse mesh-to-material index, bounded by that
mesh's bindings, and association tracking is inactive when discovery is off.
The abandoned capture is retained under aborted-performance-drive_spin-reference;
it is not an accepted sample or a performance comparison. Settled paired captures
must use the subsequent isolated DLL. Earlier combined parity predates this
bookkeeping-only cleanup; defaults remain off without final individual gates.


Settled native CPU fixture (120 warm/600 samples,2000 synthetic cached-math
entries; persistent std::thread group using the production callback):

| Build / workers | Median ms | P95 ms | P99 ms |
|---|---:|---:|---:|
| Debug /0 |0.6504|0.9295|1.0170|
| Debug /2 |0.4401|0.5463|0.6311|
| Debug /4 |0.3030|0.4832|0.6159|
| Release /0 |0.0818|0.1715|0.2096|
| Release /2 |0.0838|0.1279|0.1461|
| Release /4 |0.0555|0.0911|0.1378|

These timings exclude input snapshots, Packed conversion, engine API, uploads and
Godot WorkerThreadPool scheduling. Native Debug is unoptimized; the production
Debug bridge keeps its prior host-O2 option. This fixture cannot establish game
FPS or select a production worker count. Input/output storage is976000/576000
bytes per2000-entry fixture; per-view snapshot cost must also be measured.

Reproduction commands (all captures under ignored local/):

```powershell
cmake --preset godot-debug -DPORT_GODOT_PROJECT_DIR=D:/dev/hp2_godot/local/task0072/candidate/godot -DPORT_GODOT_BIN_DIR=D:/dev/hp2_godot/local/task0072/cpu-submit-phase/bin
cmake --build build/godot-debug --target ps2_port submit_math_tests depth_sync_tests -j 6
ctest --test-dir build/godot-debug -R '^(submit_math_native|parameter_staging_registry|mesh_prepare|track_runtime|depth_sync_native)$' --output-on-failure
cmake --preset godot-release -DPORT_GODOT_PROJECT_DIR=D:/dev/hp2_godot/local/task0072/candidate/godot -DPORT_GODOT_BIN_DIR=D:/dev/hp2_godot/local/task0072/cpu-submit-phase/release-bin
cmake --build build/godot-release --target ps2_port submit_math_tests depth_sync_tests -j 6
ctest --test-dir build/godot-release -R '^(submit_math_native|depth_sync_native)$' --output-on-failure
./build/godot-debug/submit_math_tests.exe --bench
./build/godot-release/submit_math_tests.exe --bench
py -3 -m unittest discover -s reverse/tests -p test_cpu_submission_analysis.py
py -3 tools/task0072_driving_compare.py local/task0072/workers/parity-final/safe-0/run.json local/task0072/cpu-submit-phase/parity-safe2/run.json --output local/task0072/cpu-submit-phase/parity-comparison.json
```

The timed matrix uses tools/run_track_costs.ps1 with Context driving/Route
drive_spin or Context viewer/Route short_spin, Track11, CORVETTE, Height1695,
Width2880, WarmFrames120, Samples600, Repeats3, ProfileUi, RenderThread safe.
Reference uses PrepareWorkers0 and sun resident true; repair uses PrepareWorkers2
plus submit math, vehicle staging and depth discovery true. Both use the same
settled Debug DLL and prior host compiler options; neutral handle/StringName
refactoring is present in both. Combined results cannot attribute gains to each
switch. The strict comparison mode cpu_submit_repair requires exactly these
three false->true flag changes and0/Safe->2/4/Safe; actual math state must agree.
No default promotion can use this combined diagnostic alone.


## Final paired result: candidates rejected for default enablement

Track11/CORVETTE,2880x1695,Vulkan Forward+,Debug+Profile UI,120 warm/600 measured frames x3. All four processes exited cleanly. Deterministic driving trajectories and native selection evidence match. Both strict configuration comparisons pass; both performance and60FPS gates fail. These are combined-switch diagnostics, not independent ablations.

| Route / repeat | Reference median / P95 / P99 ms | Repair2 median / P95 / P99 ms | Reference / repair maximum ms | Repair FPS |
|---|---:|---:|---:|---:|
| drive_spin/0 | 37.527 / 48.054 / 54.635 | 37.417 / 48.665 / 55.375 | 894.689 / 840.230 | 26.73 |
| drive_spin/1 | 37.752 / 48.030 / 54.374 | 38.182 / 48.136 / 55.522 | 807.829 / 811.650 | 26.19 |
| drive_spin/2 | 37.515 / 48.094 / 53.526 | 38.611 / 50.753 / 56.792 | 803.861 / 815.022 | 25.90 |
| short_spin/0 | 17.433 / 27.358 / 32.685 | 19.910 / 36.932 / 50.410 | 664.432 / 678.671 | 50.23 |
| short_spin/1 | 18.352 / 28.223 / 35.583 | 20.442 / 49.875 / 66.344 | 667.602 / 670.667 | 48.92 |
| short_spin/2 | 18.168 / 28.525 / 33.795 | 19.020 / 29.283 / 34.340 | 678.441 / 685.906 | 52.58 |

Driving median paired gain is -0.430ms (baseline repeat range0.237ms). Moving
rotation gain is -2.090ms (baseline range0.919ms). Every paired repeat has a
P95/P99 regression. Do not enable this combination or select a higher worker
count on these data. Defaults remain0/Safe; math/staging/discovery candidates off.
Separate and failed residency remain off. No60FPS completion or Verified claim.

CPU attribution, preserving inclusive scope overlap:

- Driving reflection median6.47..6.64 ->7.19..7.30ms; main2.26..2.34
  ->2.61..2.67ms; post/depth3.67..3.75 ->3.60..3.72ms.
- Driving uniform-read increments fall1767926 ->191358 per599 intervals
  (89.18%), but proxy creations remain8185. Discovery scans558 ->557:
  real source lifecycle churn still invalidates discovery almost every frame.
  Suppressing unrelated viewport/UI invalidations does not remove this work.
- Main driving math adds529 median entries: draw snapshot0.138..0.142ms
  versus0.079..0.084ms; group wall0.164..0.170ms and join0.153..0.159ms
  overlap. Moving rotation has259.5 median entries and group wall0.189..0.201ms
  versus prior mesh-only0.089..0.091ms. These are per-view scopes, never summed
  with each other or inclusive frame totals. Preparation snapshots from older
  reflection source frames are excluded (1800 rows per driving repeat).
- The moved arithmetic executes in track_mesh_batches.cpp. Existing Debug
  host-O2 source coverage does NOT include this translation unit; it retains
  -g,strict FP and exceptions. ps2_track_renderer.cpp remains host-O2. The two
  captures use identical compiler settings, but source placement matters. A
  future compile-coverage candidate must be explicit and separately paired;
  no native core compiler changes are authorized or made here.
- Whole-frame/source-verified GPU time remains N/A (zero verified direct samples).
  Root/per-viewport query values are retained as unverified diagnostics only.
  Resident Sun CPU completion is measured separately; it is not a GPU fence-time
  decomposition and must not be presented as whole-frame GPU time.

Each raw maximum occurs on logical frame719, with pipeline compilation delta0.
The profiler automatically synchronously exports at its600-sample limit inside
that frame (begin_capture(0,sample_count); profile_debug.gd auto export path).
All raw maxima above retain that export event. They are not sustained-render
maxima; source inspection supports export overlap, not exclusive attribution of
every microsecond. Export-separated sustained acceptance remains pending. Do
not silently remove these frames. Follow-up: defer benchmark export until after
sampling while preserving600 profiler records, then compare small-work serial
preparation, explicit bridge compilation coverage and depth lifecycle work. No
additional Godot run was made after these four consolidated timing processes.

Native checks: Debug5/5,Release2/2; Python analyzer/protocol7/7,worker7/7,
performance21/21. PowerShell parser/help validates the three explicit candidate
flags without launching an engine. Source protection passes39 prior/102 resulting
review entries, with no unreviewed identities or changed stubs; all552 game_core
.cpp/.hpp files match phase-start hashes. Workspace DLL remains untouched.
No commit was created. Profile-off, individual ablations, actual4-worker timing,
five-minute soak and original PS2/PCSX2 acceptance remain NOT RUN in this phase.

Continuation: TASK-0072, inspect paired-cpu-analysis.json and
paired-detail-metrics.json under local/task0072/cpu-submit-phase; preserve the
four raw captures and rejected comparisons. Do not promote the combined result.
Use tools/task0072_performance.py --optimization cpu_submit_repair for the exact
three-flag0/Safe->2/4/Safe configuration check; use the recorded native test commands
before further adapter changes. All current tasks remain active where their full
acceptance criteria are incomplete.
