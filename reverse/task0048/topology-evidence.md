# TASK-0048 topology evidence

Target: Retail `SLUS_203.62`, SHA-256 `7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`. Status: Reconstructed, not Verified. Read-only Ghidra queries; raw decompilation stays in ignored `local/task0048`.

## E-0048-T01: tree, packed polygon and elevation

Read-only `inspect/decompile` at Retail `001514a8`, `001517d8`, `00151d30`, `00152070`, `00152b10`, `00152c10`, `00152d30`, `00152db0`, `00152e08`, `00152f38`, `00153148`, `001531f8`, `001533c8`. Debug declarations establish class ownership/spelling; source owner is `indep/src/track.cpp` (existing E-0042-S001).

Confirmed:

- `TopologyTree::Loader` recognizes 0x34130/31/32/33: 8-byte nodes and 12-byte squares align to the next 16-byte boundary after the header; polygons have 32-byte stride; index records have 2-byte stride. Original polygon pool indices at +6 initialize to -1; port-owned decoded polygons do not require the PS2 slot pool.
- Node traversal starts at (-16384,-16384), depth 10, divides a 32768-unit square down to 32-unit cells; child 0 is absent. Leaf indices address 12-byte squares. Square +8 is index-array start, signed +10 count.
- Packed polygon: material signed byte +2, flags +3, common signed height +4, four signed X values +8, four signed Y +16, four signed relative Z +24. XY scale 1/8, relative Z scale 1/256, flags 4 scales decoded Z by four. Flags 16 indicates four vertices, otherwise three. Byte1 packs extrema vertex selectors.
- Expanded `TrackPolygon` uses normalized cross of v1-v0 and v2-v0, plane distance dot(v0,normal). Its decoded vertices and material are distinct from render geometry/materials.
- `SetInterestBBox` builds an 8-unit radius plus 0.5-unit margin for the fresh elevation query, collects at most 160 tree polygons, partitions flags 2 and 8 away from road candidates.
- `GetElevation` chooses the highest containing plane below query Z+1; if none, repeats with maximum float ceiling. Strictly interior polygons without flags1 return immediately; that ordering is preserved. The triangle path still reads the fourth packed vertex for edge 3->0, also preserved.
- `memory/read` Retail0032DE94 (Retail GP003358F0 - 0x7A5C) returned `00feff46`: 32767, the strict upper query-domain bound.
- `memory/read` Retail00330324 length8 returned `ffff7fffffff7f7f`: -FLT_MAX and FLT_MAX.

Host bindings:

- `Topology` owns vectors; original loader/unloader/tree traversal/polygon expansion and elevation computations live in the recovered declaring classes with explicit arguments for host storage.
- Every `Topology::elevation` performs a fresh bounded candidate query. Persistent original `TopologyCoordinate` interest-box and cached-fast-polygon lifetime/optimization are deliberately not modeled; no complete object ABI equivalence is claimed.
- Unknown no-argument recovered placeholders remain throwing. New typed overloads are the implemented host bindings.
- Malformed counts, range references, truncated chunks and duplicate records reject loading transactionally.

## E-0048-T02: intersection and actual static collision boundary

Read-only Retail00151990 and 00151a88; Debug001515e0,001516d8,00152500,001d46d8,001d4920,001d53a0.

Confirmed:

- `TrackPolygon::SegmentIntersects` uses strictly opposite signs of endpoint plane distances; coplanar endpoints do not intersect. It interpolates by abs(d1)/(abs(d1)+abs(d2)) then calls `PointInShaft`.
- `PointInShaft` checks each edge-relative cross product dotted with polygon normal and optionally publishes negative plane distance.
- Actual vehicle static collision creates up to 48 nearby `TrackPolygonCollisionBody` objects after `DynamicCollisionBody::TrackPolygonCheck`, sorts by original packed-polygon address, constructs a `WaferPolyhedron`, and enters the Caffeine witness/contact pipeline. `TopologyCoordinate::ConstrainToTopology` is a different positional constraint and cannot be substituted for vehicle impulse response.
- `TrackPolygonCheck` uses body cylinder axis dot polygon normal and signed center-plane distance; it selects top/base height by sign and compares absolute distance against axial support plus sqrt(1-axisDot^2)*radius.

Implemented `DynamicCollisionBody::TrackPolygonCheck` in its original translation unit (`collisionbody.cpp`), with port-owned cylinder/decoded-polygon inputs. `Topology::collision_candidates` exposes a conservative host XY search region and up to 48 sorted matching indices; this is a diagnostic candidate publication, not a recreation of body/witness lifecycle.

Unresolved: original complete collision witness/VClip/contact impulse closure and non-topology static object creation. Segment intersection is implemented as a geometry primitive, not a substitute collision response.

## Validation

`cmake --build build/native-debug --target game_core -j 4`: PASS (MinGW C++17). Standalone `track_topology_tests` linked against `libgame_core.a`: PASS. Synthetic tests cover fixed-point negative corners, expansion, material publication, shaft distance, outside point, stacked planes, strict segment crossing, transactional replacement and clear.

Decoded all 30 shipped `TRACKB*.LZC` bundles using the existing archive decoder into ignored `local/task0048`, then passed all to `topology_test.exe`. All loaded and returned road contacts at eligible road polygon centroids. Polygon counts range 12,801..29,628. TRACKB11: 16,720 polygons and 11,845 eligible centroid contacts. These are port checks, not original runtime comparisons. No proprietary files were added to tracked paths.


## Collision dependency continuation

Debug declarations and decompilation additionally inspected:

| Entry | Original owner/function | Required next dependency |
|---|---|---|
| 001D3D90 | DynamicCollisionBody::BuildBoundingVolumes | Original car VCPolyhedron vertices; original body matrix/position |
| 001D46D8 | DynamicCollisionBody::CreateTrackPolygonCollisionBodies | TrackPolygonCheck, body ID/slot ownership, TrackPolygonCollisionBody constructor |
| 001D53A0 | TrackPolygonCollisionBody constructor | WaferPolyhedron::Init, StaticCollisionBody and Caffeine transforms |
| 002132C8 | WaferPolyhedron::Init | Wafer/TriWafer adjacency tables, VCVertex/VCEdge/VCFace/VCVoronoiPlane and original quantization helpers |
| 001D3230 | CollisionBodyWitness::TestForContact | VCWitness::VClip001CF360, contact allocation and original collision timestamps |
| 001D13C8 | Contact constructor | Contact::FindContactTime, WorldLoopCounter |
| 001D1F58 | Contact::CollisionImpulse | Contact geometry, coefficient tables, witness updates and body virtual impulse functions |
| 001D4F50 | CarCollisionBody::ApplyImpulse | RigidBody::ApplyImpulseWithScaledInverseInertiaTensor, damage, collision reaction |

The existing tire solver is not a collision-body impulse solver. Broadphase and segment predicates cannot supply the missing contact time/feature selection or original car polyhedron. A wall clamp or sphere impulse would create an unevidenced replacement, so neither is installed.
