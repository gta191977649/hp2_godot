# TASK-0072 host draw residency candidate

## Evidence and status

E-0072-R01 is host-only design/diagnostic evidence. No recovered algorithm,
original visibility, LOD, submission order or effect permission is changed.
The original binary authority remains Retail SLUS_203.62 SHA-256
7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2;
Debug driving remains NFSHP2.ELF SHA-256
815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
No new Ghidra behavior claim or Verified promotion follows from these data.

Inputs: ignored local/task0072/measurements/drive-pilot/run.json and
profile-track-11-default-repeat-1.json. Vulkan Forward+, Intel Arc B390,
2880x1695, Debug host build, actual CORVETTE Test drive with Profile UI,
GPU-resident Sun enabled; residency/dynamic planner/depth discovery/UI bulk/
compact history/async export candidates disabled. Pilot requested120warm,
600sample,3repeat, but persisted only2repeats. Repeat0 stopped after400samples
with `Production driving lifecycle is not DRIVING; no automatic restart`.
Repeat1 contains600samples, yet does not validate the incomplete pilot matrix.
Do not use this pilot for acceptance, paired gain claims or default enablement.
The corrected strict driving wrapper and fresh three-repeat matrix supersede it.

## Observed costs and churn

The following are recomputed repeat1 sample medians and nearest-rank P95 in ms;
CPU scopes overlap and must not be added as an independent frame budget.

| Scope | Median | P95 |
|---|---:|---:|
| Wall |53.458|71.020|
| Prepare |36.369|52.095|
| Host submission |25.058|39.873|
| Reflection submission |9.607|14.105|
| Postprocess submission |6.078|10.913|
| Main submission |3.619|7.284|
| Main native selection |3.393|5.909|
| Main batch planning, included in submission |0.185|0.359|
| Previous Sun readback |0.0009|0.0015|

Postprocess proxy creations grow2752->10937 over599adjacent sample transitions:
8185new proxies, per-transition median7/P9553/P9986/max104. Retained depth
proxies vary175..953 and depth materials492..1270. Source mesh creations grow
312->527; batch mesh builds705->799. Sorting updates grow147822->750221,
per-transition median973/P951459. Parameter sets grow69262->320465,
median382/P95924. Discovery visits grow268904->1522941, median2198/P952600
per transition; uniform reads386279->2154205, median2789/P953887.
These counters show repeated host work; proxy creation alone does not prove
which eviction policy caused every allocation.

Postprocess wall correlates with adjacent proxy-creation delta (Pearson0.791).
Zero-new-proxy transitions(n137) have post median4.803ms; >=20new(n137)
9.067ms; >=50new(n37)11.407ms. This supports an isolated residency ablation,
not a causal claim or a reason to reduce native selections. Excluding only the
last sample for descriptive correlation, wall correlation is0.748with main,
0.718with post and0.625with reflection wall. Keep the complete timings for
acceptance; this exclusion is not an acceptance filter.

Per-view repeat1 timings preserve their different sample counts: main id1 has
600rows; ids4/9 have600 each and ids5..8 have150 each. Main planning median
0.185ms; other per-view planning medians0.032..0.070ms. Reflection view
selection medians1.189..2.454ms; submissions0.506..2.516ms. Preserve the
original capture schedule: changing cadence would alter behavior. Planning
caches have less directly observed budget than resource/proxy lifetime and
reflection selection/submission.

Profile GPU completed timestamp pairs report colour median4.242/P956.443ms,
encoded depth2.530/4.301, root1.375/2.548; alternating history medians0.840/0.867.
These are delayed individual targets, never summed or subtracted from wall to
infer a total GPU/CPU budget. The run's direct viewport queries remain null.
Measured pipeline specialization/surface/mesh/draw/canvas delta counters are
zero throughout repeat1: runtime compilation is not established as its cause.
Profile DebugTools inclusive wall median5.428/P957.018ms is another material
host cost; collector/export/UI candidates need their own independent ablation.

Repeat1 wall P99 is78.456ms, maximum926.819ms on final frame719; frame340
has89.833ms with post17.262/reflection15.464ms. The final spike is not explained
by measured renderer scopes. Synchronous export was enabled in the pilot;
inspect the collection/export boundary with the corrected wrapper before
attributing it to rendering. Every recorded measured driving step_delta is1;
native_dt ranges0.0166666675..0.0169444457. Preserve the recovered timestep
sequence and compare complete native/body/camera timelines across candidates.

## Independent candidate and memory limits

Root-owned flag `--track-residency-cache=true` is default-off pending gates.
It changes only inactive host retention: draw count256->4096, accounted draw
mesh payload16MiB->128MiB, age120->1800renderer frames; unused source meshes
256->2048 and age120->1800. Existing active records across all per-view lists
remain protected. Release/load clears scene-owned caches. Native command
selection, materials, transforms, ordering and capture cadence remain authority.

These are separate eviction thresholds, not a128MiB total memory cap. Active
records are excluded; mesh accounting omits node/material/shader/uniform maps,
encoded-depth proxies and driver allocation overhead. Source meshes have a
count limit without a byte cap. Shared/batch mesh references can pin resources;
the batch cache independently retains referenced entries above its64MiB target.
1800frames means30seconds at60Hz but90seconds at20Hz. A4K inactive draw limit
can retain additional reflection nodes and proxies and amplify scene-tree scans
when discovery caching is disabled. Report actual peak process/GPU memory,
resident/inactive draw counts, mesh bytes, source counts, evictions and proxy
allocations during route rotation and scene teardown.

Correctness traps: preserve the union of all active views, including capture
faces not scheduled this frame; hide inactive nodes without changing original
selected commands; refresh transforms/material parameters/sorting and notify
depth proxies when reactivating. Existing draw keys use view/instance/solid/
surface identity, while shader construction happens on creation: establish
that state-changing controls either preserve those descriptor assumptions or
invalidate resources before extending their lifetime. Keep full batch-source
validation and normal shader/material separation. Confirm thresholds exported
by diagnostics match the enabled flag (old fixed-limit fields can mislead).

## Required gates

Pair the corrected actual-driving and rotation matrices with the flag alone;
require equal body/camera/input/timestep histories and native command digests,
exact colour/depth/final/reflection output at matched states, scene exit/reload
cleanup and bounded inactive residency after pressure/age eviction. Measure
120warm/600sample/3repeat median/P95/P99, per-frame proxy creation/eviction,
CPU sections and memory. Repeat broad turns long enough to revisit prior views.
Default enablement requires reproducible tail improvement exceeding baseline
noise and no persistent regressions; a larger cache by itself is not a win.

