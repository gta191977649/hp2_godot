# TASK-0075 wheel terrain particle producer

## Evidence E-0075-T01

Authority: Debug NFSHP2.ELF, revision unknown, SHA-256
815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
Read-only Ghidra `inspect decompile` at 00133988 and `inspect listing`
00133988..00133D60, plus `data_types get CarEffect/Car/CarState`.
Private raw exports: ignored `local/task0075/00133988.json` and
`local/task0075/0-producer.json` through `3-producer.json`.

## Confirmed

- Exact recovered entry spelling: Car::TriggerCarTerrainEffects(float), Debug
  00133988. This implementation deliberately projects only its four-wheel loop
  00133A20..00133CB4 through a port-side API. The full entry throws for its remainder.
- State is Car+0x40. Wheel matrices are Car+0xF0 stride0x40; contact bytes
  Car+0x350; terrain words Car+0x354 stride4; slip pairs Car+0x374 stride8;
  skidding counters Car+0x870; body velocity Car+0x70.
- Contact is required. Airborne skips all counter changes. Contact at distance
  >=150 or slip magnitude <=terrain+0x58 clears its counter. Otherwise the
  unsigned byte saturates at255, even when the emitter group is null/empty.
- TerrainTypeInfoTable is003102E8, stride0x90. Smoke group/min/max are
  +0x54/+0x58/+0x5C. Magnitude is sqrt(x*x+y*y). Normalized intensity is multiplied
  by0 for count<2,0.25 for<20,0.5 for<60,0.75 for<120, otherwise1; then clamped
  with floating max/min (listing00133B7C/80), never integer conversions.
- Smoke calls Car::TriggerEffect0011E4E0 with CARFX_SKID_SMOKE=3 and null velocity.
  Packed parameter is ((wheel+5)<<16)+terrain, matching listing001339A8,
  00133BA4 and00133C9C.
- Tire spew requires distance<40, wheel index>1, slip.x>terrain+0x64, and
  nonempty group+0x60. Intensity clamps (x-min)/(max-min) with max at+0x68.
  It calls CARFX_TIRE_SPEW=4. These exact enum names/values are in CarEffect.
- Spew velocity is bMulMatrix00254278(WheelMatrix,{slip.x,slip.y,0}) plus
  State.Velocity. Existing bMulMatrix receives the original mutable bUnitVector4
  at00317B20; translation is retained by the original matrix operation.
- Smoke is emitted before spew for each wheel, wheels traverse0..3. This loop
  has no timestep or random draw; those belong to the downstream effect chain.

## Unresolved boundaries

The rest of00133988 includes topology-based drive-over and bottom-out effects,
water/wiper/sound/replay state, damage, nitro and flat-tire branches. They are
not silently included by the bounded projection. Car::TriggerEffect and particle
resources/update/render require their own evidence and service implementation.
Native tests establish translation branch coverage only; no original-runtime
comparison or Verified claim is made.

## Evidence E-0075-C01: wheel Car::TriggerEffect binding

Same Debug authority. Fresh read-only decompile0011E4E0,
constructor0021CB40, afxGetEmitter001272F0, afxGetEmitters00127438;
read-only datatype OverrideRenderingPosition establishes OverrideViewBits+0x50,
and Car datatype establishes TheOverrideRenderingPosition+0x4C0.
Private exports are under ignored local/task0075, including constructor.json,
overrides.json and the address-named producer exports.

Confirmed: core entry0011E4E0 skips absent render info, intensity0, effect0,
or nonzero OverrideViewBits. The host render availability is explicit in services;
no fictional target CarRenderInfo is allocated. Wheel override positions5..8 use
State.Position, BodyMatrix and original static configuration positions, not
runtime suspension contact positions. The original matrix operation is
eMulMatrix(orientation,local_world). Velocity XYZ is inherited, W reset0;
TriggerEmitter receives negative intensity. The final saved emitter orientation
retains the last static local position, matching the downstream adapter's explicit
restoration-matrix argument.

Confirmed: constructor0021CB40 traverses terrain0..32 and chooses group strings
at row+0x54/0x60 for smoke/spew. afxGetEmitter searches the AutoEmitterList by
EmitterData.GroupNameHash. afxGetEmitters appends copies in search order.
The port adapter uses ParticleSimulation.auto_emitter_order(), not file order,
and one list per effect/terrain, shared across all wheels. It stores simulation
indices only and must be recreated whenever the simulation is replaced.

Validation: `cmake --build build/native-debug --target driving_terrain_effects_tests
port_runtime --parallel 2` and `.\build\native-debug\driving_terrain_effects_tests.exe`
passed with MinGW GCC13.2.0. Tests cover strict distance/slip branches, counter
bands/saturation/persistence, terrain parameters, null/empty groups, signed rear
spew, matrix point translation and inherited velocity, effect transform and
restoration, negative intensity, and render/override/zero-intensity gates.

Raw response SHA-256 identities:

| Capture | SHA-256 |
|---|---|
| 0-producer.json | e86d6a286b5430d4c67a60be59f8bd89e2eeaa5e8a128287b227844b6979fe76 |
| 3-producer.json | edb9e77e9f7609f327e3d4b6763f7434594357426df8190cc96c3095202d2285 |
| constructor.json | 1626c1e387c4eaaa6575ba8ab3d0e1bb42e5d6a95347d92f66d99727e01080cf |
| 001272f0-producer.json | d3dd2dc772c13a0a098e98e189b6009095d3b373cccdd1126d8b93ccced33295 |
| 00127438-producer.json | 4922b33b1ea9d8623d146575592cf31c1ba6af6f8d9c428dd73d26a1c41d47e0 |
