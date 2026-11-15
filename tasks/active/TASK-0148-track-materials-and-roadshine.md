# TASK-0148 - Track materials and Roadshine

The Godot race, rear view and Track Viewer use the original track texture bytes,
RGB5A1 modulation, TEX1 sampling, alpha tests and depth state for the affected
materials. Roadshine is enabled by default. The `effect` window and the Track
Viewer Render tab share one checkbox state. The switch changes only the
packet 320 highlight contribution. The authored road, masks and depth coverage
continue to render. No game-core changes were needed.

## Original evidence

- Ghidra MCP inspected Debug `NFSHP2.ELF`, SHA-256
  `815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`.
  `CreateTextureContext` 00108AE0 supplies TEX0, TEX1, ALPHA and CLAMP.
  `RenderStrips` 0010E120 and `AttachAlphaPolys` 0010BBF8 provide the
  strip and sorting correspondence.
- Retail `SLUS_203.62`, SHA-256
  `7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`,
  supplies `CreateTextureContext` 00108CC0, `RenderStrips` 0010DD58,
  solid and alpha attachment 0010BB20 and 0010B838, and the projection
  table read at 002C9E20. Sub_0010AC50 binds packet 320 with
  `SPECULARFALLOFF` (0184D1F8), base level, repeat addressing and ALPHA58.
  The VU path at 002A6F78 rewrites alpha and retains the geometry and ST/Q.
  See [the prior packet trace](../../reverse/task0052/roadshine.md) and
  [the coverage proof](../../reverse/task0051/blend.md).
- The Godot vertex shader follows the decoded Road exponent and amplitude,
  reflection direction and alpha coefficients used by
  `EcstasyLitVertexColor`. Those inputs are already decoded for a native
  power operation. The fragment shader uses byte-domain modulation and
  destination alpha from the base fragment to fuse the two proven passes.

## Validation

- The retail runtime inventory covered all 30 routes and all loaded section
  model LODs, with 742,279 strip occurrences, 468 shaft occurrences,
  78,189 strip-40 occurrences and 21,943 bias-1 occurrences. The counts
  include repeated models from different sections. Resolved bindings used
  neither bias 2 nor any missing texture, and no replacement changed a resolved
  binding in the scanned section models. Detailed rows and route totals are in
  `local/track-materials/audit/materials.csv` and `coverage.csv`.
- Vulkan and D3D12 rendered shafts on Tracks 12, 22, 31, 41 and 61,
  wet overlays on Track 12, and Roadshine on Track 11. The first Track 11
  comparison changed 3,226 sampled pixels on both drivers when Roadshine
  was toggled. Both drivers passed the Track 33 race lifecycle with mirror
  and reflection checks.
- Track 12 was captured at HP2 (-1482, 427, -59) and four positions inside
  the portal. The capture keeps the base road visible while toggling just
  the tunnel mask, and also records scene depth. The prominent bright lines
  from the earlier capture are reduced. The portal still has a visible
  material boundary and the capture does not establish exact edge parity.
  No geometry was modified.
- Debug and Release staging passed. The Debug Vulkan Track Viewer smoke
  loaded Track 11, switched to Track 12, clicked the default-enabled
  Roadshine checkbox off and on, moved the camera and returned to the
  menu. Run it at 1200×800 with `HP2_MVP_VIEWER_ROADSHINE=1`.
  The in-race Effect window also toggled Roadshine off and on in the
  Track 12 Debug Vulkan capture at 960×720.

```powershell
cmake --build --preset godot-mvp-debug --target stage_mvp --parallel 8
cmake --build --preset godot-mvp-release --target stage_mvp --parallel 8
$env:HP2_PS2_ASSET_ROOT = 'D:/ps2_game/Need for Speed - Hot Pursuit 2 (USA).iso'
py -3 tools/audit_mvp_track_materials.py --assets $env:HP2_PS2_ASSET_ROOT --out local/track-materials/audit
$engine = "$PWD/local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe"
$env:HP2_PROFILE_DIR = "$PWD/local/track-materials/fixture-profile"
$env:HP2_MVP_TEST_SCENERY_CAMERA = '1'
$env:HP2_PHOTO_DIR = (New-Item -ItemType Directory -Force local/track-materials/fixture).FullName
$env:HP2_PHOTO_SURFACE = 'tunnel'
& $engine --path build/godot-mvp-release/mvp-project --rendering-driver vulkan --resolution 960x720 --script "$PWD/tools/capture_mvp_track_scenery.gd" -- --race-track=12
```

Use `roadshine` and Track 11 for the Roadshine on/off comparison. Use
`shaft` or `water` for the material visibility probes. Repeat with D3D12.
Forward+ still blends the host destinations in linear color. The shaft
source-over weight is fitted for a dark destination. The road shader
evaluates the decoded curve with GPU floating-point math. Exact original
pixel parity and GS raster precision remain unverified. All runtime strip-40
states exercised satisfy the two-pass fusion guard. General destination-alpha
feedback for a state outside that guard remains unsupported and raises an
explicit error.

### Rear-view sky

The mirror already included the sky layer. Its world camera stops at 192 m
(`carAttachedCameraAdvance`), while the authored sky is several kilometres
across. The native sky pass uses an independent 100–50000 m projection
(`ecstasye.cpp`, PAL112E38–112F24) and disables depth writes. The Godot sky
shaders place the authored geometry at the far depth and draw its base and
alpha layers before the sun. World geometry keeps the original mirror range
and occludes the sky.

Release staging and the Track 12 fixed-camera pixel check passed on Vulkan
and D3D12. The sky filled at least 2,636 previously black pixels and
left more than 23,400 foreground pixels unchanged on each driver. The Vulkan Track 12
sun alignment and GS blend probes also passed. Logs and mirror captures
are in `local/rear-sky`.

## Track 33 shoulder and scenery color

The column-side fixture is in Mediterranean Paradise, around HP2
`(-490.4113, 1160.793, 151.9114)`. A ray from the fixed capture at pixel
`(300,530)` hit `RD_SECTION20_CHOP1`, texture `8253B146`. The resolved
strip is prelit and uses ALPHA `0x0A`, four authored mip levels, no depth
bias and depth writes. One triangle vertex is RGB `(192,184,144)` with
effective K `86/16`. The old Godot converter limited all three RGB
modulation factors to one. The nearby terrain uses the same unclipped
vertex RGB on the sampled strip with texture `AA987B5A`.

The render source in `D:/dev/hp2_ps2_rev` and the existing retail recovery
records identify `CalcAutoK` `0010C0F0`, `SetMipMapK` `0011C328`,
`RenderStrips` `0010DD58` and `CreateTextureContext` `00108CC0`. Retail US
`SLUS_203.62` was verified on disk with SHA-256
`7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`.
The per-strip TEX1 K byte and projected Q now select the original mip
levels. Raw texel bytes are modulated by raw vertex bytes before the
result is clamped and converted once for Forward+. The new opaque path
is limited to prelit MODULATE/copy surfaces. GS overlays use the same
strip K transport and keep their existing blend and coverage rules.
Static GS copy shaders omit the instance animation parameter so these
surfaces fit the GPU instance parameter budget. No game-core edits were
needed.

At 960×720, the central shoulder crop in frame 5 measured median RGB
`(101,96,88)` before and `(150,136,97)` after on each backend. The adjacent
terrain crop measured `(90,88,77)` and `(132,125,82)`. The twelve moving
camera samples retain the authored road/terrain edge. Frame 5 is identical
with Roadshine enabled or disabled. In the distinct Track 11 wet-road
fixture the toggle still changes 4,054 Vulkan and 4,055 D3D12 pixels.
The supplied PS2 reference is a different position and the captured moving
camera sequence does not establish a matching full-speed original run.

The HUD rank color differed between the provided captures. The HUD is
drawn after the 3D output and uses its own atlas and canvas modulation.
The current `color()` conversion also clamps modulation before sampling.
The incoming bytes for the exact reference glyph have not been matched,
so the HUD path and global grading were left alone. The existing race post
smoke passed fog, filtering, depth and radial history on both backends.

Debug `hp2_track12` and Release `stage_mvp` built. The Debug viewer ran in
an isolated staging copy because a live user session held the standard
Debug DLL. The Track 33 race lifecycle and mirror/reflection checks,
Track 12 shaft and tunnel mask, and Track 11 Roadshine passed on Vulkan
and D3D12. Track Viewer track switching and Roadshine toggle passed in
Debug Vulkan. The production GPU shader fixture reports `(144,123,64)`
and `(48,72,48)` for two per-strip K values on both backends.

```powershell
$engine = "$PWD/local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe"
$project = "$PWD/build/godot-mvp-release/mvp-project"
$env:HP2_MVP_TEST_SCENERY_CAMERA = '1'
$env:HP2_PHOTO_SURFACE = 'shoulder'
$env:HP2_PHOTO_TAKE = '17'
$env:HP2_PHOTO_DIR = (New-Item -ItemType Directory -Force local/track-color/reproduce).FullName
& $engine --path $project --resolution 960x720 --rendering-driver vulkan --script "$PWD/tools/capture_mvp_track_scenery.gd" -- --race-track=33
& $engine --path $project --resolution 640x480 --rendering-driver vulkan --script "$PWD/tools/test_mvp_track_material.gd"
```

Use an isolated `HP2_PROFILE_DIR` and the retail disc in
`HP2_PS2_ASSET_ROOT`. Repeat with `d3d12`. Logs, before/after PNGs, and the
moving-camera frame sequences are under `local/track-color`.

### Sky color follow-up

The sky uses its own far-depth materials. It previously missed the GS RGB
modulation correction applied to the track. `SkyResources::StuffSky` in
the donor (`src/skye.cpp`, retail `0011C6F8`) submits the authored sky model
through the solid-strip path. The Godot sky now samples the original RGB
bytes, preserves the full vertex RGB5 values, and converts the result to
linear only after the GS multiply and clamp. The mirror keeps its 192 m
world clip and the sky keeps its existing render order.

The GPU fixture expects `(144,123,64)` from sky texel `(96,110,128)` and
vertex `(192,144,64)`. At the fixed Track 33 viewpoint, the central sky
crop changed by 0.52 RGB byte levels on average. The median moved from
`(113,116,118)` to `(113,115,117)` on both backends. This accounts for a
small sky tint difference. A full-frame recmp-to-Godot comparison still
needs matching pose, asset, and weather data. No exposure or saturation
adjustment was applied to the final image. Captures and logs are under
`local/sky-color`.

### Road puddle follow-up

Recomp revision `8ae05937be6865dde1e45c108cecd79f247d6b7a` uses the
authored ALPHA register in `recomp/runtime/native.cpp` (`set_blend`) and
composites the premultiplied contributions into an unorm render target in
`native_vk.cpp` and `shaders/native.frag`. The original order and depth
provenance are `AttachSolidPolys` at retail `0010BB20` and `002A6F78`
for the `0x40` specular vertex alpha. The ELF identity is recorded above.

The existing 30-route runtime inventory contains 422 affected strips in
routes 11–16 and 41–46. The wet asphalt (`0x44`) is order 2 and writes
depth. Its additive (`0x48`) masks use order 3 or 4 and retain their
unbiased or bias-1 depth test. Previously the non-sorted draws all shared
one priority and the additive signal was decoded against black before
being added to a lit road. Also, hiding the actual `W_RDWATER` underlay
exposed the sky at the Track 12 site. The underlay is the only road there.

Godot uses the same wet-material path in race and Track Viewer. It samples
the opaque scene for the byte-space blend when the depth is at the wet
road, within a pixel's depth variation for resolved depth. The original
asphalt texture provides the background where the wet strip is the only
road. The additive mask contributes in the authored order. It converts
the increment against the opaque reference, so overlapping transparent
layers can still differ from Recomp's live destination color.

`tools/test_mvp_track_material.gd` passed on Vulkan and D3D12 for the
underlay `(88,93,80)`, mask `(64,104,96)`, and replacement `(32,64,96)`
pixel references. The backends also rendered the Track 12 and Track 41
retail materials with `tools/capture_mvp_track_scenery.gd`. The matched
Godot Track 12 camera showed the far-scene silhouettes removed from the
asphalt, with the separate mask still affecting the road. No matched
Recomp runtime frame was captured. Logs and images are in `local/puddle`.

### Restore alpha over the original sky

The preceding asphalt fallback incorrectly made the entire wet strip
opaque where it provided the road. `SkyResources::StuffSky` (retail
`0011C6F8`) and the sky packet group in `EcstasyDisplayFrame` establish
that the original scene draws and flushes the sky before world scenery.
`AttachSolidPolys` then orders the non-depth-sorted wet asphalt and mask
by their authored material priority. This uses the existing retail ELF
identity above and required no game-core changes.

A depth-only diagnostic on Track 13 showed that the old tree silhouettes
coincided with opaque pixels behind the wet strip. Scene color at the road
is still used where its depth matches. Where the wet strip is the road,
the primary view and Track Viewer reuse the sky-only viewport that serves
sun glare. The shader applies the texture's byte alpha over that sky.
Without a sky reference, including the rear camera, the shader retains
source alpha over the live sky when the opaque screen copy has no depth.
The additive mask and depth tests retain their authored order.

Vulkan and D3D12 passed the distinct opaque-road, additive-mask,
live-sky and clean-sky pixel checks in `tools/test_mvp_track_material.gd`.
Both captured the forest puddle on Track 13, including the uphill view
toward the hazard post. The blue-gray sheen is visible again and the
opposite view no longer shows background tree silhouettes. The asphalt
shade still steps at the strip boundary in the uphill view. No matched
Recomp runtime frame was captured. Logs and images are in `local/puddle-alpha`.
The Debug Track Viewer switch-and-return test passed on both backends
(D3D12 passed on retry). `build.ps1 --jobs 8` exported Release, and the
export loaded and exited Track 13 on Vulkan.

### Track 13 wet-to-dry asphalt

Captured the retail Scenic Drive delivery in Recomp
`8ae05937be6865dde1e45c108cecd79f247d6b7a`. The stock `speed.exe`
SHA-256 was `3e985020f6f67e1086080d7320b73176cd9a1bd34fcae2d093e73ceeddb9d15e`.
The disc executable was the retail USA ELF recorded above. A local
Recomp hook at `speed.exe+0xD21E0` set the primary camera before the world
packet was built. `GetView` (`00107C90`) and `CreateViewMatricies`
(`0010E6B4`) provided the view and camera fields. The applied Godot
eye and target were `(-585.8,-81.8,-501.7)` and
`(-593.936,-83.45987,-501.7293)`, with a 53.9993-degree vertical FOV
derived from the Recomp projection. The captures are 1228 by 896.
The race UI and some distant scenery differ between the captures.

The native draw dump shows the depth-writing `W_RDWATER` in bucket 2,
`W_RDWATER_MASK` in bucket 3 with strip flag `0x40`, and projected
scenery shadows in bucket 4. A separate capture with Recomp
`HP2_NATIVEOFF=1` and a Godot Roadshine-off capture isolate its contribution.
At the wet sample near screen (800,635), Roadshine added about
`(36,34,26)` in Recomp and `(10,9,7)` in Godot. The separate Godot
additive draw converted its increment against the opaque snapshot, which
lacks the transparent wet asphalt. The dry asphalt was already close.

Godot compares positions, triangle indices, UVs, and both material
states before pairing a `W_RDWATER` or `W_RDWATER02` underlay with
`W_RDWATER_MASK`. The matched fragment uses the authored wet alpha,
followed by the mask and optional packet-320 contribution in byte space.
The normal, vertex tint and filter are taken from the mask. The other
wet material states keep their separate draws. Projected shadows with
the observed bucket-4 state draw after the pair. The live-sky fallback
for views without a clean sky capture remains an approximation.

`cmake --build --preset godot-mvp-debug --target stage_mvp --parallel 8`
and `build.ps1 --jobs 8` staged and exported the change. On both Vulkan
and D3D12, the before/after wet sample moved from `(88,82,67)` to
`(117,109,87)`, against Recomp `(116,108,87)`. The neighboring dry
sample stayed at `(118,112,88)`. The blue-gray patch near (800,450)
measured `(129,125,105)`, against Recomp `(127,123,104)`.
`tools/test_mvp_track_material.gd` passed on both APIs, including the
paired blend at 50 percent alpha with both Roadshine states. The fixed
camera, comparison frames, and logs are in `local/matched-puddle`.
The Debug Track Viewer loaded, switched tracks, and returned to the menu
on both APIs. D3D12 passed on retry at 60 FPS after the first automated
console input timed out. The isolated Release package published Track 13
on Vulkan using its `launcher.cfg` ISO path and exited normally.
The comparison validates this Recomp reference at the reported site.

### All-route wet surface sweep

The resolved retail catalogue audit scanned 30 routes, including shared
bindings, replacements and all authored LOD entries. It found 488 wet
surface strips in routes 11–16 and 41–46. All eight effective textures
occur at LOD 0 with no replacement binding. The other 18 routes have no
road puddle material in this catalogue.

| Family | Base strips | Mask strips | Original submission |
| --- | ---: | ---: | --- |
| `W_RDWATER`, `W_RDWATER02` | 108 | 108 | Base in bucket 2, mask in bucket 3 with projection 0 |
| `RD_PUDDLE`, `RD_PUDDLE2` | 103 | 103 | Base in bucket 2, mask in bucket 4 with projection 1 |
| `W_DIRTWATER` | 66 | 0 | Source-over base in bucket 2 |

`RD_PUDDLE_MASK` includes 27 strips without the `0x40` second pass.
Both states of that texture and the `RD_PUDDLE2_MASK` state now sample
the destination produced by their own wet asphalt. The Godot mask
submission keeps its original vertices, projection and draw bucket.
It carries the base's independent UV, tint and mip bias using a
triangle-corner match. The match includes reversed winding, reordered
strips and authored coincident triangles. Neither road geometry nor the
alpha textures are changed. `W_DIRTWATER` now uses byte-domain source-over
with the same sky and road depth checks.

Retail `CreateTextureContext` at `00108CC0`, `AttachSolidPolys` at
`0010BB20` and the Z24 packs at `002C9E20` define the submission.
Packet 320 (`0010AC50`, `002A6F78`) still controls Roadshine. The retail
ELF SHA-256 is the one recorded above. No game-core changes were needed.
The paired additive path reconstructs the wet destination from the opaque
scene or clean sky. Framebuffer feedback from intervening transparent
draws and views without a clean sky reference remain approximate.

Validated with the headless `tools/audit_mvp_track_materials.py` scan and
the GPU `tools/test_mvp_track_material.gd` fixture. All 79 submitted biased
mask surfaces resolved to a base across routes 41–46. Both Vulkan and
D3D12 reproduced `(92,144,196)` for a biased mask over a half-transparent
wet road. A depth sample between packs 0 and 1 received just the mask
`(160,155,112)`. A sample in front of both kept `(144,123,64)`.
Both APIs captured all 12 affected routes. The 41 and 44 before/after
captures disable only the new destination lookup for the control.
The isolated Release export loaded Track 41 and exited normally on
Vulkan after `build.ps1 --jobs 8`. Debug was staged with
`cmake --build --preset godot-mvp-debug --target stage_mvp --parallel 8`.
The Track Viewer switch-and-return smoke passed on both APIs.
Inventories and captures are under `local/all-puddles`. The new routes
were compared within Godot using fixed cameras.
