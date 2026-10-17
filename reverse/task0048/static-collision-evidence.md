# TASK-0048 static track collision orchestration

## E-0048-T06 â€” original single-car/static-topology branch

Authority: Debug `NFSHP2.ELF`, SHA-256 `815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`. Status: Reconstructed, not Verified. Read-only Ghidra decompilation/listing; no database mutation. Geometry/VClip evidence: [T03..T05](collision-geometry-evidence.md); Contact/impulse evidence: driving worker E-0048-D04.

Read-only entries:

| Address | Original function | Implemented scope |
|---|---|---|
| 001D3D90 | DynamicCollisionBody::BuildBoundingVolumes | Original vertex XY radius, Z extrema and cylinder padding |
| 001D3FA0 | DynamicCollisionBody::BeginMove | Previous position/rotation/linear velocity and movement interval |
| 001D4040 | DynamicCollisionBody::Interpolate | Original componentwise position/rotation interpolation; no normalization |
| 001D41E8 | DynamicCollisionBody::InterpolateVelocity | Original linear velocity interpolation |
| 001D4298 | DynamicCollisionBody::Backup | Restores X/R/P only; angular momentum remains current |
| 001D4348 | DynamicCollisionBody::RecomputeAuxilliaries | Canonical PhysicsCar system callback |
| 001D3F28 | DynamicCollisionBody::UpdatePosition | XY bbox uses current center +/- cylinder radius |
| 001D46D8 | DynamicCollisionBody::CreateTrackPolygonCollisionBodies | Original cylinder predicate, at most48 new polygons, packed-address creation order |
| 001D3788 | CollisionBodyFlipFunction | Static-body neighbor order by BBoxMax.x then original allocation ID |
| 001D3230 | CollisionBodyWitness::TestForContact | Persistent original VClip cache; contacts only on VC_PENETRATION |
| 001D2AC8 | CollisionBodyWitness constructor | Actual dynamic-car first/static-track second order retained |
| 001D13A0 | ContactTimeSorting | Descending backup time; stable tie ordering corroborated by bList::AddSorted002498C0 |
| 0014D630 | World::DoTimestepCollisions | Original single-active-car resolve/remove/recheck loop, cap20 |

## Confirmed behavior and adapter boundaries

- Static collision geometry is read from the same original car `GEOMETRY.BIN` bytes as the viewer through `NativeBackend::collision_geometry`. The bridge performs configured-root file access; the physics core neither hardcodes an asset path nor guesses a working directory. Missing file/input is explicit. A valid file without the requested `_CV` hash follows the original rectangular fallback branch.
- Cylinder padding Debug00325AD8 is0.05; interpolation short-interval bounds00325ADC/AE0 are1e-6 (memory read `cdcc4c3dbd378635bd378635`). Extrema start at zero, as in the original.
- A port-owned map holds immutable decoded wafers and original feature-code witness caches. New candidate polygons retain the original48-body cap and original packed-address creation order, with static neighbors visited in recovered BBoxMax.x/allocation order. A separate vector preserves witness insertion order across frames.
- The active car is already in the recovered PhysicsCar mode. Static bodies have the original immutable identity pose. No dynamic smokeable, other car, player damage/reaction or AI behavior is silently substituted.
- `World::DoTimestepCollisions` lives in the original class/translation unit. Port runtime callbacks supply ownership and list operations; `Contact::Resolve` owns original backup/crowbar/recompute/impulse/recompute/remaining-move order. Recompute enters the canonical full PhysicsCar system. Remaining movement calls original PhysicsCar::Move, after a new BeginMove backup.
- Backup interpolation changes rigid-body position, rotation and linear momentum only. It never restores the whole Memory image or angular momentum. Original cached CP1/CP2 are retained after FindContactTime/Crowbar; no extra VClip call is inserted before ContactPoint transformation.
- The actual original car-first/wafer-second branch is used. Crowbar services represent actual car displacement; original Resolve subtracts the car-first CrowbarVector, so the adapter's vector direction is explicitly normalized at that boundary.
- Unsupported Retail material semantics are validated only after a real VClip penetration is detected, before Contact/impulse mutation. Nearby unsupported material geometry remains inspectable. No water/puddle/CAR material is silently treated as a generic solid.
- Runtime counters and per-polygon impulse counts are port-side read-only diagnostics. `NativeBackend::advance` copies the collision runtime alongside candidate Memory and commits both only after a valid publication. Failed steps preserve the prior witness state, counters, skid arena and published snapshot.

## Lifetime limits

This is the single active car/immutable track-polygon branch. Original slot-pool allocation IDs, Caffeine plotting, global collision statistics and `Collision` visual/audio lifetime are not host ABI claims. Host witness eviction uses the recovered overlapping-XY condition; the full multi-body active-Collision lifetime/expiry state machine is outside this branch. Non-topology smokeables/destructible object lifecycle and inter-car collisions remain explicit gaps. No Godot physics, position clamp, sphere bounce or additional grip model is used.

## Validation

- Source syntax checks passed with MinGW GCC13.2, C++17, without writing shared build artifacts.
- Root rebuilt the production targets and ran `static_collision_tests`: original911CV, native initialized RigidBody, topology wall wafer, VClip, backup, original Contact impulse and rigid momentum passed. Full NativeBackend throttle-to20m-wall scenario passed with an actual opposing impulse and changed original velocity.
- Root ran true-track regression:30 original track starts,120 settling steps, real material samples and recording/replay passed with the collision chain installed.
- Root rebuilt and ran the final `static_collision_tests`: `FULL_NATIVE_WALL_IMPACT_OK impulses=1`, `FULL_NATIVE_CORNER_IMPACT_OK`, and `NATIVE_COLLISION_ROLLBACK_OK` all passed. The corner requires distinct impulses against both wall polygons. Failure/retry verifies unsupported detected material cannot commit body position/witness/counter changes; after fixture repair the retried body/velocity exactly match a clean control.
- These are PC checks, not original hardware/PCSX2 comparisons. No function is marked Verified.


## E-0048-T07 — bounded scenery collision scope

Read-only Debug `CollisionBodyType` enumerates CAR0, SMOKEABLE1, VIRGIN_SMOKEABLE2, TRACK_POLYGON3. No separate `SceneryCollisionBody` class was found in the bounded function-name search; this does not establish a global negative claim.

`LoaderSmokeables00219C90` handles34026 SmokeableInfo224-byte rows and34027 eight-byte spawners. `SpawnSmokeables0021A4E8` and `VirginSmokeable0021A9F8` tie the initial static body to ScenerySectionHeader/SceneryInstance. `Smokeable::IsMoveable0021C828` returns AnchorStrength==0. `ConsiderImpulse0021BC48` compares impulse/moment against AnchorStrength; on a sufficient impulse it calls Awaken, clears AnchorStrength and activates anchor emitters, then updates BreakingStrength. Therefore an initially immovable anchored scenery object belongs to the separate movable/breakaway lifecycle, not the immutable track-wall branch.

A byte-level scan of the30 loaded Retail TRACKB bundles using the corroborated224-byte row stride found560 SmokeableInfo rows, AnchorStrength in0..10, no negative anchor values. Examples include light posts, construction barriers, traffic cones, pumps and signs. This corroborates the distinct lifecycle; it does not substitute guessed collision for those objects.

The requested plan explicitly excludes destructible objects and dynamic track interactions. Immutable road/wall/obstacle polygons use the implemented chain. Separate anchored/movable Smokeable scenery remains an explicitly excluded lifecycle even while initially at rest. Rendered scenery is never given an invented generic collision body in this port.
