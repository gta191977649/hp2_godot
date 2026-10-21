# TASK-0079 - Driving chain and per-frame state lifecycle

Status: Completed (bounded source audit and state reset; not original equivalence)
Date: 2026-10-21 Australia/Sydney

## Scope

Audit the requested Debug CarDriver, CarState, DriveTrain and Engine coverage
against the current native controller. Recover and connect necessary missing
per-frame state functions using read-only Ghidra evidence. Preserve all existing
workspace changes. Behavior authority is Debug NFSHP2.ELF SHA-256
815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b;
revision/build date remains independently unknown.

## Acceptance

- [x] Record used functions, unused boundaries and conservative progress limits.
- [x] Recover CarState::BeginTimestep 001BD938 and its original calling context.
- [x] Connect the reset once per simulation frame, before contact accumulation;
      preserve accumulation across collision retries within the same frame.
- [x] Test reset write extent, terrain transitions, and native integration.
- [x] Run relevant builds/tests, refresh generated progress, record limitations.

## Evidence and continuation

E-0079-S01: BeginTimestep 001BD938 decompile/listing, xref00131E50,
Car::BeginMovement00131D78. Findings and source audit are in
reverse/task0079/driving-chain.md. No original-runtime comparison or Verified claim.

## Validation

- `cmake --build --preset native-debug --target driving_player_tests driving_backend_tests driving_engine_tests driving_drivetrain_tests driving_surface_tests driving_mover_tests -j 6`: passed.
- `ctest --preset native-debug -R '^(driving_player|driving_backend|driving_engine|driving_drivetrain|driving_surfaces|driving_mover)$' --output-on-failure`:6/6 passed,26.21s; original inputs available and SHA-bound backend ran, not skipped.
- `cmake --build --preset godot-debug --target ps2_port -j 6`: passed, extension rebuilt.
- `py -3 -m unittest discover -s reverse/tests -p test_debug_driving_chain.py`:6 passed.
- `py -3 -m unittest discover -s reverse/tests -p test_recovery_progress.py`:29 passed.
- `ctest --preset native-debug -R '^(debug_driving_migration_rules|debug_driving_single_chain|progress_tables|existing_body_preservation|complete_class_members)$' --output-on-failure`: initial4/5 passed; preservation correctly rejected the two intentional new function identities under the old TASK0077 rendering-only freeze.
- Added the strict TASK0079 successor for exactly those two functions, preserving frozen predecessor JSON and unrelated identities/auxiliary pins. `ctest --preset native-debug -R 'existing_body_preservation' --output-on-failure`:1/1 passed,7.87s after the amendment.
- `py -3 -m unittest discover -s reverse/tests -p test_task0079_source_review.py`:5 passed; the same command with test_task0077_source_review.py:5 passed. Tests reject scope widening, unrelated changes, predecessor changes and unexpected stub transitions.
- `py -3 reverse/scripts/generate_progress.py` and `py -3 reverse/scripts/generate_progress.py --check`: passed. `git diff --check`: passed (line-ending notices only).

Tool versions: CMake3.29.2, Ninja1.12.0, GCC13.2.0, Python3.14.4;
godot-cpp e4b7c25e721ce3435a029087e3917a30aa73f06b matches pin. Existing
native_backend.hpp:25 misleading-indentation warning retained. No graphical run,
full suite or synchronized original comparison; no architecture/physics model
change. No commit created. Pre-existing workspace edits preserved.

## Next action

Use this task and docs/DRIVING_GAPS.md as the continuation baseline. Open a new
scoped task for original Player nitro/radial-blur dependencies before enabling
nitro input; retain addressed throws until those dependencies are recovered.
