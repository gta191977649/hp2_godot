# TASK0052 environment recovery

## E-0052-E01: Retail environment reflections

Authority: SLUS_203.62 SHA256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2. Names and original TU ownership from Debug NFSHP2.ELF SHA256 815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b and ECOFF source ownership. Original TU `src/ecstasy/eenvmape.cpp`.

Paired addresses: ComputeReflectionST Retail001179B0/Debug00117D78; CreateRefSampleTable 00117A38/00117E00; CreateEnvMap00118150/00118518; CreateBackEnvMap00118498/00118860; eCreateEnvMapPrims00118810/00118BD8; eEnvMap::UpdateCameras001175C8/001179C0. Read-only decompilation and original initialized direction/up/schedule data cached under local/task0052. Typed host values replace target pointers, with no ABI reinterpretation.

The original samples a 6x6 populated portion of 7x7 storage. Step0.4, back stretch0.16. Five front faces and four curved back patches produce the final 2D reflection image; this is not a generic cubemap. Each row strip has12vertices. Screen coordinates retain original integer truncation. Inactive source faces and destination content must remain persistent across frames.

Retail schedule differs from Debug and is implemented from Retail: viewmodes1/2 update faces {0,1,5},{0,2,5},{0,3,5},{0,4,5}; other modes update one face per six frames. Cameras use original FE/IG direction/up tables, FOV0x75E4, near0.5, far10000. Local host tests cover these values and topology, not original visual equivalence.

## E-0052-E02: Weather

Original TU indep/src/weatherman.cpp. LoaderWeatherMan Retail001A8258/Debug001A5A38: chunk34250, payload aligned16, version2 at+8, count+12,128-byte records. Record offsets: type80, packedRGB84, FogStart88, Parameter1 92, position96, radius108, intensity112, FarFalloffStart116. Name/group hashes8/12, fixed strings16/48. Only unsigned types below4 enter active lists.

WeatherManQuery Retail001A84F0/Debug001A5CD0 paired decompilations agree on four ordered lists, nearest hard region, up to three positive squared-distance falloff regions, and normalization. Fog weights each hit by its influence. Rain/snow/wind hit intensity uses only the shared normalization scale, without each influence; that apparently unusual original operation is deliberately retained. No arbitrary clamping was added. Debug tweaker overrides and mutation of region activation are outside the immutable asset query binding.

## E-0052-E03: Sky

Original TU src/skye.cpp. SkyInitModel Retail0011C450/Debug0011C7C8; SkyLoader0011C5F8/0011C970; StuffSky0011C6F8/0011CA70. Main hashBE43EDBB; environment hash72923C81 falls back to main. Search compares SceneryInfo.NameHash[0], preserving original section-list order (sections>=90 insert at front). Matrix uses raw signed16 rotation /16384 plus original position even for scenery flag200. SkyInitModel uninitializes that scenery info's three ordinary models; host `sky_instances` identifies submissions to omit. StuffSky chooses main for viewIDs1..3 and environment otherwise. This bounded binding does not claim target allocator/model object ABI restoration.

## E-0052-E04: View depth

CreateViewMatricies Retail0010E638/Debug0010EA18 rescales near only for viewIDs1/2 whose mover type is neither CM_DRIVE1 nor CM_BUMPER5. With original table sine/cosine, near*=tan(0x238E/2)/tan(FOV/2), far preserves original span. Nonzero forceNear/forceFar apply afterwards. Camera constructor uses0.5/10000; drive in-car near0.4; sky stage forces100/50000. `original_view_depth` is an explicitly bounded port projection of this branch, not a new original function owner.

## Validation

Private MinGW C++20 test src/tests/track_environment_tests.cpp passes TRACK_ENVIRONMENT_OK: hard and falloff fog, outside, reflection primitive dimensions, Retail frame schedule, sky fallback/rawmatrix, truncated input. Syntax passes new TUs. Original numeric/frame comparison remains unverified. All30 dependency loading and GPU integration are tracked by the parent task.

## E-0052-E05: Sun

Original src/sune.cpp. Retail LoaderSun00124F30 instruction listing establishes chunk34202, aligned16, version2,244-byte SunChunkInfoE. Debug001250B0 names/layout corroborate offsets: position4,shadowposition16,layers28; six36-byte layers contain texture0,alpha type4,intensity8,size12,offset16/20,colour24,angle28,sweep32. Enum names independently read from imported types: CENTER0/HALO1/MAJORRAYS2/MINORRAYS3/RING4, BLEND0/ADD1. SunTrackLoader Retail001251C0/Debug00125340 installs five hashes and RenderingOrder7.

StuffSun Retail001252B0/Debug00125430 post-projection branch generates six rotated screen quads. Native input includes world-screen projected sun position, positive view Z, viewport, original ZMax, and actual uncovered depth samples from the32x32 readback (24-bit depth<2). Occlusion alpha is count/1024. Negative layer intensity bypasses occlusion with negative intensity*128; positive intensity uses coverage*intensity*128, byte conversion preserved. First layer depth1, other layers ZMax. Only viewID1 is eligible. StuffSunGlare is an upstream distinct call and is not approximated by this bounded packet generator.

## E-0052-E06: Car shadow helper island

Original source owner `src/care.cpp` is established by ECOFF markers for all four functions, not inferred from temporary compiler filenames. Paired entries: convex_hull Retail001209F8/Debug00120BF0; smooth_shadow_corners00120C00/00120DF8; sh_Setup00120D60/00120F58; ch2d00120980/00120B78; make_chain00120850/00120A48; cmpl001207A8/001209A0; cmph00120828/00120A20.

The helper chain retains comparator order, two sorted chains, topology elevation rejection, and three smoothed vertices per original hull corner using exact original0.3330000042915344 constant. `sh_Setup` uses SunInfo.CarShadowPosition or original(-16384,0,7000) fallback, angle floor0x11F8, table trigonometry, and original Manhattan XY redistribution.

Retail and Debug shadow thresholds differ: Retail convex_hull rejects elevation difference>4.0 and absolute plane distance>=1.5; Debug uses2.5/1.0. Retail caller bias0.06 differs from Debug0.04. Implementation uses Retail thresholds. Cached paired responses and raw constant reads establish these differences; never map constant addresses across versions merely by GP delta.

The bounded live static-track shadow branch is now a typed CarRenderInfo::Render overload in original src/care.cpp. The port supplies actual raw CV vertices in geometry space, GeometryPosition/localWorld, original topology, current LOD/projected size and decoded SHADOWRAMP/prefix_SHADOWIG texture resources. No collision COG shift or generated box is substituted. Fancy projection preserves Retail0.85XY scale,0.275Z offset,0.06 ground bias, hull/smoothing and original LightScale alpha/edge rules; fallback preserves the original4x4 wheel-plane/elevation grid and0.04/0.08 offsets. Native fallback geometry tests pass. TrackShadowRenderer consumes the native strips using original texture/blend state. The complete fallback eligibility PartCuller13 gate and additional eView::BiasMatrixForZSorting raster adjustment remain unresolved and explicitly gated; host/source equivalence is not Verified.

## Host sky adapter

`platform/godot_bridge/track_environment_renderer.cpp` consumes uploaded native mesh/texture/mip callbacks. It calls original StuffSky and eModel::Render with NULL lighting context, preserves fixed raw native model matrix, suppresses automatic host occlusion/frustum culling, and uses the existing native track shader. Original sky submission writes no depth and precedes world; the host uses lowest transparent priority and background depth to preserve this ordering relative to ordinary opaque world. Fragment view depth retains original100..50000 sky interval. This host depth adaptation is explicitly not a bit-identical GS buffer implementation. Shader runtime compilation and actual frame inspection remain pending parent validation.

All30 actual assets loaded successfully after environment extension: every track resolves main and environment sky and two ordinary-scene sky instances; weather regions count5..76. Private stdout local/task0052/assets-all30.log. The exact weather query at origin produces nonzero fog0.12..0.65, demonstrating raw region data propagation. Native tests additionally pass original hull/smoothing dimensions.


### Sun host projection and depth correction (TASK0052)

The host sun uses the original 512 by 448 pixel coordinate system. Host camera projection is normalized into that coordinate system before layer sizes, offsets, sweep and the 32 by 32 occlusion rectangle are evaluated. The tiny GPU readback samples the corresponding normalized coordinates in the host depth texture; it never downloads the full viewport. Its source dimensions must equal the actual host viewport. Completion is explicitly scheduled at frame_post_draw, and the next update consumes the completed cached rectangle.

Cached Debug ePoly::Render0010FE48 proves the CPU SunPoly packet writes integer vertex Z shifted by four into packed XYZF2; it does not use the world VU floating screen-depth encoding. Layer zero therefore has integer Z1. Other layers use the render target ZMax. Retail UpdateGifTags00105AC0 selects GP-0x8020 for mode0x31, and bytes at0032D910 are FFFF7F4B (float16777215). Cached eDisplayFrame initializes the in-game targets to mode0x31. Host fragments explicitly compare this integer layer Z against the raw GS Z24 depth source with GEQUAL; host depth testing is disabled only because this comparison is performed directly. No sun layer writes depth. The former provisional ZMax4 was incorrect and has been removed.

The ePoly Render tail routes unsorted textures to each active texture DmaDataList; sorted headers instead create alpha keys using vertex Z. SunTrackLoader sets RenderingOrder7, but the cached frame decompile is truncated before the full attachment sequence. Final host priority remains pending the parent frame-order review. This is an explicit unresolved ordering dependency, not proof that call order alone equals draw order. The live read-only Ghidra endpoint refused connection on resume; no new binary interpretation was substituted. Host C++ syntax check passes; actual GPU compilation is pending root integration.


Sun ordering dependency resolved after Ghidra resumed: full Retail eDisplayFrame00112858 calls StuffSun001252B0 at00113284, attaches ordinary solids in stage2 and alpha in stage5. The original Sun textures are all alpha-sorted: direct native Hp2TpkArchive scan of all30 resource pairs finds150 Sun texture records with Field4D1, Bias0, semitransparency1, Field740. Thus StuffSun call position does not place the Sun before world alpha. Original RadixSortAlphaKeys groups by reverse RenderingOrder bucket and ascending depth, then SortAlphaKeys reverses the buffer into the emitted list: RenderingOrder7 follows lower orders; equal-depth Sun keys reverse insertion order. First layerZ1 follows the other layers atZMax. Host Sun material priority is now127 minus surviving layer insertion index, preserving that reversed six-layer order after track alpha. Cache identity includes insertion index to prevent repeated hashes from overwriting material priority. An unexpected unsorted Sun header fails explicitly. Raw captures sun-render-order.json, sun-full-display-debug.json, sun-SortAlphaKeys.json, sun-RadixSortAlphaKeys.json and asset log sun-headers-all30.log remain ignored.


## E-0052-E07: Original static-world AcidFX lifecycle (partial closure)

Retail TriggerEmitter00127700 / Debug00127858 and Retail DoTimeStep00128718 / Debug00128870 establish the static emitter RNG, volume/direction generation, countdown, compressed particle creation, cycle scheduling and integration branches. AddAutoEmitter Retail001274A0 / Debug001275F8 initializes randomized cycle duration and inserts state0/1 emitters at the list head; dormant/unsupported states go to its tail. Traversal stops after reaching the first state>=2. All represented behavior lives in typed overloads of the original AcidEffectSystem class in original src/acidfxe.cpp; ParticleSimulation is explicitly the port-owned allocation/list/transaction adapter.

Both original afxInit entries (Retail00126D70 / Debug00126EC8) allocate1000 particle slots of32 bytes,400 active-group slots of28 bytes and600 emitter slots of192 bytes. ParticleSimulation implements the WorldEffects static-emitter slice with those shared particle/group bounds, without approximating native pointer layouts. Its global RNG is supplied by the caller and must persist across track unload/load; initial bytes27579823 at Retail0032FD20 / Debug00326D78 decode0x23985727. TriggerEmitter advances xorshift17/13/5 and uses original single-precision scale2^-31. Raw paired constants at Retail0032DB88 / Debug00324C7C prove InitialAngleRange scale182.04444885253906 and speed cap65535; these were not inferred from degrees or integer range.

Particle lifetime remains signed16 ticks of1/1024, velocity is uint16 speed/128 times int8 direction/127, inherited motion is uint8/255. The native per-frame order triggers all eligible emitters, transitions cycles (retaining overshoot), then integrates all active groups, including newly created particles. The adapter commits cloned state only on successful completion. A non-progressing emission interval fails explicitly instead of hanging or inventing a different emission rate. The native original-pool-full branch continues RNG/countdown while omitting allocation.

All30 resource weather regions are exclusively type1 fog; actual assets contain no rain/snow/wind regions. Static world AcidFX emitters do exist (fire, smoke, waterfall and other original named definitions). RainSystem::Update Retail0012A028 / Debug0012A180 explicitly disables its four camera emitters before stepping. Neither observation licenses a modern rain generator. World particle visual packet submission remains pending: cached original RenderEffects Retail00128DA0 / Debug00128EF8 and hermite_parameter Retail00126AF8 / Debug00126C50 establish the next dependency. No generic Godot particles are substituted.

Validation: private fresh native executable prints PARTICLE_ORIGINAL_MATH_OK and TRACK_ENVIRONMENT_OK with --assets. The original Debug math image drives a controlled10-per-second emitter:0.11sec produces2 particles, life912 after same-frame integration, exact22 xorshift transitions, then no new spawn with all three interest points beyond FarClip. Existing gravity/inherited-motion/compression tests and auto-emitter transition/overshoot tests also pass. PC tests establish internal behavior only; original-runtime equivalence remains unverified.

Sun asset-order bound: fresh load_scene scan of every geometry surface's resolved texture on all30 tracks finds zero sorted world textures with RenderingOrder negative or>=7. Consequently no current world alpha shares the Sun bucket7, and the host Sun priorities122..127 can follow the current world alpha priorities. The ignored probe source and log are sun_world_order_probe.cpp and sun-world-orders-all30.log. New assets outside this bound require shared alpha ranking rather than assuming Sun always follows world.


E-0052-E07 counterevidence: the first real all30 emitter stress run failed on Track24 lightning with Speed0/Gravity0. Debug decomp suggested VRSQRT, but the authoritative Retail listing proves SQRT.S00128C60 then DIV.S00128C64 with numerator1. The existing TASK0038 approved port-side EE zero-denominator DIV.S carrier therefore applies at this additional confirmed instruction site; no new VU ISA assumption is needed. The original finite maximal quotient multiplies zero velocity to zero, preserving stationary lightning instead of propagating host IEEE NaN. A native no-force/zero-speed regression test covers this path. The first failed run and corrected rerun are explicitly different observations; no original game epsilon or forced direction was added.

GPU adapter scope: TrackParticleRenderer prepares native screen packets, binds original texture metadata and Q1 particle ST, preserves original per-group keys for the root common alpha queue, and hides nodes until common ranks are supplied. Original source order is scenery, Sun, FrameEffects, WorldEffects (Retail frame calls00113284 Sun,00113378 FrameEffects,00113388 WorldEffects). Six actual particle texture records on tracks21..26 are order0 copy and Z-writing; their custom shader reconstructs original packed native Z and projects it into host DEPTH. It does not sample its own raw-depth source. Nonwriting particles retain raw Z comparison. Host floating depth precision remains unverified; actual GPU validation is required before claiming this branch closed. Three texture records on24/25/26 are unsorted additive and still fail explicitly because their solid texture-chain submission is not connected. All94 original particle texture references resolve across selected track/shared archives, with no mips. Main-view presentation delta is supplied externally, never an invented fixed original timestep; the host exports/imports the persistent original global seed across renderer recreation.


Native static-world particle stress validation completed all30 tracks after the confirmed DIV.S correction:180 steps of1/60 per track, each using actual decoded definitions/emitters, first three emitter positions as explicit interest inputs, original Debug math and one persistent global RNG across the sweep. It checks original1000-particle/400-group bounds and finite positions, and invokes the restored RenderEffects packet branch through a controlled projection service. Every track passes; cumulative packet counts are native structural checks, not a claim of matched original images. Log local/task0052/particle-simulation-all30.log and ignored reproducible source particle_simulation_probe.cpp. A separate original-math unit checks exact22 RNG transitions for2spawns, signed lifetime ticks, compressed velocity, group-depth key,7/8 pixel anisotropy, contact-sheet UV, clamped cubic RGBA and native depth bits.


Read-only shadow follow-up (source unchanged during validation freeze): eView::BiasMatrixForZSorting Debug00107310 / Retail001074B8 scales a model about the current camera, not by adding a depth offset. Debug caller listing00121700 performs DIV.S f20,f20,f20, then passes f20 at00121738 and00121748, so ordinary finite/nonzero input supplies1. Exact camera-subtract/add rounding is still absent from the host packet; no guessed raster offset is justified. Debug imported enum establishes index13 as CULLABLE_CAR_PART_SHADOW. Its global table is initially zero BSS, then _GLOBAL__I_CarPartCullingPlaneInfoTable001246A8 fills one plane normal(0,0,-1), zero distance and ALL_MUST_BE_VISIBLE polarity. This is a lead toward the eligibility closure, not an implemented fallback gate: per-car InitPart position ownership and Retail initialization correspondence still require proof. Existing fallback remains explicitly gated.

Parent actual-window result: Track11 original worldparticle GPU test passed13.12sec with active/visible packets and checkbox-caused image changes. This establishes functioning real data/GPU submission, not original-image equivalence. The first forestfire camera was inside an actual tree trunk; raw scenery AABBs confirm immediate blockers. Test-only camera relocation is separate from original rendering behavior. No original gameplay position is altered by that visual test.


### Integrated host checkpoint and counterevidence (2026-10-18)

The earlier statement that WorldEffects packet submission was pending is superseded
by the integrated TrackParticleRenderer. It consumes original RenderEffects packets,
keeps original native integer Z and common alpha keys, and receives presentation time
once per main frame. GPU nodes do not run a Godot particle simulation. The original
static RNG is exported before renderer reset and reused by the session after loading.
All30 actual definitions pass180 native simulation/render steps; actual Track11 GPU
packets change pixels when the shared permission is toggled. The camera in that first
capture is against a tree trunk; this is packet/permission evidence, not a claim of
complete visual equivalence or a clear forest-fire reference image.

The geometry-only Sun bucket scan excluded AcidFX. Particle textures DO use bucket7,
so the earlier fixed122..127 host priority bound is insufficient after particles are
integrated. Current main submission reconstructs scenery insertion order, appends Sun
then static WorldEffects, and runs the original alpha_order_001055B0 across all these
keys. Retail eDisplayFrame calls StuffSun at00113284, FrameEffects RenderEffects at
00113378 and WorldEffects RenderEffects at00113388, after StuffScenery. Unrepresented
FrameEffects/car-key producers remain a gap. Identical alpha-depth batches preserve
adjacency; differing depths cannot merge across a new producer's intervening key.

Writing particle depth shaders retain their original packed integer Z and reconstruct
host depth separately. Their depth variants emit that integer directly, instead of
interpreting screen-coordinate vertexZ as world viewZ. Nonwriting particles compare
the explicit GS-depth texture; writing variants use hardware depth to avoid sampling
their own proxy output. Actual unsorted additive texture3682853782 on tracks24/25/26
still throws at the original RenderEffects boundary until its solid-chain submission
is represented. The fixed-route start-camera performance probe may not trigger it.

The host sky far-depth sign was wrong for Compatibility: +w put it in front of opaque
cars. Current code consumes sky_far_depth_vertex using CLIP_SPACE_FAR*w. E0052D15
contains actual car captures and a GPU negative-control test that reproduces the old
opaque-only occlusion. This is a host fix; no original sky behavior was invented.

TASK0059 superseding host note: source-over high-alpha guard still throws
NotImplemented, but its per-quad port boundary reports/omits unsupported packets
instead of tearing down unrelated scenery. Atlas bounds are per animation cell
plus sampler neighbors. No original simulation or full blend-compositor recovery
claim; see E0059P01 and completed TASK0059.
