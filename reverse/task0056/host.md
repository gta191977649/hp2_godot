# E0056H01 - User-authorized Godot HLOD adapter

Original input authority: Retail SHA2567335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2,
ScenerySectionHeader::DrawAScenery0014FE18 and E-0051-S01/S02. Debug SHA256815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b remains explicit math-image/driving authority.
User explicitly accepts nonidentical distance boundaries and engine-selected levels.
New descriptor, interval mapping and alternate frame assembly are port-side policy,
not recovered game functions. Existing original selection bodies remain unchanged.

Godot4.4.1-stable source renderer_scene_cull.cpp: visibility position is transformed
AABB center (1545,1918); _visibility_range_check uses distance with end > and begin <
(2828..2844). Source URL:
https://github.com/godotengine/godot/blob/4.4.1-stable/servers/rendering/renderer_scene_cull.cpp
Custom AABBs preserve actual geometry while centering each level/pass on one anchor.
Intervals are configured for all levels; no host per-frame selected-level decision.
This is an authorized alternative to exact original rendering, never Verified.

Final prewarm correction: allocation stamps temporarily use the host frame counter,
but successful AND cancelled prewarm restore the entry counter. GPU fixture cancels
mid-prewarm and checks this explicitly; all30 host checks require unchanged counter
before first real view and identical node count after clear. The public visual counter
is diagnostic only. No effect timestep or original reflection schedule is advanced by
resource creation. Final review:11 prior port identities replaced,19 after identities,
including2 added bounded recovered functions documented separately under E0056T01.

## Show LOD debug increment

User requested simultaneous display of all authored LODs. The port-only flag
uses the shared all-level frame assembly for either backend, bypasses distance
ranges (including dependent passes), and preserves section selection. Independent
per-level keys prevent original-mode cache collisions. This is intentionally
not original game behavior or a performance feature.

## E0056H02 - Track11 authored LOD inventory (2026-10-19)

Read-only original asset probe:
`build/godot-debug/static_lods_tests.exe --assets 11 --lod-inventory`
PASS; output `local/task0056/lod-inventory-track11.log` (ignored).
Scene has8407 instances; descriptor excludes2 sky instances. Of8405 non-sky
instances,6922 reference one distinct model hash and1483 reference two. There are
24 distinct two-model reference tuples; all use authored XT_REDWOOD*, XT_SHRB*,
or XT_FORESTLINE* names. No three-distinct-model instance exists in this sample.
The50 model tuples named RD_SECTION*/ROADVENT* all have one distinct reference;
sampled TRN_SECTION*/TRN_BACKGROUND and tunnel tuples likewise have one.
Names are literal asset strings, not a new semantic parser. Six unresolved model
slots remain counted as references, not claimed available geometry. The probe
retains duplicates and uses distinct hashes for this histogram.

Conclusion bounded to Track11: multi-model LOD is concentrated in vegetation;
road/terrain samples do not gain a low model from Godot HLOD. Section/VisibleZone,
frustum and projected-size rejection are separate from alternate model selection
(Retail ScenerySectionHeader::DrawAScenery0014FE18, E-0051-S01). Do not describe
all track geometry as having authored low meshes. No all30 generalization and no
PS2 runtime Verified claim. Host Show LOD cannot manufacture absent resources.
