# TASK-0075 vehicle emitter positions and scheduling

Authority: Debug `NFSHP2.ELF`, SHA-256
`815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`.
Fresh Ghidra decompilation and bounded instruction listings were read-only.
Status: Decompiled, not Verified. No original-runtime comparison was performed.
Raw responses remain ignored under `local/task0075`.

## E-0075-P01: static wheel emitter positions

The symbol catalogue and fresh decompilation identify the Debug
`CarRenderInfo::CarRenderInfo` entry as `0011EA88`, not the proposed `0011E860`.
The constructor initializes the list sentinels, but the population belongs to
`Car::InitRenderInfo 0011DEA8` (fresh interior query at `0011E3C0` resolves that
entry). It allocates a `0x930` CarRenderInfo and stores it at `Car+4B0`.

`CarRenderInfo+448` is the base of the eight-byte position-list headers. Each
CarEmitterPosition node is16 bytes: Next+0, X+4, Y+8, Z+C. InitRenderInfo appends:

| Position index | Coordinate source |
|---|---|
| 0 | zero XYZ |
| 1 | `(Wheel0.Position + Wheel1.Position) * 0.5` |
| 2 | `(Wheel3.Position + Wheel2.Position) * 0.5` |
| 3 | `(Wheel0.Position + Wheel3.Position) * 0.5` |
| 4 | `(Wheel1.Position + Wheel2.Position) * 0.5` |
| 5..8 | Wheel0..3.Position XYZ, respectively |

These are static CarTypeInfo wheel coordinates. `Car+0C` points to CarTypeInfo;
its Wheels begin at+120, each WheelInfo occupies20 bytes, XYZ+0/+4/+8 and
Radius+C. Listing `0011E0D0..0011E198` corroborates list headers+470/+478/+480/+488
and loads from the four original configuration positions. There is no wheel radius
subtraction, suspension displacement or current WheelMatrix read in this slice.
Marker-based other position lists are a separate subsequent InitRenderInfo branch.

Counterevidence: taking translation from the currently rotated/spinning wheel
matrix, or moving the emitter down one tire radius, is not this original path.
The original static position is subsequently composed with car state by P02.

## E-0075-P02: TriggerEffect composition and gates

`Car::TriggerEffect 0011E4E0` requires non-null RenderInfo, nonzero intensity,
effect !=CARFX_NONE, and OverrideViewBits at Car+510 equal zero.
Its list defaults to RenderInfo+378+effect*8. For effects1..5 and non-null
pTerrainEffectEmitters, list selection instead uses
`terrain_emitters + (parameter & FFFF)*8 + (effect-1)*108`.

Effects3/4/8/9 derive override_position using arithmetic `parameter >>16`.
Override positions1..8 select `Car.State.Position` (Car+60); otherwise translation
is GeometryPosition (Car+50). BodyMatrix is Car+90; velocity defaults to Car+70,
unless the explicit override pointer is non-null. These offsets are independently
corroborated by the imported Car/CarState field catalogue and live instructions.

Per emitter, the original copies its LocalWorld matrix, selects either its
EmitterData.CarPosition or the nonzero override index, and walks that position
list in order. It writes each node's XYZ and W=1 into orientation.v3, calls
`eMulMatrix 0011B5F0(emitter_world, orientation, local_world)`, copies the result
to emitter.LocalWorld, sets InheritVelocity XYZ with W=0, then calls
`AcidEffectSystem::TriggerEmitter 00127858` with **negative intensity** and Car.
It subsequently restores the saved orientation matrix. Its saved translation has
already been overwritten by the last selected position; preserve this original
store order rather than assuming the complete prior LocalWorld is restored.

Listing anchors: entry gates `0011E520..0011E554`, terrain list `0011E558..0011E598`,
override/source selection `0011E59C..0011E620`, position override
`0011E678..0011E690`, XYZ/composition `0011E6B4..0011E6DC`, inherit/call
`0011E714..0011E730`, restoration `0011E740..0011E760`.

## E-0075-P03: world visual order

`World::DoTimestep 0014D9A8` orders movement/collision/FinishMovement before the
effects phase. It calls WorldEffects.TweakerCheck at `0014DDD4`, then
WorldEffects.DoTimeStep at `0014DDE0` (delay slot passes the simulation timestep).
Only afterward does it walk cars and query distance to the closest player.

The terrain-effect call at `0014DE44` targets
`Car::TriggerCarTerrainEffects 00133988`. Its f12 is the **closest-player
distance**, passed from f20 in delay slot `0014DE48`, not timestep. It requires
strict distance<150 (`0014DE18..0014DE2C`) and ShouldVisualEffectsBeGenerated
`00135E70` true (`0014DE34..0014DE40`). Cars retain table order. FakeEngineTask
comes after this loop; event processing and rain update follow it.

Therefore freshly triggered terrain particles enter after that tick's existing
WorldEffects update. A host adapter must not run a second particle update from
capture views or silently substitute frame elapsed time for closest-player distance.
A playerless single-car fixture needs an explicit port distance policy; it must
not claim to reconstruct the original World player lookup.

## Retained response identities

SHA-256 of raw JSON responses under `local/task0075`:

| Capture | SHA-256 |
|---|---|
| `0011ea88-positions-raw.json` | `87c70d9432585ff58d0fc0fb3225d9a2c1121072795de0954c5169ca3dc92578` |
| `0011e3c0-positions-extra.json` | `2ff1eca38dd93041b26785eb6b383959c80bbabd4fe195be25c11180700fad32` |
| `0011e0a0-positions-listing.json` | `d3695dcf425fa134c154ba349ec7e5506d9317c8b1698b68dec14ef06000d479` |
| `0011e4e0-positions-listing.json` | `47a4b28ccfa4456f7bef5fea8255d70fefb91040c46e0115a1951d889f341a61` |
| `0014d9a8-positions-raw.json` | `bab155f652aa4354e33b6419f6027bb0cf725e8a6f664571c8a999750d37176d` |
| `0014ddd0-positions-listing.json` | `d4365fda60c90e1f62df56b9af8899bd0cb52e309af360f9f12508e89224184b` |
| `0014de00-positions-listing.json` | `6a866beb5d122144cbb2672e1cd8420ffaec95f7f36622ffd7a3de4aa4913fdc` |

No production changes or numerical tests were made in this evidence-only slice.
Unresolved: full original render-info and slot-pool ownership, marker positions
outside the wheel slice, original runtime numerical/visual comparison.
