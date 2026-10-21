# Track culling and generated mesh LOD investigation (TASK-0057)

## Original Retail mechanism: E0057R01

Authority: Retail SLUS_203.62 SHA256
7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
Paired original owner/name evidence remains E-0051-S01, reverse/task0051/scenery.md.
Fresh read-only Ghidra decompilation on2026-10-19 saved privately under
local/task0057/{00150950,00150378,0014FE18}.json. No Ghidra mutation.

1. GrandSceneryCullInfo::WhatSectionsShouldWeDraw00150950 calls FindVisibleZone
   using view position. Inside a zone it selects loaded sections90..99 plus that
   zone's stored visible-section list; without a zone it falls back to loaded
   sections below100, bounded by output capacity. A fallback can be broad.
2. ScenerySectionHeader::TreeCull00150378 traverses the stored24-byte spatial
   nodes using64-entry LIFO state. ClassifyAABB0010CCB0 rejects outside nodes
   before visiting descendants; fully inside classification propagates to children.
   Thus rejected subtrees never submit their instances.
3. ScenerySectionHeader::DrawAScenery0014FE18 tests remaining instance AABBs,
   exclusion flags, camera-forward/radius rejection, projected size and three
   model slots. With R=Radius+4 and d=distance(view,instance)-R, projected size
   is H nearby or R*H/d farther away, converted to integer. Size<2 draws nothing.
   First nonnull model whose FarClipSize threshold qualifies is selected; a
   single-model instance still has a cutoff, not a fabricated lower model.
4. Selected geometry goes through StuffScenery/eModel and original strip queues
   (prior E0051 material/ordering records). Resource residence/upload is different
   from per-frame selection and actual GPU draws.

These are confirmed bounded mechanisms, not proof that all original optimization,
streaming, occlusion, distance initialization or PS2 runtime timing is recovered.
Do not infer hardware occlusion or a single fixed-meter global distance from them.
The visual result still needs original runtime comparison; status Reconstructed.

## Current port distinction

Original backend invokes WhatSectionsShouldWeDraw -> TreeCull -> DrawAScenery.
Experimental godot_hlod invokes section selection, expands all authored levels,
and lets engine visibility ranges/frustum culling select GPU draws. It bypasses
native per-instance/tree selection in frame generation. Resident node count is
not actual draw count, but all-level CPU preparation can itself be expensive.
Show LOD intentionally disables level distance ranges; turn it off for ordinary
rendering/performance assessment. Current persistent resources do not establish
that the GPU renders every section simultaneously.

Recommended first optimization: retain original section/zone and hierarchical
candidate rejection, cache unchanged submissions, then measure candidate instances,
selected levels, submitted surfaces, actual engine draws/triangles, CPU and GPU.
Do not blindly apply main-camera culling to independent reflection capture views.
A candidate-tree optimization must preserve anchors/bounds for all authored levels;
reintroducing original final model selection would defeat the authorized HLOD mode.

## Godot4.4.1 feasibility: E0057G01

Official version-pinned sources:
- https://github.com/godotengine/godot/blob/4.4.1-stable/scene/resources/3d/importer_mesh.cpp
- https://github.com/godotengine/godot/blob/4.4.1-stable/modules/meshoptimizer/register_types.cpp
- https://github.com/godotengine/godot/blob/4.4.1-stable/modules/meshoptimizer/config.py
- https://github.com/godotengine/godot/blob/4.4.1-stable/drivers/gles3/rasterizer_scene_gles3.cpp
- https://docs.godotengine.org/en/4.4/tutorials/3d/mesh_lod.html
- https://docs.godotengine.org/en/4.4/classes/class_arraymesh.html

ImporterMesh.generate_lods(merge_angle,split_angle,bone_array) binds the generator;
4.4.1 split_angle binding is compatibility-only. It creates per-surface reduced
index buffers over existing vertices. ArrayMesh.add_surface_from_arrays accepts
these LOD dictionaries; GLES3 selection explicitly consumes surface LODs using
view screen threshold/projection/distance. This is mesh LOD, distinct from HLOD
whole-model visibility ranges. No extra mesh instance per generated level needed.
Module registration is scene-level, not editor-only; stock module config permits
release. Custom exported builds omitting meshoptimizer need capability checking.
Prefer local preprocessing/cache, not generating during driving or every frame.
No OBJ/glTF conversion or modern replacement shader is required.

Source constraints: generator merges by positions, UV/UV2, normal proximity and
tangent sign, locks borders, and feeds normals as simplification attributes.
It does not include HP2 COLOR/CUSTOM0 in its weld criteria/error metric. Our
original normals are raw CUSTOM0 (not standard NORMAL) and raw vertex colors
carry original lighting/alpha. Passing them through unchanged does not guarantee
that remapped indices/interpolation preserve shading, masks or seams. Do not weld
vertices across distinct colors/custom attributes merely to increase reduction.

## One-track probe: E0057G02

Stock engine4.4.1 official49a5bc7b6, headless, no performance claim. Synthetic
subdivided PlaneMesh reference generated1LOD, proving generator callback exists.
Track11 first24 qualifying visible RD_SECTION/TRN_SECTION uploaded batches:
5 generated lower levels, all named TRN_SECTION. Others (including road samples)
produced no lower level. Summed4017 base triangles vs3717 using coarsest available
indices (about7.5% reduction). These are batches, not unique source model totals.
Original Track11 start inspection frame prepared18430 triangles and331 resident
surface groups; headless actual draw counts are not meaningful GPU evidence.
No geometry was replaced in the running renderer. Raw COLOR/CUSTOM0 arrays survived
generation, but pixel equivalence, boundary cracks, safe welding and FPS remain
untested. Quantitative host results: reverse/task0057/generated-lod-probe-track11.json.
Small/strip-like geometry and locked boundaries are possible reduction constraints;
the probe does not prove one sole cause for every unchanged surface.

Reproduce from repository root (private original asset configuration required):

```powershell
cmake --build build/godot-debug --target static_lods_tests -j 6
build/godot-debug/static_lods_tests.exe --assets 11 --lod-inventory > local/task0056/lod-inventory-track11.log
local/tools/godot-4.4.1/Godot_v4.4.1-stable_win64_console.exe --headless --path godot --script reverse/scripts/task0057_generated_lod_probe.gd
```

Use an absolute --script path if the engine resolves relative scripts against
project rather than shell root. Outputs local/task0057/generator-results.json.

## Recommended bounded implementation

Keep original authored LODs for vegetation. For single-model opaque terrain and
static scenery, build optional generated mesh LOD during local cache preparation;
retain LOD0 byte data and shaders. Engine chooses index level per viewport. Preserve
section/spatial ownership, material/pass boundaries, custom attributes, UV seams,
shared section-border positions and dependency meshes. No whole-track merge.
Cache keys include asset/content hash, engine/generator version, mesh transport,
settings and eligibility rules; no proprietary cache in Git.

Initial exclusions: alpha-cutout/facades, roads at curbs/material transitions,
tunnel/water overlay/decal surfaces and coupled effect geometry until constraints
are demonstrated. Base/depth/reflection/extra-pass geometry must share compatible
mesh/index LOD, transforms, bounds, bias and viewport threshold. Simplifying an
underlying road independently from its overlay can produce gaps/z-fighting.
Collision/TopologyTree/vehicle physics continue using original data at all distances.

Single-track gate: inspect near/far wireframe, section seams, COLOR/alpha shading,
base-depth agreement, tunnel/water masks and independent captures. Compare actual
triangles, draw calls, CPU/GPU/frame time with Show LOD off and equal camera/effects.
Generated mesh LOD mainly reduces geometry; it does not inherently reduce draw calls,
per-instance CPU work, transparency overdraw or reflection-view cost. No promised
FPS uplift. Safe source-body preservation and explicit opt-in before rollout.
Default rendering has not changed in this investigation.
