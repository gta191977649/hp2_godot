# TASK-0098 production selection roundtrip

E-0098-PB01. Behavior authority Debug NFSHP2.ELF SHA-256
`815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`;
Retail assets/profile/frontend authority SLUS_203.62 SHA-256
`7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`.
Revisions unknown. Read-only MCP captures under local/task0098/shared-target,
SHA-indexed in evidence.json; no Ghidra writes. This is port integration, not Verified.

## Implemented scope

FrontEndRuntime owns SelectionRuntime/SelectionPage across real document entry and
exit. Actual EventTree SetRace/MakeEventCarsAvailable and original asset car/skin
catalogue feed the native selector. Real authored FRONTB screen, widgets, camera,
lighting, asynchronous model requests, named PLATFORM/CARCOVER, input registrations,
nebula pool (shared host RNG persists across page entry/exit), original sound samples and cancellation leases are bound. A worker owns
its source/cache; completion is consumed only by the owning page/job. Disposal joins
workers; it never schedules callbacks into old screen storage. Skin reinitialization
uses the completed cache and actual requested skin resource.

The supported event profile-state adapter is deliberately bounded to untouched
Hot Pursuit tree1 event1/type5 (first delivery). Other event-state layouts retain an
explicit exception. No profile unlocks, synthetic model, direct menu_open in the
acceptance route, or test replacement services are used. First event permits one
car (Elise/type41), so directional car input legitimately retains that one car.
Swatch input changes original skin IDs. Fast multi-car cancellation is not covered
by this single-car acceptance and remains in the page/native tests.

Back runs original SetSelectedCars and page close/preload/audio/resource sequence;
fieldwise reviewed settings/current-mode ride fields are committed. Event details
state2 is persisted at the original transition completion, restoring details after
Back. Original001F5E28 and existing Retail001FA698 transition evidence agree.
Debug001F3630/001F3460 build the four-slot unlock-map entries from child prerequisites.
Retail00198518 loads player points0030CE54 against profile base002FC050 (+10E04),
with player stride6D0; no whole-profile overlay is inferred.

## Shared device target

A port-owned RenderingDevice executes one ordered vertex/draw packet into a single
RGB/RGBA colour attachment and depth attachment: main solid stage0, background UI
stage1, platform/effects stage2, reflected solid and main alpha stage4, combined
native alpha sort stage5, foreground UI stage7. Reflection flags0x11 exclude sorted
strips/environment. Native transformed strip AABBs and texture RenderingOrder feed
001053B8. Alpha's frontend default pack1 follows0010BBF8. GS bit-packed depth and
Flailer layer depths use existing native reference functions. Original material
lighting/reflection math supplies vertices; original texture bytes supply sampling.
CarScaleMatrix is identity at0011DE18 (existing E-0098-R01 capture).

Fresh Camera001566E0 and CreateViewMatricies0010EA18 evidence corrects the initial
unadjusted-near projection: CM_SELECT_CAR=11 takes view1 FOV-adjusted near/far;
baseline FOV0x238E is written at001149FC, far is static00324F90. The camera's
published FOV is used. Original startup globals are read from the SHA-bound image.

The final scanout consumes RGB, so the host canvas image is RGB8. Treating GS target
alpha as host canvas transparency previously hid the already rendered vehicle;
that state-only green run is explicitly rejected. One diagnostic geometry-only
image established the cause; that extra render/hardcoded output was removed.

Current transport uses local-device synchronous readback and an ImageTexture upload.
It has NOT passed the Release16.67ms contract. Float GPU depth, texture filtering,
blending quantization, complete edge clipping, target/multisample variants and final
original image/audio equivalence are unverified; this is not GS rasterizer certification.
Unknown second-pass320, locked rectangle publication, Help, orbit speech/audio setters,
transmission/handling popups, race continuation and later event-state adapters still
throw explicitly. They are outside the tested enter/cancel milestone.

## Reproduce

```
cmake --build --preset godot-debug --target ps2_port -j 6
py -3 reverse/scripts/check_frontend_selection_roundtrip.py
cmake --build --preset native-debug --target selection_page_tests frontend_composition_tests frontend_actions_tests ultimate_event_tests race_parameters_tests --parallel 6
ctest --preset native-debug -R '^(selection_page|frontend_composition|frontend_actions|ultimate_event|race_parameters)$' --output-on-failure
```

Pinned Godot4.7.2, Vulkan/Forward+ Intel Arc B390; Python3.14.4, CMake3.29.2, Ninja1.12.0, MinGW GCC13.2.0. Runner records exact executable/DLL hashes, isolated
absent input profile/save output, command, result and screenshots per fresh run.
GDScript injects Input.parse_input_event into actual frontend_native.tscn. It waits
for loaded native car, zero fade and shared GPU output; checks swatch input; rejects
blank central vehicle region with a port smoke threshold (>1000 pixels above0.06,
ROI100..549/150..379). That metric is presence detection, not original-image tolerance.
Three full enter/cancel/reentries must pass, with no native tick/render/input errors,
script errors, crash, leaked-RID warnings, missing screenshots or stale checkpoints.

Native five regressions passed5/5 in5.92s before final projection refinement; final
results and the final fresh run ID are appended below. All tasks remain active;
no full frontend/career, original certification, resource/performance or Verified claim.

## Final acceptance,2026-10-28

PASS `py -3 reverse/scripts/check_frontend_selection_roundtrip.py` after final
shared-RNG/projection/scanout/error-reporting changes: fresh run-xci09xbl, exit0,
three actual enter/loaded-car/cancel/reentry cycles, swatch change assertion,
visible-vehicle pixel checks, no runtime errors/crash/RID leaks. Private manifest,
result.json and select-car-0/1/2.png preserve commands/hashes/checkpoints/images.
Native final rebuild passed; eight selected regressions/contracts passed8/8 in7.64s:
`ctest --preset native-debug -R '^(selection_page|frontend_composition|frontend_actions|ultimate_event|race_parameters|progress_tables|recovery_progress_contracts|track_optimization_performance_rules)$' --output-on-failure`.
Godot Debug DLL normal rebuild passed. No staging/commit or Verified promotion.

Continuation: TASK-0098 remains active. The specific first-event enter/cancel
production milestone is accepted. Next recover/bind selection Help/locked text,
orbit speech/audio and transmission/handling popup/continuation, then reviewed
remaining event-state adapters. TASK-0099 real race and all32 event certification
remain separate unfinished work. Do not regress to the old001F6A28 entry guard.
