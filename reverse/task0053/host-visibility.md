# TASK-0053 host visibility regression

## E-0053-H01: per-view layer isolation

The prior host called RenderingServer.instance_set_ignore_culling(true) on sky,
Sun and world-particle MeshInstance3D objects. Actual per-viewport GPU counters
show this bypasses layer filtering as well as frustum rejection. At the fixed
Track11 start, all370 sky nodes from main and six capture resources were submitted
to every active view, including the main camera with mask0 and the native-depth
camera with layer20. Original mode3921draws vs Sky-off1701, difference2220.
The root camera alone emitted370 3D calls although it should only composite color.

The corrected host removes ignore_culling. Its custom CPU bounds contain the actual
camera frustum, transformed into each mesh's local coordinates; original vertices,
UVs, colors and shader projection are unchanged. Normal layer and visibility tests
remain active. This bound is host transport for custom-projected geometry, not a
new PS2 draw distance. Native-depth proxies copy the source custom bound for writing
screen-space particles. Sun's original depth and cached visibility are not forced.

After correction the identical diagnostic gives1912draws, Sky-off1701, difference211
(main52 plus three scheduled capture skies). Root mask0 emits zero 3D calls. Raw
native-depth view decreases951 to581calls and excludes sky. E-0053-S01 independently
proves clear Sun coverage1024 and road-occluded coverage0 after the correction.
Commands/logs/data: local/task0052/draw-breakdown.json (before),
local/task0053/draw-breakdown-after.json and .log. Native arithmetic is unaffected.

## E-0053-H02: original visibility diagnostics

Track11 fixed start loads8407 instances in128 sections, considers230 instances in
14 selected sections and submits102 visible instances. This disproves whole-track
geometry submission at that pose; loaded/cache ownership is not per-frame drawing.
The native zone/section/tree/frustum/pixel-size/LOD chain remains unchanged from
E-0051-S01 and E0052D06. Its original camera constructor near0.5/far10000 and
CreateViewMatricies scaling remain the bounded source for the default host view;
no arbitrary short range or disabled effect was substituted as a performance fix.
Visibility diagnostics expose loaded/considered/visible counts and selected/loaded
sections in the existing common ImGui group. These are read-only host counters.

Actual layer-aware GPU counters, not scene-node ownership alone, establish which
views render. Native ordered packets remain separately tested. Original-console
visual equivalence is not claimed.


## E-0053-H03: Roadshine permission avoids extra sampling

The original mask-alpha rewrite still contributes to the base road/water blend and
must remain when the high-light permission is disabled. The earlier host shader
sampled specular_texture unconditionally, then replaced its RGB by zero. Therefore
that earlier A/B measures the permission implementation, not removal of all work
associated with the strip40 path. The corrected fragment branches on the uniform
before the extra specular texture fetch/modulation; the enabled arithmetic and
integer fusion are identical, and disabled mode retains base-mask alpha/fusion.
No empirical shine attenuation or new material coefficient is introduced.
Numeric fusion and actual wet-road GPU checks remain required after this change.
