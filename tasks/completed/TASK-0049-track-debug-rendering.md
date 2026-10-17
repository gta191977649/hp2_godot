# TASK-0049 - Track texture orientation and topology debug rendering

- Status: completed (PC acceptance; no original-game Verified claim)
- Scope: fix vertically inverted track textures; TrackViewer-only collision and physical terrain material debug checkboxes.
- Authority: existing original texture bytes, UVs, topology query evidence and explicitly port-side display adapters.
- Preserve existing dirty work; Ghidra read-only; do not change original physics, alpha rules or historical review pins.

## Acceptance

- [x] Base/mip direction consistent for indexed and linear textures; asymmetric and actual-window checks.
- [x] Two independent default-off ImGui controls, disabled without ready topology.
- [x] Original wheel-contact polygons, all layers, perimeter edges and stable material-ID legend.
- [x] Four toggle combinations, switch/clear/failure/reload and resource release tested.
- [x] Native/Godot build, full regression and actual-window captures recorded.
- [x] Evidence and user documentation updated; no Verified promotion.

## Allocation

REA workflow step6: independent texture evidence/adapter, topology debug geometry, and UI/action workers. Root owns Godot drawing, integration, serialized builds, windows and documentation. Host debug colors/offsets are tool choices, never original-game constants.


## Findings and evidence

- E-0049-T01 `reverse/task0049/texture-direction.md`: indexed internal row reversal,
  fresh read-only Retail ST read and track-only base/mip orientation adapter.
- E-0049-G01 `reverse/task0049/debug-geometry.md`: original flag eligibility, exact
  zero-Z exclusion, plane projection and perimeters; palette is explicitly tool-side.
- E-0049-H01 `reverse/task0049/host-debug-rendering.md`: cached Godot fill/edge nodes,
  independent states, depth occlusion, port-only offset and resource ownership.
- `reverse/task0049/source-review.json`:8before/16after scoped successor, historical
  TASK0044..48 JSON pins unchanged. No recovered physics body or alpha algorithm changed.

## Validation

- `cmake --preset godot-debug`, `cmake --build --preset godot-debug -j 6`: PASS.
- Asymmetric4/8-bit indexed and linear32 base/mip RGBA fixtures: PASS; vehicle texture
  and shared TPK decoder regression: PASS.
- `build/godot-debug/track_debug_geometry_tests.exe (Get-ChildItem local/task0048/trackb*.bin).FullName`:
  PASS30 original uncompressed track bundles, contact-plane values and perimeters.
- Original-query fixtures: slope, triangle/quad, flags2/8 exclusion, flags1 multilayer
  selection, noncoplanar fourth corner, adjacent material boundary, unknown ID and
  invalid data rejection: PASS. No topology mutation.
- Headless TrackViewer smoke: PASS four toggle combinations, unchanged cache counts,
  switch, missing resource, reload, driving hide/return restore and clear release.
- Native ImGui pointer checks: independent actions, disabled controls and driving hides
  targets: PASS. Strict source-review Python checks:32PASS.
- Actual Compatibility windows inspected: local/task0049/track11-texture-fixed.png,
  track11-collision.png, track11-material.png and track11-both.png. Trees upright;
  transparency remains intact, contact faces/edges and material legend visible.
- Initial test-only expectations assumed flags0 selected the highest layer and that
  collision-only had no fill; corrected to original query semantics and approved
  neutral collision fill. Empty clear state now uses the explicit native snapshot.

## Limits

Overlay colors, opacity and0.015 host-up offset are tool choices, not recovered game
constants. Visibility respects original scene depth. This displays contact topology,
not every rendered object or wall collision volume. Existing reflection/worldLOD gaps
remain unchanged. PC windows and tests do not establish original-game equivalence.

- External full scene regression: `build/godot-debug/track_asset_tests.exe D:/ps2_game/GAME/ZZDATA`
  PASS all30 geometry, texture, original mip and light references after direction fix.
- Actual CarViewer911TURBO regression inspected: local/task0049/car-regression.png;
  load and original vehicle texture presentation remain intact.

- Final full `ctest --preset godot-debug --output-on-failure`: **87/87 PASS**, zero
  failures,187.51 seconds (local/task0049/final-ctest.log).
- Final source successor audit, generated progress/class templates and whitespace
  checks PASS. No original physics/alpha algorithm changed and no Ghidra mutation.

## Continuation

Reproduce via F8 -> trackviewer -> load a track -> toggle the two browse controls.
See docs/TRACK_VIEWER.md for command-line captures and explicit display semantics.
Original-game comparison and pre-existing environment reflection/worldLOD gaps remain
separate work; no further implementation is required for this approved tool scope.
