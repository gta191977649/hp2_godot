# TASK-0087 water elevation and native wheel contact

Evidence **E-0087-W01**. Behavior authority: Debug `NFSHP2.ELF`, SHA-256
`815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`;
version independently unknown. Retail track material identities retain the proven
E-0048-D01 semantic map from `SLUS_203.62`, SHA-256
`7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`.
Read-only Ghidra MCP captures remain ignored under `local/task0087/`.
Original runtime comparison unavailable; not Verified.

## Confirmed original handling

The original TerrainType enum identifies PUDDLE=4, DEEP_WATER=5 and MUD_PUDDLE=20.
`TopologyCoordinate::GetElevation` Debug `0x00151CC0` publishes the selected
polygon's original terrain unchanged with height and normal. It does not reject
water or substitute ROAD. `Wheel::SetState` `0x001A1948` stores that terrain in
Wheel and CarState through the ordinary contact path.

The existing original wheel force chain already consumes those rows correctly:

- `Wheel::GuessGroundForce` `0x001A2108` selects the terrain's original
  DynamicFrictionMultiplier for sliding tyre force.
- `Wheel::FinalizeGroundForce` `0x001A26B8` selects StaticFrictionMultiplier for
  the original force limit, preserving traction boost/catch-up/pilot factors.
- `Wheel::GetDerivative` `0x001A1E88` selects RollingFrictionMultiplier for wheel
  angular acceleration. These functions have no special water rejection branch.
- `Wheel::MaybeMakeSkids` `0x001A2CF8` explicitly sets PUDDLE skid intensity to
  zero; this existing recovered path remains in use. Other terrain skid width
  and material paths continue to use original data.

The separate `Car::TriggerCarTerrainEffects` `0x00133988` recognizes all three
water IDs while counting grounded wheels. At player distance<60 and speed>10,
it requests original timed wipers and splash audio; deeper water also affects
replay event/state publication. Those are distinct visual/audio/gameplay services,
not a hidden replacement tyre force law. Full water splash/wiper/replay production
is not reconstructed by this correction. The existing bounded wheel FX consumer
and full-entry explicit remaining-branches failure are unchanged.

## Static-body dependency recovery and correction

The original static chain is also already represented by the bounded native
runtime, and its water rejection was port policy rather than original behavior:

- DynamicCollisionBody::CreateTrackPolygonCollisionBodies `0x001D46D8` and
  TrackPolygonCheck `0x001D4920` select candidates geometrically without a water
  terrain filter.
- TrackPolygonCollisionBody constructor `0x001D53A0` builds the same wafer shape;
  GetTerrainType `0x001D5760` returns its original polygon terrain unchanged.
- Inherited StaticCollisionBody::GetFriction `0x001D5CC8` returns original scalar
  `0x00325B18`; GetBounciness `0x001D5CD8` returns `0x00325B1C`. These are constant
  body properties for this class, independent of terrain; using them for water
  is the original algorithm, not a ROAD substitution.
- Contact::CollisionImpulse `0x001D1F58` uses body friction/bounciness/mass/inertia
  and does not read TerrainType. CarCollisionBody::ApplyImpulse `0x001D4F50`
  applies the same original scaled impulse and damage/reaction calls, with no
  water-dependent force/reset/submerge branch.
- Collision::AddImpulse `0x00219400` selects original per-terrain impact/scrape
  effect and audio thresholds after accumulating the impulse. It has no water
  special-case force branch. Existing collision audio material priority remains
  data-driven and receives the original water ID. Full collision particle effect,
  damage/reaction ownership and multiplayer services remain the existing bounded
  closure; this correction does not claim to recover them.

After that dependency inspection, the obsolete 4/5/20 rejection was removed from
`debug_terrain_from_retail`. Existing elevation, spawn validation and static-body
consumers now retain the original IDs. CAR32 and invalid IDs remain rejected.
No additional helper API, coefficient, force, collision geometry, fallback surface
or Godot physics was introduced. No original polygons are silently skipped.

## Actual Track12 findings

The configured original Track12 forward marker starts on ROAD1. Its start was
not the source of the water rejection. The original topology contains 66 PUDDLE4
polygons and 23 DEEP_WATER5 polygons. All 89 selected polygon-centre elevation
queries reproduced the old policy boundary and now publish the exact original
height, normal, found flag and terrain without error. No MUD_PUDDLE20 polygon
was present in this Track12 fixture; its downstream contact path is covered by
the controlled original-physics test instead.

## Validation

CMake 3.29.2 / MinGW-w64 GCC 13.2.0, 2026-10-21:

- `cmake --build --preset native-debug --target water_contact_tests -j 4`: passed
  by the parallel asset worker after root registered the target.
- `ctest --preset native-debug -R '^water_contacts$' --output-on-failure`: 1/1
  passed, 19.57 seconds, original assets/ELF present, not skipped.

`water_contact_tests.cpp` checks exact semantic identity for every elevation ID,
invalid/CAR boundaries, and continued invalid/CAR rejection while water remains unchanged. It queries the real original Track12 water polygons and start.
For each of 4/5/20, two NativeBackends run the same original 911TURBO configuration
and original collision geometry for 120 frames over a clearly labelled port test
plane. All 480 wheel contact observations per terrain retain the exact original
terrain, forces stay finite, and replay body/linear/angular states match exactly.
An additional coupled VClip/Contact/impulse fixture penetrates identical geometry
with material IDs 1/4/5/20. It requires a real upward opposing impulse and checks
exact native impulse/momentum parity across those IDs, as required by the
original terrain-independent static-body constants; original collision audio
priority is checked separately rather than requiring material effects to match.

The user's particular car was not supplied. Original 911TURBO is the recorded
native wheel fixture; no claim about a specific unreported car is made.

## Source and continuation

Only `debug_terrain_from_retail` in `track_contact.cpp` changed in production.
The existing `driving_track_tests.cpp` water-map rejection expectation now checks
original identity. New native test is `src/tests/water_contact_tests.cpp`. No original function implementation, signature,
whole-entry stub status or physics result was replaced. Ghidra findings above
corroborate existing recovered native wheel paths; no full CarTerrainEffects or
water collision implementation is claimed.

Run the commands above with configured original ELF and ZZDATA for continuation.
Further work on splash/wiper/replay and full collision effects must recover their
original missing services first. The removed guard does not establish completion
of those separate paths or original runtime equivalence.

Final validation after static-body dependency recovery:

- `cmake --build --preset native-debug --target water_contact_tests driving_track_tests static_collision_tests -j 4`: passed.
- `ctest --preset native-debug -R '^(water_contacts|driving_tracks|static_collisions)$' --output-on-failure`:
  water_contacts passed 22.67 s and driving_tracks passed 18.26 s. The old static
  rollback test failed because it deliberately marked its wall PUDDLE4 and still
  expected the removed policy rejection. The fixture was corrected to CAR32,
  retaining its original unsupported-ownership/failure-rollback purpose.
- Rebuilt the final water and static tests, then ran
  `ctest --preset native-debug -R '^(water_contacts|static_collisions)$' --output-on-failure`:
  2/2 passed, static_collisions 13.88 s and water_contacts 24.75 s, total 38.64 s.
- Coupled actual Contact impulses: terrain 1/4/5/20 each produced identical
  upward impulse Z=18.0015, with exact resulting native momentum parity. Material
  audio priority retained the original terrain/CAR priority table rule.

The earlier 19.57-second test preceded removal of the static-body guard and is
retained as history only; the final run above covers the complete correction.
Private read-only capture hashes are in `local/task0087/water-manifest.json`.
No original whole-entry stubs transitioned, and no original function bodies changed.
