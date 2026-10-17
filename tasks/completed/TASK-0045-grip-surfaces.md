# TASK-0045 - Grip and terrain material recovery and Test drive

- Status: completed
- Certification: original numerical comparison blocked; not Verified.
- Related: TASK-0038, TASK-0044.
- Target: Debug NFSHP2.ELF SHA-256 815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
- Scope: evidence audit of the existing original-class tire/contact/load/friction chain,
  all 33 terrain rows, canonical fixes only when demonstrated, and port-owned selectable
  uniform/five-zone test surfaces with read-only four-wheel diagnostics.
- Preserve all existing uncommitted TASK-0044 work. No independent grip calculation,
  Godot physics, tuning sliders, additional keys or Run/Pause UI. Ghidra read-only.

## Acceptance

- [x] Audit contact/load/slip/lateral/iteration/friction/rolling/noise and boost sources,
  original signatures/addresses/types/instructions; classify all33 original terrain rows.
- [x] ROAD/DIRT/GRASS/SAND/SNOW use exact original data. Uniform defaultsROAD;
  zones on nativeX boundaries25/50/75/100, per-wheel contact, first/last extend outward.
- [x] Surface choice commits with the next successful step without pose/gear reset;
  failures preserve last complete snapshot; recording/replay includes surface settings.
- [x] Read-only original wheel diagnostics and material data are published from the same
  completed step; UI/render use the same tool layout descriptor, never another physics chain.
- [x] Existing immediate drive/keyboard/camera/focus/return interactions preserved;
  real-window surface changes, zones and diagnostics exercised.
- [x] Relevant native/Godot/generator/structural checks pass. Explicit append-only review
  amendments retain TASK0044 integrity controls. No proprietary tracked artifacts.
- [x] Original Debug comparison attempted or concrete blocker documented without Verified.
- [x] Update docs, evidence, symbols if findings change, progress and continuation.

## Work allocation and evidence

REA step6 independent scopes: solver worker audits Ghidra and tire fixtures; runtime worker
owns bridge/UI/presentation and host fixtures; constructor worker owns append-only source
review machinery. Root owns SurfaceSettings/contact/catalogue, Session/backend, integrated
scenarios and final builds. Baseline source identities retained privately at
local/task0045/pre-source.json before edits. Raw captures stay ignored under local/task0045;
durable evidence uses E-0045 identifiers. Existing dirty working tree is intentional.

Original numeric equivalence is not established by any PC test. Full original track topology,
water/wall/object behavior, effects/sound and unimplemented game branches remain outside this
synthetic test scope and must not be replaced by guesses.


## Findings and delivered behavior

E-0045-T01..T04/W01/U01 evidence is in reverse/task0045; usage and boundaries are in
[DRIVING_GRIP.md](../../docs/DRIVING_GRIP.md). All33 terrain rows and31 bounded functions
were audited. Five newly recorded Debug functions are Decompiled only; main Debug index
now contains212 unique addresses (185 Implemented/27 Decompiled), with all historical Retail
and TASK0044 records retained. Raw bit/table/capture hashes and original type/field provenance
are durable. No ordinary finite-path arithmetic mismatch was demonstrated for the first five
materials. The only recovered-body token changed is the exceptional VRSQRT diagnostic anchor
Retail001A52EC -> Debug001A2AC4. CAR32 noise guard remains a documented excluded-case gap.

Port surface settings feed per-wheel queries on the canonical PhysicsCar/Wheel chain. Uniform
choices and nativeX zones25/50/75/100 use exact Debug static/dynamic/rolling/noise data.
Changes commit with a complete next step without restarting physics, and Session records and
replays surface settings alongside controls/elapsed time. Original stored grip fields are
read after Move/UpdateCar, with separate tire/material coefficients and no inferred SI units,
combined grip score or tuning sliders. Shader/labels use the same published descriptor.

Actual captured-world-click -> Escape -> Return exposed an existing input bug: ImGui saw the
press but not its release after pointer capture. The captured branch now forwards button
releases to ImGui while keeping camera motion separate. No driving pause or physical state
was introduced. Return stays outside the scrolling diagnostics body. Screenshot review also
corrected the test's actual expanded-header coordinate and moved small depth-tested labels to
the lane side. Final screenshot shows all four contact/terrain rows with the header expanded.

## Validation

- `cmake --preset native-debug`; `cmake --build --preset native-debug --target driving_surface_tests driving_backend_tests -j6`:
  passed after adding the required asset-reader link through port_runtime.
- `.\build\native-debug\driving_surface_tests.exe`: passed the five same-input material
  trajectories, all-zone/mixed-wheel traversal, independent queries, boundary/negative cases,
  transactional settings and replay. Qualitative outputs are not original golden trajectories.
- `cmake --build --preset native-debug --target driving_tire_tests -j6`;
  `ctest --preset native-debug -R '^driving_tire$' --output-on-failure`:1/1 passed in0.27s.
- `cmake --preset godot-debug`; `cmake --build --preset godot-debug -j6`: passed. The first
  UI compile exposed pinned ImGui1.77, so unsupported Tables/BeginDisabled calls were replaced
  with supported legacy Columns/child layout; no dependency upgrade.
- `ctest --preset godot-debug --output-on-failure`:72 checks in141.93s;70 passed, one old
  lexical Debug-loader assertion failed after atomic catalogue loading, and the new surface
  test skipped because its default working directory did not find external config. Corrected
  the assertion to require Debug load/catalogue validation/image commit and set the test's
  repository working directory. No production physics changed for these test corrections.
- `cmake --preset godot-debug`; `ctest --preset godot-debug -R '^(driving_surfaces|debug_driving_single_chain)$' --output-on-failure`:
  **2/2 passed in16.51s** (surface16.28s), completing validation of all72 distinct checks
  across the full run and targeted rerun. The full run's other70 passed, including all six
  Godot hosts/batches, native driving/renderer, template and source-integrity checks.
- `py -3 tools/test_drive_window.py --surfaces --output local/task0045/window-final`:
  **31 actual OS checks passed**. Actual ImGui selection of all five materials, per-wheel
  diagnostics, acceleration across every zone/mixed materials, camera/input/F8/focus lifecycle,
  Return/re-entry and label cleanup all passed. Root inspected expanded-table/lane-label
  surfaces.png. Earlier desktop focus loss was isolated; the user provided an idle desktop.
- `py -3 reverse/scripts/generate_class_templates.py --check` and
  `py -3 reverse/scripts/complete_class_members.py`: passed; foreign Debug rows retained.
- `py -3 reverse/scripts/audit_task0044_migration.py`: passed chained historical/frozen/current
  checks. TASK0044 pins remain unchanged; separate TASK0045 review covers11-before/17-after
  identities, including only the evidenced diagnostic change in recovered physics. Negative
  review tests passed. No accept-current mode was added.
- `py -3 reverse/scripts/generate_progress.py`; generated progress check and `git diff --check`:
  passed at final documentation checkpoint. New-file UTF-8/restricted-extension and local-link
  inspections passed. ELF/static raw rows/assets/screenshots stay outside Git. No commit made.

Toolchain: GCC13.2.0 MinGW, CMake3.29.2, Ninja1.12.0, Python3.14.4, Godot4.4.1
49a5bc7b6, existing pinned godot-cpp and ImGui retained. Real window: Compatibility/OpenGL
Intel Arc B390 driver32.0.101.9150. Debug ELF independently rehashed to the full target hash.
Ghidra access remained read-only.

## Certification and continuation

`py -3 -m tools.oracle probe --output local/task0045/pcsx2-probe.json` returned blocked /
timed out; no active PCSX2 process or runnable synchronized Debug driving state was available.
No emulator/save/settings were changed, and Retail oracle hooks were not reused. This meets
the approved blocker-reporting alternative, not numerical certification. TASK0038/0023 retain
original comparison work. Connect the exact Debug executable in a known running/paused
scene, establish owned Move00137D18 and wheel/component entry-return hooks with an explicit
Debug oracle mapping, match initial state/input/time/material/contact, then record component
and four-wheel errors/coverage. Do not promote Verified until such evidence exists.
