# TASK0055 Godot submission policy and caches

## E0055H01 - Host-only optimization boundary

Frozen TASK0054 Debug SHA256b9fc9f64718be9504a808a6261cbaad184fe91779c2ea7ccc39628e966ff2cfa;
Release3dd1735783780ea6c55c37084990f25841dfda6aaaa0557b3d5afe40890ac4f1.
Copies remain local/task0055/baseline; frozen54 review/scope unchanged. Native
Runtime/scenery/eModel packets, original materials/LOD/view schedule and physics
are not edited. All changes here are Godot4.4.1 host transport/diagnostics.

Configuration is before-load only and atomic. Baseline turns all new policies off;
optimized requests resource reuse/particle reuse only. Conservative instancing remains experimental because Track24 failed exact color parity. Occlusion
and opaque queue are independent experimental flags, off by default. Normal
startup defaults remain baseline until the all-route numeric/visual gates pass.
Release supports host policy configuration without allowing Debug effect overrides.
Invalid CLI/profile/options are explicit errors; unsupported batching falls back
to the existing host submission, never masks an original NotImplemented.

Mesh source identity comparisons skip unchanged batch-cache key allocations and
lookups. Full shader code still defines shader sharing. Only unlit/no-camera-
parameter/noextra-pass/Bias0 material instances are shared, keyed by exact shader,
texture,flag4 and priority; dynamic lit materials remain separate. Host uniform
values prevent duplicate writes and depth notifications. Final native-alpha ranks
are not written twice before/after the common queue when reuse is enabled.
Postprocess stage parameters and unchanged camera properties/sizes are cached.
Native depth material generation is factored into one shared factory; MultiMesh
proxies use EXACTsame MultiMesh/transform resource and original depth shader rules.
Particle resource layout validation and callbacks are E0055P01. Conservative
instance plan, matrix/section/LOD boundaries and bulk buffer layout E-0055-I01.

Actual scheduled viewport RIDs are captured before draw from explicit main,
postprocess/history, Sunreadback, reflectionpendingfaces/atlas scheduling. Timers
are diagnostic only; retained inactive viewport values are never summed as frame
latency. Baseline54 lacks scheduling metadata and reports GPU timer unavailable.
Benchmark wall duration is captured before optional timer-query overhead. Each
repeat uses fresh native sessions, deterministic dt1/60/camera/host seed, original
emitter seeds and original reflection frame scheduling. Full digest is separate
from measured loops. Missing/failed routes are retained, never marked passed.

Experimental occluders use only submitted Copy/noalpha/Always-test/write-depth/
GEQUAL/Bias0/noextra-pass triangles, exact ArrayMesh vertices/indices and transforms.
Only main color/native-depth viewport occlusion is enabled; capture views keep
independent default state. Transparent/alpha-tested/screen-effect geometry is not
used as an occluder. Opaque-queue experiment removes ALPHA output only for the same
strict state class, with no GS blend change. Neither class is automatically deemed
safe: order/ties, occlusion and intermediate-depth comparisons plus performance
gates are required. They remain disabled if any proof or measurement is missing.

Correctness status: initial actualGPU optimized Track11threeposes plus original
forestfire pass exact logical digest/final/intermediatecolor/rawdepth parity;
particleactive/visible counts515/86 match. Broader all30/profile/performance tests
are in progress; no Verified or unconditional speed claim.


Closed vehicle depth-uniform contract E0055V01 is consumed only after a complete
initial sync. Shader replacement rebuilds the variant and its uniform names;
explicit dirty invalidation forces full sync. Unknown materials poll all names.
Tagged stable BASE materials poll exactly the eight existing camera/light
writers. Source ShaderMaterial.set_shader_parameter does not emit Resource.changed
in pinned Godot4.4.1, so an unverified changed-signal shortcut is not used.

Counterevidence: all30 initial MultiMesh run passed Tracks11-23, but Track24 pose50
changed17602 intermediate-color pixels (max4bytes) and15118 final pixels (max2),
with exact raw depth. Reuse-only Track24 passes all poses. No eligibility exception
or image tolerance is invented: instancing is excluded from optimized/default.
Occlusion and opaque submission remain experimental pending independent proof.

Reflection fixture first passed poses0/12, then both baseline and optimized
failed Degenerate original environment view at offset-8: camera lay directly
over target and look_at had colinear target/up. Fixture uses offset-4 instead;
the failed run is retained, not counted as a rendering optimization regression.


TASK0055 all30 reuse GPU completion: TRACK_OPTIMIZATION_GPU_OK tracks=30,
90 actual static poses exact final/intermediate-color/rawdepth and logical packet
parity. Post-sequence forest-fire pair516active/91visible equal, exact pixels;
clear/reload/invalid-track cleanup PASS. Log local/task0055/parity-reuse-all30.log.
Counts differ from the initial single-track fixture because the renderer preserves
its original RNG across prior track loads; each pair receives the same sequence.
Benchmark creates fresh renderer/session each repeat, resetting this caller seed.


Release optimized actual-window race smoke PASS: native control, focus, failure,
restart/front-end/re-entry; screenshot inspected, original rendering gaps remain
explicit. Independent opaque and occlusion Track24 three poses plus forestfire
parity PASS but both zero eligible submissions/resources: this is no-op coverage,
not proof of usefulness or broader safety; both remain off.

Track11 Release diagnostic120warm/600samples/3repeats: median paired wall change
-14.9239%, prepare-29.0389%, submit-35.0658%; no persistent wall median/P95/P99
regression. This is ONLY one route and acceptance_protocol=false. Baseline20.5
to20.906ms medians; optimized repeat2 median17.786ms. Full30 gate still pending.
GPU/viewport timers unavailable for both; no substituted invented GPU estimate.
Logs baseline-diag/optimized-diag under local/task0055, diagnostic/comparison.json.


Read-only get_render_targets binding and body are available in Release to inspect
actual intermediate color/native-depth after timed loops. This merely returns
existing viewport textures; rendering/effect gates are untouched. It is required
to investigate tiny independent-process car-edge pixel variability present even
within baseline repeats. No tolerance or causality assumption is approved.


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
