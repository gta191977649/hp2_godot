# TASK-0048 collision geometry and Voronoi recovery

Debug authority: `NFSHP2.ELF`, SHA-256 `815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`. Status: Reconstructed, not Verified. Ghidra was accessed read-only through `tools/ghidra.py`; raw responses remain ignored under `local/task0048`.

## E-0048-T03: wafer and rectangular polyhedra

Read-only decompilation: `WaferPolyhedron::Init002132C8`, `RectPolyhedron` constructor `00212EB0`, `VCPolyhedron::NewVertex001D02D8`, `NewEdge001D0340`, `NewFace001D03E8`, `SetVoronoiPlanes001D0990`, `VCVertex::SetVoronoiPlanes001CF6A8`, `VCEdge::SetVoronoiPlanes001CF8F0`, `VCFace::SetVoronoiPlanes001CFC68`, `VCVertex::GetNextEdgeInFace001CF7F8`, `bEqual00251E28`.

Confirmed:

- The exact original header `../indep/inc/Polyhedra.hpp` occurs in Debug decompilation inline provenance. The host companion definition is placed in that header; no unknown original `.cpp` location is asserted.
- Wafer triangle selection compares vertices2/3 componentwise with float0.001 (Debug00325EEC `6f12833a`); otherwise four vertices. The shape has two opposite faces and one edge per unique vertex. All original adjacency tables are retained.
- Ghidra `memory/read` Debug003135D0 length384 returns original Rect adjacency (`RectEdgesNearFaces`, `RectEdgesNearVertices`, `RectFacesNearEdges`, `RectVerticesNearEdges`). Debug00313750 length192 returns original quad and triangle Wafer adjacency.
- Edge direction compression truncates normalized direction*8192 to signed16; plane construction reads direction/8192, preserving original quantization. Vertex and face positions/normals remain float memory-image values.
- Feature-plane construction order is vertices, faces, edges. Face edge-plane orientation uses the original neighboring-edge lookup and opposite vertex sign test. Edge planes0/1 are endpoint planes; planes2/3 reverse adjacent face Voronoi normals.
- Rect construction preserves original bit-based eight-vertex order, twelve edge and six face adjacencies. It is available only as the original constructor binding, never silently substituted for an existing pipeline shape.

## E-0048-T04: original car collision volumes

Read-only decompilation: `LoaderCollisionVolumes00213A30`, `VCPolyhedron::MemoryImageLoad001D01C8`, `ConvertToPointers001D0D10`, `GetFeature001D0B08`, `FindPipelinePolyhedron00213C98`, `PipelinePolyhedron::MoveCenterOfGravity00213D00`, `CarCollisionBody` constructor `001D4A80`.

Confirmed:

- `LoaderCollisionVolumes` handles container80034020, five subchunks per volume: header, vertices, edges, faces, planes. Each record section is 16-byte aligned relative to its original header. Strides are20,12,24,24. The original header is96 bytes, name hash+48, name[32]+52, COG-fixed+84.
- Plane references in features are plane indices. Plane neighbors are feature codes: low nibble0vertex/1edge/2face, index in upper bits; FFFFFFFF is null. No raw target pointer is reinterpreted on the host.
- The car constructor searches `BaseModelName + "_CV"` (Debug00328420), adjusts its center of gravity once and uses original Rect fallback only if the hash lookup actually returns null.
- The host loader decodes existing stored adjacency, then runs the recovered canonical plane constructors. `FindPipelinePolyhedron` uses first matching hash and only applies COG shift once. Host vectors replace PS2 linked lists/allocation ownership.
- Retail asset bytes corroborate Debug layout: `CARS/911TURBO/GEOMETRY.BIN` has container at3D0, header3D8, aligned data3E0, exact name `911TURBO_CV` and coherent36vertices/102edges/68faces. This is an explicit cross-version format adapter; no broad Debug/Retail ABI compatibility is implied.

## E-0048-T05: original VClip feature walk

Read-only decompilation: `VCTransform001CCF28`, transformed vertex001CD0C8 and edge001CD138; `VCVoronoiClip::ClipPlanes001CD220`, `ClipVertexPlanes001CD388`, `ClipFacePlanes001CD590`; `VCWitness` constructor001CD860, `CheckVertexVertex001CDA08`, `CheckVertexEdge001CDD00`, `CheckVertexFace001CE038`, `SubCheckEdgeEdge001CE5F8`, `CheckEdgeEdge001CE6D8`, `CheckEdgeFace001CEC10`, `VClip001CF360`.

Confirmed:

- Relative transforms use transpose of end rotation, multiplied by start rotation and translated by start position minus end position. The host pose supplies the original three native basis columns and separate position; it does not assert a general nonorthonormal affine matrix API.
- Original vertex/edge/face pairing and SwitchState are preserved; witness feature codes persist between calls. CP1/CP2 are local to their respective shapes. Return codes0CONTINUE,1DISJOINT,2PENETRATION come from the Debug `VCStatus` enum.
- Original clipping order, neighbor updates, strict signs, endpoint choices, closest-edge direction quantization and80-iteration cap are preserved. An iteration-cap return remains original CONTINUE, never fabricated penetration.
- Memory00325A84 length20: `acc52737acc5273717b7d13817b7d138acc52737` establishes squared-distance1e-5, face-distance1e-4 and VV-distance1e-5 thresholds. Memory00325A98 establishes VF1e-4 and parallel-edge1e-6 thresholds.
- PS2 decompiler masks can be misleading: listings are the authority where a vector appears to subtract itself. For example Debug001D1630 is `vsub.w vf2,vf2,vf2`, clearing W only. The same fMath conversion idiom must retain XYZ in plane tests.

## Original wafer normal continuation

`FindWaferNormal001D15C0` listing001D15C0..001D1690 confirms: start with wafer face0 normal; negate if dot(normal, dynamic-body world position) <= face0 distance - WaferBias. `WaferBias` Debug003283A0 is0.5 (`0000003f`). Static track wafer center is zero in `TrackPolygonCollisionBody001D53A0`. Contact impulse/time/normal integration is owned by the driving recovery slice, not silently approximated in this geometry module.

## Validation and boundary

Isolated private executable compilation (no shared build directory):

`C:/Strawberry/c/bin/c++.exe -std=c++17 -I src src/tests/track_collision_geometry_tests.cpp src/game_core/indep/src/voronoiclip.cpp src/game_core/port/assets/elab_chunks.cpp -o local/task0048/collision_geometry_tests.exe`

PASS synthetic quad/triangle wafer adjacency, quantization, Rect vertex order, finite planes, one-time COG correction, missing hash, malformed record stride; convex box separated/penetrating/persistent witness; box against wafer separated/penetrating.

PASS all43 original `CARS/*/GEOMETRY.BIN` files:29 decode a collision volume (28 standard car shapes36/102/68 and BUS8/18/12),14 contain none. Every decoded shape is tested above an original wafer and intersecting it. These are port checks, not original runtime equivalence. Raw results remain ignored in `local/task0048/vclip-results.txt`. No original assets or Ghidra databases were added to Git.

Dynamic damage/reaction, non-track smokeable object lifecycle, Caffeine visual plotting and full original slot-pool memory ownership are outside these host geometry bindings. Unknown legacy no-argument entrypoints remain throwing. Contact time/impulse integration has separate evidence and must pass its tests before wall response is claimed.
