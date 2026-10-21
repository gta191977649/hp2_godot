# TASK-0076 source review and validation

## E-0076-R01: port-side render lifetime and reflection policy

User request: retain a track that vanishes mid-drive and disable costly vehicle
reflections. This is host policy evidence, not recovered PS2 behavior. No original
function or verification status changes. TASK0075 frozen review/scope remain pinned;
only its live checker delegates to this successor. Six exact function slots and
explicit auxiliary hashes are permitted; all other identity changes fail closed.

RenderSettings defaults and restore now deny Environment; the existing UI can
explicitly enable it. This prevents capture-view selection/submission and viewport
work, rather than merely hiding the result. Other effect permissions remain.

Previously update_view and complete_visual_readback caught rendering exceptions
and called release_display, reset scene/environment and invalidated ok. Car display
is independently owned. Those catches now retain loaded scene/collision/resources,
expose render_ok/render_error/readback_error, warn once per distinct failure and
retry the normal path on subsequent calls. Successful calls clear their own gap.
Explicit unload and actual load failure still release content. No alternate PS2
behavior or substitute render output is introduced.

The supplied screenshot is consistent with this teardown mechanism, but the exact
exception in that session is unknown. Existing app logs did not identify it.
The regression injects a real missing-active-Camera3D exception after rendering
Track11: topology and geometry survive, and valid camera rendering recovers.
Readback retention is source checked; an actual readback fault was not injected.

## Validation (2026-10-20)

Tools: Godot4.7.2 ed1daf0bf, Vulkan1.4.359 Forward+ Intel Arc B390; CMake/Ninja,
GCC as recorded in local/task0076/build-final.log and CMakeCache.txt. Python via py -3.

- Main build was blocked by the user's running game holding godot/bin/ps2_port.dll.
  User processes were preserved. Built the identical source into isolated project:
  `cmake --preset godot-debug -DPORT_GODOT_BIN_DIR=D:/dev/hp2_godot/local/task0076/project/bin -DPORT_GODOT_PROJECT_DIR=D:/dev/hp2_godot/local/task0076/project`
  `cmake --build --preset godot-debug -j4` passed. Imported isolated project with
  Godot `--headless --editor --path local/task0076/project --import`.
- `ctest --preset godot-debug --timeout 60 -R '^(track_effects|track_controls|track_render_retention_host|race_runtime_host|driving_particles_host|track_default_projection_host)$' --output-on-failure`
  passed5 selected tests (track_controls was not a test name).
- `local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe --path local/task0076/project --script res://scripts/runtime/track_retention_smoke.gd -- --retention-capture=D:/dev/hp2_godot/local/task0076/retained-track.png`
  passed with only the deliberately injected warning. Inspected recovered road
  capture. Default submits only main view1; explicit reflection opt-in produces
  capture views and available atlas; restore removes capture work again.
- Earlier screenshot save attempt used an invalid project-relative path; corrected
  fixture accepts an explicit absolute output and checks save result. Clean rerun
  above supersedes that failed attempt.

No FPS improvement percentage or original-game equivalence is claimed. The user app later exited independently. Normal CMake output paths were restored;
main Debug build passed and godot/bin contains the new DLL for next launch.
No proprietary assets or screenshots are tracked.

Continuation: TASK-0076; if disappearance recurs on the rebuilt app, capture the
new 'Track retained after ... gap' warning and render_error/readback_error state.
Investigate that precise throwing dependency before claiming the field cause fixed.

Final: strict successor audit passes (6 replaced /6 reconstructed exact slots);
14 TASK0075 review contracts pass. controls host also passes. Main output rebuild
passed after the app independently exited. CMake3.29.2/Ninja1.12.0/GCC13.2.0,
Python3.14.4. Readback failure itself remains source checked only.


Final main-project CTest command: `ctest --preset godot-debug --timeout 60 -R '^(track_controls_host|track_render_retention_host|track_optimization_reflection_gpu|driving_effect_source_review_rules|driving_effect_review_contracts)$' --output-on-failure` passed5/5 in36.91s.
`ctest --preset godot-debug -R '^progress_tables$' --output-on-failure` passed1/1.
Scoped source review and diff inspection passed; pre-existing user changes preserved.
