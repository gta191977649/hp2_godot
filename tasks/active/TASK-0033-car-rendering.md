# TASK-0033 — Vehicle exterior rendering and native ImGui viewers

- Status: active
- Target: SLUS_203.62; revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
- Scope: original BIN exterior geometry, textures, static parts/LOD, complete original car/skin catalog, base materials and native ImGui CarViewer/TrackViewer controls.
- Excluded: handling, physics, dashboard, dynamic damage/effects, shadows and reflection passes.
- Preserve existing console/viewer work and user commits.

## Acceptance criteria

- [x] Evidence-backed bounded geometry/texture/parts/LOD slices and whole-entry failures in appropriate native classes.
- [x] Original 64-car/235-skin directory, ID selection, shared resource mapping and missing-resource diagnostics.
- [x] Native material parameter/VU numerical projections, explicit unlit debug override.
- [x] Shared ImGui session, native viewer controls/FPS, deferred requests and console-independent Debug input.
- [x] Base material parameters, initialized frontend light/VU operation and supported static GS alpha range.
- [x] Full native/Godot Debug, console-off Debug and Release validation; default Debug/on restored.
- [x] Evidence/symbol/template/progress synchronization and final validation log.
- [x] No Verified promotion without original appearance comparison; that comparison remains outstanding.
- [ ] Whole render entry/partial-view cone and clipping recovery; explicitly outside the tested base slice.

## Confirmed findings

E-0033-G01..G05 recover bounded geometry/VIF/topology/normals/VU material slices.
E-0033-B01..B15 cover CarRenderInfo receiver provenance, static assembly, LOD,
texture/material/UV rebinding, frontend lighting and catalog resource text.
E-0033-T01..T07 cover shared archives, original texture lookup, raw palette alpha,
MODULATE/filter/clamp/TEST and supported GS ALPHA equations.

Original samples 911TURBO/BMWM5COP/4DR_SEDAN load real runtime resources.
Full native catalog probe retained 64 cars/235 skin associations: 228 available associations
loaded without failures; 7 unavailable associations belong to DIABLO/DIABLOHP (IDs 45/46).
Missing entries remain visible and never substitute another resource.
No original hardware/PCSX2 appearance comparison; no Verified status.

## Scope and remaining recovery

The VU/base-material and static GS state slices are implemented. General flag4 source-over
can exceed alpha 128 and needs ordered destination access; it remains an explicit failure.
Corpus inventory confirms this is not a necessary dependency for the 228 available static
car/skin associations: source-over strip4 is clear, texture74 is zero, frontend vertex alpha
is 128 and raw texture alpha<=127. Do not describe the synthetic alpha 192 fixture as an
observed current exterior failure. Full original render entry, partial-view cone/clipping,
extra post-base passes and original-output appearance comparison remain outside the tested
base slice. Task remains active while those remaining scope/appearance checks are audited.

## Validation (final source)

- Native Debug configure/build and full CTest:36/36 passed,32.20s.
- Godot Debug/console ON full CTest:39/39 passed,55.48s.
- Optional real-GPU Debug suite:40/40 passed,49.32s, including numeric material output.
- Debug/console OFF: GPU and three host checks4/4 passed,24.75s; the final missing-car
  color-selection check then passed in car_render_host20.17s.
- Release/console OFF, successful rebuild followed by full CTest:39/39 passed,8.61s.
- Restored Debug/console ON with PORT_ENABLE_GPU_TESTS=OFF. A final responsive-button
  layout check passed native ImGui plus viewer host2/2,2.11s.
- Full final Release catalog probe:64 cars/235 associations;228 loaded,7 unavailable,
  0 failed. Probes validate geometry, material/texture/UV binding, supported GS state and
  static source-over bounds. Reproduce: vehicle_catalog_tests.exe --all-skins.
- Player/police/traffic Compatibility captures inspected after the depth fix; geometry,
  skin texture/material output, windows and camera framing are visible. Images/logs stay
  ignored. Missing original car selection retains its own IDs/colors and diagnostic.
- Real GPU checks: copy, source-over, rawalpha0, strict TEST48/49 boundaries, and paint
  occluding a later transparent surface. Actual paint attribute CPU/GPU agrees190,190,211.
- Native parser tests reject malformed bounds/offsets, unknown layouts, NaN positions and
  infinite UVs. Native ImGui/host tests cover capture, resizing, F8, queued actions,
  load/reload, parts/LOD, clear/fixture, scene transitions and resource cleanup.
- hp2_tpk_texture CTest is a legacy executable SKIP without its generated Python reference;
  new default/raw-alpha fixture assertions ran in vehicle_textures. No Verified claim.

## Next steps

Current static base viewer acceptance is checked. Compare original-output appearance
and recover partial-view cone/clipping before claiming the whole render entry restored. Keep original ELF/assets and
raw analysis/captures ignored. See docs/VEHICLE_RENDERING.md for exact supported scope.

## Related commits

No agent commit; preserve the user's existing commits and working tree.


### Final visual correction (E-0033-T07)

Window QA exposed black bonnet triangles. Original00105AC0 packet pointers show that
state bit0 selects TEST while bit1 selects ZMASK. Corrected the helper to use raw+0x50
and the phase gate for depth writes; paint writes Z even when+0x74 selects TEST1.
The corrected vehicle no longer shows the interior through its bonnet. Added real GPU
occlusion regression, not a speculative depth prepass. Current source-over range remains
unchanged. Final configuration retests after this correction are recorded in Validation above.

## Compact viewer / loading follow-up — 2026-10-14

User-approved scope: contain every ImGui window, simplify controls, clickable native paint
previews, near-only default parsing/upload, lazy side-by-side LOD comparison, asynchronous
loading with cancellation and bounded caches. This is port tooling; original render-entry
recovery gaps above remain active, and no Verified promotion is made.

Implemented shared viewport containment; compact versioned panel with Advanced collapsed;
original car/skin tiles and tooltip diagnostics; near solid-hash filtering; original per-LOD
attachment gates with host comparison transforms/labels; metadata resource probes; native
single-worker session; staged cooperative uploads; geometry/pixel/GPU/shader caches and repaint
node reuse. Clear/Fixture/exit cancel tickets. Explicit extension termination joins the worker.
E-0033-B16 records why representative tiles are port-side, not original menu RGB.

Initial failures resolved: fixed-frame host assertions ran before async completion; host waits
now have deadlines. The first worker singleton hung at DLL unload under Windows loader lock;
termination now explicitly joins it. Staged material overrides initially preceded set_mesh;
corrected ordering and all host checks reran. Progress-table snapshots must be regenerated
when source files change. No assertions or failure regexes were disabled.

Performance sample (Debug, Windows Intel Arc B390, headless host, not original hardware):
911TURBO cold worker166.14ms, warm worker1.05ms, cached repaint1.35ms. Cold upload28.94ms
spread over frames; max upload slice2.02ms; sampled loading frame interval6.90ms. BMWM5COP
worker53.66ms, 4DR_SEDAN27.44ms. Warm repaint reports zero geometry reads / zero pixel decodes.
These are cold-versus-cache measurements of this implementation; no historical before/after
benchmark is claimed. Driver compilation can exceed the cooperative slice budget.

### Follow-up validation

- Native Debug configure/build, full CTest36/36 passed29.77s; final worker/UI focused
  checks2/2 passed4.66s, including actual paint-tile and Show LODs mouse actions.
- Godot Debug/console ON full CTest39/39 passed26.48s; final containment/host focused
  checks4/4 passed3.24s after tooltip bounds repair.
- Real Compatibility GPU material oracle1/1 passed3.47s using per-instance material
  overrides; final framebuffer RGB/alpha/depth behavior remains unchanged.
- Debug/console OFF rebuild and three host checks3/3 passed3.58s.
- Release/console OFF full configure/build and CTest39/39 passed7.86s.
- Final Release catalog resource probe:64 cars,235 associations;228 loaded,7 unavailable,
  zero failures. Output is ignored local/task0033/catalog-current.log.
- Restored godot-debug, HP2_ENABLE_DEBUG_CONSOLE=ON, PORT_ENABLE_GPU_TESTS=OFF;
  rebuilt and focused ImGui/host/template/progress checks8/8 passed5.29s.
- Real 1280x800 Compatibility captures of compact paint controls and side-by-side
  LOD0..3 inspected under ignored local/task0033. Capture waits for async completion.
- git diff --check and generated progress check passed. No commit created; all earlier
  console/viewer changes preserved. No raw assets, decompiled listings or captures added.

Continuation: cmake --preset godot-debug -DHP2_ENABLE_DEBUG_CONSOLE=ON
-DPORT_ENABLE_GPU_TESTS=OFF; cmake --build --preset godot-debug; ctest --preset godot-debug.
Launch car_viewer.tscn, choose an original ID/paint tile, toggle Show LODs. Inspect Advanced
for phase timings/counters. Whole render-entry/partial-view recovery remains active.

Final QA follow-up: the collapsed panel now caps saved sizes at420x320 (Advanced permits
viewport-height scrolling/resizing), preventing an old enlarged layout from covering the car.
Final Debug ImGui/progress/three-host checks5/5 passed4.62s; final native worker/UI checks2/2
passed4.80s, including exactly three cold file reads (GLOBAL, geometry, car TPK) and zero
catalog geometry reads. Inspected the final compact LOD comparison capture again. Documentation
is UTF-8, generated progress and whitespace checks pass. Default Debug/on remains installed.
