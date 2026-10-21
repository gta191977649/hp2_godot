# TASK-0080 - Complete current driving-control chain audit

Status: Completed (bounded end-to-end audit, not original equivalence)
Date: 2026-10-21 Australia/Sydney

## Scope

Review input, player/controller, frame lifecycle, drivetrain, tire/suspension/body,
contact/collision, effects and state publication as one connected chain. Compare
the actual supported native fixture with original Debug orchestration. This is
an evidence/source audit, not authorization to invent or implement unknown behavior.
Preserve all pre-existing workspace changes and frozen source-review records.

Driving authority: NFSHP2.ELF SHA-256
815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
Independent date/revision unknown; Retail asset/topology provenance stays separate.

## Acceptance

- [x] Trace host input through publication, separating original logic and host services.
- [x] Reread original Car/World frame orchestration and compare omitted calls.
- [x] Record gaps with concrete source/evidence, impact, priority and next dependency.
- [x] Separate unsupported scenarios from actual supported-path omissions.
- [x] Record checks and update durable progress; no Verified promotion.

## Evidence plan and continuation

E-0080-L01: fresh read-only original lifecycle queries, retained privately in
local/task0080; public findings in reverse/task0080/driving-chain-audit.md.
Input, physics and contact reviews may proceed independently within this one task.
No gameplay code changes planned. Tests are for regression context, not an oracle.

## Findings and validation

- E-0080-L01/I01/P01:36 successful read-only decompilation captures,20 new
  per-function Decompiled records; existing statuses unchanged. Import-file
  SHA and r5900 language rechecked. Public index: reverse/task0080/evidence.json.
- Confirmed missing ResetMe consumer in fixture versus Car::Move00131B90;
  incomplete Begin/FinishMovement route/recovery state. Full gaps/priorities
  and counterevidence to generic nitro-next are in the audit report.
- `cmake --build --preset native-debug --target driving_player_tests driving_backend_tests driving_engine_tests driving_drivetrain_tests driving_surface_tests driving_mover_tests -j 6`: passed, no work needed.
- `ctest --preset native-debug -R '^(driving_player|driving_backend|driving_engine|driving_drivetrain|driving_surfaces|driving_mover)$' --output-on-failure`:6/6 passed,27.14s; no skipped original-data backend checks.
- Pinned Godot4.7.2 headless input-device-probe.gd:18 printed cases, device0
  responds and device1/3 do not with default mappings. Exact command in report.
  Physical hardware/hot-unplug and original runtime were not exercised.
- No new unit tests because production code is unchanged. Source protection,
  metadata consistency and generated-progress checks run at closure.
- `ctest --preset native-debug -R '^(existing_body_preservation|progress_tables)$' --output-on-failure`:2/2 passed,11.12s.
- `py -3 reverse/scripts/generate_progress.py` and `--check`: passed.
  Metadata validation passed36 captures/20 unique SHA-address rows, all new records
  Decompiled with valid record/evidence links. `git diff --check`: passed;
  existing line-ending notices only. Private raw material remains ignored.

Tools: CMake3.29.2, Ninja1.12.0, Python3.14.4, GCC13.2.0 (existing native build),
Godot4.7.2.stable.official.ed1daf0bf, godot-cpp
e4b7c25e721ce3435a029087e3917a30aa73f06b. No full suite/GPU comparison,
proprietary files, Ghidra mutations or commit. Pre-existing edits preserved.

## Exact next action

Read reverse/task0080/driving-chain-audit.md, then create a scoped implementation
task for route/controller/reset ownership at Car::Move00131B90,
ResetToRoutePoint00132FB0 and ResetCore00132DF0, including necessary
BeginMovement00131D78/FinishMovement00131E58 state. Do not substitute host session
restart for original route recovery. Recover missing route data/ownership before
wiring the whole entries; leave unresolved paths throwing. Hardware comparison
requires a runnable matching Debug scene and equal input/time/contact state.
