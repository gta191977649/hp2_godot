# TASK-0044 - Debug vehicle driving audit and canonical implementation

- Status: completed
- Certification: original numerical comparison blocked; not Verified.
- Related: TASK-0038, TASK-0041, TASK-0042, TASK-0043.
- Target: user-authorized NFSHP2.ELF, SHA-256 815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b. Debug build date is user-provided, not independently verified.
- Decision: the user explicitly selected Debug behavior over Retail behavior and approved the complete migration plan. This supersedes the Retail-only behavior baseline for this driving closure, not for unrelated systems.
- Objective: audit every existing vehicle-driving dependency against read-only Debug Ghidra evidence; implement one canonical original-class call chain used by Test drive.
- Constraints: preserve historical Retail evidence; no Ghidra mutations; no guessed behavior; PS2 addresses remain explicit; no Godot dependencies in recovered code; missing recovered paths throw hp2::NotImplemented.

## Acceptance

- [x] Complete per-function ledger covering input, initialization, scheduling, steering, engine/transmission, suspension, tires/contact, body/integration/force solver, publication and math/static dependencies.
- [x] Debug addresses, signatures, original owners/source paths, field/global layouts and algorithm differences have recorded evidence.
- [x] Canonical original-class implementations replace driving placeholders and duplicate algorithms; compatibility surfaces only forward.
- [x] Test drive uses only the canonical chain; Debug ELF/configuration/data are identity-bound and cannot mix Retail addresses/layouts.
- [x] Synthetic environment and port input/camera policies remain explicitly distinguished from original behavior.
- [x] Relevant native/Godot/structure/generator regression checks pass and real-window driving interactions are exercised.
- [x] Original Debug synchronized numerical comparison performed, or concrete runtime blocker/reproduction recorded without verification promotion.
- [x] Documentation, evidence, symbols, task and generated progress updated; final diff checked.

## Initial observations

E-0044-I01: live Ghidra contains both SLUS_203.62 and NFSHP2.ELF. The Debug file at the MCP-reported path hashes to the target above. Read-only decompilation of Debug 00137D18 exposes PhysicsCar::Move(float), while Retail 00137D90 remains a separate address/version.

E-0044-I02: current PhysicsCar source contains whole-entry throwing placeholders under hp2::recovered alongside working port::driving projections. NativeBackend calls native::Move, not the original class entry. Merely sharing the original source path has not consolidated implementation ownership.

## Initial scope and next action

Baseline Git status was clean. Apply the REA staged workflow with independent engine, PhysicsCar/timing and solver audits; root owns shared interfaces, version binding, integration and validation. Raw decompilation/listings remain ignored under local/task0044; durable findings are under reverse/task0044. This was the initial scope; results and remaining certification boundary follow.


## Integration results

See [Debug audit](../../docs/DEBUG_DRIVING_AUDIT.md) for the canonical chain, exhaustive
151-entry dispositions, 207 unique Debug records, explicit test policies, corrected
algorithm/data differences and unsupported game paths. Every ledger entry has a disposition;
185 records are Implemented and 22 Decompiled, none Verified. The 14 original-class owners
are canonical; old adapters forward and the old composite solver is removed. Generator and
cross-binary symbol handling are explicit and protected by mutation/collision regressions.

Actual SHA-bound Debug backend acceptance passed rest/acceleration/braking, opposite-turn
trajectories and exact replay. The owned-window Win32 test passed 23 checks, including
physical W/A/D/S, actual entry/return clicks, mouse camera, Esc/F8/focus recovery and repeated
entry. Passing run: local/task0044/window-idle/report.json. Prior desktop-interference
attempts are retained privately; no assertion was relaxed. Temporary camera changes were
withdrawn; only test diagnostics/owned-window focus and target-PID checks were retained.

Original numerical certification: blocked. Exact command
`py -3 -m tools.oracle probe --output local/task0044/pcsx2-probe.json`
returned blocked/timed out; no runnable Debug original scene/process was available. The
existing Retail oracle is invalid for this target. No emulator/save/settings were changed.
This satisfies the approved explicit-blocker reporting alternative; it does not certify
anything. Reproduction and exact continuation prerequisites are in the Debug audit document.

Final validation (2026-10-17):

- `cmake --preset native-debug`; `cmake --build --preset native-debug -j6`: passed.
- `ctest --preset native-debug --output-on-failure`: **64/64 passed**, 142.26 seconds,
  including all driving, class/template/evidence, source migration and generated progress checks.
- `cmake --preset godot-debug`; `cmake --build --preset godot-debug -j6`: passed.
- `ctest --preset godot-debug -R 'driving_.*host|debug_console$|debug_viewers_host|car_render_host' --output-on-failure`:
  **5/5 passed**, 12.17 seconds. `ctest --preset godot-debug -R 'debug_console_host$' --output-on-failure`:
  **1/1 passed**, 0.54 seconds. These include both driving host modes, renderer and viewer.
- `py -3 tools/test_drive_window.py --output local/task0044/window-idle`: passed 23 real
  OS interaction checks; inspected rear-camera screenshot. Requires an idle desktop.
- `py -3 reverse/scripts/generate_class_templates.py --check`: passed.
- `py -3 reverse/scripts/complete_class_members.py`: passed (default mode is check;
  an attempted unsupported `--check` flag was corrected, not counted as a passing run).
- `py -3 reverse/scripts/audit_task0044_migration.py`: passed; 249 before/383 after body
  permissions checked, no unreviewed production body; no acceptance refresh option.
- `py -3 reverse/scripts/generate_progress.py`: generated tables; `progress_tables` passed.
- `git diff --check`: passed. Final inventory inspected: no proprietary/restricted binary
  material or raw runtime captures tracked; all new records UTF-8. The seven pre-existing
  Retail duplicate-address rows remain unchanged; Debug index has no duplicate addresses.

Toolchain: MinGW GCC 13.2.0, CMake 3.29.2, Ninja 1.12.0, Python 3.14.4,
Godot 4.4.1 stable 49a5bc7b6; pinned godot-cpp retained. Window uses Compatibility/OpenGL
Intel Arc B390, driver 32.0.101.9150. Ghidra queries selected NFSHP2.ELF explicitly;
Ghidra server build version was not independently recorded. No commit created.

Initial full-regression failures concerned two synthetic class-generator fixture identities,
historical coverage supersession and stale generated progress tables. These were fixed and
all relevant checks re-run in the final 64/64 suite. The source migration audit was not
weakened to accept changes. No original equivalence or whole-game recovery is claimed.

## Continuation

Migration acceptance is complete under the approved explicit-blocker alternative for
original numerical certification. TASK-0038 retains that separate outstanding work. Connect
PCSX2 to the exact Debug driving scene, establish owned entry/return hooks at 00137D18 and
component entries, build an explicit Debug oracle map, then compare equal initial state,
input, time and contacts. See DEBUG_DRIVING_AUDIT.md. Do not reuse Retail addresses and do
not promote any function to Verified before such evidence exists.
