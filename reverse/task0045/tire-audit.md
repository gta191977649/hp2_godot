# TASK-0045: terrain, tire grip and diagnostic audit

Target: Debug NFSHP2.ELF, SHA-256 `815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`. The authorized local ELF was rehashed during this task and matched. Behavior claims below come from fresh read-only Ghidra metadata, decompilation, explicitly bounded instructions and memory reads. No original-runtime equivalence is claimed.

## Evidence

- **E-0045-T01:** live TerrainType/TerrainTypeInfo/TireInfo/Wheel/ControlInfo/Car/PhysicsCar/CarState/CarTypeInfo types, all 33 terrain rows, pointed-to names, exact relevant scalar bits and whole-row hashes. [terrain-rows.json](terrain-rows.json) retains names, row identity, physics/noise bits and exclusion classification. Complete 144-byte row responses remain ignored under local/task0045/tire.
- **E-0045-T02:** contact/load/slip/lateral/iteration/finalization/rolling/noise audit. [tire-audit.json](tire-audit.json) contains all fresh function query ranges, instruction counts, listing UTF-8 SHA-256 values, direct call targets, source markers and function-record links.
- **E-0045-T03:** original traction/catchup/yaw source traces, setters and mutable globals.
- **E-0045-T04:** added finite tire fixtures, described below.
- **E-0045-W01:** read-only raw-field publication contract below.

This is a bounded audit of the existing Test drive closure. Full original topology, external racing/AI writers, water/wall/object responses, skid/effects/audio and general EE exceptional arithmetic remain outside it. Synthetic surface selection does not introduce a second force model.

## All 33 terrain rows

Live TerrainType contains values 0..32 and NUM_TERRAIN_TYPES=33. TerrainTypeInfo is 0x90 bytes, base003102e8, stride0x90. Every row's Type word equals its index and Name agrees with its enum suffix. Relevant exact fields: RoadNoiseGridSpacingInverse+18, RoadNoiseScale+1c, StaticFrictionMultiplier+20, DynamicFrictionMultiplier+24, RollingFrictionMultiplier+28. Other fields/names are recorded as evidence, not a license to approximate their behavior.

Only ROAD1, DIRT2, GRASS6, SAND7 and SNOW8 are approved for initial synthetic selection. NONE0 is a sentinel/no-contact row. Every other row is explicitly excluded by tool scope, rather than treated as an unknown enum or guessed plane equivalent.

| ID | Original Name | Initial set | Static / dynamic / rolling | Noise scale |
|---|---|---|---|---|
| 0 | `NONE` | excluded | 1 / 1 / 1 | 0 |
| 1 | `ROAD` | supported | 1 / 1 / 1 | 0 |
| 2 | `DIRT` | supported | 0.97000003 / 0.95787501 / 1 | 0.050000001 |
| 3 | `DIRTROAD` | excluded | 0.99000001 / 0.9813 / 1 | 0.02 |
| 4 | `PUDDLE` | excluded | 0.95999998 / 0.95130002 / 1.1 | 0.0099999998 |
| 5 | `DEEP_WATER` | excluded | 0.95999998 / 0.94800001 / 1.5 | 0.029999999 |
| 6 | `GRASS` | supported | 0.95999998 / 0.94800001 / 1.05 | 0.050000001 |
| 7 | `SAND` | supported | 0.95999998 / 0.94999999 / 2 | 0.050000001 |
| 8 | `SNOW` | supported | 0.94999999 / 0.93812501 / 1 | 0.0099999998 |
| 9 | `DIRT_SHOULDER` | excluded | 0.98000002 / 0.97130001 / 1 | 0.02 |
| 10 | `SAND_SHOULDER` | excluded | 0.97000003 / 0.96130002 / 1.2 | 0.015 |
| 11 | `SNOW_SHOULDER` | excluded | 0.97000003 / 0.95787501 / 1 | 0.0099999998 |
| 12 | `SIDEWALK` | excluded | 1 / 1 / 1 | 0.029999999 |
| 13 | `SNOW_SIDEWALK` | excluded | 0.97000003 / 0.95787501 / 1 | 0.029999999 |
| 14 | `ROCK` | excluded | 1 / 1 / 1.01 | 0.02 |
| 15 | `WOOD_WALL` | excluded | 1 / 1 / 1 | 0.0099999998 |
| 16 | `CACTUS_WALL` | excluded | 1 / 1 / 1 | 0.0099999998 |
| 17 | `SOLID_WALL` | excluded | 1 / 1 / 1 | 0.0099999998 |
| 18 | `BOARDWALK` | excluded | 1 / 1 / 1.02 | 0.041999999 |
| 19 | `COBBLESTONE` | excluded | 1 / 1 / 1.02 | 0.048 |
| 20 | `MUD_PUDDLE` | excluded | 0.95999998 / 0.95130002 / 1.1 | 0.0099999998 |
| 21 | `ROUGH_ROAD` | excluded | 1 / 1 / 1.01 | 0.02 |
| 22 | `GRASS_SHOULDER` | excluded | 0.98000002 / 0.97130001 / 1.02 | 0.0099999998 |
| 23 | `AUTUMN_LEAVES` | excluded | 0.97000003 / 0.95999998 / 1 | 0.02 |
| 24 | `AUTUMN_LEAVES_SHOULDER` | excluded | 0.98000002 / 0.97130001 / 1 | 0.0099999998 |
| 25 | `MULCH` | excluded | 0.97000003 / 0.95787501 / 1.05 | 0.029999999 |
| 26 | `MULCH_SHOULDER` | excluded | 0.98000002 / 0.97130001 / 1.05 | 0.015 |
| 27 | `ROAD_AUTUMN` | excluded | 0.99000001 / 0.9813 / 1 | 0 |
| 28 | `PLANT` | excluded | 0.99000001 / 0.98100001 / 1 | 0 |
| 29 | `POST` | excluded | 0.99000001 / 0.98100001 / 1 | 0 |
| 30 | `METAL_GRATE` | excluded | 0.99000001 / 0.9813 / 1 | 0 |
| 31 | `POTHOLE` | excluded | 1 / 1 / 1 | 0 |
| 32 | `CAR` | excluded | 0.99000001 / 0.9813 / 1 | 0 |

The selected static/dynamic multipliers are relatively close; sand's separate rolling multiplier is twice road's. All five noise-spacing inverses are 1. JSON raw float32 bits, rather than the rounded decimal display, are authoritative. No arbitrary low-friction presets replace these values.

## Contact, load and force audit

1. Suspension::ComputeAuxiliaries001a37a0 transforms each wheel rest point through body state and queries Wheel::FindDrivingSurface001a1d58. CheckTerrain001a1d80 calls the provider with that individual world point and receives material/normal outputs. Original TopologyCoordinate::GetElevation00151cc0 selects polygon material and normalXYZ/plane-distanceW. The tool provider supplies explicitly synthetic geometry/material; it does not implement original polygon selection.
2. CheckTerrain calls GetRoadNoise001530e8 for the queried material and stores RoadNoise+15c at001a1dec before its normal/rotation denominator rejection. Rejection sets terrainNONE and returns-100 after the noise store; rejected contact does not imply a newly zeroed RoadNoise.
3. Suspension retains original bump stops, spring/progression/damping, sway and ground penalty. ResolveForces001a3b10 passes request/contact to ExchangeZForces001a1c80. The latter stores contact byte+155, conditionally updates contact point and sets ZForce+158. Positive requests multiply the exact double-cross/dot factor with lower bound0.3000000119; nonpositive requests store zero. ZForce is not an independent tool weight estimate.
4. GuessGroundForce001a2108 preserves brake/e-brake order and local velocity transform. CurrentSlip = State.w * WI.Radius - local_velocity.x;001a2268 stores it and the next delay-slot store also updates OldSlip. Sliding capacity combines TireInfo.DynamicFrictionCoefficient, CatchupFrictionMultiplier, TractionBoost, material.DynamicFrictionMultiplier and optional pilot factor. The original low-speed branch can divide force back by accumulated scale, so material dynamic multiplier is not universally the final force ratio. Strict MaxStaticSlip comparison matters: equality can retain non-sliding when the other conditions permit it.
5. GetLateralForce001a2500 retains both branches. UseNewTireForces=0 scales the old CorneringForce curve using stored wheelZForce, ignoring its formal load argument in that branch. The new branch uses formal load*LoadFactor/TI.MaxLoad, original angle conversion/MaxSlipAngle and CVT.W.S indexing; index<6 blends adjacent families, otherwise family6 is selected. Preserve actual conversion/comparison conventions without assigning an inferred physical unit to MaxSlipAngle.
6. PhysicsCar::IterateForces00138ec0 guesses clutch and all four wheel forces, executes exactly five original corrections using mass/inertia/moment arms, skips wheel modification when slipping or brake locked, updates clutch each pass, then finalizes each wheel. Selecting a surface must retain this solver.
7. FinalizeGroundForce001a26b8 retains positive-drive ellipse scaling, static cap, pilot factor, usage publication, later velocity-coupling and force-frame projection. The static cap combines original ZForceScale, catchup, material static multiplier, TI static coefficient, ZForce, traction boost and optional pilot factor with original grouping. Material selection is not another cap calculation in UI.
8. GetDerivative001a1e88 uses selected material.RollingFrictionMultiplier, separate damage drag, inertia division, brake-lock zeroing and original non-sliding slip correction. Rolling resistance must not be inferred from static/dynamic friction.
9. GetRoadNoiseGridValue001530a8 preserves its masked integer hash. GetRoadNoise001530e8 floors coordinates using truncation plus negative correction, bilinearly combines four samples and multiplies original material noise amplitude. Through CheckTerrain it affects original wheel height/load calculations. The synthetic flat normal is not recomputed from another noise physics surface.

Fresh initial mutable globals: UseNewTireForces00327d60 word1; WheelForceFrame00327d6c word1; ZForceScale00327d50=1; PilotFactor00327d54 about.85; NewCorneringScale00327d64=2.5; LoadFactor00327d68 about.8; TireForceEllipseRatio00327d70=1.5; MaxStaticSlip00327d3c=.5. Raw bits and integer/float distinctions are recorded in the manifest. Their existence does not authorize tuning controls.

## Boost/catchup/yaw traces (E-0045-T03)

Wheel constructor001a16e8 initializes TractionBoost+178 and CatchupFrictionMultiplier+17c independently to1. Original standalone leaves SetCatchupFrictionMultiplier001a3368 stores+17c, AdjustTractionBoost001a3370 multiplies+178 and SetTractionBoost001a3380 stores+178. Move uses equivalent original inlined stores; these factors must remain separate.

PhysicsCar::SetBoost00139418 receives Car.State.BoostFactor at the start of Move: CarState+3a0, hence Car+3e0. It writes engine catchup torque, bodyCatchUpMultiplier, aeroBoost and all four Wheel.CatchupFrictionMultiplier fields (0013945c), plus aeroDraft=1-Car.DraftFactor. A later original damage override can call SetBoost again. Native fixture BoostFactor initialization is a port scene input; external racing catchup AI is not recovered by this trace.

TractionBoost starts from PhysicsCar.FrictionBoost+70, with an additional DriverMode=0 global factor. Constructor initializes FrictionBoost=1; PhysicsCar::SetFrictionBoost001395d0 stores it. Original Car::SetFrictionBoost001316d8 stores Car.FrictionBoost+454 and, in its movement-mode branch, mover+70. The standalone Car setter remains outside the implemented Test drive invocation surface. This audit does not claim all external writers are implemented.

Move updates PhysicsCar.YawControlMultiplier+74 from time/e-brake. At wheel index2 it invokes YawFrictionBoost00137c10 with original yaw, CarState.VelocityMagnitude, Control.YawControl/YawControlNFS, DriverMode, body grade and Car::GetPostCollisionYawControlBoost00135f10. The blended factor changes the running TractionBoost for wheel indices2 and3. Original BurnOutState/BurnOutTraction applies its additional per-index factor. Stores occur at00138508 and00138548.

YawFrictionBoost stores abs(grade), uses baseabs(grade)+1, and multiplies NFS control by post-collision boost when DriverMode!=0. E-brake>.5 and sufficiently small yaw returns the base. Otherwise original mutable LowSpeedYawBoost/HighSpeedYawBoost and LowSpeedSpeed/HighSpeedSpeed interpolate the speed factor without an invented interpolation clamp; MaxYawBonus caps the added term. GetPostCollisionYawControlBoost reads original collision timestamp/world time and samples its graph only within its four-unit window, otherwise zero. Source names/types, scalar bits and exact instructions are recorded.

## Diagnostic phase contract (E-0045-W01)

Publish original stores from the same completed step; do not recompute a combined grip coefficient.

| Original field | Debug offset | Original phase / interpretation boundary |
|---|---|---|
| Wheel.Terrain | +150 int | Per-wheel query result; can become NONE after slope rejection |
| Wheel.Slipping / TouchingGround | +154/+155 bytes | Slip classification / load-exchange contact; contact is already in WheelSnapshot |
| Wheel.ZForce | +158 | Nonnegative original request result; no separately inferred SI unit |
| Wheel.RoadNoise | +15c | Original sampled height perturbation, before terrain denominator rejection |
| Wheel.CurrentSlip | +160 | Radius*angular state minus localX velocity, preserving original scalar units |
| Wheel.Slip | +100/+104 | Sliding stores current slip/localY velocity; non-sliding zeros both; not interchangeable with CurrentSlip |
| Wheel.TractionUsage | +168 | Original norm/cap ratio before later force correction, capped1; forced1 for sliding/lock and0 for no-contact; zero load can still yield1 |
| Wheel.TractionBoost / CatchupFrictionMultiplier | +178/+17c | Separate factors written by Move / SetBoost |
| Wheel.GroundForce | +114/+118 | Original final two-component force; not necessarily derivable from published usage |
| TI.StaticFrictionCoefficient / DynamicFrictionCoefficient | TI+c/+10 | Tire configuration, separate from material multipliers |

These are original labels/formulas, not SI-unit certification or general EE equivalence. observe_grip must not change any Wheel byte. CAR32 remains a valid diagnostic enum despite exclusion from the synthetic selector. The observer samples stored phases; quantities produced earlier in the step are not falsely presented as a simultaneous force-budget recomputation.

## Counterevidence and unresolved scope

- Corrected provenance only: canonical mode3 degenerate VU normalization formerly named historical Retail001a52ec. Fresh Debug VRSQRT is001a2ac4. Root changed only that diagnostic string, no arithmetic. The Debug001a26b8 record has an append-only TASK0045 amendment. General exceptional VU equivalence remains unknown.
- Excluded-case gap: current port GetRoadNoise rejects material>=32, while original enum/table include CAR32/count33 and original captured function has no matching bound guard. No source change is made because CAR is outside the approved five surfaces; do not label this guard original behavior.
- No additional arithmetic mismatch was demonstrated for the audited ordinary finite paths of the initial five materials. This is not original-runtime verification. Full topology and water/wall/object responses remain outside the synthetic tool; excluded rows must not be guessed.
- EE exceptional DIV.S, CVT.W.S overflow/NaN, subnormal handling, VU rounding/flags and original uninitialized fourth lanes retain TASK0044 limitations. Port fixtures cannot close those questions.

## Fixtures and reproduction

src/tests/driving_tire_tests.cpp adds T04 tests for all five exact material bit sets: saturated static cap, dynamic row selection, independent rolling drag/nonzero noise, normal-load2/4/zero, separate traction/catchup factors, old/new lateral-load semantics and family6, strict MaxStaticSlip equality/nextafter, mutable yaw driver/e-brake branches, and W01 raw fields/full-image read-only invariance. Curves, grid samples and motion inputs are explicitly synthetic independent arithmetic fixtures.

Root's serialized run passed driving_tire1/1 in.27s. This worker ran no build. Reproduction: cmake --build build/native-debug --target driving_tire_tests; then ctest --test-dir build/native-debug -R '^driving_tire$' --output-on-failure. Native surface/zone/replay and real-window acceptance belong to root/runtime. This worker performed no original Debug runtime comparison.

## Per-function audit links

Fresh query ranges, hashes and actual call targets are in tire-audit.json. Original source markers retain their existing ECOFF evidence.

| Debug entry / record | Original signature | Fresh evidence |
|---|---|---|
| [`0x001316d8`](../../reverse/functions/sha256-815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b/driving/001316d8.md) | `Car::SetFrictionBoost(float)` | E-0045-T03 |
| [`0x00135f10`](../../reverse/functions/sha256-815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b/driving/00135f10.md) | `Car::GetPostCollisionYawControlBoost(void)` | E-0045-T03 |
| [`0x001375b0`](../../reverse/functions/sha256-815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b/driving/001375b0.md) | `PhysicsCar::Calibrate(void)` | E-0045-T02 |
| [`0x00137c10`](../../reverse/functions/sha256-815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b/driving/00137c10.md) | `YawFrictionBoost(float, float, float, float, float, int, float &, float)` | E-0045-T03 |
| [`0x00137d18`](../../reverse/functions/sha256-815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b/driving/00137d18.md) | `PhysicsCar::Move(float)` | E-0045-T03 |
| [`0x00138d90`](../../reverse/functions/sha256-815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b/driving/00138d90.md) | `PhysicsCar::ResolveForces(void)` | E-0045-T02 |
| [`0x00138ec0`](../../reverse/functions/sha256-815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b/driving/00138ec0.md) | `PhysicsCar::IterateForces(void)` | E-0045-T02 |
| [`0x00139418`](../../reverse/functions/sha256-815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b/driving/00139418.md) | `PhysicsCar::SetBoost(float)` | E-0045-T03 |
| [`0x001395d0`](../../reverse/functions/sha256-815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b/driving/001395d0.md) | `SetFrictionBoost__10PhysicsCarf` | E-0045-T03 |
| [`0x00151cc0`](../../reverse/functions/sha256-815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b/driving/00151cc0.md) | `TopologyCoordinate::GetElevation(bVector3 const *, TerrainType *, bVector3 *, bool *)` | E-0045-T02 |
| [`0x001530a8`](../../reverse/functions/sha256-815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b/driving/001530a8.md) | `GetRoadNoiseGridValue__Fii` | E-0045-T02 |
| [`0x001530e8`](../../reverse/functions/sha256-815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b/driving/001530e8.md) | `GetRoadNoise__FP8bVector211TerrainType` | E-0045-T02 |
| [`0x001a16e8`](../../reverse/functions/sha256-815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b/driving/001a16e8.md) | `Wheel::Wheel(WheelInfo *, TireInfo *, ControlInfo *, CarState *)` | E-0045-T03 |
| [`0x001a1948`](../../reverse/functions/sha256-815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b/driving/001a1948.md) | `Wheel::SetState(CarState *, CarTypeInfo *, float, float, unsigned short, float)` | E-0045-T02 |
| [`0x001a1b30`](../../reverse/functions/sha256-815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b/driving/001a1b30.md) | `Wheel::ComputeAuxiliaries(void)` | E-0045-T02 |
| [`0x001a1c80`](../../reverse/functions/sha256-815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b/driving/001a1c80.md) | `Wheel::ExchangeZForces(int, float)` | E-0045-T02 |
| [`0x001a1d58`](../../reverse/functions/sha256-815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b/driving/001a1d58.md) | `FindDrivingSurface__5WheelP8bVector3P8bMatrix4` | E-0045-T02 |
| [`0x001a1d80`](../../reverse/functions/sha256-815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b/driving/001a1d80.md) | `CheckTerrain__5WheelP8bVector3P11TerrainTypeT1P8bMatrix4` | E-0045-T02 |
| [`0x001a1e88`](../../reverse/functions/sha256-815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b/driving/001a1e88.md) | `Wheel::GetDerivative(float *)` | E-0045-T02 |
| [`0x001a1f50`](../../reverse/functions/sha256-815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b/driving/001a1f50.md) | `Wheel::CheckForBrakeLock(float)` | E-0045-T02 |
| [`0x001a1ff0`](../../reverse/functions/sha256-815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b/driving/001a1ff0.md) | `Wheel::GetPilotFactor(void)` | E-0045-T02 |
| [`0x001a2108`](../../reverse/functions/sha256-815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b/driving/001a2108.md) | `Wheel::GuessGroundForce(bVector2 *, int)` | E-0045-T02 |
| [`0x001a2500`](../../reverse/functions/sha256-815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b/driving/001a2500.md) | `Wheel::GetLateralForce(float, float)` | E-0045-T02 |
| [`0x001a2660`](../../reverse/functions/sha256-815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b/driving/001a2660.md) | `Wheel::ModifyGroundForce(bVector2 *)` | E-0045-T02 |
| [`0x001a26b8`](../../reverse/functions/sha256-815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b/driving/001a26b8.md) | `Wheel::FinalizeGroundForce(int)` | E-0045-T02 |
| [`0x001a3368`](../../reverse/functions/sha256-815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b/driving/001a3368.md) | `Wheel::SetCatchupFrictionMultiplier(float)` | E-0045-T03 |
| [`0x001a3370`](../../reverse/functions/sha256-815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b/driving/001a3370.md) | `Wheel::AdjustTractionBoost(float)` | E-0045-T03 |
| [`0x001a3380`](../../reverse/functions/sha256-815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b/driving/001a3380.md) | `Wheel::SetTractionBoost(float)` | E-0045-T03 |
| [`0x001a37a0`](../../reverse/functions/sha256-815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b/driving/001a37a0.md) | `ComputeAuxiliaries__10Suspension` | E-0045-T02 |
| [`0x001a3b10`](../../reverse/functions/sha256-815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b/driving/001a3b10.md) | `Suspension::ResolveForces(float)` | E-0045-T02 |
| [`0x001a3b50`](../../reverse/functions/sha256-815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b/driving/001a3b50.md) | `Suspension::ResolveMoreForces(void)` | E-0045-T02 |
