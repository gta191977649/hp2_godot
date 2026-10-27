# TASK-0098 native frontend stage metadata

## Authority and confirmed evidence

Behavior authority: Debug `NFSHP2.ELF`, SHA-256
`815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`;
executable revision/date unknown. E-0098-F01 is retained in
[display-dependencies.md](display-dependencies.md), with the ignored capture
`local/task0096/race-agent/selector-edisplayframe-00112c60.json`.

`eDisplayFrame00112C60` submits view0 Flailer layers24..20 in stage1, after sky;
stage7 submits layers15..0,19..16,28..25. Main car solids are in stage0,
platform callbacks/FrameEffects in stage2, and reflected car then main-car alpha
submission in stage4. E-0098-F03 corrects earlier wording: intrinsic reflection
flags0x11 suppress envmap/sorted-alpha, stages2/4 flush solids only, and the shared
pending alpha list flushes in stage5. See [counterevidence](compositor-correction.md).
The reflected car uses the existing view/render
targets and the EVIEWMODE_ONE gate. These observations do not establish all
reflection clipping/material or render-target blend behavior.

## Port-side implementation

`port::render::FrontendStage` and `FrontendQuadRange` retain the stage1/stage7
boundary alongside the existing flat quad vector. They are port packet metadata,
not recovered PS2 types. `build_frame` preserves its existing layer sort,
reverse authored tie order, geometry, clipping and glyph emission. It records
only quads actually emitted, merges adjacent ranges with the same stage, and
clears old ranges when rebuilding a frame. Missing/hidden/suppressed/clipped
widgets and empty stages produce no range. The exclusive movie path also retains
its authored widget stage; an unsupported exclusive-path layer is tagged
`unspecified`, without changing that path's previous draw behavior.

`append_frontend_frame` preserves source/destination quad sequence, rebases
ranges and merges only adjacent equal-stage ranges. Existing metadata gaps stay
visible. Overlapping/out-of-bounds ranges are rejected before appending.
`frontend_ranges_complete` requires known-stage contiguous coverage of all quads;
it returns false after untagged legacy appends and for unspecified stages.
An empty frame has complete empty coverage. It does not assert that the quad
sequence has been regrouped into globally correct cross-document stage order.

The existing flat `Ps2RenderHost` backend can continue consuming `Frame::quads`
unchanged. It does not yet use these ranges to composite selection3D.

## Remaining integration and unknowns

- `FrontEndRuntime::build_frame` now concatenates autosave/sibling documents with
  the append utility and clears metadata on an invalid-screen return. A real
  main-menu asset regression requires complete stage coverage.
- `event_tree_view.cpp` appends legacy quads directly. Their original stage
  classification is not established by this change; coverage remains incomplete
  until independently reviewed.
- The production frontend has one canvas item and no stage0/2/4 compositor.
  Merely adding a vehicle viewport between background and UI does not reproduce
  original shared-depth ordering.
- `Ps2VehicleRenderer::apply_frontend_pose` supplies pose and lights. Its existing
  material `reflection` next pass is environment mapping, not the separate
  mirrored `FrontEndRenderingCar::Reflection` lifecycle. Main solid/alpha and
  reflected solid/alpha submission still need reviewed separation and shared
  render-target composition.
- FrameEffects, platform render allocation and reflection material/clipping
  remain outside this metadata change. Camera-dependent absent stages are
  representable as empty ranges; no camera/view admission policy is implemented.

No original runtime/visual match, complete selection page, race, performance
acceptance or Verified promotion is claimed.

## Validation, 2026-10-26

Tools: CMake3.29.2, GCC13.2.0, Python3.14.4; dependency pins unchanged.

- `cmake --build --preset native-debug --target frame_builder_tests --parallel 4`:
  passed. Existing unrelated compiler warnings remain.
- `ctest --preset native-debug -R '^frame_builder$' --output-on-failure`:
  passed1/1 in0.44s after the final clipped-range assertion.
- `git diff --check -- src/port/render/frame.hpp src/port/render/frame.cpp
  src/port/runtime/frame_builder.cpp src/tests/frame_builder_tests.cpp`: passed.

Tests exercise all29 supported layers and their exact previous flat order;
nonoverlapping complete coverage; empty background/foreground/frame rebuilds;
glyph, missing-asset, exclusive-movie and clipped emission; rebasing/merging,
self append, partial legacy gaps, unspecified stages and malformed ranges.

Next: recover and implement the selection compositor with original shared depth and render
target behavior before enabling the formal event-to-selection entry.
