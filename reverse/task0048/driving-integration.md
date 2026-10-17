# TASK-0048 driving integration evidence

All Ghidra access was read-only. Retail `SLUS_203.62` SHA-256
`7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`
owns the shipped geometry; Debug `NFSHP2.ELF` SHA-256
`815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`
owns vehicle algorithms under the existing TASK-0044 exception. Nothing here is Verified.

## E-0048-D01: explicit material bridge

Retail `GetRoadNoise00153498` accesses spacing and scale at `318c18/318c1c + Type*90`.
Ghidra memory reads independently recovered all 33 Retail table rows at `318c00`,
their embedded Type words and pointed-to Name strings. Compare with the independently
recorded Debug table `3102e8`, stride90, from E-0045-T01. Every index, Name and five
noise/friction scalar bit patterns agrees. [terrain-mapping.json](terrain-mapping.json)
records these observations. The adapter therefore maps each semantic Type index
explicitly; it never casts a Retail object to a Debug structure. The wheel's existing
Debug terrain table continues to own grip/noise/friction calculation.

PUDDLE4, DEEP_WATER5, MUD_PUDDLE20 and CAR32 raise a diagnostic at the adapter.
Their semantic IDs are known; this exclusion reflects missing special behavior,
not an unknown number. All 33 raw Debug material rows remain available for inspection.

## E-0048-D02: true starting marker and path direction

Debug `CalculateRacerPositions00149e80` chooses the hash of
`ForwardStartingPosition` or `BackwardStartingPosition`, finds a marker with Param equal
to the racer index, copies XY, then queries TopologyCoordinate with initial Z0.
Only forward racer0 is exposed by this tool. Missing markers fail; the original
TrackPathCoordinate distance100/lateral3 fallback is deliberately not substituted.

Retail `LoaderTrackGeometry00153608` and Debug `00153258` both load marker chunk34146
with stride0x18. Debug imported TrackPositionMarker fields are NameHash8, Paramc,
Position10. Existing bStringHash semantics are seedffffffff, multiply33, add byte.

Retail `TrackPathManager::Loader00207b28` and Debug `002029c0` both read container
80034147: first child payload has 0x28-byte TrackPath rows, next has0x14-byte TrackPathPoint
rows. Debug imported fields establish BeginPointNumber2, EndPointNumber4, NumPoints6,
Looping1; point Position0, PathNumber8, NeighbourNumbersa. The port parser checks all
byte spans, row strides, index ranges and point counts before crossing this boundary.

Debug `AddRacingCar00149d78` constructs a TrackPathCoordinate from spawn XY and calls
existing recovered `bATan(PathDirection.x, PathDirection.y)` for the original bAngle.
`TrackPathManager::Init00202780` builds OVERHAUSER splines from point XY with LOOP or
EXTRAPOLATED endpoints. First-position `UpdatePosition00202f30` calls closest point,
same-path adjacent selection, then the three neighbour slots, computes spline parameter
and normalized first derivative. The third-slot traversal reads the current candidate,
so a successful neighbour selection changes the base for the next slot.

The bounded original-class overload implements this first-position heading projection.
It does not claim persistent coordinate caching, distance/ratio publication, reverse-track
behavior, CalibrateLength or full path-manager lifecycle. QuickSpline methods stay in
the original quickspline.cpp class, with port-owned XY storage. Original endpoint
expansion, basis coefficients (InitBasisMatricies00236b10), parameter wrapping
(bFMod00253898), GetPoint00237448 and FindClosestParam00237748 are the sources.
Debug tolerance `325da0` has raw bits3d4ccccd. The closest-param loop retains limit1,
iteration count3e6, secant update, tolerance early exit and sign-change limit halving.

## E-0048-D03: original flat placement

Debug Car constructor00130b90 assigns spawn position/angle then calls PutFlatOnTrack001329c0.
Its FindFlatOnTrackPosition00132788 probes XY offsets `(0,0),(1,0),(-1,0),(0,1),(0,-1)`.
Slope angle calculations multiply differences by0.5 and original scalar324d14
(raw bits46230eac). Z uses terrain center height + wheel3 radius - wheel3 localZ + CGz.
It then composes original eRotateZ(angle), eRotateX(quantizedY), eRotateY(-quantizedX).
The recovered Car member is now called by the native fixture on real-track startup;
existing bATan/eSin/eRotateZ/eRotateY supply original angle/table behavior.
New eRotateX0011bb80 uses original constants324b3c/324b40 and eMulMatrix.
This restores the placement calculation, not the unrelated sound/damage/race constructor.

## Port boundaries and remaining gaps

`TrackEnvironment` owns an immutable topology and spawn. Session changes reset native
state, clear input/time/surface recordings and retain the chosen track across resets.
NativeBackend selects a ContactProvider for all four suspension wheels and skid probes;
successful-step publications report the same terrain IDs and original wheel grip fields.
Synthetic test surfaces still use PlaneContact and remain independently selectable.

Track polygons supply road height/normal/material. The bounded native single-car/static-track
collision chain now uses original collision-volume selection, topology broad phase, wafer/
Voronoi witnesses, Contact time/crowbar/Resolve and scaled original car/rigid-body impulse.
See [collision-geometry-evidence.md](collision-geometry-evidence.md) and
[collision-impulse.md](collision-impulse.md), with runtime closure in
[static-collision-evidence.md](static-collision-evidence.md). The temporary port-owned body-center segment
guard was removed when this chain was connected; its earlier diagnostic-only status does
not describe the final runtime. Arbitrary static scenery-instance volumes, dynamic objects,
damage/reaction/player/police consumers and full original allocator ownership remain bounded
out. Unsupported water/puddle/CAR and non-null excluded effect dependencies fail visibly.

Function response hashes and statuses are in [driving-functions.json](driving-functions.json).
Raw decompilations stay ignored under local/task0048/driving. Tests are native acceptance,
not original numerical comparison; root task validation records exact build/run results.

## Native validation status

Root's native `driving_track_tests` runs passed all 30 shipped forward marker/path/topology
starts (`loaded=30 blocked=0`), 120 native settling steps with all four wheel contacts,
three original polygon material samples, 15-step real-track input replay equality and clear
back to the synthetic scenario. `static_collision_tests` subsequently passed a complete
NativeBackend drive into a wall 20 native units away with original contact impulse applied.
The root reported seven targeted checks passed, including the track viewer host, and actual
GL drive capture with original track geometry/textures. Later root runs also passed `FULL_NATIVE_CORNER_IMPACT_OK` and
`NATIVE_COLLISION_ROLLBACK_OK`, plus the unsupported-terrain guard fixtures. Exact commands
and broader integrated results remain in the root task.
Standalone GCC13.2 `-std=c++17 -Isrc -fsyntax-only` checks passed for the new native
adapter, original spline/path/Car placement/eRotateX, contact and test source. All results
remain port-side acceptance, not Verified or original numerical certification.
