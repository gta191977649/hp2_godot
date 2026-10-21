# TASK-0075 dynamic particle storage adapter

Authority: Debug NFSHP2.ELF SHA-256
`815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`;
revision unknown. Port tests are not original-runtime equivalence.

## Confirmed observations

- `afxCopyEmitter` 0x00127380 clears 0xb8 bytes from the new record+8,
  hashes and copies the library name (63 characters plus terminator), calls
  `SetEmitterData` 0x00127518, then copies LocalWorld+0x30..0x60.
  Read-only Ghidra listing 0x00127380..0x00127430 checked in this session.
- `SetEmitterData` only assigns a non-null data pointer. No state, cycle,
  countdown, random seed, inherited velocity or DontEmit flag is inherited.
- `afxGetEmitters` 0x00127438 appends copies to its supplied list; it does
  not register them through AddAutoEmitter. Parent evidence E-0075-001.
- Existing recovered TriggerEmitter implements negative-duration state bypass,
  lazy RNG initialization before distance rejection and negative-trigger
  countdown reset. Dynamic calls reuse this implementation unchanged.

## Port boundary

`ParticleSimulation::add_dynamic_emitter` binds the copied data by hash and
appends stable indices, without adding the copy to automatic traversal.
`trigger_dynamic` shares the simulation's groups, 400-group/1000-particle
limits and RNG. LocalWorld is temporary; callers can supply the orientation
containing the final selected local position required by E-0075-P02, rather
than restoring the initial library translation. Inherited velocity remains live for
existing particles. The caller stages the complete event/update transaction.
The adapter retains the decoded library name hash (original valid names
already carry bStringHash); no new game constants or particle definitions are
introduced. Missing data throws at the original SetEmitterData boundary.

## Tests and unresolved scope

`driving_particle_tests` covers zero-state copying, no auto-emission, far
rejection and lazy RNG without ELF math. Its optional original-image mode
covers negative/positive state gates, countdown, temporary world placement,
aging/expiry, shared particle and group capacities. Original-runtime visual
comparison and the caller's Car trigger/terrain selection are outside this
storage adapter and remain separately tracked by TASK-0075.

2026-10-20 validation: `cmake --preset native-debug`,
`cmake --build --preset native-debug --target driving_particle_tests -j4`
and `ctest --preset native-debug -R '^driving_particles(_original)?$' --output-on-failure`
passed (2/2). Dynamic detach/rebind and explicit final-local-position restoration
are included. The third optional `--wheel-assets` argument loads actual shared
library/textures and exercises every wheel with synthetic car/config records;
its first run exposed missing INGAMEB texture loading (111 definitions,
112 emitters, zero textures), reported to the resource integration owner.
After resource integration included original INGAMEB textures,
`./build/native-debug/driving_particle_tests.exe --original-image --wheel-assets`
passed with 111 definitions, 112 emitters and 13 textures, printing
`DRIVING_WHEEL_PARTICLE_ASSETS_OK`. The fixture selects the first actual
nonempty terrain smoke group matching the loaded definitions; terrain0 has
no such smoke binding. Four isolated wheel contacts each emit through the
original table, configured wheel-position, TriggerEffect and TriggerEmitter
chain. This validates the host connection with synthetic car/config data,
not original runtime appearance or numerical equivalence.

## E-0075-D02: MotionLive and precomputed curve audit

Read-only Debug `AcidActiveGroup` type query returns size0x1c:
Next0, Prev4, EmitterData8, Emitter0xc, pCar0x10, ParticleList0x14.
DoTimeStep listing00128BA4..00128BF0 requires MotionLive at data+0x1e,
nonnull group.pCar and MotionInherit at data+0x50 strictly greater than zero.
It reads current Car velocity+0x70/+0x74/+0x78, multiplies by timestep,
then uses each packed particle Inherit byte at00128D8C..00128DA0.
Emitter.InheritVelocity is not the active-loop velocity source. Null-pCar
static groups contribute zero live motion. The single-car adapter provides
the current source-frame car velocity before stepping; capture views must
not advance this state. Tests change car velocity from emission2 to current10
without another trigger and assert displacement1 over0.1 seconds; a static
MotionLive group ignores both emitter99 and unrelated car10 velocities.

`LoaderAcidFX`00126FA8 adds definitions/binds emitters without changing their
precomputed bases. DoTimeStep only regenerates ColourBasis/ExtraBasis in the
frontend branch. Shipped ExtraBasis x/y contain size/angle coefficients;
unused z/w padding is not a reason to replace these precomputed curves.

Actual GLOBALB `XETT_RoadSk_SmkA` has MotionLive0 and MotionInherit0.05.
Its ColourBasis alpha coefficients are1521.9949951171875,
-2806.658203125,1284.6634521484375,0. Sampling the cubic at1001 points
over0..1 gives maximum173.917198551625, above raw128. Autumn leaves and
mulch debris do use MotionLive1 with MotionInherit0.1. These observations
require preserving explicit renderer alpha bounds rather than assuming all
new smoke fits ordinary alpha blending.

Canonical JSON SHA-256 (`json.dumps(response,sort_keys=True)`):

| Read-only query | SHA-256 |
|---|---|
| listing00128B00..00128C50 | 0b700fbb4a9c23a4e02c18dcef3cdde585d14c8cb75e9eda5a603d46de827fec |
| listing00128D80..00128DA4 | e27c2782fd4847d77cf22bf6f741f552785ef7e0ea276e6334033c1755f0e562 |
| decompile00126FA8 | 3765cc7a83b26fbf1345ff4eb3d42f826ccec92edfdbd0dbf1bf97bfd8ccca71 |

2026-10-20: rebuilt driving_particle_tests; targeted CTest particle tests3/3
passed after the MotionLive source correction (2.33seconds). No original
runtime visual equivalence claim.

## E-0075-D03: outside-view dynamic render branch

Fresh read-only Debug RenderEffects00128EF8 decompilation confirms pCar groups
first test Car+514 per-view visibility. CarPosition1/5/6 suppress when the
camera's CarNotToRender matches and RenderDash is nonzero;9/10/11/26 have
rear-view/CarNotToRender selection. Supported outside tool cameras have null
CarNotToRender and no dashboard, while the owned car mask is all views. Therefore
these gates admit the same bounded particle packet path as static world groups.
The bridge explicitly rejects other masks at00128EF8; first-person/mirror/editor
and multiple-car ownership are not reconstructed. Positions remain world-space;
no extra car transform is applied by RenderEffects to spawned particles.

Raw decompile response: local/task0075/00128ef8-render.json (ignored), SHA-256
`33c8214c06b000ec88a885c14c44f4bb0a39f8baab0e884dcf6f75eaafb48c71`.
Existing per-packet signed high-alpha, solid and nonzero matrix-pack gaps remain
explicit. Flat Vulkan smoke recorded469 active/467 visible, Track11 recorded497
active/398 visible; no gaps in these bounded runs, not an all-surface proof.
