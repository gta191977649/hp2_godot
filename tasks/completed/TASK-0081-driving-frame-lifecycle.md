# TASK-0081 - Recover driving frame lifecycle and reset prerequisites

Status: Completed (bounded state/reset-leaf stage; full route recovery remains open)
Date: 2026-10-21 Australia/Sydney

## Scope

Continue TASK0080's highest-priority Car lifecycle work. Recover exact frame
motion/state bookkeeping and route-reset dependencies from the authorized
Debug NFSHP2.ELF, SHA-256
815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
Build date/revision independently unknown. Keep Retail asset authority separate.
No synthetic reset-to-spawn substitute and no unrelated input/effects work.

## Acceptance and evidence plan

- [x] Read exact Begin/FinishMovement state arithmetic, offsets and gates;
      implement the supported slice with explicit full-entry boundaries.
- [x] Follow GetRoute/GetRoutePoint/ResetCore dependencies and establish which
      prerequisites exist; implement only evidence-backed reset work.
- [x] Connect frame bookkeeping once around movement/collision retries,
      retaining transactional publication and reporting unsupported recovery.
- [x] Add exact state/write-extent, zero-timestep and backend regression tests.
- [x] Record evidence, limitations, strict source review and validation results;
      regenerate progress and leave an exact continuation.

Primary read-only captures: local/task0081. Public evidence E-0081-L01/R01
and findings: reverse/task0081/lifecycle.md. User files and predecessor review
JSON remain untouched; original-runtime verification remains a separate requirement.

## Implemented result and remaining boundary

Port-side state helpers in original car.cpp now capture previous motion and
complete whole-frame acceleration/odometer/distance exactly once around move and
collision retries. Full BeginMovement/FinishMovement are partial/throwing at their
untranslated topology/path/Player boundary. Full ResetCar001BD6C0 is implemented
and used before fixture placement. Deferred reset requests now fail visibly at
00132FB0 without publishing candidate state. No teleport/session-reset substitute.

Route assets exist, as evidenced by bounded Track11 framing. Missing code is
route/signpost initialization, persistent wrapped track progress, controller point
ownership and ResetCore mover lifetime. This task completes the state/reset-leaf
stage only; it does not complete automatic or player-selected route recovery.

## Validation

- `cmake --build --preset native-debug --target driving_player_tests driving_backend_tests -j 6`: passed.
- `ctest --preset native-debug -R '^(driving_player|driving_backend)$' --output-on-failure`: initial2/2 passed,13.50s; after matching ResetCar store groups/order, rebuilt and rerun2/2 passed,16.81s.
- `cmake --build --preset native-debug --target driving_engine_tests driving_drivetrain_tests driving_surface_tests driving_mover_tests driving_contact_tests driving_track_tests -j 6`: passed.
- `ctest --preset native-debug -R '^(driving_player|driving_backend|driving_engine|driving_drivetrain|driving_surfaces|driving_mover|driving_contacts|driving_tracks)$' --output-on-failure`:8/8 passed,40.75s before the final ResetCar store-order-only adjustment; the directly affected2 tests were rebuilt/rerun afterward as above.
- `cmake --build --preset godot-debug --target ps2_port -j 6`: passed on final source. Existing misleading-indentation warnings in native_backend.hpp/care.cpp retained.
- `py -3 -m unittest discover -s reverse/tests -p test_task0081_source_review.py`:6 passed. Same command with test_task0079_source_review.py:5 passed. Strict successor permits exactly2 replacements/5 additions, with explicit header/test pins; frozen predecessor JSON unchanged.
- Final source-protection, generated-progress and metadata checks are recorded at closure below.
- `ctest --preset native-debug -R '^(complete_class_members|debug_driving_migration_rules|debug_driving_single_chain|existing_body_preservation)$' --output-on-failure`:4/4 passed,27.63s.
- `py -3 reverse/scripts/generate_progress.py` and `--check`: passed.
  Metadata validation passed35 retained captures/14 unique SHA-address records:
  ResetCar plus2 partial lifecycle claims and11 Decompiled route dependencies.
  `git diff --check`: passed; line-ending notices only. Private inputs ignored.
- `ctest --preset native-debug -R '^progress_tables$' --output-on-failure`:1/1 passed,1.34s; final generator --check passed afterward.

Tools: CMake3.29.2, Ninja1.12.0, GCC13.2.0 (native compiler), Python3.14.4;
godot-cpp e4b7c25e721ce3435a029087e3917a30aa73f06b matches pin. Import file SHA
and r5900:LE:32:default rechecked in ignored local/task0081/identity.json.
No original synchronized comparison, full-suite or graphical smoke run; no
Verified promotion or commit. Pre-existing workspace changes retained.

## Exact continuation

Start a scoped route-loader/controller implementation task. Read this task and
reverse/task0081/lifecycle.md, then inspect LoaderTrackRoutes001BFFC8,
TrackRouteManager loader001BF9B0, TrackRoutePoint::GetDistanceAway001BE750,
TrackedController::FindWhereIAm00233F10 and TrackPathCoordinate progress/length-wrap
dependencies. Establish the Retail34121/34122 -> Debug field bridge, not only
framing, before loading those routes. Connect ResetCore00132DF0 and
ResetToRoutePoint00132FB0 only after that ownership exists. Keep the explicit
00132FB0 guard until it can be replaced by the real chain.
