# TASK-0077 - Render baseline and submission boundaries

- Status: active
- Authorization: implement the Fukami-inspired plan, starting with P0 only. P1 submission boundaries, P2 depth source lifetime, and P3 build organization follow independent acceptance; they are not implemented in this phase.
- Scope: port-side benchmark sampling/export separation, explicit environment-capture policy, immutable run identities, A/A controls and first-divergence reports. No recovered code, physics, renderer defaults, or native compilation changes.
- Baseline: existing dirty work preserved; exact edited inputs and game_core hashes saved under ignored local/task0077/baseline. Isolated projects use a copied current TASK0076 DLL; never replace a loaded workspace DLL.

## Acceptance

- [ ] Bounded Profile collection stops at its limit without snapshot/export inside benchmark measurement, including long stress captures; exports remain observable and errors invalidate the run.
- [ ] Record source/DLL/engine/build-recipe identity before and after execution; reject mutation or missing identity rather than claiming reproducibility.
- [ ] Default environment capture stays off; explicit opt-in is a distinct scenario with actual effects reported.
- [ ] A/A controls precede candidate comparison; diagnostics identify the first frame/view/command difference where evidence exists, and never fabricate unavailable command detail or GPU times.
- [ ] Relevant asset-free and engine contracts pass; run sequential isolated route controls and report exact scope and limitations.
- [ ] Source protection, task/report/log/architecture decisions and generated progress updated.

## Findings and next action

E-0077-P01: ProfileDebug._process currently calls export_profile at its limit; synchronous JSON/hash/file work overlaps the final measured frame. track_performance._scenario_settings assumes environment=true for non-default scenarios even though TASK0076 defaults it off. Preserve legacy records as counterevidence; new controls must explicitly bind the current policy.

Next: implement held export, explicit capture configuration and identity/control reporting; validate without changing recovered source or optimization defaults. Original runtime equivalence and 60FPS are not acceptance claims for this tooling task.

## Related commits

## Task switch checkpoint (2026-10-21)

The user switched priority to concise automatic progress (TASK0078). P0 source
changes and initial contracts remain; the sequential graphical matrix was
interrupted and contains failed/incomplete runs. Completed process/identity
checks do not establish A/A acceptance. Preserve local/task0077/matrix-status.json
and raw logs; do not promote P0 completion or start P1-P3 from these data. No
owned Godot/matrix process remained at the TASK0078 startup check. Further P0
comparison/recovery is deferred; this new documentation task launches no Godot.

None created.

## 2026-11-11 production Championship continuation

Scope: audit the production PS2 instruction-execution boundary and collect
three isolated actual-window first-Championship baselines, then isolate one
host-side rendering candidate. Keep original simulation, visibility, ordering,
materials and resource root policy. See TASK-0018 for the production call chain.

Initial source and PE audit found no instruction executor dependency. NIS
first-use asset decoding and per-pose native skin/upload remain separately
identified costs. The first two captures reached source steps 176 through 775
at 1920x1080, with 600 samples and held export. The source-step window agrees,
while independently delivered physical input and exact poses can diverge.
Depth proxies are already hidden and reused while their source is alive.
The initial baseline observed 572 source discovery scans and 1,046 proxy
creations across 599 sampled intervals. Isolate the existing source discovery
cache before considering retention across real source destruction. Do not
enable any candidate without a stable timing and image/lifecycle gate.

Baseline evidence is under ignored `local/task0077/championship-a1` through
`championship-a3`. Each capture used the real Class 5 Parkland Race (Track 12,
TS50 skin 2, manual transmission, five authored cars), 120 warm frames and
600 samples at source steps 176..775. Vulkan Forward+, VSync enabled, 60 FPS
cap, profiler panel hidden. The physical driver used owned Win32 window
messages with the read-only localized road. The first exact input divergence
from A1 was sample 0 for A2 and sample 21 for A3. This is route-level sampling
with asynchronous input, separate from deterministic frame parity.

| Repeat | Wall median / P95 / P99 ms | Max ms | Depth discovery scans | Depth node visits |
| --- | --- | --- | --- | --- |
| A1 | 78.623 / 93.330 / 108.158 | 185.073 | 572 | 1,097,596 |
| A2 | 75.874 / 87.849 / 179.829 | 441.063 | 565 | 1,085,864 |
| A3 | 80.798 / 93.985 / 106.513 | 188.095 | 553 | 1,065,337 |

All 1,800 samples exceed the 16.667 ms budget in this captured Debug
configuration. A1 inclusive median World tick plus bridge result was 20.808
ms, track view 24.038 ms, depth/post 11.043 ms (inside track), vehicle view
6.921 ms and reflection submit 0.004 ms. GPU viewport timings are delayed
completed-source-frame pairs and do not sum to a frame time. The worst A1
frame had no depth proxy creation or source mesh upload, so those counters
alone cannot explain the frame-time tail. Track and vehicle loading pumps
and the initial player's decode/upload timings are exported separately.

Verified setup: Godot 4.7.2.stable.official.ed1daf0bf, Intel Arc B390,
Vulkan 1.4.359, Python 3.14.4, CMake/CTest 3.29.2, godot-cpp
e4b7c25e721ce3435a029087e3917a30aa73f06b. Frozen extension SHA-256
2ca5b3affc287dc3f60af71e9dad6c89b7b1c036015273beadec8d54898b94b0.
Configured Ninja recipe hashes are recorded separately from the binary
identity. No native source was changed or relinked for these runs.

```powershell
ctest --test-dir build/native-debug -R '^(driving_schedule|event_world_original|race_finish_original|frontend_finish_original)$' --output-on-failure
py -3 tools/test_event_window.py --championship-performance --result-timeout 150 --output local/task0077/championship-a1
py -3 tools/championship_performance.py local/task0077/championship-a1 local/task0077/championship-a2 local/task0077/championship-a3
```

Native CTest passed 4/4 (10.13 seconds). Each window run passed 12 launch/input/
sample checks and pre/post input identity. Extend the first command's run
directory to a fresh name for reproduction. The independent candidate adds
`--performance-depth-discovery` to the window harness.

## Visual Profiler confirmation (2026-11-11)

Ran the same first Championship with the editor debug server and the built-in
Debugger > Visual Profiler enabled, plus the runtime --gpu-profile flag and
the existing source-tagged ProfileDebug export. The isolated 1920x1080 Vulkan
Forward+ run captured 600 driving frames at source steps 176..775. Runtime
profile and identity check are under local/task0077/championship-visual-profiler.
Launch wrapper: py -3 local/task0077/run-visual-profiler.py. The editor used
--debug-server tcp://127.0.0.1:6137; the runtime used --remote-debug on that URI.

The editor displayed Render Viewports at 4.84 ms CPU / 8.56 ms GPU on displayed
frame 1317. ProfileDebug identifies the per-viewport pairs across the race:
main colour median 3.12 ms CPU / 3.44 ms GPU, encoded depth (native_depth)
1.47 ms CPU / 2.20 ms GPU. These are separate render measurements. The host
TrackPostprocess::update median was 10.91 ms, within track-view wall time
23.47 ms. Whole frame median/P95/P99/max: 76.670/95.618/107.861/364.289 ms.

Worst sampled frame was source step 762: World tick including bridge return
313.427 ms, track view 22.253 ms, TrackPostprocess::update 10.484 ms.
The delayed GPU samples have their own source-frame numbering and were not
attributed to that precise wall frame. The next action is to split native
advance from bridge result construction and split depth host update into
discovery, proxy and material work. This confirms one Debug production
route; it establishes neither Release throughput nor PS2 parity.

## Championship timing split (2026-11-11)

Enabled C++ wall scopes only for --championship-performance. Tick measures
WorldSession::advance and get_state separately. Depth measures discovery
(including source cache checks), proxy synchronization exclusive of material
work, and shader variant/parameter work. Material assignment to a proxy is
part of proxy synchronization. The inclusive scopes also contain unclassified
setup and bookkeeping, and medians are independent statistics.

The Godot 4.7.2 editor Visual Profiler and runtime GPU profile were active for
the same 1080p Vulkan Forward+ Debug Championship run. All 600 samples
(steps 176..775) passed scope containment and input identity checks.

| Scope | Median ms | P95 ms |
| --- | ---: | ---: |
| World native advance | 20.6825 | 29.2529 |
| World bridge result | 0.05685 | 0.0721 |
| Depth discovery | 0.96135 | 1.2059 |
| Depth proxy synchronization | 3.0956 | 3.5461 |
| Depth material variants and uniforms | 7.07465 | 7.6996 |
| Inclusive depth postprocess | 11.22535 | 12.3676 |
| Encoded depth viewport GPU | 2.67917 | 5.22755 |
| Whole frame | 77.771 | 98.800 |

Worst frame 530.512 ms at step 607: native advance 474.1395 ms, bridge
result 0.0597 ms and inclusive depth postprocess 10.5542 ms. Whole-frame P99
155.368 ms. The five slowest samples all concentrate in native advance.
The next scoped investigation is native World update subphases for spikes,
and variant lookup versus uniform reads/writes for sustained depth cost.
This is a profiling change and does not establish Release or PS2 equivalence.

CMake 3.29.2 and Python 3.14.4. The two changed C++ objects compiled and
linked in the existing Debug recipe to an isolated DLL. SHA-256:
1e48c2d7cedbab8e71d342361d387d43d656e62b0ebe0810b8fb4f5e662c43e3.

```powershell
cmake -S . -B build/godot-debug -DPORT_GODOT_BIN_DIR=D:/dev/hp2_godot/local/task0077/split-bin
cmake --build build/godot-debug --target ps2_port --parallel 8
ctest --test-dir build/native-debug -R '^(driving_schedule|event_world_original|race_finish_original|frontend_finish_original)$' --output-on-failure
py -3 local/task0077/run-split-visual.py
py -3 local/task0077/analyze-split-visual.py local/task0077/championship-split-visual
```

Native tests passed 4/4 in 8.98 s. The wrapper attaches to the local editor
debug server at tcp://127.0.0.1:6137; start Visual Profiler before driving.
Use a fresh output name for a repeat. The configured normal DLL output path
was restored after capture. Artifacts are ignored under the run directory.

## 2026-11-11 independent vehicle depth staging candidate

TASK-0077 extends the existing opt-in vehicle staging transport to publish
the exact BASE texture and flag setter operands. The consumer keeps its
original read point and getter fallback. Native physics, recovered code,
viewport scheduling and original render selection are outside this change.

The isolated Debug DLL builds successfully. Parameter staging registry
(including the new slots, missing value, unchanged version, invalidation
and shader replacement), driving schedule and original event World passed
3/3 in 14.25 s. The Championship harness now verifies the effective
candidate flag and reports actual depth uniform reads/writes and snapshot
counters. Sequential A/B uses the Python owned-window keyboard driver.
The in-engine GPU profiler is enabled via --gpu-profile on both sides;
editor interaction is not used for this matrix.

Commands and artifacts are under local/task0077/vehicle-staging-matrix.
Default promotion remains gated on measured benefit and visual parity.

## 2026-11-11 Championship Release follow-up

Scope authorized by the user: diagnose and improve the production first
Championship race on Track 12, with an end-to-end 1080p Release P95 budget of
16.67 ms. Continue TASK-0077 with port-side timings, depth synchronization,
and independent submission/thread candidates.

- [ ] Frozen three-run Debug and Release race baselines, plus wall-only and
  uncapped comparisons with actual committed source steps.
- [ ] Attribute Replay, World timestep/publication, secondary view, shadow,
  depth and engine draw costs. Keep render-signal time distinct from GPU and
  OS-present timing.
- [ ] Accept a candidate only after repeated tail improvement, deterministic
  state/render checks and race lifecycle checks.

The wall-only sampler bypasses renderer diagnostics and viewport timestamp
queries. During sampling the owned keyboard test receives compact read-only
pose updates over loopback UDP instead of the observer's state files. The
driver uses the initialized road course in both wall-only and paired
diagnostic runs. It still submits ordinary Win32 input to the owned window.

The depth candidate reads a scoped main-thread staging view. It caches slot
indices and applies each parameter at its own version, with a getter fallback
for missing, invalidated, or unregistered data. Shader replacement resets
the consumer state. The existing experimental MultiMesh path can now use
authored static sections in a World frame, excluding overridden and
independent World objects from instanced spans. Track selection and command
order remain the source of the host submission.

### Release baseline and attribution

Captured three wall-only capped Safe/0 reference runs (A1–A3) and three
independent vehicle depth staging runs (B1–B3) in the same Release DLL. Each
accepted run contains 600 intervals with actual five-car driving, 120 warmup
frames and 12 real-window harness checks. The fixture uses asynchronous physical
keyboard messages, so committed poses are not bit-identical between repeats.

| 1080p run | P50 / P95 / P99 / max (ms) |
| --- | --- |
| A1 | 43.345 / 51.049 / 57.058 / 72.371 |
| A2 | 43.992 / 49.319 / 58.484 / 83.877 |
| A3 | 43.184 / 49.389 / 52.789 / 75.473 |
| B1 | 42.368 / 47.092 / 56.386 / 78.324 |
| B2 | 43.036 / 50.857 / 53.990 / 73.931 |
| B3 | 41.695 / 47.316 / 55.365 / 86.744 |

The per-slot staging candidate improved median wall time in all three pairs,
with mixed P95/P99 changes. It remains opt-in.

The accepted Release diagnostic reference measured 47.849 / 56.151 / 60.887 /
80.687 ms. The race script median was 27.661 ms, the engine pre/post-draw signal
interval 15.677 ms (P95 18.126 ms), and the external file observer 4.322 ms.
Pre/post-draw includes engine work and waits within those signals. The OS
presentation wait has not been independently resolved. Main colour and encoded
depth GPU medians were 2.57 and 1.42 ms, delayed per-viewport timestamps. The
all-pass draw call median was 5,561.5.

Native World advance median was 2.752 ms, including DoTimestep 1.459 ms,
publication 1.150 ms and Replay 0.002 ms. Track view was 16.747 ms, including
depth 8.026 ms (material 4.193 ms, proxies 2.876 ms, discovery 0.886 ms) and
shadows 2.685 ms. Mirror selection/submission were 0.309/0.423 ms. Five vehicle
views were 5.569 ms. The slowest wall interval concentrated in track submission
and depth work while World advance was 2.609 ms. Release priority is reducing
host render calls and depth work. Debug's long World stalls remain an open,
separate profiling result.

An earlier Release diagnostic run exported samples but failed input identity
after harness edits and is excluded. The later staging diagnostic and uncapped
attempts exited before export. At the user's request, stopped the remaining
repeat matrix and closed its in-progress instancing window. Separate, worker
count, uncapped throughput and Championship instancing have no new valid
comparisons. Safe remains the default.

### Vehicle view submission follow-up

Share the transform, camera projection operands, reflection rows and pure
light-context calculation across contiguous surfaces of the same Godot mesh
instance. Compare each resulting light vector bitwise before submitting it.
The host still submits every surface in the original order with its own
texture, material and light values.

One actual Release diagnostic comparison was run after the user asked to stop
repeating benchmarks. Vehicle-view CPU P50/P95 fell from 5.569/6.024 ms to
2.200/2.637 ms. Full-frame P50/P95/P99/max changed from
47.849/56.151/60.887/80.687 ms to 43.839/53.514/62.283/76.268 ms. All 600
samples remained over budget. The asynchronous external driver committed
source steps 186–785 in the reference and 192–791 in the follow-up. This
single comparison identifies the local CPU saving; P99 did not improve.

Median draw calls were 5,585.5, track view 16.816 ms, depth 8.024 ms and
engine pre/post draw 15.735 ms. Per-viewport visible draw medians were
2,752 for main colour, 2,538.5 for encoded depth and 208 for the mirror.
The next action is to identify compatible encoded-depth surfaces by shader,
depth-write and discard state, then test a batch at a fixed pose before
another race timing run.

Runs and exact identities are under ignored
`local/task0077/championship-p1-*`. The Release diagnostic reference DLL
SHA-256 is `731ef05ae2fcd0a43272d3a8f5e1d0ab6fbad30564c130b74854ebc7c3cf142e`.
Vehicle submission follow-up SHA-256 is
`95798d207aca0f60beee323c7813fd98d4d9112fc19c7006722af3f463bc8c2b`.
Godot 4.7.2.stable.official.ed1daf0bf, Vulkan 1.4.359, Arc B390,
MinGW-w64 GCC 13.2.0, CMake/CTest 3.29.2 and Python 3.14.4.

```powershell
cmake -S . -B build/godot-release -DPORT_GODOT_BIN_DIR=D:/dev/hp2_godot/local/task0077/championship-release-pose-bin
cmake --build build/godot-release --target ps2_port parameter_staging_registry_tests depth_sync_tests track_instance_batches_tests mesh_prepare_tests track_runtime_tests driving_schedule_tests event_world_tests race_finish_tests frontend_finish_tests --parallel 8
ctest --test-dir build/godot-release -R '^(driving_schedule|event_world_original|race_finish_original|frontend_finish_original|parameter_staging_registry|depth_sync_native|track_runtime|mesh_prepare|track_instance_batches)$' --output-on-failure
py -3 -B tools/test_event_window.py --championship-performance --road-course-driver --performance-dll local/task0077/championship-release-pose-bin/ps2_port.dll --result-timeout 105 --output local/task0077/championship-p1-release-pose-repeat
```

The nine rebuilt native tests passed (2.08 s) and the real Championship
diagnostic harness passed all 12 checks. The benchmark uses an isolated
DLL and private saves. No recovered routine, original evidence level or
workspace DLL was changed.

Fixed-image parity remains open. The legacy TASK-0072 Test drive fixture
requires Debug. Its isolated Debug reference entered the current authored
World route and then dereferenced a freed viewer at line 79 of
`task0072_driving_parity.gd`. Closed the owned window and skipped the second
side. The formal Championship capture logged no script errors or RID leak
diagnostics on exit. Fixed-image, pause/retry and render-thread parity still
need targeted acceptance. No repeat matrix is queued.

## Championship stable-60 execution

User authorized opt-in 75% 3D raster and 30 Hz rear-view presentation.
Current scope is Track 12 Release, 1920×1080 output, bounded fixed-pose
checks and a short production wall-time comparison. Goal is uncapped
P95 ≤16.67 ms and P99 ≤18 ms. Keep original command selection, LOD and
World stepping. New batching candidates begin disabled.

Two opt-in candidates share the existing mesh cache. The encoded-depth
candidate groups only selected contiguous static surfaces with the same
section, LOD and ordinary depth contract, then bypasses their legacy proxies
for that frame. The race static candidate gives World overrides individual
submission barriers, with the override mask included in the plan cache key.
The existing MultiMesh option remains independent.

Track 12 scene-only GPU fixture with three camera poses from the actual race
passed exact native packet, colour, encoded-depth and final image comparisons
in both modes. These captures exclude live World actors and a live HUD.
Independent depth batching saved 1 / 0 / 0 all-pass draws and remains off.
Race static batching reduced fixed-pose all-pass draws 2333→791, 2041→808
and 293→171. The actual race gave the following results instead:

| Uncapped 1080p, 600 intervals | P50 / P95 / P99 / max ms |
| --- | --- |
| Fidelity control | 40.660 / 48.143 / 62.779 / 85.182 |
| Race static batching | 42.548 / 49.392 / 66.358 / 77.336 |
| Performance60, corrected Sun raster binding | 41.362 / 49.595 / 56.005 / 71.906 |

All three accepted runs passed 12 harness checks and input identity.
Control and static candidate share the same DLL SHA-256
`8f1c3393576d368919f3bfea36613070abbaafdf11f2f7a5e7c517a23f219523`.
Static batching had 2,718 depth proxy creations by export versus 2,067
for the control. Geometry cache churn is a hypothesis to check, and draw
count alone did not predict moving-race throughput. Static batching stays
off by default. No run meets the 60 FPS budget.

The performance profile rendered colour/depth at 1440×810 inside 1920×1080
output and recorded 679 mirror refreshes and 42 holds, including warmup.
The first run reported Sun source-raster size errors and is excluded.
Sun's consumers address the actual encoded texture using normalized UV
and original 512×448 coordinates. After the host dimension guard was
corrected, the run completed without runtime errors and emitted a post-timing
screenshot with HUD and rear-view output. It is a single live image, not
matched-frame visual parity. The profile stays opt-in.

Artifacts: `local/task0077/championship-p2-fixed` and
`local/task0077/championship-p2-wall-*`. Nine native checks passed.
The user's follow-up redirects work toward the dominant stage. The
unconnected CompositorEffect experiment was removed. The scene-depth
replacement and Separate-thread gates remain open.

### CPU and engine frame boundaries

The performance-engine-breakdown option keeps the diagnostic collector and
Godot GPU timestamps, with compact UDP pose updates instead of per-frame
state files. The delayed viewport range is paired to the draw interval that
contains both CPU endpoints, with repeated device frames discarded.

The uncapped fidelity run measured wall P50/P95/P99 42.082/50.299/56.035 ms.
Track host view cost 17.116 ms median, including 8.186 ms depth work
(4.257 ms materials and 2.942 ms proxy synchronization), 2.863 ms cache
trimming and 2.737 ms shadows. World advance was 2.659 ms. Godot draw signals
spanned 15.334 ms, with viewport GPU median 5.348 ms. The gap from draw end
to the next race process entry was 0.325 ms.

One cache experiment avoided rebuilding the active set and sorting when
there was nothing to evict. It changed trim P50/P95 from 2.863/4.047 to
2.846/3.716 ms and frame P50/P95/P99 to 42.770/49.006/62.082 ms.
Restored the original cache implementation after this mixed result.
The same diagnostic run contained 597 valid absolute timestamp pairs.
Median draw before the viewport group was 2.532 ms, viewport CPU 5.524 ms
and draw after the group 7.009 ms (after-group P95 7.863 ms). In pinned Godot
ed1daf0bf the after-group interval includes render-target blit, device
end/execute/next-frame work and callbacks. The separate costs of those steps
remain unknown. Next action is to time command-graph finalization, queue
submit/present and the next-frame fence inside that interval, and reduce
actual depth/material updates.

The two engine diagnostic captures passed 12 production route checks each.
Use the current isolated DLL and a fresh output directory:

    py -3 -B tools/test_event_window.py --championship-performance --performance-engine-breakdown --performance-uncapped --performance-dll local/task0077/championship-p2-bin/ps2_port.dll --result-timeout 115 --output local/task0077/championship-p2-engine-next

Archived captures under local/task0077 are championship-p2-engine-breakdown
(DLL b24bb113b5fc8dad9c1d6e82712dafda12a6008f63babc739c52b90de1cd2a46)
and championship-p2-engine-trim (DLL
a18a078f1f6e30645c687bea56bbfeb900f7432c22864c39174512a09f699af3).
Toolchain and engine match the prior Release section. Task stays active.

### One short Vulkan tail capture and targeted upload candidate

E-0077-VK01: Track 12, five cars, 1080p Release, Vulkan Forward+, Safe,
uncapped, 120 warmup frames and 120 measured intervals. All 12 production
route checks passed. 117 intervals matched the delayed viewport range, the
presenting thread and the Vulkan command markers. The QPC calibration bracket
was 19.6 µs. The three unmatched rows had no completed viewport timestamp pair.
Full-frame P50/P95/P99 was 49.674/58.107/67.903 ms, with 70.386 ms maximum.
This short instrumented route segment is an attribution sample.

| After viewport group, matched presenting thread | P50 ms | P95 ms |
| --- | ---: | ---: |
| Entire interval to post-draw signal | 7.675 | 9.193 |
| Viewport end to start of final `vkEndCommandBuffer` | 7.180 | 8.683 |
| Of that, viewport end to first Vulkan render pass begin | 3.807 | 4.613 |
| Of that, first Vulkan render pass to command buffer end entry | 3.368 | 4.461 |
| `vkEndCommandBuffer` calls | 0.007 | 0.009 |
| `vkQueueSubmit` calls, including transfer workers | 0.127 | 0.182 |
| `vkQueuePresentKHR` | 0.012 | 0.020 |
| Blocking `vkWaitForFences` calls | 0 calls | 0 calls |
| Present return to post-draw signal | 0.372 | 0.593 |

Per-call rows can overlap elapsed-time rows and percentiles need not add.
The before-end interval covers screen acquire, blit setup, transfer
preparation and render-graph finalization/replay. Screen acquire itself
was 0.005 ms median. The trace cannot isolate the graph's sort from its
barriers, replay and other work before the first render pass. In this
engine pin the fence path first polls status and calls the blocking wait
only when necessary. The 117 fence resets were observed. [Pinned device end frame](https://github.com/godotengine/godot/blob/ed1daf0bf001b61586d9930840f2f1394092c079/servers/rendering/rendering_device.cpp#L8102-L8127) · [Pinned Vulkan fence code](https://github.com/godotengine/godot/blob/ed1daf0bf001b61586d9930840f2f1394092c079/drivers/vulkan/rendering_device_driver_vulkan.cpp#L3050-L3061)

The World shadow host rebuilt the strip mesh and immutable indices per frame.
Its inclusive submission cost was 2.889 ms median in this capture, measured
outside the viewport tail. The opt-in `--track-opt-shadow_stream=true`
keeps the mesh RID and indices while topology is stable, uploads changed
regions only, and maintains stock per-strip bounds, winding and draw order.
The isolated GPU fixture confirmed stationary/no uploads, stable RID on
position and attribute changes, rebuild on topology change, exact packed
arrays and bounds, and byte-for-byte equal nonempty Vulkan raster output.
This verifies host transport for the fixture. The candidate's moving-race
FPS, tail reduction, and full shadow scene parity remain open.

The first layer preflight used a relative DLL manifest path and Godot showed
its generic Vulkan initialization error. An absolute DLL path succeeded in
the next frontend preflight and the single real race. The harness now
validates that path before launch. Layer variables live only in the child
environment. The normal extension SHA-256 remains
`2ca5b3affc287dc3f60af71e9dad6c89b7b1c036015273beadec8d54898b94b0`.
The minimal asset-free project's optional headless editor import exited with
`0xc0000005` after writing the extension list. The isolated Vulkan fixture
using that list passed and exited 0. The normal Championship import passed.

Run identity: Godot 4.7.2.stable.official.ed1daf0bf, Arc B390 Vulkan 1.4.359,
Vulkan-Headers v1.4.359 commit `f9973cd97e6f3584707e7ef1c425e336f1b92a5b`,
MinGW-w64 GCC 13.2.0, CMake/CTest 3.29.2, Python 3.14.4.
Diagnostic DLL SHA-256
`a9c249daecd6cdb8ddb60f24c76d284beed1d6ac46761d712d1c459cc496187f`.
Layer SHA-256
`6f89194296898057cd267fca9cc19c15c5374d6f7d820da3ac5219ba75f68935`.
Shadow fixture DLL SHA-256
`58514a148b8031180f36e1a9bfa2eceb38c23cd403dc579e990d0c5f0c149653`.
The raw CSV, profile, identity and result are in
`local/task0077/championship-vulkan-tail-a`. No second race was run.

```powershell
git clone --depth 1 --branch v1.4.359 https://github.com/KhronosGroup/Vulkan-Headers.git local/task0077/vulkan-headers
$layerDir=(New-Item -ItemType Directory -Force local/task0077/vulkan-layer).FullName
g++ -std=c++17 -O2 -Wall -Wextra -Werror -shared -static-libgcc -static-libstdc++ -Ilocal/task0077/vulkan-headers/include tools/championship_vulkan_tail.cpp -o "$layerDir/hp2_vulkan_tail.dll"
[ordered]@{file_format_version='1.0.0';layer=@{name='VK_LAYER_HP2_tail';type='GLOBAL';library_path=(Join-Path $layerDir 'hp2_vulkan_tail.dll');api_version='1.4.359';implementation_version='1';description='HP2 render tail timing'}} | ConvertTo-Json | Set-Content -Encoding ascii "$layerDir/VK_LAYER_HP2_tail.json"
cmake -S . -B build/godot-release -DPORT_GODOT_BIN_DIR=D:/dev/hp2_godot/local/task0077/championship-shadow-bin
cmake --build build/godot-release --target ps2_port track_environment_tests --parallel 8
ctest --test-dir build/godot-release -R '^(driving_schedule|event_world_original|race_finish_original|frontend_finish_original|parameter_staging_registry|depth_sync_native|track_runtime|mesh_prepare|track_instance_batches|track_environment)$' --output-on-failure
py -3 -B tools/analyze_championship_vulkan_tail.py local/task0077/championship-vulkan-tail-a
```

Ten native checks passed. To continue TASK-0077 with the next authorized
race sample, build as above and use a fresh isolated output and private save:

```powershell
py -3 -B tools/test_event_window.py --championship-performance --performance-engine-breakdown --performance-uncapped --performance-vulkan-layer local/task0077/vulkan-layer --performance-shadow-stream --performance-dll local/task0077/championship-shadow-bin/ps2_port.dll --result-timeout 85 --output local/task0077/championship-vulkan-tail-shadow
py -3 -B tools/analyze_championship_vulkan_tail.py local/task0077/championship-vulkan-tail-shadow
```

### Shadow stream short race and command replay attribution

E-0077-VK02: ran the opt-in candidate once at 1080p Release, Track 12, five
cars, Vulkan Forward+, Safe, uncapped, 120 warmup and 120 measured frames.
All 12 production route checks and identity verification passed. The
candidate matched 117 viewport and Vulkan frames with a 19.9 µs QPC bracket.
Raw results are under `local/task0077/championship-vulkan-tail-shadow`.

| Interval | Baseline P50 / P95 ms | Shadow stream P50 / P95 ms | P95 change ms |
| --- | ---: | ---: | ---: |
| Whole frame | 49.674 / 58.107 | 46.906 / 52.989 | -5.118 |
| World shadow host submission | 2.889 / 3.676 | 0.631 / 0.865 | -2.811 |
| Viewport end to draw end | 7.675 / 9.193 | 7.695 / 8.629 | -0.564 |
| Viewport end to final buffer end entry | 7.180 / 8.683 | 7.438 / 8.412 | -0.271 |
| First Vulkan render pass to final buffer end entry | 3.368 / 4.461 | 3.412 / 4.297 | -0.164 |

Whole frame P99 changed 67.903→61.173 ms, maximum 70.386→66.795 ms.
The candidate's shadow mesh creations per frame were P50/P95 0/0 and
region updates 190/236. Candidate draw count median was 6,236.5 versus
6,197.5. Of 114 shared World steps (188–301), 69 had equal sampled input.
On the shared steps, whole frame P95 changed 58.107→53.206 ms and host
shadow P95 3.676→0.882 ms. This is a moving route comparison with different
camera poses, a new layer with additional hooks and a new port DLL.
The same engine commit and hashed asset/script inputs match apart from
the candidate DLL and the diagnostic profile script. The large local
shadow change is consistent with eliminating resource creation. The small
command recording P95 difference is not an established speedup, and the
whole-frame delta includes other variation. The opt-in policy remains.

The candidate layer also logged `vkCmdWriteTimestamp`, render pass end
and `vkCmdPipelineBarrier`, allowing an additional split in the same race.
The first replay marker is the earlier of the first replayed graph timestamp
and the first render pass on the presenting thread. The Godot graph replay
calls the driver to write its timestamp command. The pre-marker bound can
include already replayed commands and pre-graph work. Direct driver call
durations are subsets of the elapsed intervals below. [Pinned graph replay](https://github.com/godotengine/godot/blob/ed1daf0bf001b61586d9930840f2f1394092c079/servers/rendering/rendering_device_graph.cpp#L1076-L1225) · [Pinned graph finalization](https://github.com/godotengine/godot/blob/ed1daf0bf001b61586d9930840f2f1394092c079/servers/rendering/rendering_device_graph.cpp#L2583-L2760)

| Candidate subinterval | P50 ms | P95 ms |
| --- | ---: | ---: |
| Viewport end to first observed replay marker | 1.726 | 2.336 |
| Last transfer submit return to first replay marker | 1.643 | 2.229 |
| First replay marker to final buffer end entry | 6.087 | 7.338 |
| First replay marker to first render pass | 2.088 | 3.801 |
| Inside render passes, 23 per frame | 3.210 | 4.085 |
| Between and after render passes | 0.202 | 0.298 |
| Direct Vulkan pipeline barrier calls | 0.072 | 0.085 |
| Direct Vulkan timestamp calls | 0.011 | 0.015 |

The largest observed phase is replay/recording after the first marker.
Render-pass bodies are the largest measured component. Early replay before
the first render pass also has a substantial P95. The layer measures host
intervals and cannot give an exclusive CPU cost for graph sorting, command
type handlers, or individual draw list roles. The next exact boundary is
`RenderingDeviceGraph::end` scheduling versus
`_group_barriers_for_render_commands` and `_run_render_commands`.
Use the pinned engine's profiling zones or an isolated instrumented build
for these boundaries and retain colour/depth/mirror identity checks if
reducing the commands recorded. No second candidate or baseline race was
launched.

Candidate DLL SHA-256
`58514a148b8031180f36e1a9bfa2eceb38c23cd403dc579e990d0c5f0c149653`.
New layer SHA-256
`7d1a35d5d3c59db7b7f5ed09e6f2fff02306765071d4d7332c6b85d9243507a0`.
Build the layer with the previous section's command, using
`local/task0077/vulkan-replay-layer` for its directory. The asset-free
Vulkan shadow smoke passed with this layer before the race. Exact race
command is the previous section's candidate command with
`--performance-vulkan-layer local/task0077/vulkan-replay-layer`.
The same analyzer successfully processed both archived CSVs.

### 2026-11-12 Godot Server integration

The native World continues to publish the single authoritative physics
snapshot. The production race currently has no PhysicsServer3D query
consumer, so no secondary space is allocated. The default audio path now
packs one native PCM block into Godot in a single allocation and uses
block offsets to retain partial writes. AudioServer playback remains
through AudioStreamGeneratorPlayback at 48 kHz stereo. The asset-free
transport test passed ordered partial writes, rejection/backpressure and
silence. The actual race passed pause/resume with frozen original state
and cleared PCM, retry cancellation, retry and return to Event Tree.

The opt-in static depth producer publishes selected meshes, transforms, sort
keys and material/shader identities directly. Existing parameter notifications
handle changed values. The legacy consumer handles unregistered geometry and
previously registered sources that become ineligible. The MultiMesh pool keys
slots by scene-owned view/section/LOD and first source draw identity, reuses
unchanged buffers, expires inactive slots after 120 view submissions and
records allocations and upload bytes. Its asset-free Debug Vulkan test passed
unchanged transforms, changed transforms, section isolation and retirement.
RenderingServer remains on Safe. The existing CPU-only worker contract remains.

Three recorded Track 12 camera poses passed exact native packet digest, colour,
encoded depth and final-image comparison for both candidates. Static depth
selected 2/1/0 sources and left fixed draw counts unchanged. MultiMesh reduced
fixed-pose draw counts 2333→1969, 2041→1725 and 293→239. The fixed fixture
excludes live vehicles and HUD. A post-timing production image shows the mirror
and HUD. It has no deterministic paired live reference.

The Forward+ camera compositor probe attached only in the fixed fixture.
At PRE_TRANSPARENT it missed all 2,052,978 / 2,046,678 / 2,034,830 encoded
foreground pixels. POST_TRANSPARENT had full foreground coverage, with
1,105 / 1,142 / 1,626 unequal encoded pixels out of 2,073,600 per pose,
using the legacy consumer's byte-rounding rule. The stock colour shader
writes ALPHA and enters the transparent pass. The particle shader samples
encoded depth during that pass. The late capture cannot serve that same-frame
consumer. Independent depth proxies remain active. The failed initial probe
had a 32-byte push constant for a 24-byte shader block, corrected before
collecting the reported results. [Compositor contract](https://docs.godotengine.org/en/stable/tutorials/rendering/compositor.html)

All runs below used 120 warmup and 120 measured intervals, Track 12, five
cars, 1080p Vulkan Forward+ Release, Safe, uncapped, private saves and the
same isolated DLL with shadow streaming enabled. The old baseline predated
the audio bridge change, so one matching control was necessary. Each
candidate ran once. The two ordinary routes passed 12 checks, while the
static-depth run passed 23 including lifecycle checks outside timing. Each
run provided 117 matched Vulkan replay intervals. These diagnostic runs
include the profiler and Vulkan layer, and are not 60 FPS acceptance runs.

| Policy | Frame P50 / P95 / P99 / max ms | Replay P50 / P95 ms | Draw calls P50 | Depth host P50 ms |
| --- | ---: | ---: | ---: | ---: |
| Control | 47.654 / 61.933 / 67.957 / 72.895 | 6.221 / 7.341 | 6225.5 | 9.180 |
| Static depth publication | 48.145 / 59.815 / 70.914 / 71.965 | 6.019 / 7.345 | 6227.0 | 9.330 |
| MultiMesh | 46.997 / 61.485 / 69.359 / 70.533 | 5.492 / 6.946 | 5883.0 | 11.608 |

Static publication handled 1 source median. MultiMesh saved 342.5 draws
median and 0.729 ms replay median, while depth proxy sync rose
3.396→5.586 ms. At export the main view had 122 MultiMesh batches and no
buffer uploads. The mirror had 30 batches, two allocations and 192 bytes
of instance data. The three runs reported 9 / 11 / 0 skipped AudioServer
mixes, respectively, no queued PCM at sampling points and 1005 / 968 /
1092 µs total block submission time during the windows. They all used the
new audio transport, so these are observations rather than before/after
audio savings. Both rendering candidates remain opt-in. The time budget
is still exceeded. The next action is to stage only current MultiMesh
depth sources and cache shader write eligibility, then verify whether
this removes the extra depth synchronization cost.

The main view's control selected seven original sections, considered 735
instances and retained 283 instances (medians across the 120 intervals).
The 0.856 ms median native preparation includes selection and command
preparation, rather than timing the frustum test alone. Static publication
and MultiMesh still consume the same original section/tree/instance and LOD
selection. The fixed-pose command digests match. MultiMesh retains section
boundaries and batches the selected instances; each batch is a single
Godot frustum-culling unit.

Artifacts are under `local/task0077/server-bridge-{fixed,control,candidate,multimesh}`.
Measured DLL SHA-256 is
`b29b03f1f45356eb7ee65e8d1940c4eab38cb3f3e72e74401cc8c172f55f032d`.
The Godot and layer pins match E-0077-VK02. Rebuilds used CMake 3.29.2,
MinGW-w64 GCC 13.2.0 and Python 3.14.4. Seventeen native regression tests
passed. The initial CTest selection had four unbuilt executables, which
were built and passed on the rerun. The final diagnostic code additionally
exports per-sample view counters. No extra full race was run after that
instrumentation-only change. Final isolated Release DLL SHA-256 is
`2f83d04cc53b9049bba5d42334b76f6ecc07d375e7d0a19b24d0072bca80e71b`.

```powershell
cmake -S . -B build/godot-release -DPORT_GODOT_BIN_DIR=D:/dev/hp2_godot/local/task0077/server-bridge-final-bin
cmake --build build/godot-release --target ps2_port car_audio_foundation_tests world_frame_tests world_snapshot_tests car_construction_tests --parallel 8
ctest --test-dir build/godot-release -R '^(driving_schedule|event_world_original|race_finish_original|frontend_finish_original|parameter_staging_registry|depth_sync_native|track_runtime|mesh_prepare|track_instance_batches|track_environment|world_frame|world_snapshot|car_audio_foundation|world_clock_audio_frame_original|world_player_movement_original|world_frame_original|authored_championship_clock_original)$' --output-on-failure
$common=@('--championship-performance','--performance-engine-breakdown','--performance-uncapped','--performance-vulkan-layer','local/task0077/vulkan-replay-layer','--performance-shadow-stream','--performance-dll','local/task0077/server-bridge-bin/ps2_port.dll','--result-timeout','85')
py -3 -B tools/test_event_window.py @common --output local/task0077/server-bridge-control
py -3 -B tools/test_event_window.py @common --performance-static-depth-sources --performance-lifecycle --performance-final-screenshot --output local/task0077/server-bridge-candidate
py -3 -B tools/test_event_window.py @common --performance-instancing --output local/task0077/server-bridge-multimesh
```

Use fresh output names for reproduction. For the next isolated build,
replace the DLL argument with the new build path. The fixed fixture is
`res://scenes/track_optimization_smoke.tscn` with
`--optimization-track=12 --optimization-poses=<absolute poses.json>`.
Use `--optimization-mode=static_depth_sources --probe-scene-depth` for
depth and `--optimization-mode=multimesh` for the pool. Use
`--headless --script res://scripts/debug/task0077_world_audio_smoke.gd`
for the audio transport.

### Godot FXAA and race texture sampling

The authored race now requests engine FXAA on the main colour SubViewport
and native linear or linear-mipmap sampling for ordinary track materials.
The performance60 profile leaves FXAA off by default. The original alpha
test, mixed wrap, biased road and multipass shader paths remain. The engine
mip chain retains authored bytes and generates missing levels. Both controls
can be disabled with `--race-godot-sampling=false --race-fxaa=false`.

All three Track 12 fixed poses kept exact packet and encoded-depth bytes,
with the expected colour changes. Narrowed native mip-chain allocation
from 233 to 38 eligible textures and reran the fixed gate. The single
live 120/120 Track 12 Release Vulkan Forward+ Safe uncapped run passed all
12 route checks, enabled FXAA and used 34 native-sampler materials and
38 mip chains. Full-frame P50/P95/P99/max was
44.935/59.541/69.204/70.591 ms. This run confirms the active route, with
no matching A/B FPS claim. Six native regressions passed. Artifacts are
under `local/task0077/race-godot-quality-{fixed,live}`. Tested isolated
DLL SHA-256 is
`5de8d2fbed06889b9757ef5ba14e9bab2059e771005c6df5118a86c3df8251f6`.
Next performance target remains MultiMesh depth proxy synchronization.

```powershell
cmake -S . -B build/godot-release -DPORT_GODOT_BIN_DIR=D:/dev/hp2_godot/local/task0077/race-godot-quality-bin
cmake --build build/godot-release --target ps2_port --parallel 8
ctest --test-dir build/godot-release -R '^(track_environment|depth_sync_native|mesh_prepare|track_instance_batches|track_runtime|event_world_original)$' --output-on-failure
py -3 -B tools/test_event_window.py --championship-performance --performance-engine-breakdown --performance-uncapped --performance-vulkan-layer local/task0077/vulkan-replay-layer --performance-dll local/task0077/race-godot-quality-bin/ps2_port.dll --performance-final-screenshot --result-timeout 85 --output local/task0077/race-godot-quality-live
```
