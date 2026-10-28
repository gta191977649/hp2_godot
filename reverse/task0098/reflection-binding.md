# TASK-0098 script-driven window acceptance and intrinsic reflection flags

2026-10-28, E-0098-RB01. Behavior authority: Debug NFSHP2.ELF SHA-256
`815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b`,
revision unknown. Existing Retail frontend remains SLUS_203.62 SHA-256
`7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`.
Read-only MCP captures are ignored under local/task0098/reflection-binding and
hash-indexed in evidence.json. No original-runtime equivalence is established.

## Confirmed flags and implemented host binding

Fresh RenderFrontEndCars00123EA8/RenderStrips0010E120 decompilation and listings
confirm reflection's intrinsic0x11 mask before independent stage solid/alpha
masks. Bit1 suppresses environment mapping; bit16 skips ApplyAlphaSorting strips.
Fresh Debug TextureInfo type data places ApplyAlphaSorting at4Dhex, matching the
existing decoded asset sorting byte. This is a specific field correspondence,
not whole-version texture-context compatibility.

The direct shared Ps2VehicleRenderer now consumes these intrinsic flags during
apply_frontend_pose. Each original surface retains its sorting eligibility;
reflected sorted surfaces discard before any colour/depth contribution and
environment next-passes are detached. Switching to a main-car pose restores
eligibility. A later environment-capture binding respects the same disable bit;
head/brake texture switches refresh sorting eligibility. The added suppression
uniform participates in the depth consumer's dynamic fallback contract.

This supersedes E-0098-F03/F04's statement that apply_frontend_pose ignores all
reflection flags. Their required shared physical colour/depth executor, ordered
solid/alpha flushes and complete Debug context requirements remain OPEN. Shader
discard represents a skipped strip's pixel contribution only; it is not original
packet submission and provides no original performance/whole-compositor claim.

## Actual game-window reproduction

User explicitly required script-based testing and excluded computer-use. Python
starts the pinned real Godot Vulkan/Forward+ process; the SceneTree harness loads
the actual frontend scene and injects InputEventKey through Input.parse_input_event.
No replacement page services, direct menu_open, forced unlocks or fake race state
are used. Input profile and save output are isolated to a fresh ignored run folder.
Autoload debug settings use the existing test-session path.

Run:

`py -3 reverse/scripts/check_frontend_selection_roundtrip.py`

The runner requires actual Main Menu -> Cops On Mode Select -> Ultimate Racer ->
details -> Select Car -> cancel -> reentry routes, three cycles. Checkpoints,
runtime log, final screenshot and DLL/engine hashes are private per-run artifacts.
Its gate checks runtime errors, process failure/timeout, missing result/checkpoints
and missing PNG. This gate covers routes and input delivery; native readiness,
exact skin/car values, loader generations and visual/original comparisons still
need their own production observability/acceptance after binding.

CURRENT RESULT: FAIL, exit1. The production input stack reproduces
Retail001F6A28 exactly; no Select Car checkpoint exists. Latest run:
local/task0098/selection-roundtrip/run-00ulygx6. It is not a passing acceptance
test and is not registered as a default green CTest or waived.

The earlier capture-only path saved a screenshot and exited0 even after reporting
NotImplemented. FrontendNative now counts reported runtime errors; capture still
saves diagnostics but prints FRONTEND_CAPTURE_FAIL and exits1 after such errors.
Actual-window controls confirm clean Main Menu capture exits0, while the same
event confirmation trace saves a frame and exits1. Capturing a frame is no longer
misreported as a successful route.

## Validation and limitations

- Native11-target rebuild passed. Scoped11 CTests passed23.91s, including
  frontend_composition intrinsic masks, main/reflection eligibility restoration,
  existing selection/page/event/launch and frontend runtime regressions.
- Normal `cmake --build --preset godot-debug --target ps2_port --parallel 4`
  passed, including actual DLL relink. The earlier in-use DLL blocker was absent.
- Actual-window GPU command:
  `local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe --path godot res://scenes/debug/car_material_gpu_smoke.tscn -- --material-gpu-smoke`
  passed. Actual shader samples validate omitted opaque/transparent colour,
  restored rendering, and no depth write (later surface RGB133/55/37). Existing
  VU modulation/environment alpha/depth tests also pass, tolerance1 encoded unit.
- Initial expanded GPU fixture failed because the transparent test material still
  retained the deliberately enabled skip uniform before subsequent tests. The
  fixture now explicitly restores it; production shader logic was unchanged.
  The failed invocation is not counted as validation. Final private log and
  input hashes are in reflection-binding/gpu-manifest.json.
- Initial ad-hoc menu trace navigated Be The Cop by mistake; it is not the Event
  reproduction. Corrected scripted trace and final harness reach Ultimate Racer
  through the actual three Down inputs and reproduce the requested input stack.

Only bounded native masks and GPU material output passed. The complete production
page/roundtrip, shared compositor, same-SHA original runtime, race/results/save,
resource lifetime and performance criteria remain unpassed. No Verified promotion.

Exact continuation: TASK-0098, E-0098-F03/F04 shared physical colour/depth executor
and real SelectionPage resource/input/load/audio/help/profile services, then
FrontEndRuntime ownership/entry. Keep rerunning the failing script until the
actual route passes. Do not delete001F6A28 or substitute the unit-test callbacks
to turn this gate green.

Exact native checkpoint commands:

```
cmake --build --preset native-debug --target frontend_runtime_tests frontend_actions_tests ultimate_event_tests race_parameters_tests ultimate_selection_tests ultimate_selection_camera_tests ultimate_selection_widgets_tests ultimate_selection_lifecycle_tests selection_locked_tests selection_page_tests frontend_composition_tests --parallel 4
ctest --preset native-debug -R '^(frontend_runtime|frontend_actions|ultimate_event|race_parameters|ultimate_selection|ultimate_selection_camera|ultimate_selection_widgets|ultimate_selection_lifecycle|selection_locked|selection_page|frontend_composition)$' --output-on-failure
ctest --preset native-debug -R '^(progress_tables|recovery_progress_contracts|track_optimization_performance_rules)$' --output-on-failure
```

Final source/progress checks3/3 passed1.97s. All indexed TASK-0098 capture hashes
match; the three scoped entry/render records are unique and not Verified.
`git diff --check` passed (CRLF notices only), staged files0. No commit made.
