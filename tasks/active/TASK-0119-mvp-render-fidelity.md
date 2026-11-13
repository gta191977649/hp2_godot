# TASK-0119 - Entities Bridge materials, shadows and postprocessing

- Status: active
- Dependencies: TASK-0052, TASK-0115, TASK-0116, TASK-0117 and TASK-0118.
- Authority: Debug NFSHP2.ELF SHA-256 `815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`, documented Retail asset adapters and local ZZDATA. Godot mappings are port-side.

## Acceptance

- [x] Track, sky, vehicle body, paint, glass and additive shader resources consume decoded HP2 texture and strip fields.
- [ ] Source environment, shadows and postprocessing compared on Tracks 13, 12 and 11.
- [ ] First-entry loading compilation and warmed collision allocation measured.
- [ ] Three matched 1080p Vulkan Forward+ runs average at least 120 FPS with P95 at or below 8.33 ms.
- [x] Direct3D 12 FreeRun with effects off and on.
- [ ] Source visual equivalence and Metal window validation.

## Boundaries

The source clock and detached scenery validation are tracked by TASK-0118. PCSX2 is outside this session. Keep local screenshots and timing data under `local/task0119/`.

## Evidence and host mapping

- Surface colours use the decoded VIF RGB5/TEXA lanes and GS MODULATE divisor 128. The texture header drives alpha reference, addressing, copy, source-over or additive selection. See E-0033-T06/T07 and the existing source track texture adapters. Host anisotropic mip sampling, light evaluation, and alpha depth prepass remain approximations. Per-instance VU lighting, GS destination reads and auxiliary road-shine routes remain open.
- The recovered reflective strip flag `0x100` selects eligible paint surfaces. The shader binds texture `0x4ecc769c` and the original material block used by context 2. E-0035-R03 describes the authored coefficients. The Forward+ sphere map uses Godot view vectors and a fused emissive term. Live six-face source environment captures, VU evaluation and per-car reflection flags remain open.
- The scene provides the sky mesh, light packs, weather regions and SunInfo shadow position. Godot evaluates a shared directional shadow from the track-start anchor, and the ambient light uses the source query at the committed player position. E-0052-E06 documents `sh_Setup` Debug `0x00120F58`, including the original car-specific angle floor. That floor, per-car projected silhouette/ramp, and SHADOWIG contact shadows remain open.
- Fog uses per-view weather, the original depth-slot conversion and Godot depth fog. Debug `CreateFogPalette` `0x001196D0` (E-0052-P01) specifies the remaining GS palette curve. DOF uses CurrentKey +0x74/+0x78 when opted in. E-0052-P02/P04 records the source depth lookup and the normal Player camera's zero focus. Glow stays the existing optional Godot host effect. Source sun occlusion and glare, screen filters, radial blur, previous-frame blending and monochrome remain open.
- The Godot files and material cache are under `godot_mvp/presentation/shaders/` and `src/mvp/godot/native_race_materials.*`. Outside these paths, the changes cover camera pose and focus publication, detached scenery validation and reuse, project staging, graphics backend selection, the Windows test harness, and task records.

## Windows validation

Godot 4.7.2 stable `ed1daf0bf`, Intel Arc B390, WASAPI, Forward+, 1920×1080. Each matched Track 12 run used Release DLL SHA-256 `1a2119205cef8d8941ee3fa2713afd0f7ecd143377d64422ab548d2f6f415a41` with `--debug`, uncapped and VSync off, 120 warm-up and 600 measured frames. The older TASK-0117 tape diverged on tick 0. A new tape recorded with this DLL passed state checks in all three replays. Matching screenshots and complete timing data are local.

| Sample | Avg FPS | Mean ms | P95 ms | P99 ms | Root GPU P95 ms | Avg draw calls |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Track 12 replay 1 | 109.55 | 9.13 | 11.65 | 12.62 | 8.48 | 2314 |
| Track 12 replay 2 | 97.17 | 10.29 | 13.26 | 14.63 | 9.26 | 2314 |
| Track 12 replay 3 | 115.90 | 8.63 | 10.90 | 12.33 | 7.95 | 2314 |
| Track 11 FreeRun, pre-focus update | 195.02 | 5.13 | 6.02 | 6.66 | 5.76 | 1014 |

Track 11 with effects enabled passed on Vulkan at 178.28 FPS, P95 7.20 ms.
Direct3D 12 passed Track 11 with effects off at 308.50 FPS, P95 3.88 ms,
and with effects on at 238.24 FPS, P95 5.06 ms. The D3D12 runs used the
basic sampler. The initial run with `--gpu-profile` and effects enabled
lost the device with `0x887a0005` during texture creation. The cause remains
open and the successful runs excluded that option. The effects flag
permits source focus and enables host glow. It does not force a blur on
a view whose source focus fields are zero.

Track 12 mean DoTimestep CPU was 0.47–0.60 ms per timed call. Entity publication was 0.30–0.34 ms and section selection 0.03–0.04 ms per timed call. GPU scopes are delayed viewport timestamps, with separate mirror and UI scopes. These cannot be summed. Replay samples each created one native mesh and 189 nodes during the measurement. Track 13 fixed-clock active samples after 120 frames recorded 104.12 FPS, P95 15.34 ms, P99 16.14 ms and five intervals over 16.67 ms in the first 600 intervals. The main viewport GPU P95 was 8.88 ms and render CPU P95 was 1.71 ms. Source-clock Track 13 recorded 90.01 FPS and P95 13.93 ms in that window. With VSync on, these two Track 13 runs are diagnostic and separate from the uncapped replay comparison. Neither run reproduced section 41 record 8. No warmed collision resource claim is made.

For the Track 13 source run, pipeline counters (mesh, surface, draw, specialization) were `[0,20,0,22]` under the loading screen, `[0,24,0,24]` on the first active sample, and remained unchanged over the sampled drive. The longer fixed-clock chase ended at `[0,30,0,30]`, with zero draw-triggered pipeline compilations. Known resources prepare on load, and the remaining counters identify later surface and specialization work.

### Reproduction

```powershell
cmake --build --preset godot-mvp-release --target ps2_port --parallel 10
$common = @('--debug','--entities-project','build/godot-mvp-release/project','--championship-performance','--performance-uncapped','--performance-engine-breakdown','--performance-final-screenshot','--result-timeout','95')
py -3 -B tools/test_event_window.py @common --race-tape-record local/task0119/track12.tape --output local/task0119/championship-record
foreach ($run in 1..3) {
    py -3 -B tools/test_event_window.py @common --race-tape-replay local/task0119/track12.tape --output "local/task0119/championship-match-$run"
}
py -3 -B tools/test_event_window.py --debug --entities-project build/godot-mvp-release/project --mvp-freerun-performance --performance-engine-breakdown --performance-final-screenshot --result-timeout 100 --output local/task0119/freerun
py -3 -B tools/test_event_window.py --debug --entities-project build/godot-mvp-release/project --race-clock source --clock-1080p --clock-render-thread safe --clock-profile --clock-render-sample-limit 2800 --render-cap 0 --clock-display-sync --drive-route --road-course-driver --delivery-pace --launch-only --drive-steps 1000 --result-timeout 100 --output local/task0119/source-pose
py -3 -B tools/test_event_window.py --debug --entities-project build/godot-mvp-release/project --mvp-freerun-performance --performance-engine-breakdown --performance-final-screenshot --race-post on --result-timeout 70 --output local/task0119/vulkan-effects
py -3 -B tools/test_event_window.py --debug --entities-project build/godot-mvp-release/project --rendering-driver d3d12 --mvp-freerun-performance --performance-final-screenshot --race-post off --result-timeout 60 --output local/task0119/d3d12-verified
py -3 -B tools/test_event_window.py --debug --entities-project build/godot-mvp-release/project --rendering-driver d3d12 --mvp-freerun-performance --performance-final-screenshot --race-post on --result-timeout 60 --output local/task0119/d3d12-effects
py -3 -B tools/test_event_window.py --debug --entities-project build/godot-mvp-release/project --championship --loading-smoke --launch-only --result-timeout 75 --output local/task0119/loading-smoke
py -3 -B tools/test_event_window.py --debug --race-dll local/task0115/bin/ps2_port.dll --championship --launch-only --result-timeout 70 --output local/task0119/legacy-smoke
```

Launch the normal MVP with `./build.ps1 --run --mvp --debug --jobs 16`.
Add `--rendering-driver=d3d12` to select Direct3D 12 on Windows. The
normal legacy entry remains `./build.ps1 --run`. The legacy Championship
window passed launch, throttle, pause and return. The MVP Championship
window passed the separate original loading-screen smoke. Both extension
builds and the six focused native tests passed.

Next work: compare real collision frames and separately profile the high draw count and remaining effect pool creation. Source visual parity, contact shadows and sustained 120 FPS remain open.

## Follow-up rendering integration

- Track strips without 0x80 use unshaded GS texture/vertex modulation. Lit strips preserve the C++ ambient contribution separately from direct Godot lighting. Track 12 and Track 13 window captures show readable road texture and lane markings in the forest. The user's exact Track 13 position has no matching local input tape.
- Retail CarRenderInfo::Render 0x00120EF8, paired Debug 0x001210F0, and Debug sh_Setup 0x00120F58 produce topology-projected silhouettes with original collision geometry and SHADOWRAMP. Eligible actors use one persistent dynamic mesh. Their generic Godot caster is disabled. PartCuller13 eligibility and SHADOWIG submission remain open.
- The shared 64×64 capture faces and 256×256 atlas use E-0052-E01's Retail schedule and warps. The environment sky, low-byte scenery exclusions and section selector drive Godot viewport masks. Context-2 source material coefficients drive the paint shader. Original VU evaluation and GS raster precision remain open.
- Sun uses StuffSun Debug 0x00125430 and original layers, colours, blend and order. The MVP samples 64 positions over the original 32×32 aperture, using Godot reverse-Z scene depth. The central disk is clipped per fragment and negative intensity keeps its coverage bypass. StuffSunGlare and exact GS Z24 and transparent occlusion remain open.
- Fog and focus palettes use E-0052-P01/P02. Monochrome consumes the Debug eView.BlackAndWhiteMode at +0x74 and the E-0052-P05 alpha-bit route. History uses pBlendMask, the E-0099-W60/W63 alpha bytes and C++ prepare_screen. Generation, mover, POV, NIS and pose-reset changes invalidate history. Exact focus rasterization, GS feedback history and in-race monochrome/radial activation remain open. The original filter texture 02C8711C is absent in the reviewed bundles. The user-provided recomp checkout is at D:/dev/nfshp2-port-win/recomp. Its Retail symbol index helped locate DisplayFilter. Generated translated function bodies were unavailable in that checkout.

### Follow-up validation

Five CTests passed with Godot 4.7.2 build tools. On Intel Arc B390, the
first three 1080p Vulkan matched replays used DLL SHA-256
1dfe5c80cc243aa8693a15b9cae3f483c11af269af54950925a2bbb0cca41060.
They averaged 159.78, 161.24 and 163.02 FPS, with P95 10.19, 10.39 and
10.44 ms. P99 was 11.45, 11.38 and 11.94 ms. Each run had approximately
2355 draws per frame, zero new native meshes and 99 new native nodes.
The three runs passed the input tape state checks. Later profiled runs
measured about 0.09 ms for contact shadow preparation per World step,
0.019 ms for reflection submission and 0.013 ms for sun submission per
render frame. GPU timestamp measurements are per viewport. Full GPU
isolation of sun, shadows and reflections remains open.

The final lifecycle build (DLL SHA-256
59867a1851577b7c34dba97c9f7f7dc6bed2c50511bb10ed670a697e4319d859)
also passed all three matched replays with 120 warm-up and 600 measured
frames. The source tape was recorded on the first candidate and matched
the final build at every replayed World step.

| Final replay | Avg FPS | Mean ms | P95 ms | P99 ms | Root GPU P95 ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| 1 | 154.45 | 6.48 | 11.02 | 12.72 | 4.15 |
| 2 | 138.32 | 7.23 | 12.01 | 13.67 | 4.96 |
| 3 | 146.33 | 6.83 | 11.40 | 12.27 | 3.66 |

Each final replay retained zero new meshes and 99 new nodes during
measurement. Capture directories are `local/task0119/followup/lifecycle-replay-1`
through `lifecycle-replay-3`. Use the same replay command with those
output names after rebuilding. The Track 13 lifecycle recorded pipeline
counts [0,28,0,25] under loading and [0,38,0,37] at exit, in mesh,
surface, draw and specialization order. No draw-triggered compilation
was recorded. No section 41 record 8 overflow occurred in that run.

Track 11 Direct3D 12 with effects on measured 182.64 FPS, P95 9.66 ms
and P99 12.05 ms. Track 13 reached pursuit, airborne driving, the original
ticket scene, pause and return to the event tree after the capture teardown
fix. The lifecycle sample observed 50 airborne and 510 cinematic frames.
Its first 600 active intervals after 120 warm-up frames measured 87.85
FPS, P95 17.04 ms and P99 20.25 ms. The earlier build measured 114.03 FPS,
P95 11.82 ms and P99 12.87 ms, then exited before the return check. The
lifecycle run had 36 intervals over 16.67 ms in the 600-interval slice.
Collision-specific timing, matched sun occlusion, source parity, Metal
and stable 120 FPS remain open.

```powershell
cmake --build --preset godot-mvp-release --target ps2_port --parallel 10
cmake --build --preset godot-release --target entity_state_bridge_tests track_environment_tests track_postprocess_tests event_world_tests --parallel 8
ctest --test-dir build/godot-release -C Release --output-on-failure -R '^(entity_state_bridge|track_environment|track_postprocess|event_world_original|progress_tables)$'
$common = @('--debug','--entities-project','build/godot-mvp-release/project','--championship-performance','--performance-uncapped','--performance-engine-breakdown','--performance-final-screenshot','--result-timeout','80')
py -3 -B tools/test_event_window.py @common --race-tape-record local/task0119/followup/matched.tape --output local/task0119/followup/record
foreach ($run in 1..3) {
    py -3 -B tools/test_event_window.py @common --race-tape-replay local/task0119/followup/matched.tape --output "local/task0119/followup/replay-$run"
}
py -3 -B tools/test_event_window.py --debug --entities-project build/godot-mvp-release/project --rendering-driver d3d12 --mvp-freerun-performance --performance-final-screenshot --race-post on --result-timeout 60 --output local/task0119/followup/d3d12
py -3 -B tools/test_event_window.py --debug --entities-project build/godot-mvp-release/project --race-clock fixed --clock-1080p --clock-render-thread safe --clock-profile --clock-render-sample-limit 8000 --render-cap 0 --clock-display-sync --drive-route --road-course-driver --delivery-pace --launch-only --drive-steps 5000 --brake-in-pursuit --result-timeout 120 --output local/task0119/followup/elise-lifecycle
```

## MVP vertex reflection and half-resolution focus

Recomp's Retail symbol index identifies CarRenderInfo::Render and the post
attachments. Read-only Ghidra MCP checked Debug 001210F0, 001196D0, 00119FE8,
0011A338, 0011ADB8, 0010F660, 0010F8D0 and 0010FA88. The Debug ELF hash
above and Retail E-0035-R03 and E0052D08/D13 remain distinct authorities.
The Retail VU records at 002A7F90, 002A8308 and 002A8AD8 describe the ray,
base colour and environment calculations. Debug 0011A338 confirms the
focus palette call and the separate jitter offsets and blend.

The MVP uploads raw source normals in CUSTOM0. Its vertex shaders evaluate
the reflected ray, environment UV and truncated alpha. The shader keeps
perspective interpolation for UV and screen-linear interpolation for alpha.
The car selection uses CARSELECTENVMAP and the recovered look-at calculation.
The race uses the shared environment atlas and its capture basis when ready.
Material bindings include source strip flags and the material hash on
reflective surfaces.

E0052D08/D13 establish two different race context-2 packets. Packet 318
uses source alpha. Packet 31C can use destination alpha when flag 4 is
effective, including texture Field_0x74. The frontend mode 3 override uses
source alpha. The MVP applies the new vertex reflection to the supported
source-alpha route, keeps the earlier paint mapping for race packet 31C
and uses the base glass pass for that destination-alpha case. Forward+
still folds the reflection into material emission. The host retains its
base shading, base alpha test, depth and transparent composition. Full
002A8308 lighting, flag-4 second-block RGB, independent context-2 alpha
test, framebuffer alpha updates and GS blend order remain open.

Focus creates a persistent 256x224 target for each active view and samples
a same-aspect fogged source capture with original 512x448-normalized offsets.
Each FIX64 contribution truncates before the half-target sum. The composite
uses the C++ focus palette and skips zero-alpha pixels. Ordinary driving
has zero focus. Inactive and missing views disable the captures. The source
capture rerenders the scene in Godot and retains host sampling and depth.
The monochrome low bit derives from current saturated depth when the default
DepthIntoAlpha pass supplies MainDisp alpha, as documented in E-0052-P05 and
its clarification. MVP derives that value from Godot depth. The full native
MainDisp attachment is still open. Stage 7 remains Filter, RadialBlur,
BlendPrevious. The configured assets still lack filter texture 02C8711C.
The existing conditional history and invalidation stay in place.

### Validation

Godot 4.7.2 ed1daf0bf, Vulkan 1.4.359 Forward+, Intel Arc B390. The Release
MVP extension built. Three existing native tests passed. The Forward+ GPU
probe passed source UV/alpha, glass, disabled focus, FIX64 truncation and
focus composite checks. The synthetic focus probe exercises the production
shaders. It does not assert that a normal race camera activates focus.

Track 12 used the same committed input tape at 1920x1080, 120 warm-up and
600 measured frames, uncapped and VSync off, focus and glow off. Both
runs matched World steps 184-379. Baseline DLL e7fe4165bdd5. Final DLL
31520c354738. Matching state screenshots stay local.

| Metric | Baseline | Final |
| --- | ---: | ---: |
| Average FPS | 170.77 | 143.93 |
| Mean / P95 / P99 frame ms | 5.856 / 9.844 / 10.527 | 6.948 / 11.348 / 12.553 |
| Root GPU P95 ms | 3.738 | 3.965 |
| Mean draw calls | 2351.76 | 2382.98 |
| Native mesh / node creation delta | 0 / 99 | 0 / 99 |

The final sample misses the 8.33 ms P95 target. The increased material
distinctions split some draw batches. This pair does not isolate all causes
of the observed frame-time change. No original-runtime parity is claimed.

```powershell
cmake --build --preset godot-mvp-release --target ps2_port --parallel 8
cmake --build --preset godot-release --target vehicle_base_material_tests track_environment_tests track_postprocess_tests --parallel 5
ctest --test-dir build/godot-release -C Release --output-on-failure -R '^(vehicle_base_material|track_environment|track_postprocess)$'
& local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe --path build/godot-mvp-release/project --rendering-driver vulkan --audio-driver Dummy --script (Resolve-Path tools/test_mvp_race_shaders.gd) --quit-after 400
$match = @('--debug','--championship-performance','--performance-uncapped','--performance-engine-breakdown','--performance-final-screenshot','--race-tape-replay','local/task0119/followup/matched.tape','--result-timeout','75')
py -3 -B tools/test_event_window.py @match --entities-project local/task0119/vertex-focus/baseline-project --output local/task0119/vertex-focus/baseline
py -3 -B tools/test_event_window.py @match --entities-project build/godot-mvp-release/project --output local/task0119/vertex-focus/final
py -3 -B tools/test_event_window.py --debug --entities-project build/godot-mvp-release/project --race-clock source --clock-1080p --clock-render-thread safe --clock-profile --clock-render-sample-limit 2800 --render-cap 0 --clock-display-sync --race-post on --drive-route --road-course-driver --delivery-pace --launch-only --drive-steps 1000 --result-timeout 100 --output local/task0119/vertex-focus/elise-final
```

The final Track 13 window passed 16 checks from the authored Elise Delivery
menu through a bounded drive and return. The local authored-world and
after-route captures cover sunlit road, tree shade and the rear-view image.
The bounded drive did not establish original image equivalence or exercise
every source post attachment.

## MVP color and fog follow-up

Read-only Debug Ghidra checks used the ELF SHA above. `RenderSingleFlailerBitmap`
001428F0 uses the Flailer blend globals, which are initialized to enabled and
GS ALPHA44 at 00327204. Texture-header ALPHA is therefore insufficient for
selecting the normal menu bitmap blend. The MVP shader receives the numeric
tint and raw CLUT alpha. It computes /128 modulation and expands alpha after
modulation. The font atlas and movie paths retain their host alpha conventions.
The menu and HUD reuse their existing nodes and warmed bitmap caches.

The Retail `COLORSWATCH_SILVER` C2FA99BC has raw RGBA (133,128,130,127).
At neutral tint over black, its original Flailer blend gives (131,127,128)
with GS truncation. The old and new MVP window pixels were both
(132,127,129), within one byte per channel. The supplied Recomp screenshot
reads (179,172,175). Recomp uses an UNORM target and its final grade bypasses
default brightness/contrast/gamma 0/100/100. The screenshot's effective
settings and host output transfer were unavailable. The background grid
assets use MODULATE and are on the same corrected raw-alpha path. Exact
grid alignment and an identical Recomp background animation sample remain open.

The Elise plate texture `ELISE_LICENSE_PLATE` 9F959188 has a modal raw value
(156,156,156,127) and a copy texture blend. The 3D route retains the source
strip flags, vertex RGB5, material vectors and light context. The shaders
evaluate recovered Retail VU 002A8308 per vertex with the authored selection
context from Debug `SetSelectCarLighting` 00115BD0 and `elSetupLightContext`
00115D88, or with the bounded in-game track-light adapter. The bridge only
uploads changed light vectors. No separate plate material rule was introduced.
The MVP reflection face texture uses a display-byte output before the atlas
quantizes to RGB5. The prior HDR face binding fed linear values to that
quantizer. A nested Forward+ viewport fixture covers the corrected boundary.
Godot transparent blending and the existing race destination-alpha reflection
approximation still differ from GS behavior.

Fog findings are in E-0052-P01 and the added TASK-0119 view routing note.
World publishes the source Player projection planes through Entity Bridge,
including the source FOV rule for non-driving movers. The depth shader samples
Godot reverse-Z, reconstructs the source PSMZ24 slot and clamps beyond the
source clip interval. The renderer reuses the existing Player fog draw and
palette, with no scene-color copy or new render pass for fog. Rear-view ID 3
is composited after the original Player effects, so the MVP removed the
fog and focus draw from that viewport. Godot fog RGB still uses host
blend precision and only samples depth available to the shader. Original GS
transparent-depth contributions, depth bias variants and byte-domain fog blend
remain open.

Retail Track 13's FOG<MAIN> has palette start 125 and intensity 0.15.
The main driving near/far planes 0.5/10000 give slots 255, 102, 40, 10,
and 3 at 2, 10, 25, 100 and 250 metres. At 10 metres the authored main
palette still has zero alpha. This rules out treating 125 as a distance
in metres. With the previous 20000 m host far plane, the same 250 m
sample used slot 4. The difference between those two far planes is small
for ordinary foreground road. The artificial rear-view fog and the
host's composition differences were separate contributors.

### Validation

Godot 4.7.2 Forward+ / Vulkan 1.4.359 / Intel Arc B390. Windows tools
CMake 3.29.2, Ninja 1.12.0, Python 3.14.4, GCC 13.2.0. The GPU fixture
passed the exact depth-slot probes, source clip boundaries, gray and
half-alpha UI samples, raw high CLUT alpha, tint, reflection atlas,
glass, VU UV/alpha and focus checks with per-channel tolerance 1 for
known byte samples. HUD interpolation passed headless. Native checks
passed 4/4. No PCSX2 was run.

Track 12 matched the input tape at steps 184–379 with 120 warm-up and
600 measured frames, 1920×1080, uncapped. Both runs passed state identity
against the same tape. Baseline was saved before this follow-up.
The final sample includes the changed-light-vector optimization. The
last fog shader range clamp was subsequently checked with the GPU fixture
and Track 13.

| Metric | Baseline | Updated |
|---|---:|---:|
| Average FPS | 164.52 | 173.46 |
| Mean frame ms | 6.078 | 5.765 |
| P95 / P99 frame ms | 10.294 / 11.107 | 9.971 / 10.722 |
| Maximum frame ms | 18.181 | 14.409 |
| Mean Godot viewport GPU ms | 3.752 | 3.412 |
| Mean draw calls | 2382.77 | 2384.93 |
| Mesh / node creation delta | 0 / 99 | 0 / 99 |

P95 remains above 8.33 ms. The single pair does not isolate individual
effect costs. Original display parity, exact Track 13 same-tick
Recomp capture, and a full performance matrix remain open. Local captures,
DLL hashes, reports and the asset probes are under
`local/task0119/brightness`.

```powershell
cmake --build --preset godot-mvp-release --target ps2_port --parallel 10
cmake --build --preset godot-release --target vehicle_base_material_tests track_environment_tests track_postprocess_tests entity_state_bridge_tests --parallel 10
ctest --test-dir build/godot-release -R '^(vehicle_base_material|track_environment|track_postprocess|entity_state_bridge)$' --output-on-failure
$godot = 'local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe'
& $godot --path build/godot-mvp-release/project --rendering-driver vulkan --audio-driver Dummy --script (Resolve-Path tools/test_mvp_race_shaders.gd) --quit-after 400
& $godot --headless --path build/godot-mvp-release/project --script (Resolve-Path tools/test_mvp_hud_interpolation.gd)
py -3 -B tools/test_event_window.py --debug --entities-project build/godot-mvp-release/project --championship-performance --performance-uncapped --performance-engine-breakdown --performance-final-screenshot --race-tape-replay local/task0119/followup/matched.tape --result-timeout 75 --output local/task0119/brightness/delivery
py -3 -B tools/test_event_window.py --debug --entities-project build/godot-mvp-release/project --clock-1080p --launch-only --drive-route --road-course-driver --delivery-pace --drive-steps 1000 --result-timeout 80 --output local/task0119/brightness/elise-delivery
py -3 -B reverse/scripts/generate_progress.py
py -3 -B reverse/scripts/generate_progress.py --check
```

The final Track 13 window passed all 16 launch/input/return checks at
1920×1080 with the debug overlay. Local `elise-delivery/selected-car.png`,
`authored-world.png`, and `after-route.png` show the black Elise, start
area, and sunlit/shaded forest approach with the mirror. This is a bounded
drive and the captures do not establish exact Recomp pixel parity.

## Track 13 start-line fog comparison

The user supplied a PS2 reference at 0 mph and 4:58.53. The former Godot
capture was during countdown. A new stationary window probe captured
before and after at 0 mph and 4:58.43. The mismatch remained at matched
Godot timing and was strongest over dark trees and fencing.

The supplied `D:/dev/hp2_ps2_rev` reference was reviewed across
`src/ecstasy/eenveffectse.cpp` and `src/ecstasy/ecstasye.cpp`. Its
`AttachFog` follows world alpha draws. Its Vulkan backend samples the
main scene depth at that boundary and composes into an RGBA8 UNORM
target. This is an implementation lead. Read-only Debug ELF review
confirmed the original `eDisplayFrame` 00112C60 stage order and
`eAttachFog` 00119B30 palette source and GS blend selectors.
The PC reference's numeric projection and pow approximation were
not used as PS2 evidence. See E-0052-P01 for the original depth and
palette operations and binary hashes.

Godot Forward+ now combines fog, focus and monochrome in one
post-transparent compute dispatch per active view. It reads the current
scene color and depth and writes color in place. The shader projects
the same committed source clip planes and consumes the same authored
palette. GS fog blends quantized display RGB with signed floor rounding
and alpha/128, then converts back to scene linear. The rear-view remains
a separate later composite. No added full-resolution texture or CPU
readback is needed in the production fog route. Existing optional focus
retains its half-resolution capture. Optional history is still a host
approximation and its exact ordering remains open.

The same 1920x1080 start-line patch in the center woods changed from
mean RGB (63.9,62.6,86.2) to (40.0,36.6,39.8). The sampled foreground
road stayed (46.3,44.4,36.0). The nearby fence lost the excessive blue
lift. Comparison to the supplied PS2 screenshot is visual because
projection, aspect and raster are different. Original depth bias and
depth-writing material differences remain open.

The first corrected run captured the intended frame but timed out
during the unrelated pause/exit checks after the harness reported a
2880x1695 viewport. The start-line capture option now exits after
collecting its requested stationary frame. The separate Track 12
matched-tape run reached the end of the tape but timed out awaiting
profile export after the same viewport-size change. The Godot log
reported roughly 0.38–0.47 ms for individual sampled post-transparent
compositor GPU blocks. These are unpaired diagnostic values, not a
matched 1080p performance result.

```powershell
cmake --build --preset godot-mvp-release --target ps2_port --parallel 10
$godot = 'local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe'
& $godot --headless --path build/godot-mvp-release/project --import
& $godot --path build/godot-mvp-release/project --rendering-driver vulkan --audio-driver Dummy --script (Resolve-Path tools/test_mvp_race_shaders.gd) --quit-after 400
ctest --test-dir build/godot-release -R '^(track_environment|track_postprocess)$' --output-on-failure
py -3 -B tools/test_event_window.py --debug --entities-project build/godot-mvp-release/project --clock-1080p --launch-only --start-line-capture --result-timeout 75 --output local/task0119/fog-distance/verified
py -3 -B tools/test_event_window.py --debug --entities-project build/godot-mvp-release/project --championship-performance --performance-uncapped --performance-engine-breakdown --race-tape-replay local/task0119/followup/matched.tape --result-timeout 75 --output local/task0119/fog-distance/performance
```
