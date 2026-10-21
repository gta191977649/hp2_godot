# TASK0052 environment capture host

## E0052D07: original target layout, views and scenery filtering

Fresh read-only Retail001064C8 assigns target IDs8..13 to64x64 FrameMode2 / ZBufferMode0x32, and target14 TARGET_ENVMAP0_FULL to256x256 FrameMode2 with FBA1 and ClearBackground0. The native six-view source captures are therefore64, not128. Fresh Retail00117508 sets view IDs4..9 and targets8..13; Debug EVIEW_ID confirms ENVMAP0F=4 through ENVMAP0D=9. These are distinct from target IDs and from player framebuffer targets15..21.

Fresh Retail0010F858 / Debug0010FC38 SetupSceneryCullInfo starts ExcludeFlags0x10 (or0x11 in two-player modes3/4), adds0x102 for viewIDs4..9, then8 for forward or4 for backward. The host passes0x11A for ordinary forward capture, preserving the original flag policy rather than admitting all track geometry.

CreateEnvMap Retail00118150 and CreateBackEnvMap00118498 read destination dimensions from the same target descriptor passed by eCreateEnvMapPrims00118810. Five front-face primitives and four back patches populate ONE target14 atlas; every back patch samples face2. Front/back texture accessors consequently alias the same atlas. The module does not introduce a cubemap.

Read-only captures: `local/task0052/retail-target-initializer.json`, `reflection-00117508.c`, `reflection-00118150.c`, `reflection-00118498.c`, `reflection-00118810.c`, `reflection-0010f858.c`, `env-filter-0010fc38.c`, `env-view-ids.txt`.

## Host lifetime and submission seam

`track_reflection_renderer.hpp/.cpp` owns six64-square SubViewports and one persistent256-square Canvas atlas. Its Camera3D transforms invert original world-camera matrices, map native XYZ to host YZX, and use original horizontal bAngle0x75E4, near0.5, far10000. It calls the existing native environment_cameras schedule and environment_warps tables. Original camera/filter values flow to a callback which MUST use the same Runtime command preparation and shared GPU submission as the main view. Source viewports share the owner's World3D; masks use bits1..6, separately from main layer0.

The module performs no geometry loading, independent culling algorithm or physics. An absent original vehicle environment origin returns unavailable. All six source faces must have been scheduled before texture access becomes available; the original phase schedule remains intact. Failed callbacks disable publication. Clear resets readiness and scheduled updates; destruction releases all owned nodes.

The atlas uses original warp topology and order, sampling scheduled source view textures with linear filtering and retaining unscheduled regions. Godot color/depth target precision remains a host adaptation; per-draw PSMCT16 quantization is not claimed Verified. Full GPU capture, original image comparison, common environment material bindings and source shadow/filter integration remain parent-owned acceptance work.

## E0052D08: context2 GS packet evidence

Retail0010AC50 writes packet318 ALPHA low0x48, default high0x10 (mode3 high0x40), TEX0 default0x620213000 (PSMCT16,TCC1,TFX0,TW8,TH8), or mode3 0x620012800 (PSMCT32). Packet31C has the same TEX0 but reads original global selectors. ELF .reginfo gives RetailGP003358F0; memory0032F70C..720 contains A0,B2,C1,D1,FIX16. Therefore its default ALPHA is0x1000000058, destination-alpha additive. Mode3 overrides both packets with source-alpha0x4000000048. Low0x48 selects source-alpha additive `(Cs-0)*As/128+Cd`; low0x58 uses destinationAd instead. FIX is ignored by both C selectors. No plain source-over/copy or unsupported31C source-alpha substitution is permitted.

Fresh RetailAttachDrawEnvironment001085C0 (Debug001083E0) writes TEXA0x8000008000 to GSregister3B:TA0=0,AEM1,TA1=128. Target14 FBA1 means writtenPSMCT16A1=1 samples as128. The host warp shader now quantizes its RGB endpoint to5bits (expand by8) and stores decodedalpha128/255. This does not reproduce intermediate per-draw16bit source-capture blending/Z precision: the live diagnostic explicitly exposes that remaining gap.

RetailUpdateGifTags00105AC0 constructs TEST2=`uVar25|0x10000`, ATE0, and ZBUF2 with ZMSK1. Environment context2 alpha testing is disabled even when basecontext1 uses TEST6. Thus the separate envshader has no inherited base alpha discard, preserves GEQUAL and does not write Z. Its BiasLevel1 DEPTH specialization matches the base projection; unsupported BiasLevel2 fails explicitly.

All30-track scan produced24 environment material groups (CSV `environment-material-bounds.csv`); environment surfaces occur only on tracks11..16/21..26. Every effective flags value is0x4100, with noflag4 and noflag40 overlap. Twelve groups use baseALPHA0A withtexturealpha127, six use44/alpha109, six use44/alpha90..92 (allmips included). Context2 M1.w/M2.w pairs bound sourcealpha to38..51 or64. Therefore shipped track100 passes use packet318 and can render as a separate additive preweighted pass after the unmodified base pass. Base44 is not incorrectly fused into a single alpha-blended draw.

The new `track_environment_shader_source` uses original sphere/reflection vertex math, secondmaterial blocks16..31, dynamic2Datlas, source48 prefolding and ALPHA1 hostaddition. It rejects31C because explicit destination alpha is required. Parent shared submission binds and orders that pass. `port/track/reflection.hpp` records packet defaults and bounded original context2 alpha analysis; private packet/range unit checks passed.

## E0052D12 - Environment target projection correction

Confirmed from Debug `CreateViewMatricies` 0010EA18 (Retail 0010E638):
`view_to_screen.v0.x = target.AspectRatioX` and homogeneous `w = z/H`,
where `H = CalculateH(camera.FieldOfView)`. The original square environment
source target has width/height64 and aspect coefficients1. Debug globals
`ClipVolumeScaleX/Y` at003268C8/003268CC both contain float0.5 (fresh read-only
memory capture `local/task0052/clip-volume-defaults.json`). Thus both raster and
clip projection horizontal coefficient are `H/32`, not `cot(FieldOfView/2)`.
The bAngle75E4 metadata uses original global ScreenWidth512 in CalculateH;
its resulting H is approximately31.9234, so the equivalent host64x64 camera
horizontal FOV is `2*atan(32/H)` (approximately90.14 degrees).

The previous host camera copied165.785 degrees directly. This widened its
projection by approximately8x and disagreed with original target projection.
TrackReflectionRenderer now requires the caller's original math-derived H,
keeps original face metadata intact, and converts only the host raster camera.
Invalid H fails explicitly. Syntax compilation passed; integrated GPU visibility,
reflection alignment and performance remain pending root regression. This is a
projection correction, not a change to original visibility distances or LOD.

## E0052D13 - Reachable vehicle packet31C is not source-alpha blending

An isolated current-TU asset probe loaded original ELISE skin1 and all selected
part/LOD resources through the existing Model loader. It found16 unique
reflective(stripflags,texture,material) groups, six with effectiveflag4 from
texture Field74=1 despite stripflags4180 containing no literal4. Examples include
ELISE_A LOD0 and both side mirrors using texture681597F6/material827D95A1.
`elise-reflection-bounds.csv` preserves all16 metadata/material rows. The private
reproducible probe is `local/task0052/car_reflection_bounds.cpp`; compile it with
current model.cpp, vehicle_geometry.cpp and hp2_tpk_texture.cpp before linking
unchanged runtime/core dependencies. This is asset evidence, not original GPU
image verification; other skins/cars were not exhaustively scanned here.

All six effectiveflag4 rows have base ALPHA0A(copy), TEX0MODULATE, constant
texture alpha127 and TEST ATST5(GEQUAL), reference0. Thus the base alpha test
accepts every unsigned alpha including0; there is no alpha-discard hole for
these six rows. Base depth rejection is still possible. Their first-pass alpha
comes from the original first material block's flag4 calculation, not the
second context's reflection-alpha coefficients. Main target PSMCT32/FBA0 stores
that modulated first-pass alpha, as established by E0052D02. The original mode1
packet31C ALPHA58 uses that framebuffer Ad. FE mode3 explicitly replaces it
with ALPHA48(As), so the existing FE source-alpha shader remains a separate
supported route.

A hypothetical bounded fusion must retain first-pass material/color calculation,
first texture UV and alpha modulation separately from second-context reflected
UV/RGB, preserve per-fragment depth acceptance and original primitive/pass order,
and use the first pass's destination alpha. Simply using second-context RGBAQ
alpha, or the atlas texture alpha128, is not equivalent. No such fusion is
implemented or certified by this audit. The current separate-pass vehicle
shader is therefore guarded with `require_environment_source_alpha`:
flag4 and mode other than3 throws hp2::NotImplemented at0010AC50. The caller
must invoke this before enabling that unsupported capture draw. Tests cover
mode1 rejection, FE mode3 acceptance and nonflag4 acceptance; private effect
reference executable passed. This explicit gap prevents a silent As substitution.

### E0052D13 follow-up: fusion prerequisites not yet satisfied

The original Debug UpdateGifTags001058D8 writes identical uVar22 FRAME state to
GS registers4C and4D. Its mask comes from FrameWriteMask; normal main-target
mask0 therefore does not preserve alpha in context2. Packet31C writes its own
modulated source alpha to the framebuffer, while RGB blending reads the previous
base alpha. A fusion that retains first-pass alpha or forces255 is not a complete
framebuffer-equivalent implementation.

A second isolated ELISE allpart/allLOD probe found268 effectiveflag4 reflective
surfaces, all with Field50=1(depthwrites). Only one surface has three indices;
the others contain4..28 triangles. Original base-all-triangles then environment-
all-triangles differs from fused per-triangle behavior for overlapping equal-depth
primitives. Geometry/camera or constant-alpha proofs are required before merging
those passes; copy blend and always-accept alpha test alone are insufficient.
`port/vehicle/environment_fusion.hpp` therefore supplies only an RGB scalar
reference and single-triangle candidate predicate, not an authorized shader path.
Private scalar tests distinguish Ad192 from second-pass As7 and test clamping,
zeroAd and multi-triangle rejection. No GPU fusion or guard removal is claimed.

## E0052D14 - Supported318 additive-alpha GPU isolation

A standalone actualCompatibility GPU scalar probe used the shared production
`hp2_framebuffer_rgb` helper and the vehicle's `blend_add` plus fractional ALPHA
route, against an encoded gray20 background and gray100 source. Arc B390 driver
32.0.101.9150/Godot4.4.1 produced45/70/120 at As32/64/128, matching GS byte
addition. At As97 it produced96 versus original truncating GS95; preweighting
RGB by floor(Cs*As/128) with ALPHA1 produced95. Thus these fixtures do not
support a large gamma amplification as the cause of the flat blue ELISE image;
they expose only a fractional rounding discrepancy in this tested range.
Private probe/log: `local/task0052/car_env_alpha_gpu.gd` and
`car-env-alpha-gpu.log`. Full real-car capture off/on and atlas isolation remains
pending the current staged DLL and exclusive GPU test slot. No production shader
was changed based on the disproven amplification hypothesis.

## E0052D15 - Flat blue vehicle was overwritten by the host sky pass

Actual staged Debug isolation at the same Track11 start camera first detached
all vehicle reflection next-passes. The blue silhouette persisted, so the atlas
and supported318 contribution were not its cause. The atlas readback contained
varied original road/trees/sky. Root then corrected the main sky clip adapter
from Compatibility near clip `+w` to `CLIP_SPACE_FAR*w`. The rebuilt DLL restored
the fully textured silver ELISE with reflection both disabled and enabled;
supported318 adds a modest visible contribution. Sky off/on retained the car.
No empirical reflection attenuation or Fog change was applied.

The host sky is transparent priority-128, while the car base is opaque and the
track is ordered transparent. A near-clip background can therefore overwrite
opaque car pixels, then be covered by subsequent transparent track pixels,
producing the observed car-shaped blue region. The new asset-free
`sky_depth_gpu_smoke` consumes the exact shared `sky_depth.hpp` GLSL adapter
and renders an opaque red foreground, transparent green foreground and blue
background. An explicit old-sign negative control produced blue/green/blue;
the production far-sign adapter produced red/green/blue. This verifies the
failure mechanism rather than merely comparing a constant string.

Actual Compatibility GPU test passed on Godot4.4.1/ArcB390 driver32.0.101.9150:
`--path local/task0051/staged/godot --rendering-method gl_compatibility
res://scenes/sky_depth_gpu_smoke.tscn`. Log `local/task0052/sky-depth-gpu.log`.
Real-scene isolation script/log/images are `car_environment_isolate.gd`,
`car-environment-isolate-fixed.log`, `car-isolate-{false,true}.png`, and
`car-isolate-sky-{false,true}.png` under the same ignored evidence directory.
These are host regression checks; no original-console Verified claim is made.
