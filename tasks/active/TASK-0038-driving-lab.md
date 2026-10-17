# TASK-0038 - DrivingLab and single-car driving recovery

- Status: active
- Target: SLUS_203.62; executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
- Objective: CarViewer Test drive mode using the same scene/shared vehicle renderer and recovered native driving logic on an explicitly synthetic horizontal plane (user correction supersedes the separate-scene design).
- Scope: 911TURBO, keyboard/gamepad, pause/step/reset, input recording/replay, telemetry, recovered minimum driving dependency closure and scoped PCSX2 comparison.
- Excluded: original tracks, AI, car/car collisions, full damage, audio, original driving cameras.
- Constraints: read-only Ghidra; no guessed dynamics or Godot physics; unknown recovered paths throw hp2::NotImplemented; English repository records; proprietary inputs remain ignored.

## Current user workflow

Click Test drive in CarViewer: validated native initialization, rear camera and captured
mouse start immediately. WASD drives, mouse orbits/idle recenters, Esc releases and world
click recaptures. The only mode button is Return to CarViewer. Focus/F8 suspend/resume
automatically without elapsed catch-up; native errors are terminal and explicit. Earlier
manual Run/Pause/debug-toolbar descriptions below are historical and superseded by the
accepted third-person redesign. See docs/DRIVING_LAB.md for the current contract.

## Accepted implementation continuation

The user approved completing the minimum drivable native loop on the synthetic
plane. Original numerical equivalence is deferred to a later acceptance task;
instruction/game-data provenance is still mandatory. Recover table/engine,
constructor/reset, tire/contact/body/integration and original scheduling closures,
then connect an owned native backend to the existing Session and DrivingLab.
Acceptance requires continuous acceleration, left/right steering, braking to rest,
pause/step/reset/replay, transactional failure and native/Godot regression checks.
Preserve existing uncommitted work. Continue required dependency recovery rather
than stopping at a function count; no Verified promotion is authorized by PC tests.

## Acceptance criteria

- [x] Direct scene and drivinglab command, Debug console on/off; Release rejects entry.
- [x] Shared original car rendering, visual synthetic plane, independent tool camera and native ImGui controls. The synthetic contact provider is implemented.
- [x] Tool input/session contracts: keyboard/gamepad input, pause, step, reset and in-memory recording/replay. Production completes native driving steps and exact pose replay.
- [x] F8, lost focus and captured driving input pause and clear controls; resume is explicit.
- [x] Evidence-backed control, configuration, timing, drivetrain, wheel/contact and body dependency closure produces a drivable car on the supported synthetic fixture.
- [x] Dynamic renderer accepts independent body/four-wheel publications, validates before applying, and rejects preview integration. The production backend supplies complete snapshots.
- [x] Bounded leaf/contract tests, host smoke, original-resource publication smoke and regression checks. Real-data native and bridge sequences cover driving and replay.
- [ ] Reproducible synchronized PCSX2 comparison with scope, precision and differences recorded (explicitly deferred from the accepted minimum-loop implementation).
- [x] Evidence, task, decisions, progress log and generated tracker updated; task remains active.

## Investigation and implementation order

1. Capture read-only metadata/decompilation/listings for 00137100, 00137D90, 00138E08 and controller dispatch. Audit dependency contracts before replacing templates.
2. Add the port-side session, bridge, scene, deferred UI/console routing and tests.
3. Recover minimum dependencies in evidence order; never make the fixture drive using an approximate substitute.
4. Establish PCSX2 synchronized capture feasibility and compare bounded recovered behavior.

## Current result - accepted minimum loop implemented

The native911TURBO synthetic-plane loop now completes sustained acceleration,
left/right turns, footbrake stop and stable held rest. Full reset and elapsed-input
replay are deterministic. Godot runs540 native steps through the bridge/renderer
and replays every pose exactly. No original numerical equivalence or Verified
status is claimed. Earlier investigation/validation sections below are historical;
the final continuation section supersedes their constructor-gap statements.

## Historical findings

E-0038-001 audits the three main entry dependencies and the five-iteration00138F38 solver. E-0038-006 audits00152070 output stores: selected surfaces copy a signed material byte and16-byte vector; the missing route writes material1 and XYZ(0,0,-1), leaving the fourth vector lane untouched. This is not a valid-plane fallback contract. E-0038-002 preserves original0x560-byte configuration rows without assigning unknown handling fields. E-0038-003/005 implement finite leaves00137C88/001A4E88 with instruction-derived fixtures. No whole entry is implemented by those leaves.

E-0038-004: PCSX2 v2.8.2, SLUS-20362, CRC1d2818af, reported game version1.00. Captured18 bounded windows twice per paused snapshot, with exact equality. Actual JAGUAR controller/mover tables and target00137D90 corroborate the pointer map. Cross input changed controls+0x214 to1.0 and wheel/motion fields changed during roughly3 seconds of non-atomic samples. The initial scenario has an original replay object, so this is not a controlled fresh-race comparison. No Verified promotion.

The user explicitly requested lightweight scripts/PINE instead of the Computer Use plugin after its initial discovery call. Subsequent runtime operations used small window captures, native input and read-only PINE; Ghidra remained read-only. Original Pad1 SDL bindings were restored via a temporary input profile, compared equal to the backup, and both task-created profiles removed. No save slot was written. PCSX2 was left paused for continuation.

DrivingLab loads the original911TURBO and reports the00137100 constructor gap on Run/Step. Simulation timing, initialization, full wheel force/drive integration, surface query00152070 and synchronized comparisons are still unrecovered. The floor is a visual fixture only. **This task is not complete and the scene is not drivable.**

## Validation

Passed so far:

- `cmake --build --preset native-debug -j6`; `ctest --preset native-debug`:37/37 passed (58.63s), including finite leaf/session/config tests.
- `cmake --preset godot-debug -DHP2_ENABLE_DEBUG_CONSOLE=ON -DPORT_ENABLE_GPU_TESTS=OFF`; build; `ctest --preset godot-debug`:42/42 passed (70.24s), before adding the asset-backed host test and final scoped refinements.
- Console-off Debug: configure OFF, build ps2_port; `ctest --preset godot-debug -R 'driving_.*host|debug_console_host|debug_viewers_host' --output-on-failure`:4/4 passed, including the actual constructor gap and independent original-resource wheel publications.
- Release: configure godot-release with console/GPU OFF, force relink the shared extension, build ps2_port; direct DrivingLab headless entry prints the disabled diagnostic and exits1 as required. `ctest --test-dir build/godot-release -R 'debug_console_host|debug_viewers_host' --output-on-failure`:2/2 passed. Full Release suite not repeated.
- `py -3 reverse/scripts/inspect_driving_lab.py --elf D:/ps2_game/SLUS_203.62`:37 captured/reused read-only queries with matching ELF SHA; raw results ignored.
- `py -3 -m tools.driving_oracle` and `--output local/task0038/pcsx2/after-throttle.json`:two identical paused captures per file,18 bounded windows each. This is layout observation, not original/native comparison.
- Final Debug/console ON: rebuilt, then `ctest --preset godot-debug -R 'driving_|vehicle_model|viewer_session|debug_console|debug_viewers_host|car_render_host|pipeline_contracts' --output-on-failure`:10/10 passed (21.83s). After the last numeric-field/UI refinement, `ctest --preset godot-debug -R 'driving_.*host|debug_console$' --output-on-failure`:3/3 passed (2.42s).
- Final native contract/publication refinement: `cmake --build --preset native-debug --target driving_lab_tests vehicle_model_tests -j6`; `ctest --preset native-debug -R 'driving_lab|vehicle_model' --output-on-failure`:2/2 passed (14.73s).
- `py -3 -m unittest tools.tests.test_driving_oracle`:4/4 passed; rejects running/wrong-game/null-World/truncated/out-of-range captures and uses read opcodes only.
- Windowed Debug launch with `--max-fps 60 --quit-after 180` and `--driving-capture=D:/dev/hp2_godot/local/task0038/driving-lab.png`:exit0, capture0. Inspected complete911TURBO and reachable panel controls/unknown telemetry. Initial tiny inherited layout was corrected by a dedicated entry size; smoke runs now use separate settings.
- `py -3 reverse/scripts/generate_class_templates.py --write`; `py -3 reverse/scripts/generate_progress.py`; generator checks and `git diff --check`:passed. Debug console ON/GPU tests OFF restored; final linked extension is Debug. No commit created.

Toolchain: CMake3.29.2, Ninja1.12.0, GCC13.2.0, Python3.14.4, Godot4.4.1 stable49a5bc7b6, Compatibility/OpenGL Intel Arc B390 driver32.0.101.9150. Ghidra MCP is live at the documented endpoint; its application build version was not exposed by the queried metadata (prior repository version12.0.4 not revalidated).

No original equivalence, completed production simulation step or native acceleration/turn/brake/stop trajectory test is claimed. Those tests require the missing behavior rather than synthetic expectations.

## Next action

Continue TASK-0038: read docs/DRIVING_LAB.md and the init/solver closure maps linked
there. Recover0018F808/0018F838/0018AC38 conversion/table contracts and0018AAB0
engine evaluation; recover001A4930/001A4EE0 tire dependencies and00138E08 body
dispatch. Resolve0019C238/0019C808 adjusted virtual calls and late constructor
initialization before installing a production backend. Recover00152070 and road
tables before a synthetic contact provider. For original comparison, enter an
active original driving scene, pause, then run `py -3 -m tools.pcsx2_mcp`; current
World is zero. Use owned entry/return breakpoints, not the custom server's step.
Keep the task active and production whole entries throwing. No commit created.

## Continuation session - 2026-10-17: dependency algorithms and custom PCSX2

Scope: continue TASK-0038's existing single-car dependency closure. Preserve all
pre-existing uncommitted DrivingLab changes. Acceptance checks for this slice:
listing-backed native functions, independent instruction fixtures, bounded runtime
capture tooling, exact remaining dependencies, native/Godot builds and regressions.
The complete driving and original-comparison acceptance criteria above stay open.

Implemented32 new bounded address projections:12 initialization functions
(E-0038-I01..I12),8 drivetrain/selector entries(E-0038-D01..D07) and12 solver/math/
suspension entries(E-0038-S01..S11). The original00137C88/001A4E88 implementations
are preserved. All new code is Godot-independent and explicitly port-side typed
projection or little-endian byte view. Whole00137100 is NOT defined by its new
dependency file and its production template still throws.00138F38 orchestration
preserves five iterations and order, with unresolved dependency hooks throwing.
0018B948/0018BAA0 leave0018AAB0 evaluation explicit and throwing when absent.

Independent review corrected a potential ADDIU upper-bound overflow discrepancy:
selector config+0x18 plus one now wraps32 bits before signed comparison, with
INT32_MAX regression coverage. Other instruction-derived cases cover each wheel's
gates, strict equality limits, original front/rear numerator asymmetry, exact
multiply grouping, untouched constructor bytes, vector W lanes, matrix inversion,
force caps, solver exclusions and call order. No physical units are invented.

Evidence maps:

- `reverse/driving/task0038-init-closure.md` (E-0038-I13).
- `reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/task0038-solver-dependency-closure.md`.
- `reverse/driving/task0038-runtime-mcp.md` (E-0038-007).
- New per-function records and32 symbol rows; `driving/native-projections.md`
  records bounded field access groups. No Verified promotion.

Supplied PCSX2 custom executable runs build d75a0ad, SLUS-20362/CRC1d2818af,
reported game version1.00. JSON/TCP21512 supports pause/registers/breakpoints;
its packaged MCP bridge uses this server. Source audit found step merely sets a
temporaryPC+4 breakpoint; this is not branch/delay-slot exact stepping. New helper
defaults to read-only paused capture and offers explicit bounded owned-breakpoint
execution with pause/cleanup. Current World00330288/replay00330148 are zero.
A natural00137C88 breakpoint timed out after3seconds. Final original state is
paused at00111F0C, cycles4032240318; owned breakpoint removed. FCR31 was captured
as01000061; conversion semantics are not inferred from that raw value. No original
numerical comparison, input/settings edits, save writes or restart occurred.

Validation commands and results:

- Initial `cmake --build --preset native-debug --target driving_drivetrain_tests -j6`
  failed on an incorrect integer address argument to `not_implemented`; corrected
  to its required string address, then the target and fixtures passed.
- `cmake --build --preset native-debug -j6`:passed.
- `ctest --preset native-debug --output-on-failure`:40/40 passed(41.71s).
  The last001A4778 addition was then rebuilt with
  `cmake --build --preset native-debug --target driving_solver_tests -j6`;
  `ctest --preset native-debug -R '^driving_(init|solver|drivetrain)$' --output-on-failure`:
  3/3 passed(0.26s), covering all final new sources.
- `cmake --build --preset godot-debug -j6`:passed, linked Debug ps2_port.dll.
- `ctest --preset godot-debug --output-on-failure`:45/46 passed(52.12s);
  only progress_tables failed while documentation/index updates were in progress.
  Regenerated tables; `py -3 reverse/scripts/generate_progress.py --check`:passed;
  `ctest --preset godot-debug --rerun-failed --output-on-failure`:1/1 passed(0.34s).
  All46 checks have passing results; no final full-suite rerun is claimed.
- `py -3 -m unittest tools.tests.test_pcsx2_mcp tools.tests.test_driving_oracle`:
  12/12 passed. Live `py -3 -m tools.pcsx2_mcp`:stable paused register capture passed.
- `py -3 reverse/scripts/inspect_driving_init.py --elf D:/ps2_game/SLUS_203.62`:
  66 read-only SHA-bound queries captured.
- `py -3 reverse/scripts/inspect_driving_drivetrain.py --elf D:/ps2_game/SLUS_203.62`:
  16 read-only SHA-bound queries captured. Solver queries/hash manifest retained
  under ignored local/task0038/solver; reproduction ranges are in its records.
- `git diff --check`:passed; generated tracker checks passed after regeneration.
  Tool versions rechecked:CMake3.29.2, Ninja1.12.0, Python3.14.4, GCC13.2.0;
  godot-cpp pin e4b7c25e721ce3435a029087e3917a30aa73f06b matches docs.

Limitations: no continuous driving integration or original equivalence; constructor,
tire and road closure remains open. Release/console-off and windowed visual checks
were not repeated: this slice changes native dependency algorithms and tooling,
not host UI/input/scene behavior. Existing host tests passed, including the actual
production constructor gap. No restricted material staged or committed. No commit
was created; the task remains active.


## Final implementation continuation - 2026-10-17

Accepted minimum-loop scope is implemented. TASK-0038 stays active for its explicitly
deferred synchronized original comparison; no gameplay dependency remains open for the
supported911TURBO synthetic-plane acceptance sequence. Original tracks/AI/collisions and
unsupported mover contexts remain outside this minimum loop and throw where reached.

Implemented closures: native constructor/reset and adjusted component dispatch;
regular/irregular original tables, engine/transmission; original steering/trig/tire;
selected-plane contact/roughness adapter; body/aero/force derivatives, five-pass solver,
component integration and publication; original clock clamp/quantization/subdivision.
NativeBackend reads SHA-bound ELF static data and the original car row, owns one bounded
logical memory state and commits only finite complete steps. Session records elapsed
clock input along with controls and respects the recovered scheduler's minimum while
accumulating render intervals. Godot consumes absolute body/four-wheel poses.

Integration findings/counterevidence:

- Port controller allocation0xA0 was too short for original001A6330+A0 stores and
  overlapped a subsequent list node. Corrected to0xB0 with unmapped allocation guards
  and a regression preserving subsequent list links. This is a port storage bound,
  not a recovered original object-size declaration.
- Original zero-wheel drivetrain ratios reach DIV.S0/0. Host IEEE NaN cannot be
  substituted for EE signed finite saturation. E-0038-E12 implements only the
  finite/signed-zero ISA seam at those audited instructions, with signed-zero fixtures.
- E-0038-E13 retires the incorrect blanket mode4 exclusion. Original Race4+A758zero
  is the running context and skips launch/freeze; diagnostic4+A758nonzero is separate.
- E-0038-E14 anchors controls+218 footbrake versus+21C handbrake to original event
  names, callback writers and front/rear factors. The tool now requests footbrake.
- Spawn centerZ is geometric min(planeHeight+radius-localWheelZ), derived from the
  original row, not a guessed equilibrium force. Static table/math initializers run
  before constructor/reset. Source/host unused temporaryW policies are explicit.

Evidence: I14..I34 constructor/schedule/virtual leaves, E01..E14 engine/mover/ISA,
T01..T17 tire/contact/math, B01..B13 body/publication/loader and C02 surface roughness.
140numeric driving function records are indexed; existing primary unknown records
and class-template boundaries are retained. No Verified promotion or proprietary input
added to Git. Source identifier provenance and exact SHA/revision accompany records.

Behavior acceptance uses original ELF and GLOBALB/911TURBO data:2s rest,4s throttle,
opposite steering from matching prefixes,4s footbrake and held rest. Observed speeds
in source units:0 ->11.7256(1s throttle) ->32.307(4s) ->0 after braking. Opposite lateral
changes relative to straight travel:-7.20283/+7.20529; no exact symmetry is assumed.
These thresholds/results are qualitative port usability checks, not original trajectories.
180mixed-input reset frames repeat all publications exactly;132 recorded Session frames
replay exactly with changed render pacing, including pause/inputclear/single-step.
Extreme finite steering reaches an explicit CVT.W.S gap; rejected candidates and invalid
elapsed arguments leave subsequent31 complete frames identical to an untouched backend.
Wrong-SHA synthetic input is rejected and its temporary file removed.

Godot asset smoke runs540 production steps, applies real poses and replays all body/wheel
publications exactly. Windowed capture local/task0038/native-driving.png was inspected:
complete911TURBO, four-wheel publication, stopped telemetry and accessible panel controls.
The capture is ignored; no original numerical equivalence is inferred from the image.

### Final validation

- `cmake --build --preset native-debug -j6`; `ctest --preset native-debug --output-on-failure`:
  **47/47 passed,48.12s** after final native changes. Earlier full attempt had only stale
  class-template/progress generated artifacts; regenerated them and final full run passed.
- `cmake --preset godot-debug -DHP2_ENABLE_DEBUG_CONSOLE=OFF -DPORT_ENABLE_GPU_TESTS=OFF`;
  `cmake --build --preset godot-debug --target ps2_port -j6`;
  `ctest --preset godot-debug -R 'driving_.*host|debug_console_host|debug_viewers_host' --output-on-failure`:
  **4/4 passed,6.30s**, including continuous production sequence/replay.
- `cmake --preset godot-release -DHP2_ENABLE_DEBUG_CONSOLE=OFF -DPORT_ENABLE_GPU_TESTS=OFF`;
  `cmake --build --preset godot-release --target ps2_port -j6`:passed. Direct headless
  DrivingLab prints disabled and exits1 (explicit exit assertion passed).
  `ctest --test-dir build/godot-release -R 'debug_console_host|debug_viewers_host' --output-on-failure`:
  **2/2 passed,0.77s**. Full Release suite/GPU numeric oracle not repeated.
- Restored Debug console ON/GPU tests OFF, removed the generated shared DLL to force Debug
  relink, then `cmake --build --preset godot-debug -j6`:passed without new compiler warnings.
- `ctest --preset godot-debug --output-on-failure`:**53/53 passed,63.61s** with final Debug console ON and GPU tests OFF.
- Windowed pinned Godot invocation documented in DRIVING_LAB.md:exit0, screenshot save0,
  production acceleration/turn/brake/replay assertions passed.
- `py -3 -m unittest tools.tests.test_pcsx2_mcp tools.tests.test_driving_oracle`:12/12 passed.
  Constructor/tire fresh read-only capture scripts and manifests are in their closure maps;
  auxiliary class metadata tests7/7 passed. No emulator state/settings mutation this slice.
- `py -3 reverse/scripts/generate_class_templates.py --write`; progress generator and both
  checks passed. Final diff/restricted-file checks recorded after documentation completion.

Tool versions: CMake3.29.2, Ninja1.12.0, GCC13.2.0, Python3.14.4,
Godot4.4.1 stable49a5bc7b6, pinned godot-cpp e4b7c25e721ce3435a029087e3917a30aa73f06b,
Compatibility/OpenGL Intel Arc B390 driver32.0.101.9150. Read-only Ghidra endpoint live;
application build not independently revalidated. PCSX2 custom buildd75a0ad capability
record retained; current original World absent and emulator left untouched/paused.
No commit created; existing user changes preserved.

### Exact continuation

For use: build godot-debug and launch `drivinglab`; W/S/A/D, F5/F6/F7/R and panel
Record/Replay are available. Local original ELF config is ignored and already points
to the authorized external executable on this machine.

For remaining TASK-0038 original certification: enter an active original driving scene,
pause and run `py -3 -m tools.pcsx2_mcp`. Establish matching owned function entry/return
captures with equal controls/contact/initial state, predeclare comparison precision and
record differences. CustomPC+4 step is insufficient across branches/delay slots. Keep
unsupported contexts throwing and nothing Verified until this scoped comparison exists.


## User correction scope - 2026-10-17

Active implementation: merge the separate DrivingLab scene into CarViewer as a Test drive
mode reached from its existing ImGui panel; remove newly introduced F5/F6/F7 driving
shortcuts; correct host left/right steering mapping against original angle evidence;
correct driving right-wheel orientation by tracing the original publication and preview
attachment path. Preserve recovered force algorithms. Acceptance: same scene/renderer/
selected vehicle and skin across enter/exit; button-only simulation lifecycle; original
steering convention; correct four-wheel bindings, focus/F8 isolation, scene lifecycle,
native/Godot host regressions and window inspection. The previous separate-scene design
is superseded by this explicit user request. No original equivalence promotion.


## User correction completion - 2026-10-17

All four requested corrections are implemented:

1. Removed new simulation F5/F6/F7 handlers and hints. Run/Pause/Step/Reset are ImGui
   actions. Existing CarViewer R/F camera shortcuts remain unchanged.
2. Removed driving_lab.tscn. CarViewer's existing ImGui panel has Test drive and Return
   to preview. A RefCounted driving controller reuses the loaded selected car/skin,
   renderer, scene and camera; the native config is selected from that same car code.
   Preview animation/auto-orbit is suspended while driving. Cached reload restores preview
   attachment bindings, skin, LOD preference and prior camera on exit. Preview animation
   stays suspended until the replacement cached model commits. Legacy drivinglab commands
   route to this same CarViewer mode; no independent driving scene remains.
3. E-0038-U01 anchors native positive steering to source+Y/driver left. Host keyboard
   changed fromD-A toA-D; stickX is negated.001A5D60 and recovered force logic are unchanged.
   Stronger backend assertions require actual left/right signs rather than only opposition.
4. E-0038-W01 traces the reported right-wheel half-turn discrepancy to the missing
   render-only00120EF8 stage after raw001C0D78 matrices. Preview and driving now share that
   recovered attachment composition, including distinct tire/blur and brake rules. Raw
   physics/snapshot matrices are unchanged; no guessed per-side rotation is appended.
   A related existing preview brake/blur group-selection mistake now uses the original
   brake matrix. Zero/nonzero spin and all four wheels/layers have explicit regressions.

Validation:

- `cmake --build --preset native-debug -j6`:passed. `ctest --preset native-debug -R
  '^(driving_backend|vehicle_model|car_render_bindings|debug_console)$' --output-on-failure`:
  4/4 passed16.46s. Independent steering, attachment and real-ImGui click regressions pass.
- `cmake --build --preset godot-debug -j6`; `ctest --preset godot-debug --output-on-failure`:
  53/53 passed55.52s, including merged-scene540-step native driving/replay, mode enter/exit,
  selected model/skin/renderer/camera identity, preview restoration, direction input,
  noF5/F6/F7 behavior and focus/F8 pauses.
- Pinned windowed Godot run against car_viewer.tscn with --driving-asset-smoke and preview,
  rest and completed-drive captures:exit0, all captures saved. Inspected ignored
  local/task0038/unified-preview.png, unified-drive-rest.png and unified-drive.png.
  Preview and rest views retain matching wheel-face orientation; the single CarViewer
  panel exposes Test drive / Return to preview. Return restores camera before preview
  auto-orbit resumes.
- Initial host attempts exposed a missing autoload mode-request variable and preview
  animation being invoked against the old driving model during cached restoration.
  Both corrected; final full suite passed. An intermediate stale error assertion failed
  explicitly, then the mode-transition ownership fix made it pass. DLL linking initially
  failed because the running frontend held the DLL; closed only that standalone run,
  then rebuilt successfully. No editor/save operations or emulator mutations.
- Console-off/Release and GPU numerical oracle were not repeated for this correction;
  prior gating checks remain documented above. Mode follows existing CarViewer Debug
  gating and existing console-independent ImGui actions. Full Debug host regressions pass.
- Fresh read-only evidence and raw output hashes stay in ignored task directories;
  public E-U01/W01 ledgers and00120EF8 symbol evidence updated. No Verified promotion.

Current next action: use CarViewer's Test drive button to inspect the corrected mode.
TASK-0038's deferred synchronized original numerical comparison remains open, with the
same pause/owned-breakpoint prerequisites documented above. No commit created. Preserve
concurrent user changes outside this correction (including frontend/render-frame work).


## WASD follow-up scope - 2026-10-17

User reports Test drive accepts no WASD. Confirmed host defects: start() leaves Session
paused, and asset_viewer._unhandled_input gates keyboard by combined mouse-or-keyboard
capture, so hovering the ImGui panel drops otherwise unclaimed driving keys. Fix tool
lifecycle/input only: enter Test drive running, respect keyboard capture separately from
mouse hover, retain explicit resume after focus/F8 pause. Add full Input.parse_input_event
regression with panel hover and actual driving process/snapshots; do not rely on direct
held-dictionary/native-step injection as proof of playable keyboard input. Physics unchanged.


### WASD follow-up result

Fixed both observed port-side causes: Test drive immediately runs the Session; keyboard
input no longer uses the combined mouse-hover capture gate. Mouse/camera isolation and
actual keyboard capture/F8/focus pause remain intact, and explicit pauses do not auto-resume.
No recovered native algorithm or new shortcut changed.

The strengthened asset host smoke uses Input.parse_input_event for real Godot W/A/S/D
press/releases while hovering a full-size ImGui panel, verifies held W reaches normalized
native throttle1 and moves the car, verifies A/D signed steering and S brake, and verifies
all controls clear on release. It then runs existing no-function-key, pause/focus/F8,
540-step native trajectory, exact replay and preview/camera restoration checks. Previous
acceptance had directly injected held state or native input; it missed this keyboard gate.
The headless harness now sets1280x800 before hover testing (default64x64 is too small).
An initial typed-variable inference error and that too-small fixture were corrected before
final runs; no weakened motion/capture assertions.

Validation commands:

- `ctest --preset godot-debug -R 'driving_.*host|debug_viewers_host|debug_console_host' --output-on-failure`:
  **4/4 passed,5.89s**. Current build includes concurrent TASK-0039 changes; no unrelated
  files modified by this fix. Native rebuild is unnecessary for this GDScript-only change.
- Pinned windowed Godot against car_viewer.tscn with --max-fps 60 --quit-after 1200 and
  --driving-asset-smoke --driving-capture=D:/dev/hp2_godot/local/task0038/wasd-input-fixed.png:
  **exit0**, all normal-input/motion and existing host assertions passed. Capture ignored.
- Generators/diff checks rerun after documentation. Full native/Godot suites, Release,
  console-off and GPU numeric comparison not repeated for this script-only correction.

Use Test drive, then W/S/A/D immediately. After F8/focus/Pause, use Run to resume.
TASK-0038 original comparison remains deferred; no Verified promotion or commit.


## Accepted third-person Test drive redesign - 2026-10-17

Implement the user-approved one-click driving plan: validate initialization before entry,
minimal Return to CarViewer/hints only, no user Run/Pause/Step/Reset/Record/Replay controls,
captured mouse orbit and vehicle-rear camera with idle recenter, Esc pointer release/world
click capture, automatic temporary suspension/resume for F8/focus, actual keyboard capture
submits zero input, terminal errors retain last complete pose without retries. Restore
preview model/skin/settings/camera on exit. Keep native algorithms and raw/render wheel
contracts unchanged. Acceptance includes actual ImGui click + normal/OS keyboard/mouse,
focus/console/capture transitions, camera math, repeated entry/exit, failures and regressions.
Concurrent TASK-0039/0040 changes are outside this scope and must be preserved.


## Third-person redesign completion - 2026-10-17

Implemented the accepted direct-drive experience in the existing scene/renderer:

- Off/Starting/Driving/Suspended/Error phases separate initialization, temporary suspension
  and genuine failure. Original config and complete first native pose must succeed before
  activation; startup failure stays in preview with its reason visible without Advanced.
  Cached LOD preparation completes before capture. Runtime native/render error retains
  the last complete displayed pose, releases the pointer and never retries automatically.
- The mode panel has only Return to CarViewer and concise input/scope hints. Removed all
  manual Run/Pause/Step/Reset/record/replay actions from the user-facing controller/UI.
  Native Session instrumentation remains unchanged for deterministic tests. No new function
  keys; existing preview camera keys cannot interfere with active driving.
- Captured relative mouse bypasses ImGui mouse processing; WASD uses normal key delivery
  with confirmed native left/right sign. Esc releases/clears input without pausing, world
  click recaptures. Actual UI keyboard capture zeroes controls while simulation continues.
  Camera release is decoupled from native pause. Pointer intent is distinct from actual
  display capture; headless DisplayServer cannot establish OS capture.
- Focus/F8 transitions clear keys and temporarily pause once, resume automatically once,
  preserve capture intent and discard the next elapsed interval even when notification
  code resumed the Session before the next process frame. No paused-duration catch-up.
- Rear camera uses confirmed host body+Z forward, transformed local bounds center,
  max(4,1.5*length) distance,15degree pitch and60degree FOV. Mouse0.15degree/pixel,
  pitch5..65degrees, wrapped yaw,1second idle and4/s exponential recenter. This is an
  explicitly port-side camera and does not claim recovered GTA/HP2 camera behavior.
- Return preserves selected car/skin and renderer identity, restores preview settings,
  camera/FOV and cached wheel bindings, and supports immediate repeated entry. Native
  driving/force/steering/math/wheel render algorithms were not changed in this redesign.

Acceptance no longer relies on injecting control dictionaries alone. The Godot host smoke
clicks actual ImGui entry/return, sends Input.parse_input_event keys/relative mouse, and
checks native motion, signed steering, brake/release, sustained turn with rear-camera
tracking, mouse idle/wrapped recenter, Esc/panel hover/world click, keyboard-capture zeroing,
F8/focus resume and elapsed discard. Injected startup/runtime gaps verify preview atomicity,
last-pose retention and no automatic retry. Repeated return/re-entry preserves model/skin,
preview/FOV/orbit settings and restores wheel animation.

Added tools/test_drive_window.py for actual Win32 SendInput against a dedicated owned Godot
window. Before each input it verifies foreground HWND/PID. Its short-lived topmost window
avoids other preview windows taking clicks; it also creates/destroys its own focus probe.
Reports/screenshots go under ignored local/task0038/third-person-os/. The OS-only harness
checks actual mouse capture separately from logical headless capture intent and balances
owned-process cleanup. It requires Windows and Pillow; it is not automatically included in
headless CTest and never directs input to unrelated apps.

### Validation

- `cmake --build --preset godot-debug -j6`:passed, final extension Debug console ON/GPU OFF.
  `ctest --preset godot-debug --output-on-failure`:**54/54 passed,52.01s**, including current
  concurrent frontend work and all new interaction/camera/error/preview checks.
- `cmake --build --preset native-debug --target debug_console_tests -j6`;
  `ctest --preset native-debug -R '^(debug_console|driving_backend|vehicle_model|car_render_bindings)$' --output-on-failure`:
  **4/4 passed,13.79s**. Native deterministic driving and original render-basis fixtures
  remain passing; UI checks ensure former toolbar locations cannot emit those actions.
- `py -3 tools/test_drive_window.py`:**22 real OS checks passed**, final owned PID69684 in
  the first successful full run and a second complete successful run after the resumed
  elapsed-time fix. Final report contains its actual PID. Checks include real entry click,
  W movement/velocity, A/D/S/releases, relative mouse and idle recenter, Esc/recapture,
  F8 suspend/resume, real focus loss/return with stable suspended step count, W after
  resume, return and re-entry/W/second return. No manual Run or control injection.
- Inspected local/task0038/third-person-os/driving.png: rear view, complete911TURBO,
  minimal return/hints panel and60FPS observation. This is usability evidence, not a GPU
  numerical oracle or original equivalence. Report/log/state/screenshot are ignored.
- Initial headless check correctly exposed that mouse_get_mode cannot prove OS capture;
  capture intent now has an explicit controller contract, and the real OS test separately
  requires actual captured/visible mouse mode. Early OS attempts exited before re-entry
  or failed foreground isolation; the harness now uses sufficient lifetime, PID guards and
  owned-window isolation. No acceptance assertion was relaxed to conceal input failure.
- `py -3 reverse/scripts/generate_progress.py` and both generator checks/diff checks passed
  after documentation. Console-off/Release/full native/GPU numeric oracle not repeated in
  this scoped tool redesign. Previous gating results remain recorded; Debug full suite
  includes console-independent UI unit checks and existing gating/scene regressions.

Tool versions unchanged: Godot4.4.1 stable49a5bc7b6, CMake3.29.2, Ninja1.12.0,
GCC13.2.0, Python3.14.4; Compatibility/OpenGL Intel Arc B390 driver32.0.101.9150.
No emulator/save/settings mutations or proprietary inputs committed. Concurrent frontend
and symbol-recovery edits are preserved. No commit created; no Verified promotion.

Continuation: open CarViewer and use its Test drive button directly. Remaining TASK-0038
work is the previously deferred synchronized original numerical comparison, with original
active-scene/paused/owned entry-return prerequisites above. This redesign is complete.
