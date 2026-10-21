# Wheel particles and driving recovery (TASK-0075)

CarViewer/TrackViewer Test drive and the single-car runtime now connect original
wheel smoke and rear-wheel tire-spew to shared AcidFX simulation/rendering.
Ground skid marks retain their separate chain. No original-runtime equivalence
or Verified status is claimed.

## Confirmed original behavior

Authority: Debug NFSHP2.ELF, revision unknown, SHA-256
`815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`.

- Car::TriggerCarTerrainEffects00133988 reads grounded wheel slip, terrain
  thresholds and saturating consecutive-skid counters. Smoke has distance<150;
  rear tire-spew has distance<40 and signed longitudinal slip. Skid-mark conditions
  are different and are not substituted.
- Car::TriggerEffect0011E4E0 selects terrain emitter lists and wheel positions5..8.
  InitRenderInfo0011DEA8 supplies static configuration wheel XYZ, without radius
  subtraction or suspension/contact substitution. Original body orientation,
  State.Position and inherited/override velocity produce the emission transform.
  Negative intensity reaches TriggerEmitter unchanged.
- TerrainEffectEmitters0021CB40, afxGetEmitters00127438, afxGetEmitter001272F0
  and afxCopyEmitter00127380 establish separate copied terrain emitters. The
  shared library has111 definitions/112 sources; texture lookup spans the original
  GLOBALB and INGAMEB archives. No generated smoke texture or guessed curve is used.
- World0014DDE0 updates existing particles before terrain emission0014DE44.
  Dynamic/static groups share1000 particle slots,400 group slots and RNG. Live
  motion uses the current owning car velocity; static groups have no owning car.
  Gameplay retains shipped precomputed Hermite size/angle/colour curves.

Evidence: [producer/dispatch](../reverse/task0075/terrain-producer.md),
[positions/scheduling](../reverse/task0075/positions-scheduling.md),
[dynamic storage/live motion](../reverse/task0075/dynamic-particles.md).

## Port boundaries and remaining gaps

The one-car tool explicitly supplies closest-player distance0. Its outside camera
has no CarNotToRender/dashboard suppression, and the supported car visibility mask
is all views; other masks throw at00128EF8. Full World player lookup, multi-car
ownership, first-person/mirror/editor filtering remain outside this adapter.

Track driving shares WorldEffects, native depth and alpha ordering with the track
renderer. The flat fixture uses the same particle packet and ordinary host depth
testing because its synthetic floor has no GS depth target. Host projection and
rasterization remain explicit adapters.

Full terrain drive-over/water/damage/nitro/flat-tire branches remain a throwing
whole-entry boundary. Existing signed high-alpha GS compositing and nonzero
matrix-pack/solid particle routes remain reported gaps. Unsupported packets are
never clamped into a guessed blend. Road smoke curves can exceed raw alpha128;
the renderer checks the actual texture/vertex alpha bound per packet.

TrackViewer browsing uses Camera3D defaults75 degrees/KEEP_HEIGHT. Preview controls
affect CarViewer only. Driving/race retain their separate outside projection, and
returning from driving restores the browsing projection.

## Validation and continuation

- New native tests4/4 pass, including actual resources and two resets x120 steps
  comparing exact911TURBO body, velocity, wheel and force results with/without
  visual particles. The scenario requires particles to spawn.
- Existing driving/skid/backend/environment/runtime regressions10/10 pass.
- Headless input/driving/race/pipeline/default-projection regressions7/7 pass.
  Projection checks cover initial/reset, preview no-overwrite,4:3/16:9/portrait,
  and return from real-track driving.
- Godot4.7.2 Vulkan/Forward+ on Intel Arc B390: flat fixture peak469 active/467
  visible; Track11 peak497 active/398 visible. Both bounded runs report no
  unsupported packets. Inspected screenshots remain ignored under local/.
- Original runtime probe returned `blocked: timed out`; no Verified promotion.

```powershell
cmake --build --preset godot-debug -j4
ctest --preset native-debug -R '^driving_(terrain_effects|particles|wheel_particles)' --output-on-failure
ctest --preset godot-debug -R '^(driving_(particles_host|input_host|asset_host|lab_host)|track_default_projection_host|race_runtime_host|track_pipeline_parity)$' --output-on-failure
& ./local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe --path godot --audio-driver Dummy --resolution 960x540 --script res://scripts/debug/driving_particles_smoke.gd -- --particle-track
```

See [current physics gaps](DRIVING_GAPS.md) for follow-up tasks. Certification
continues under TASK-0038 with a runnable matching Debug scenario and identical
initial/input/time/contact state. Retail oracle hooks cannot certify Debug.


## TASK-0085/86 GPU particles and attached light sprites

The optional godot_gpu particle backend retains original emission/RNG/lifetime
allocation and runs custom GPU curves with transactional visual epochs. Select
--particle-backend=godot_gpu before loading; native remains default because the
user stopped extended benchmarking. No performance gain is asserted.

Original marker-driven head/brake/reverse/cop flare glow is now bound to driving
snapshots and fixed attached billboard instances. Normal-car horn flashes heads;
original COP-class horn toggles FAST police state and subsequent phases. Original
textures/normals/size/colour/ZBias are used, with actual depth occlusion. See
VEHICLE_LIGHT_FLARES.md and reverse/task0086 for evidence and limitations.

## TASK-0089 water spray, splash sound and timed wipers

Original DriveOver1/2 groups now join the visual particle transaction, retaining
original speed thresholds/channel overlap, body terrain query and wheel-pair
midpoints. Water spray uses authored GLOBAL emitters/textures, not new splash
presets. PUDDLE and DEEP_WATER have original groups; the audited MUD_PUDDLE lists
are empty and remain empty. Native/GPU backends consume the same original births.

Grounded water contacts schedule original timed wipers and splash requests once
per committed native frame. Original sound cooldown, enable flags, sample RNG
and InGame Splash1/2/3 patches are retained. Original wiper solids, skin/default
texture lookup and discrete rigid-animation matrices are decoded at load time;
visibility follows the original timed interval/state and projected-size>40 rule.
Missing optional wipers remain absent. Required unresolved animation throws;
passive external animation metadata remains explicitly diagnosed so static track
objects are not rejected merely because an unused animation is unbound.

See reverse/task0089/water-research.md, wiper-consumer.md and host-consumers.md
for tests, source addresses, assets and boundaries. Listener/world sound-task
ownership and original replay capture remain bounded services; the separate
rain-screen RainSplash system is not restored here. No benchmark or Verified claim.
