# TASK-0051 scenery runtime

## E-0051-S01 — source authority and bounded correspondence

Retail SLUS_203.62 SHA-2567335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2 remains track-format/render authority. Debug NFSHP2.ELF SHA-256815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b supplies original names, declaring owners and structure fields. Existing E-0042-S001 establishes original TU indep/src/scenery.cpp and header indep/inc/Scenery.hpp. Raw read-only Ghidra responses remain ignored in local/task0051. Status Reconstructed, not Verified.

| Original owner/function | Retail | Debug | Matching anchors |
| --- | --- | --- | --- |
| IsPointInPoly free function | 0014F1E8 | 0014EE38 | half-open Y crossing and strict X intersection parity |
| FindVisibleZone free function | 0014F2B0 | 0014EF00 | original zone list search and successful move to front |
| ScenerySectionHeader::DrawAScenery | 0014FE18 | 0014FA68 | instance48, info40, radius+4, size cutoff2, three FarClipSize thresholds, flags masksFF/100/200 |
| ScenerySectionHeader::TreeCull | 00150378 | 0014FFC8 | original64-entry LIFO stack, negative child codes index nodes, nonnegative index instances |
| GrandSceneryCullInfo::WhatSectionsShouldWeDraw | 00150950 | 001505A0 | current zone, always-loaded sections90..99, list fallback below100, capacity128 |
| GrandSceneryCullInfo::CullView | 00150B10 | 00150760 | selected sections then each section TreeCull |
| GrandSceneryCullInfo::DoCulling | 00150D98 | 001509E8 | original view position/direction/H feed section culling |
| GrandSceneryCullInfo::StuffScenery | 00150EB8 | 00150B08 | per-draw matrix/identity selection, visibility==2 caller flag4, lit model context |

No original structure is reinterpreted as a host object. The asset adapter expands original fields into owning vectors. The original free IsPointInPoly/FindVisibleZone functions now have explicit host-value overloads; no complete original object ABI or editor/caffeine object is claimed.

## Resource decoding and immutable lifetime

port/track/assets.hpp Scene retains every original SceneryInfo LOD hash, signed FarClipSize, facade flag and Radius; original instance Position, flags and short world BBox; original section number and24-byte scenery tree. Original LoaderScenery Debug0014E920 confirms34101 header aligned16,34102 info40 unaligned,34103 instance48 aligned16,34104 tree24 unaligned and34106 VisibleZone288 unaligned. Debug type metadata independently confirms all offsets. Existing E-0048-A02 supplied Retail chunk-span compatibility; the new fields are decoded explicitly from the same records.

Zones preserve original section lists and16-point maximum perimeter. Section header original loader inserts section numbers>=90 at list front and lower numbers at back; fallback traversal reconstructs that order, while successful zone lookup moves the selected zone to front. Selected-zone traversal appends loaded90..99 followed by the exact stored visible-section sequence. No distance sort of scenery instances is introduced. Tree traversal processes immediate instance children in original child order and pushes node children onto the LIFO stack. Host range/cycle/stack guards fail explicitly rather than corrupt memory.

All three LODs participate in model, material and texture dependencies, including shared INGAMEB models. Material flags40,80,100 use original material lookup;40 also requests original pTextureInfoSpecularFalloff hash0184D1F8 (Debug eFixUpTables00103D28). Unknown resources remain explicit gaps and a selected draw with unresolved texture fails in the command stage.

Runtime owns shared_ptr<const Scene>, transactional load, synchronous poll_load completion, unload and per-runtime visible-zone cache. Async source work and GPU upload scheduling remain host responsibilities. prepare_frame returns per-surface commands with native matrix, LOD, instance/solid/surface identity, visibility, original submit index, lighting and effect state. The camera supplies native homogeneous clip matrix, unit forward direction, pixel focal H and original exclude flags; no Godot math or guessed LOD distances enter the recovered classes. Runtime routes the original eDisplayFrame track slice restored by the effects worker; see blend.md for strip masks, alpha order and material-pass scope.

## E-0051-L01 — per-instance in-game light context

Retail elSetupLightContext00115978 and Debug00115D88 establish the same bounded in-game branch. QueryLightDatabase runs at the original instance matrix translation, rather than world origin. Sun direction uses initialized original direction times10000 minus object position; up to two point lights use light position minus object position. Inverse rotation is transpose of the original local-world basis; vectors are transformed then normalized. Original ambient fallback85, alpha128, directional RGB80 times clamped LightScale, and original zero blue point-light output are retained. Light-pack ownership now survives resource loading, allowing the same query every frame for facades/transformed instances. Identity geometry retains the original shared world-light context.

This new overload does not change the existing audited world-identity overload. Its28 floats are the existing VU local-colour and local-direction upload block. LocalEyePosition/EnvMapMatrix are separate downstream effect inputs and must not be inferred from this block. Frontend/car-select override lighting remains outside this track-runtime slice.

## Validation

New native test src/tests/track_runtime_tests.cpp checks original pixel-size threshold LOD changes, per-surface ordering, behind-camera radius rejection, exclude flags, tree clipping, zone selection, per-instance ambient queries, unload and query-after-unload rejection. Isolated compilation uses current texture/header translation units together, avoiding stale-archive ABI mismatch from added metadata. Unit test passed. Real TRACK11 loaded128 sections,23 zones,8407 instances and produced3838 surface commands for an inspection view. This is PC reconstruction evidence, not matched original-runtime verification. Complete catalogue sweep results are recorded after completion below.

Final private catalogue sweep: all30 tracks loaded allLOD resources and prepared ordered surface frames successfully. Section counts62..159, zone counts23..52, instance counts8247..20598. Logs local/task0051/runtime-all-tracks.log and material-alpha40-all-tracks.log. Latest unit additionally checks identity-light equality with the existing audited world context and retry after a missing selected texture fails. A byte scan checked518621 original instance/node bounding boxes across30 assets: zero inverted boxes.

The all30 flags40 material scan found only Road hash0x001C3745, M0.z=.1 and M3=(8,-7.45244e9,20,108). This is a coefficient observation, not by itself a proof of the final alpha upper bound; reflection-vector and VU bit-power bounds belong to E-0051 blend evidence.

Original IsLit Retail0010CB00 / Debug0010CEF8 reads eSolid aggregate StripFlags ushort+0x32 &0x1C0. Identity scenery always receives shared world context. Nonidentity scenery receives per-instance lighting only when this aggregate says lit; otherwise has_light_context=false and the downstream original RenderStrips mask clears1C0. The parser retains that aggregate directly rather than deriving it from surface flags. A selected nonzero model hash with absent Solid does not cause lower-LOD substitution: original loader creates a nonnull eModel even when its Solid lookup fails, so the selected draw remains absent with a resource gap.

Final all30 strip40 state scan (local/task0051/alpha40-tuples-all-tracks.log): all raw flags0x4041, FIX0, sorting Field4D0, Road material0x001C3745. Both base ALPHA0A copy and ALPHA48 additive occur. Copy variants have semi0, Field50=1, Field74=0 or1; additive variants have semi1, Field74=0, Field50=0 or1. Decoded source texture maximum alpha is123..127. Do not assume every strip40 uses base additive blending. SPECULARFALLOFF hash0184D1F8 exists in all30 TEXnnTRACK.BIN packages,256x512 PSMT4; it is requested as an actual second-pass dependency. Its pixels/state validation belongs to the effects evidence. Latest private native unit passes after the aggregate IsLit and missing-Solid LOD corrections.

## E-0051-S02 — external track shader adapter

New standalone platform/shaders/track_shader.cpp specializes the shared car base shader without changing native local positions or CUSTOM0 normals. Per-command source_camera is the original inverse-rotation local eye position, material350..353 includes alpha40 coefficients even for unlit base strips, and light35C..362 is the native per-instance upload. Explicit original mip pixels are sampled through external track_sampling.glsl; derivative-based LOD remains a host projection, not a claim of GS Q/subpixel equivalence.

External track_alpha_vertex.glsl uses the recovered VU reflection/affine-bit helpers and replaces only Gouraud alpha before screen-space colour interpolation. Packet320 preserves the same ST/Q and geometry; track_fragment.glsl samples actual SPECULARFALLOFF pixels at those UVs and uses only the native-proven Copy/Additive Fusion40 routes. FusionNone on a40 strip rejects explicitly. Positive additive increments, including ordinary ALPHA48, are precomputed with original floor divisions and emitted with host alpha1, allowing increments originating from source alpha above128 without normalized-alpha truncation. All copy draws likewise use alpha1 so native submission order remains a host-transparent ordering input. Ordinary ALPHA44 retains its separate bounded source-over adapter; its source-alpha range must be independently validated.

Original normal render targets select ZBufferMode31 (Retail001064C8), hence original ZRanges Z24 row: pack0 words40000010/40800000, pack1 words40000040/40802000. Pack0 writes no custom DEPTH, preserving exact host car/track alignment. Pack1 derives effective inverse view-Z as q+((Ap-A0)+(Bp-B0)*q)/B0, then maps it through host projection coefficients. Computing this difference avoids subtracting two screen-depth floats near2 in the fragment. Host coefficients are computed in the vertex stage from the actual projection matrix and passed as a flat varying; global helper functions cannot access Godot's stage-specific projection builtin. Bind depth_near/depth_far to the actual view projection values. This preserves the relative range ordering but does not reproduce the complete original GS24 float-bit depth precision. BiasLevel2 requires a separate host depth range and rejects explicitly; the all30 TRACKB surface/header scan finds only Bias0/1 (local/task0051/used-bias-all-tracks.txt).

Debug fill and perimeter shader sources moved unchanged to track_debug_fill.gdshader and track_debug_edges.gdshader, embedded through the same build-generated header. No source-tree shader files are needed at runtime.

All30 base-and-all-supplied-mip alpha bounds were subsequently measured (local/task0051/alpha40-all-mip-ranges.log). Every40 texture in the TEST5030D copy case (semi0,Field74=0,Field50=1) has constant127 alpha across all sampled images, matching SPECULARFALLOFF's constant127. Other40 copy/additive cases route TEST5000B, whose GEQUAL0 test cannot reject finite nonnegative modulation alpha. The proof is per texture range, not inferred from a material name; host fusion still calls the native eligibility helper with actual ranges.

Parent shared Godot validation after the stage-builtin and helper-name corrections: local/task0051/ctest-track-third.log reports100% pass, including track_pipeline_gpu, race_runtime_host and track_pipeline_parity. This confirms shader compilation and bounded runtime execution in the PC host; it does not mark original rasterizer equivalence Verified. Track11 mask targeting data for actual-window inspection is retained at local/task0051/mask-views.json:30 instance/LOD/surface records containing original TUNNELROAD_MASK orW_RDWATER_MASK triangles, with native world vertices, centroid and normal.


Final owner audit: original DrawAScenery constructs the selected identity/null, rotation or facade matrix and applies flag100 height adjustment. Original GrandSceneryCullInfo::StuffScenery owns the subsequent shared-world/per-instance/null light-context selection and eModel submission. The typed projection is now split accordingly in the original class/header/TU. StuffScenery submits one selected model to the typed eModel::Render slice; that method owns per-surface expansion and RenderStrips dispatch, while eDisplayFrame owns attachment ordering. No matrix, lighting or selection algorithm is changed by the ownership split. Original static draw-list storage and allocation are represented by explicit host values; full PS2 object ABI and animated model submission are not asserted.

## E-0051-S02 - original asset LOD spot/count recheck, 2026-10-19

Read-only raw chunk scan of the configured authorized ZZDATA tree found38
TRACKB BUN/LZC files (including duplicate uncompressed/compressed variants).
All30 LZC packages contain SceneryInfo rows with multiple distinct nonzero
model hashes. Counts are per-section40-byte info rows, NOT unique models,
instances or triangles. TRACKB11:1895 rows with one distinct model and398
with two; TRACKB21:725 with one,277 with two,130 with three.
TRACKB11 decompressed offset0x6FC3A8: hashes D1BDA271/6071D212/0,
FarClipSize150/16/0. TRACKB21 offset0x719218: E75A365F/0ECE784F/0ECF04B0,
thresholds20/10/3. These are original logical projected-size thresholds, not meters.
Reproduce: py -3 local/task0055/scan_original_lod.py; ignored detailed output
local/task0055/original-lod-resource-scan.json. Existing E-0051-S01 Retail
0014FE18 confirms selection executes on projected size (cutoff2, three slots).
This establishes actual multi-model scenery LOD resource use, not that every
road/tunnel/terrain surface has simplified versions. Asset names and per-LOD
triangle reduction were not classified by this scan. No runtime equivalence or
Verified promotion; no source/binary changes needed for this read-only check.
