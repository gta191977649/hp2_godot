# E-0085-G01: Godot GPU particle transport and clock probe

TASK-0085, 2026-10-21. Port-side implementation, not newly recovered game
functions and not Verified. The original logic authority remains the existing
AcidFX records for `DoTimeStep` Debug00128870 and `RenderEffects` Debug00128DA0,
including Debug ELF SHA256
`815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`.

## Implementation

`src/platform/godot_bridge/gpu_particle_renderer.hpp/.cpp` owns persistent
GPUParticles3D nodes per logical original group, each with1000 slots (the native
global maximum). Custom process materials carry original packed life, angle,
frame and inherited fraction; velocity is quantized back to original speed and
direction after each effective visual step. CPU emission and life ledger remain
separate. No runtime particle state readback or per-frame quad upload exists.

Draw materials are cached by immutable definition, preserving recovered texture,
atlas, curves and texture-chain material shader. Native screen rounding and GS
depth decoding remain in the draw vertex/fragment path. Unsupported solid chains,
matrix packs and atlases beyond packed frame range fail explicitly. Source-over
alpha exceeding the supported normalized blend bound discards whole quads;
definition-level potential diagnostics use cubic alpha extrema and per-tile
sampler footprint bounds. Exact discarded/visible counts require readback and
are deliberately unavailable in normal rendering.

Prepare performs dependency/command preflight, stages emission and process
uniform changes, and snapshots host metadata. `commit_frame` dispatches actions
only after the surrounding frame succeeds. `abort_frame` cancels actions and
restores group epochs, order and counters, deleting newly prepared nodes.

## Confirmed pinned host behavior

Godot4.7.2.stable.official.ed1daf0bf, Vulkan1.4.359, Intel ArcB390,
Forward Mobile. Isolated probe under ignored `local/task0085/gpu-probe`:

```
& local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe --path local/task0085/gpu-probe --rendering-method mobile --script probe.gd
```

With speed_scale0, the custom shader still executes at DELTA0. An explicit
integer epoch controls original visual advancement exactly once. Epoch wraps at
20bits to stay exactly representable in the particle custom float. Births before
Advance carry the old epoch; births after Advance carry the current epoch.

Adding `request_particles_process(.001)` caused a single emitted particle to
disappear in this pinned probe. Removing that request preserved emission and
clock progression. Production therefore uses epoch-controlled zero-delta
dispatch, rather than that extra manual request. This is host runtime evidence,
not a claim about all Godot drivers or original PS2 behavior.

To advance hidden/offscreen nodes, each committed changed group explicitly
calls `RenderingServer::particles_request_process(node->get_base())`. This only
schedules the normal zero-delta dispatch; it does not request extra process
time. Unchanged/paused frames do not schedule an extra epoch. The tracked
fixture hides each probe before emission, advances twice via this API, checks
ten idle render frames, and shows the node again without an advance.

Readback below is test-only `capture_aabb`; QuadMesh expands each axis by1, so
the particle position equals AABB.position+(1,1,1):

| Probe | First effective step | Second step | Idle presents |
| --- | --- | --- | --- |
| p=(1,2,3), v=(1,0,0), dt=.25 | (1.25,2,3) | (1.5,2,3) | unchanged |
| p=(1,2,3), v=(1,2,3), gravity4, drag.1, inherited=(.5,1,2), fraction128/255, dt=.25 | (1.306495,2.600490,3.694730) | (1.603689,3.173455,4.118376) | unchanged |
| Debug custom-state position projection: life10240, frame3, inherited128, interval256, mask15 | life9984, frame4, fraction128, epoch1 | life9728, frame5, fraction128, epoch2 | unchanged |

Raw outputs: `clock-result.txt`, `motion-result.txt`, `state-result.txt` in that
ignored directory. Outputs are rounded to approximately1e-6; this does not
establish original runtime equivalence. The first motion step agrees with the
explicit recovered update arithmetic to the displayed precision. Second step
includes packed velocity quantization.

The tracked reproducible fixture retrieves the actual process shader from a
production GPUParticles3D created by Ps2DrivingRuntime.present_particles. It uses
that shader directly for the motion test and a test-only final position
projection of CUSTOM for packed state readback. It does not mirror the update
implementation in GDScript:

```
& local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe --path godot --script res://scripts/debug/gpu_particle_state_smoke.gd -- --particle-backend=godot_gpu
```

Passed2026-10-21 on pinned Vulkan Forward+ with all eight position/state/idle/
off-to-on assertions, `GPU_PARTICLE_STATE_OK`, exit0. Motion tolerance1e-4, including the
driver/Godot packed-state path. A preliminary attempt overriding this project's
renderer to Mobile was correctly rejected by its existing TASK0071 Forward+
contract; only the standalone ignored probe used Mobile.

## Remaining differences and limits

GPU sin, reciprocal, length and floating-point expression contraction differ
from original EE arithmetic. Group alpha sort depth uses latest birth position
rather than exact average visible-particle depth; instance order is host GPU
order. These fall within the user's recorded GPU numeric/sorting exception.
The backend has no cross-group node pool; nodes persist and reuse within a live
logical group, while compatible definition draw materials are shared.1000 slots
per group may be expensive; performance/default acceptance is a separate gate.
Capture passes do not submit epochs. Production image/lifecycle tests and full
performance results are maintained by the parent TASK-0085 records.
