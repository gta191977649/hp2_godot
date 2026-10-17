# TASK-0049: topology contact debug geometry

## E-0049-G01 — port-owned rendering projection

This is a diagnostic visualization helper, not a recovered original renderer, material palette or physics implementation. Files: `src/game_core/port/track/debug_geometry.hpp` and `src/tests/track_debug_geometry_tests.cpp`. Existing audited topology, driving and collision bodies are unchanged.

Original eligibility and height authority remains Retail `SLUS_203.62` SHA-256 `7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`, `TopologyCoordinate::GetElevation00152070` and `SetInterestBBox00151D30`; E-0048-T01. The current recovered implementation was inspected directly before this addition.

Confirmed original query behavior:

- Candidate partitions exclude polygon flags2 and8. Flags1 is not a road exclusion; both ordinary and flagged1 roads remain eligible.
- The selected height is `(plane.W - (plane.X * x + plane.Y * y)) / plane.Z` over the polygon's original XY footprint. A zero plane.Z cannot produce a finite selectable elevation. The debug helper excludes exactly zero, with no slope epsilon or heuristic normal cutoff.
- Each stored polygon remains independently addressable; overlapping road layers must not be flattened into a single height field. Actual layer selection is still performed by the original query, not by the debug renderer.

## Why the diagnostic faces use the stored contact plane

A byte-level scan of ignored `local/task0048/trackb11.bin`, decoded from the player's original TRACKB11.LZC, used the established32-byte polygon layout: flags+3, commonZ+4, fourX+8, fourY+16, four relativeZ+24 (E-0048-T01). Among quad road candidates,5277 fourth vertices differ from the first-three-vertex plane by more than1e-5 vertically; the maximum mathematical deviation in that scan was1.4765625m.

Raw v4 triangulation would therefore show a different height from the existing tire-contact query. The diagnostic helper preserves original XY perimeter coordinates and polygon indices, and derives each output Z using the same stored-plane height expression. This is an explicitly port-side display projection. The physical topology, its stored fourth vertex and its query remain untouched.

## Public helper contract

`port::track::build_debug_geometry(const Topology&, const std::vector<port::driving::SurfaceMaterial>&)` returns:

- `triangles`: nativeXYZ vertex arrays3, original polygon index, raw terrain ID, RGB color.
- `edges`: nativeXYZ vertex arrays2, original polygon index, raw terrain ID, RGB color. Only polygon perimeter edges are emitted; the quad triangulation diagonal is omitted. Shared boundaries retain both original polygon perimeters.
- `legend`: encountered IDs in ascending order, exact caller-supplied catalogue name, RGB color, known flag and eligible polygon count.
- `skipped_polygons`: original flags2/8 and exact zero-Z planes omitted from this road-contact visualization.

The helper has no Godot headers, performs no axis conversion or vertical rendering offset, and does not require topology spatial-tree ownership for pure geometry inspection. The caller handles host coordinates and caches the output.

Catalogue IDs are restricted to the established33-row Debug table; duplicate/out-of-range catalogue entries reject explicitly. Polygon terrain IDs without a nonempty supplied name remain visible as magenta with a port-side `Unknown` label. No original terrain name is guessed. Malformed polygon counts, nonfinite coordinates/planes and nonfinite projected heights reject explicitly.

## Tool palette

The five existing Test drive colors for terrain IDs1,2,6,7,8 are reused directly from `surface_colors`/`surface_terrain_ids`. Other catalogue-known IDs receive deterministic diagnostic HSV hues by numeric ID. These colors carry no claim about original texture colors, friction, terrain families or material equivalence. Unknown IDs use exact RGB(1,0,1).

## Validation

MinGW GCC13.2 C++17 source syntax check passed. Private isolated compilation/run passed using the root-built current game_core archive:

`C:/Strawberry/c/bin/c++.exe -std=c++17 -I src src/tests/track_debug_geometry_tests.cpp build/godot-debug/libgame_core.a -o local/task0049/debug_geometry_private.exe`

`local/task0049/debug_geometry_private.exe` returned `TRACK_DEBUG_GEOMETRY_OK`.

All30 ignored uncompressed TRACKB bundles also passed the optional sweep:12196..32147 triangles and30232..75431 perimeter edges per track. Exact private results are in ignored `local/task0049/debug-geometry-results.txt`. No shared build directory was mutated by this worker. Root also owns the CMake target/build integration.

The stacked-road test uses flags1 on both overlapping candidates, preserving the original scan/max-height branch. Ordinary strictly interior flags0 roads can return early by original candidate order; this visualization does not rewrite that behavior.

Meaningful unit scenarios cover triangle/quad eligibility, flags1/2/8, zero-Z exclusion, sloped and overlapping roads compared with original `Topology::elevation`, adjacent material boundaries, off-plane stored v4 projection, topology immutability, perimeter-only edges, exact catalogue labels, missing/unknown materials, the shared five-color palette and malformed inputs. Optional executable arguments accept complete uncompressed original bundles for the30-track asset sweep.

No proprietary bundle, decoded image or Ghidra database was added to tracked paths. Passing these checks does not establish original-game rendering equivalence.
