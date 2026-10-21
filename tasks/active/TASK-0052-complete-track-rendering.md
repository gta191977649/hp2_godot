# TASK-0052 - Original complete track rendering and performance

- Status: active
- User-approved: original PS2 effects only; full single-car scene visual stages;
  shared TrackViewer/Test drive/race pipeline and session-wide Effects controls.
- Performance target: current Intel ArcB390 machine,1920x1080,60FPS, original
  default effects/view distance/LOD; separate optional debug distance override.
- No AI/ranks/race rules/splitscreen or Godot physics; Ghidra read-only; Retail
  behavior authority and explicit Debug naming/driving boundary. Preserve edits.

## Acceptance

- [x] Close TASK0051 Debug failures and Release windowed entry; freeze baseline.
- [ ] Correct Roadshine end-to-end from original framebuffer/texture/VU evidence.
- [ ] Complete original RenderStage/targets, sky/sun/flares/shadows/environment.
- [ ] Depth-alpha, Fog/Focus palettes,Fog,DOF,filters/radial and LUT distinction.
- [ ] Original weather/particles and necessary visual update dependencies.
- [ ] Shared Effects group, availability/active state/default restore and visibility.
- [ ] Native original visibility/LOD/resource scheduling, indexed/cached host;
  explicit CPU/GPU timing and fixed-route1080p Release measurements all30 tracks.
- [ ] Original/default and override regressions, parity,fullDebug/Release builds,
  complete tests/window captures,successor audit, evidence/docs updated.

## Ownership

REA step6: Roadshine worker owns existing material/effects shaders and baseline
GDScript/tests; postprocess worker owns original environment-effect TU/new port
postprocess/shader helpers; environment worker owns assets and new sky/sun/weather/
environment primitives; root owns runtime indexing, hostGPU/frame composition,
UI/settings, shared builds, benchmarks and final integration. No overlapping edits.

## Baseline

TASK0051 fullDebug96/98 plus corrected2/2; ReleasewindowedracePASS. Measured
baseline fixtures are local/task0052/track_baseline.gd and baseline-{debug,release}.json.
Detailed original evidence remains reverse/task0051; successor records go task0052.
Required paths stay incomplete until closed; PC checks do not establish Verified.

## Integration checkpoint (2026-10-18)

Confirmed evidence and implemented slices:

- E-0052-D01..D10: Roadshine packet/VU/texture/framebuffer trace and byte tests;
  no evidenced brightness coefficient defect. No empirical attenuation was added.
- Retail `CalculateH` always uses the original global512 width. Main and capture
  LOD now use that recovered calculation instead of the desktop viewport width.
- Indexed lookup, lightweight ordered sorting and fixed traversal stack preserve
  complete ordered commands across all30 asset tracks in the native comparison.
- One public renderer submits main and original scheduled six reflection views;
  the original target sizes are64 faces and one256 atlas. Per-draw PSMCT16 RGB/Z
  precision remains an explicit host gap, not an equivalence claim.
- Shared Sky/Fog/Focus/shadow/capture resources and session permission gates are
  integrated. Sun host submission/readback and AcidFX are still being closed.
- Stage5 and7 native palette/pass calculations and GPU arithmetic probes cover
  Fog, Focus jitter/composite, filters, radial/history and framebuffer BW.
  Normal player camera focus starts0 and PerfectFocus1 preserves that branch.
- Every shipped weather region in the30-track sweep is Fog (Type1). No rain,
  snow or wind is created to fill inactive original paths. World Acid emitters
  exist and their spawn/update/render closure is still required.

Validation completed at this checkpoint:

- `local/task0052/core-integration-tests.log`:5/5 pass, including all-track
  runtime assets, independent race entry and headless frame parity.
- `local/task0052/visual-core-tests.log`:6/6 pass, including environment,
  postprocess, controls and frame parity.
- `local/task0052/integrated-render-tests.log`:4/5 initially; the remaining
  fusion test lacked its numeric fixture in the isolated staging root. Adding
  the read-only reverse directory junction and rerunning passed1/1 in
  `fusion-integration-rerun.log`. This was a staging failure, not a GPU mismatch.
- Shared Debug build with native LOD completed in
  `local/task0052/build-debug-native-lod.log`; newer Sun/BW/particle additions
  need another complete build and regression.
- Cached postprocess browser fixture improved to Debug median17.46/17.27/20.21ms
  at the three fixed poses. These measurements exclude a live car/captures.
- The real runtime Track11 route (car, captures and existing shared effects)
  is NOT at target: Debug median80.91ms, P95 240.92ms,6293 median draw calls.
  See `runtime-debug-route.json`; Release all30 acceptance is still outstanding.

Next exact actions: finish conservative contiguous GPU batching and shader
prewarming; integrate Sun readback and the actual AcidFX lifecycle; run isolated
Debug/Release builds, all30 default routes, actual window/control checks and the
new TASK0052 source-review successor. Keep this task active until all acceptance
items have evidence. The staged DLL has not been promoted to the normal project.


## Validated integration checkpoint (2026-10-18, subsequent)

- Read-only Ghidra MCP connectivity restored at127.0.0.1:8080/mcp. Retail
  CalculateH00104448 fresh decompilation succeeds. E07 particle constants and
  Retail DIV.S00128C64 are now evidenced; no inferred epsilon/modern particles.
- Debug and Release staged builds pass, including Sun, static WorldEffects,
  shared alpha ranking and cached postprocess depth proxies. Original insertion
  order is scenery, Sun, FrameEffects, WorldEffects. The represented slice omits
  FrameEffects/car keys explicitly. Different alpha-depth keys are not batched.
- `full-debug-nongpu.log`:99/99 pass. `window-regression-rerun.log`:6/6 pass,
  including independent race, sky negative control, numeric postprocess,
  compositor ownership/reload and viewer/runtime GPU parity with real road-depth
  coverage. `particles-window-test.log`:1/1 pass for actual Track11 static emitters.
- Real ImGui click test initially exposed missing input forwarding in race_scene;
  fixed _input through DebugTools and clear held driving input on UI consumption.
  `effects-ui-window-final.log`:1/1 pass for Roadshine toggle and visibility
  override/restore. An earlier test teardown changed scene before quit; corrected
  the fixture to release resources directly. No application workaround retained.
- `full-release.log`:92/93 pass initially, remaining class-template symbol-link
  metadata stale after80 new symbol entries. Regenerated class inventory and
  progress; `release-regeneration-rerun.log`:4/4 pass including the failed check
  and runtime/controls. Native/host focused Release checks7/7 pass separately.
- Explicit source successor now records23 replaced and218 resulting identities;
  approved only for bounded source/evidence scope, not task completion or original
  equivalence. Source audit passes;13 negative preservation tests pass. Historical
  TASK0051 review remains frozen. All new knowledge records stay Decompiled.
- Sky far-depth sign was incorrect on Compatibility and overwrote opaque cars.
  Production host uses CLIP_SPACE_FAR*w. Actual car off/on screenshots and an
  opaque/transparent foreground negative-control GPU fixture confirm the repair.
- All30 native WorldEffects resource/simulation/render packet probes pass180steps
  using actual original math and carried RNG. There are94 definition texture
  references, none missing. Static runtime particle input is separate from physics.

Performance remains incomplete. Track11 old Debug route80.91ms/6293draws improved
first to53.40ms/3306draws; corrected cached Release route with300 samples is37.304ms
median,46.974msP95,69.383msP99,3313draws. The all30-track default1080p Release sweep
is currently running to runtime-release-all30.json. No60FPS claim is justified.
The normal application DLL has not been promoted from staging.

Remaining exact work: inspect all30 timing/gap rows, reduce actual renderer/draw
submission cost and prewarm real shader/pass variants; implement destination-alpha
vehicle31C with correct alpha lifecycle and primitive order, connect unsorted AcidFX
solid-chain texture3682853782 on tracks24/25/26, complete shadow fallback13 and all
remaining original stage/activation dependencies. Unsupported weather/filter/LUT
paths remain explicit, not counted as completed visual recovery. Rerun affected
checks after any further change and preserve the original baseline/audit chain.


## User-requested build delivery (2026-10-18)

User explicitly deferred60FPS work and requested a passing build. Stop performance
optimization and deliver the current bounded implementation. TASK0052 stays active
for original-pipeline gaps and deferred performance; build delivery is not a claim
that every original effect is recovered. The in-progress30-track measurement was
stopped at the user's request; completed route rows remain in the ignored local
report. No further optimization changes were applied during that measurement.

Normal godot/bin/ps2_port.dll is promoted Debug/ImGui-enabled output. Debug CMake
cache now targets the normal godot project/bin again, with GPU test registration
OFF; actual GPU regressions were already run against the same source. Preserved
Debug/Release DLL copies and launch shortcuts are under local/task0052/delivery.
The prior normal DLL is backed up there. Release remains runnable through its
isolated project to avoid replacing the Debug DLL. No Git commit was requested.

Validation: staged Debug99/99 nonGPU checks,6/6 window regressions, real particle
and ImGui click fixtures pass. Release full rerun had only generated progress-log
contents stale after the new checkpoint heading; regenerate before final rerun.
The exact final commands/results are recorded in delivery/BUILD-INFO.json and
local/task0052 logs. Final normal-project runtime/UI smoke and Release full suite
are the delivery checks; retain their results without promoting original Verified.


Current work switches to TASK-0053 for the user-reported runtime regressions.
Strict60FPS optimization remains deferred; retain this incomplete parent scope.
