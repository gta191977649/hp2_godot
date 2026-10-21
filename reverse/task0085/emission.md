# TASK-0085 native emission transfer

## Evidence E-0085-N01

This is a port-side scheduling and storage change. It reuses the existing
reconstruction of AcidEffectSystem emitter scheduling, TriggerEmitter, and packed
particle lifetime rules; it claims no newly recovered original functions.
Debug authority: ELF SHA-256
`815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`.
The original TriggerEmitter entry is Debug `0x00127858` (Retail counterpart
`0x00127700`); original particle stepping is Debug `0x00128870` (Retail
`0x00128718`). Existing TASK-0075 dynamic-particle records remain the source
for copied emitters and live car-velocity inheritance. No recovered function
implementation, address, original signature, or stub classification changed.

## Confirmed implementation

`ParticleSimulation` retains its default native path. Selecting GPU emission
before any live groups or pending commands activates a separate port transfer
path. Native emission still calls the same reconstructed emitter scheduler and
TriggerEmitter, in the same emitter traversal order, with the same original
random streams, distance predicates, 1000-particle capacity and 400-group capacity.
Full pools still execute the original random consumption. Backend changes with
live state are rejected; reset/replacement is the supported change boundary.

`ParticleGpuCommand` is independent of Godot. Ordered `Spawn` commands contain
original packed birth records plus stable port group ID, emitter index and
definition index. `Advance` contains timestep and current applicable car velocity;
`RemoveGroup` invalidates an expired or detached group. IDs are port bookkeeping,
not recovered original fields. Auto-emitter births precede the frame Advance;
external wheel births follow it and therefore remain unaged until the next step.
Multiple committed steps remain representable as multiple ordered commands.
The Godot consumer must drain only the committed candidate and preserve order.

GPU-mode CPU storage retains birth records and updates only the original packed
life/expiry ledger. It does not integrate position, direction, speed or animation,
and must not feed these stale fields to original RenderEffects. Thus original
capacity reuse and group expiry remain deterministic without GPU readback.

GPU step and dynamic trigger first operate on a private simulation copy. They
publish emitter state, random state, groups and pending commands together only
on success. Caller-level frame staging can discard the candidate after later
visual failures. Taking commands moves the pending queue out of the simulation.
Dynamic detach emits group removal while retaining static effects and global RNG.

## Validation on 2026-10-21

Tools: CMake 3.29.2; MinGW-w64 GCC 13.2.0.

- `cmake --build --preset native-debug --target game_core -j 6`: passed.
- `cmake --build --preset native-debug --target gpu_particle_emission_tests -j 6`:
  passed. Existing unrelated misleading-indentation warnings remain.
- `ctest --preset native-debug -R '^gpu_particle_emission$' --output-on-failure`:
  passed, 1/1, 5.43 seconds (not skipped).

The test replays transfer commands through the existing native particle step and
requires exact packed-record/position equality with the native simulation. It
covers variable steps, static and dynamic order, RNG/emitter state, inherited
velocity, absence of CPU motion integration, expiry, far-distance rejection,
1000/400 limits, failed-candidate rollback, copy/retry and detach. A 120-frame
original 911TURBO wheel-effects run uses real particle definitions and two native
vehicle backends, comparing transfer replay and following physics-frame results.

## Limits and continuation

These checks establish native scheduling/transfer consistency only. They do not
test GPU arithmetic, floating-point drift, shader curves, render sorting, actual
GPU readback, appearance, throughput or original runtime equivalence. No Verified
status or default-GPU enablement follows from them. Renderer and scene acceptance,
including failed-frame/pause/capture behavior, remain TASK-0085 integration work.
Reproduce with the build/test commands above; original Debug ELF and original
configured ZZDATA asset root are required. The test returns 77 when Debug ELF is
unavailable; the reported local run had both prerequisites.

Additional native regression command:
`ctest --preset native-debug -R '^(driving_particles|driving_particles_original|driving_wheel_particles_assets|driving_backend|driving_camera|driving_skids)$' --output-on-failure`
passed 6/6 in 38.54 seconds. Individual results: driving_skids 0.47 s,
driving_particles 0.04 s, driving_particles_original 1.12 s,
driving_wheel_particles_assets 6.72 s, driving_backend 27.03 s,
driving_camera 3.13 s.

## Actual GPU lifecycle integration check

`godot/scripts/debug/gpu_particle_lifecycle_smoke.gd` exercises production native
wheel effects through standalone driving and Track11, with `frame_post_draw`
awaited after every submitted tick. CMake registers `gpu_particle_lifecycle_gpu`
only under `PORT_ENABLE_GPU_TESTS` in Debug; timeout 240 seconds.

On 2026-10-21 the following commands passed:

- `cmake --build --preset godot-debug --target ps2_port -j 6` (no work required).
- `local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe --audio-driver Dummy --path godot --script res://scripts/debug/gpu_particle_lifecycle_smoke.gd -- --particle-backend=godot_gpu`
  printed both `GPU_PARTICLE_LIFECYCLE_CASE_OK: flat` and
  `GPU_PARTICLE_LIFECYCLE_CASE_OK: Track11`, followed by
  `GPU_PARTICLE_LIFECYCLE_OK`; process exit 0.

Runtime: Godot 4.7.2 `ed1daf0bf`, Vulkan 1.4.359, Forward+, Intel Arc B390 GPU.
Each case ran 300 actual driving ticks, required live dynamic groups, then checked
8 paused/repeated presentations without changing emitted count, manual-step count
or node count; resume advanced the GPU. Same-pointer restart, original CORVETTE
vehicle change, source detach and unload removed the corresponding dynamic groups
and nodes. The standalone case rejected a null camera and retried the same native
step successfully. This last check fails before candidate creation; there is no
public hook to inject a mid-frame failure. It does not establish later-frame
rollback under injected GPU/resource errors. The test verifies lifecycle counters
and cleanup, not rendered pixel visibility or visual equivalence.

Counter-test caveat: after this initial lifecycle run, the renderer investigation
found Godot request_particles_process double-dispatch consumed births before draw.
The counters passing did not establish that GPU particles were alive or visible.
This run must be repeated after the dispatch correction. The separate production
pixel-readback smoke is registered as `gpu_particle_visual_gpu`; it is the visible
output check. This lifecycle script does not read GPU particle state.

After the pinned-Godot dispatch correction, rebuilt the current DLL with
`cmake --build --preset godot-debug --target ps2_port -j 6` (GPU renderer compiled
and DLL linked), then repeated both windowed checks successfully:

- Lifecycle command above: both flat and Track11 passed; exit 0.
- Same Godot command with script
  `res://scripts/debug/gpu_particle_visual_smoke.gd`: passed; exit 0,
  `GPU_PARTICLE_VISUAL_OK: pixels=27018`. Test-only image readback counted
  27018 non-background pixels at frame 240 and 23729 at frame 299; this establishes
  substantive production particle output in the synthetic-floor fixture.

These remain host regression checks. Pixel presence does not establish original
appearance or numerical equivalence. Root-owned native/GPU image comparison and
strict performance acceptance are separate evidence.

Final source-freeze rerun after GPU abort snapshot rollback and draw-material
cache changes: rebuilt `ps2_port` successfully, repeated lifecycle flat+Track11
and production GPU visual commands above. Both exited 0 with their success markers;
visual counts remained frame 240=27018 and frame 299=23729. No script or shader
errors were emitted. This final run supersedes the initial counter-only run.
