# TASK-0039 - Frontend text and event-number rendering audit

- Status: active
- Objective: use read-only Ghidra MCP and original assets to establish and correct frontend text, digit placement and shared UI rendering rules.
- Scope: trace event-tree number rendering first, then shared font measurement/draw, alignment, glyph bearings, kerning, widget transforms, clipping and colours across the frontend. Preserve unrelated TASK-0038 work.
- Binary: SLUS_203.62 US, SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2; exact executable revision unknown.
- Evidence: E-0039-*; raw outputs remain in ignored local/task0039.

## Acceptance criteria

- [x] Original event-number transform and font rules captured through Ghidra MCP.
- [x] Shared frontend draw and measurement rules audited with confirmed findings separated from unknowns.
- [x] Evidence-backed fixes and regression coverage implemented.
- [x] Relevant native/host tests and visual captures checked; limitations recorded.
- [x] Durable evidence records, tracker and continuation updated. No Verified claim without original comparison.
- [ ] Complete original rendering parity across dynamic frontend paths and matched original captures.

## Findings

Initial source inspection: frame_builder sums advances without kerning; font parser discards glyph bytes +0x18/+0x1C and all pair records. Event-tree numbers reuse the authored template but replace its position with the node centre. These are investigation leads, not established original behavior.

Read-only MCP established E-0039-N01/F01..F06/Q01..Q09, documented in docs/FRONTEND_RENDER_AUDIT.md
and per-function records. Font header28 is a table offset; max heights are27/61/43/98. Event
digits now subtract half scaled height; draw applies current-glyph pairs but measurement does
not. Right alignment, fixed cells, inline buttons, squared helper X scale and UV offsets are
preserved. Shared quads retain four colours and all UVs through native packets and Godot polygons;
alpha updates preserve each RGB corner, clip keeps original colour words, layers use signed
comparison and visibility equals1. Rotation fields are retained but reached nonzero mode2 throws.

Original asset census:41 documents,1015 quads,534 text widgets,255 gradients,18 nondefault UVs,
zero authored nonzero mode2 rotations. Assets/captures/raw MCP stay ignored. BUTTONSOUT zero-pair
table offset is unused/stale; parser regression ensures this font is not rejected.

## Validation and limitations

Tool versions detected: CMake3.29.2, GCC13.2.0, Python3.14.4, Godot4.4.1;
godot-cpp pin e4b7c25e721ce3435a029087e3917a30aa73f06b confirmed. Existing Ghidra12.0.4/R5900
MCP supplies read-only inspect and memory queries; no database state changed. Reproduction script
captures41 queries with hashes under local/task0039/capture/manifest.json.

Native and Godot builds passed before final regression runs. One initial native full run47/48
failed a test's hardcoded frame sequence after adding extra draw checks; assertion now compares
the previous sequence. Godot full52/54 initially failed stale generated progress tables (being
regenerated) and unrelated preexisting CarViewer script asset_viewer.gd:399 type inference;
car_render_host timed out after that parse failure. No unrelated viewer script was changed.
Two DLL links were temporarily blocked by a preview or host test holding the extension; the
preview closed gracefully and build was retried after tests exited.

All41 document native-render smoke passed; routed Ultimate Racer event1/3 and Options captures
were inspected privately. Direct raw document39 capture showed placeholders because it bypasses
mode-select state initialization; production route captures and native route tests are used.
Final bounded results:

| Command / check | Result |
|---|---|
| `cmake --build --preset native-debug --parallel 6` | Passed; final relink after tests exited |
| `cmake --build --preset godot-debug --parallel 6` | Passed before an external concurrent build cleaned shared outputs |
| `ctest --preset native-debug --output-on-failure` |48/48 passed47.20s; latest crop change rebuilt during this run, so separately repeated relevant checks after relink |
| `ctest --preset native-debug -R 'frame_builder\|frontend_quad\|frontend_runtime\|progress_tables' --output-on-failure` |4/4 passed16.35s after final native relink |
| `build/native-debug/frame_builder_tests.exe`, `build/native-debug/frontend_quad_tests.exe` | Both final direct runs passed |
| `ctest --preset godot-debug --output-on-failure` | Earlier52/54; stale progress later corrected, unrelated CarViewer parse/timeout remains |
| Native render smoke | Earlier passed41 documents,1076 quads,7635 glyphs,119 textures; empty36 recorded. This run preceded zero-pair BUTTONSOUT correction |
| Windowed routed event1/event3 and screen24 captures | Passed/inspected; final event3 and Options show original button font restored |
| `py -3 reverse/scripts/generate_progress.py --check` | Passed |
| Final `ctest --preset godot-debug -E '^car_render_host$' --output-on-failure` / render smoke | Invalidated/blocked: another process concurrently cleaned/rebuilt build/godot-debug and shared DLL; executables and extension were absent. Canceled owned test processes, did not stop the external build |

Observed competing build: CMake PID52348, `--build --preset godot-debug --parallel 4`, Ninja92640,
compiling godot-cpp from scratch. These are point-in-time process identities, not continuation
targets. Do not kill/reuse those IDs. After that build finishes, rerun host validation sequentially.
An intermediate native relink also encountered a running frontend_runtime_tests.exe; final
relink and post-build tests passed. No commit/staging or proprietary material was introduced.

Full frontend fidelity remains unresolved: original numerical/pixel comparisons, FCR31,
sampling/raster/triangle behavior, rotation/trig, special multi-pass/animated-UV paths, complete
wrapping/control semantics and equal-layer construction order. Existing missing per-screen
dependencies and resource gaps are not claimed resolved. No status becomes Verified.

## Next action

Continue TASK-0039 at original matching Event1/3/Options captures under TASK-0023; establish
FCR31 and GS/host texel-centre behavior, then trace00143078 special-texture/001428F0 animated UV
and rotation dependencies. Current original capture is the missing prerequisite for pixel parity.
Run `py -3 reverse/scripts/inspect_frontend_rendering.py --elf (Get-Content config/ps2_elf.local.json | ConvertFrom-Json).elf`
to reproduce read-only MCP evidence, followed by the builds/tests in FRONTEND_RENDER_AUDIT.md.
Immediate host prerequisite: let the competing shared-output rebuild finish, then run Godot
build, native-render smoke and `ctest --preset godot-debug -R 'frontend_runtime|frame_builder|frontend_quad|progress_tables'`.
CarViewer's unrelated script failure remains separately recorded; do not silently widen this task.

## Related commits

None.
