# TASK-0098 - Original event-bound vehicle and skin selection

- Status: active
- Parent: TASK-0096; see parent for exact authority, constraints and acceptance.
- Dependencies: TASK-0097.
- Scope: SelectCarScreen/CarSelector/VisibleCar; all restrictions, state/readiness, camera/light/animation/audio/help and async cancellation; semantic launch handoff.
- Acceptance: scope behavior/production routes/tests/evidence complete; original comparisons
  stay open until recorded. Detailed address-bound criteria added before activation.

## Current scoped acceptance

- [x] Original Debug car availability/list/sort/skin and per-game selection state implemented.
- [ ] Original selector input/transmission/handling/orbit/readiness/Back semantics implemented.
- [ ] Required original loader/camera/light/animation/thumbnail/swatch/help/audio presentation connected.
- [ ] Real event -> selection -> valid selected-car launch preparation route connected.
- [ ] Original assets/native/Godot/window checks and stale-load/reentry/save tests recorded.
- [ ] Original comparison recorded; no Verified from PC tests.

Evidence: ignored local/task0096/selectcar-agent; E-0098-S01 onward.
Read reverse/task0097/inventory.md E-0097-S01 before implementation. Constructor,
state and type evidence exists; no generic model viewer may replace the original UI.

## Native core checkpoint,2026-10-26

Implemented explicit hp2::debug host storage and reviewed original bodies in
fred.cpp/carinfo.cpp/frontendscreens/selectcar.cpp. Evidence E-0098-S01/S02/S03
and implementation limits in reverse/task0098/selection-core.md.27 original entry
records updated:26 scoped core methods Tested, one ScreenCallback projection
retained Decompiled/partial. This is not complete screen or gameplay recovery.

Event cars apply to both players; original skin/dashboard/rules/list boundaries,
stable adjacent sorting/current identity, profile-slot selection, popup default/
cancel/readiness-before-advance and commit mapping tested. Missing original
EasterEggs/localization/popup/continuation services throw. Untranslated Tick/render/
held input/orbit retain explicit gaps; existing production event confirmation
also remains blocked until the real screen/presentation dependencies are ready.

Debug language rows24bytes versus Retail28bytes audited through fresh MCP captures.
All50 semantic identities/five common IDs match after dropping the extra Retail
word;64 full-name bindings resolve in actual English assets. Initial overly broad
specification asset tests failed43004/44004. Reviewed original001953B0 uses numeric
top speed and hides COP specs; final checks follow those actual access branches.

Validation:

- `cmake --build --preset native-debug --target ultimate_selection_tests ultimate_event_tests eventtree_tests event_name_catalog_tests frontend_audio_tests frontend_jukebox_tests jukebox_screen_tests frontend_runtime_tests --parallel 4`: passed.
- `ctest --preset native-debug -R '^(ultimate_selection|ultimate_event|eventtree|event_name_catalog|frontend_audio|frontend_jukebox|jukebox_screen|frontend_runtime)$' --output-on-failure`:8/8 passed42.11s.
- `cmake --build --preset godot-debug --target ps2_port --parallel 4`: passed normal DLL.
- Final constant-time neighbor indexing/status getter/reentry checks: `ctest --preset native-debug -R '^ultimate_selection$' --output-on-failure`1/1 passed1.08s; final normal Godot DLL rebuilt successfully.

- `ctest --preset native-debug -R '^(progress_tables|recovery_progress_contracts)$' --output-on-failure`:2/2 passed2.48s. `git diff --check` passed for this checkpoint source/docs; only line-ending notices.
- `py -3 reverse/scripts/audit_ultimate_racer.py --check`: passed all64 original event records;32 Hot Pursuit, five incompatible KO progress records.

No graphical/PCSX2 match, FPS benchmark, race retry/lifecycle acceptance or Verified
claim. Tools CMake3.29.2/GCC13.2.0/Python3.14.4/Godot4.7.2; pinned godot-cpp unchanged.
Original runtime oracle remains unavailable. No commit/staging.

Exact next action: continue TASK-0098 original camera/Quaternion/model-loader and
presentation closure (E-0098-S04). Camera0015A668 outputs world->view; inverse-view
pose and original integer-angle/Slerp ordering are required. Reuse existing math,
render and resource caches only with reviewed behavior. Then wire actual event
entry and async generation/reentry tests; do not activate TASK-0099 early.


## Display dependency checkpoint,2026-10-26

See reverse/task0098/display-dependencies.md and E-0098-C01/U01/U02/U03/R01/R02/R03/F01/V02. Camera, quaternion, spline, swatches/thumbnails/logos, pose/reflection, selector light packet and cached alpha/fade groups implemented with native tests. Actual PLATFORM/CARCOVER decoding and generation-cancelled Godot upload tested. Latest normal Godot DLL builds. Three native presentation/model/camera regressions passed14.94s. No production page or gameplay completion claim; checklist remains open.

Next: original CarSelector input/orbit/help and full Tick/loading/fade lifecycle, then FrameEffects and production event entry/render integration. Original comparison and career/performance acceptance remain outstanding.

## Input/alpha/frame-order follow-up

E-0098-I01/I02 now cover orbit input/help-table selection and typed page-message dispatch with required dependency services. Signed cached alpha restores in original head-first order, including duplicate capture. Overlay starts130 per original constructor. Latest three selection/native asset tests3/3 passed3.60s; normal Godot build passed. Vulkan named-solid resource fixture rerun passed2.51s. See display-dependencies.md for exact commands, initial corrected test failures and remaining limitations.

Remaining acceptance is unchanged: full constructor/Tick/loader/locked display/FrameEffects, original render-stage compositing, production event/selection/launch binding and save/lifecycle tests. E-0098-L01 captured locked-description dependencies; E-0098-F01 now records actual solid/background/platform/reflection/alpha/UI ordering. Do not flatten it into a viewer or claim playable selection/career. Next action: implement these dependencies in TASK-0098 before TASK-0099.

## Tick, locked display and packet stages checkpoint,2026-10-26

Current implementation scope remains TASK-0098. User renewed the full TASK-0096
request; downstream tasks are still pending, not silently included in this scope.

E-0098-T01 implements native001947D0 Tick orchestration: delayed/superseded load
polling, fade/scroll unload gate, unload/load/begin order, original duplicate
reinitialization, orbit timeout/rotation/camera globals, pose/alpha/lighting and
widget order. Palette/loader/locked update/nebula remain explicit required
services. Original00193AE0 is a confirmed JR RA/NOP, not an unresolved effect.
The bounded00196688 camera-offset stores initialize ELF zero-fill before use.
See reverse/task0098/loading-lifecycle.md and ultimate_selection_lifecycle_tests.

E-0098-L02 implements native locked-car slide/timer/signed-alpha/rectangle
submission plus task/BAC/event/points description priority and SHA-bound table
decoding. Original localization and frame-owned rectangle rendering remain
required services. See reverse/task0098/locked-display.md.

E-0098-F02 preserves stage1/7 ranges in native packets and production document
concatenation, with complete-coverage validation and explicit legacy gaps. Quad
order and existing flat rendering remain unchanged. This is a prerequisite, not
the stage0/2/4 compositor or mirrored-car renderer. See render-stages.md.

Validation (CMake3.29.2/GCC13.2.0/Python3.14.4; godot-cpp pin unchanged):

- `cmake --build --preset native-debug --target ultimate_selection_tests ultimate_selection_camera_tests ultimate_selection_widgets_tests ultimate_selection_lifecycle_tests selection_locked_tests frame_builder_tests frontend_runtime_tests ultimate_event_tests --parallel 4`: passed.
- `ctest --preset native-debug -R '^(ultimate_selection|ultimate_selection_camera|ultimate_selection_widgets|ultimate_selection_lifecycle|selection_locked|frame_builder|frontend_runtime|ultimate_event)$' --output-on-failure`:8/8 passed25.34s with configured original assets/Debug ELF; no SKIP on this machine.
- Normal `cmake --build --preset godot-debug --target ps2_port --parallel 4` passed before final packet-assembly/formatter changes; final rebuild recorded below.
- Final normal `cmake --build --preset godot-debug --target ps2_port --parallel 4`: passed after packet assembly and formatter integration; existing unrelated compiler warnings remain.
- `py -3 reverse/scripts/audit_ultimate_racer.py --check`: passed64 events/32 Hot Pursuit; five KO storage incompatibilities remain version-isolated.
- Scoped `git diff --check`: passed; line-ending notices only. New public synthetic test files explicitly exempted from the existing test ignore rule; private inputs/captures remain ignored.
- `py -3 reverse/scripts/generate_progress.py`: updated generated tracker/report/log index. `ctest --preset native-debug -R '^(progress_tables|recovery_progress_contracts)$' --output-on-failure`:2/2 passed2.04s. Final full `git diff --check`: passed.

No production selection page, loading lifecycle as a whole, race World, police,
rules/outcome/save, resource roundtrip/performance acceptance or original comparison
has completed. No staging/commit or Verified promotion. Original matching Debug
scenario is absent: PCSX2 not running and21512/28011 probes timed out. The Retail
oracle is not a Debug oracle. This blocks original certification, not independent
remaining implementation.

Exact next action: complete TASK-0098 constructor/destructor and FrameEffects
ownership plus the reviewed shared-depth stage0/1/2/4/7 compositor; bind native
Tick/LockedDisplay to actual resources/help/speech/localized rectangle services,
then connect original event entry/profile adapter and valid launch. Keep the
production entry throwing until those prerequisites exist; TASK-0099 stays backlog.
