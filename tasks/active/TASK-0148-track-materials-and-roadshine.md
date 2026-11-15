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
