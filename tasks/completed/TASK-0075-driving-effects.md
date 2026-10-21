# TASK-0075 - Driving gap inventory and original wheel particles

- Status: completed
- Requested: TrackViewer default projection; driving physics gap inventory; original-evidence-backed skid particles.
- Authority: Debug NFSHP2.ELF SHA-256 815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b for driving. Revision unknown. Retail rendering reuse requires separate evidence.
- Preserve all pre-existing working-tree edits. Ghidra read-only. No Godot physics or invented smoke constants.

## Acceptance

- [x] TrackViewer browsing uses Camera3D default75/KEEP_HEIGHT without preview overrides; driving/race/CarViewer retain their policies.
- [x] Current driving gaps documented with addresses, evidence, implementation boundaries and concrete continuation.
- [x] Wheel-particle producer, original resources, trigger/update/render dependencies audited; confirmed closure implemented, unresolved paths explicitly recorded and throwing where reached.
- [x] Relevant native and host regressions run with exact commands, results and tools recorded.
- [x] Function/symbol evidence, progress, session log and continuation updated; no Verified promotion without original comparison.

## Work and evidence

2026-10-20: startup inspected dirty workspace, AGENT.md, project architecture/development/decisions/progress/reverse notes and TASK0046/0052. Existing skid geometry is implemented; static WorldEffects particles are a separate input. REA workflow: independent viewer projection and physics inventory scopes; root owns Debug car-effect tracing and integration. No proprietary material enters Git.

## Findings and boundaries

E-0075-T01/C01/P01..P03/001/D02/D03 establish original wheel smoke/spew,
terrain list/copy order, configuration positions and update-before-emission order.
See docs/DRIVING_EFFECTS.md, docs/DRIVING_GAPS.md and reverse/task0075. Full
terrain-effect entry remains a throwing partial boundary; no original-runtime
promotion. Raw code/assets/captures stay ignored. No commit made.

CarViewer uses the common particle packet/texture adapter with synthetic host
depth. TrackViewer and race use shared static/dynamic WorldEffects, original native
depth and alpha ranking. Dynamic copies/groups detach without losing static RNG.
A one-car current-velocity input closes the evidenced MotionLive group branch.
The supported outside view/all-view mask and distance0 policy are explicit port
inputs. First-person/mirror/editor/multi-car/water/damage effects and GS high-alpha
routes remain bounded gaps, rather than substitute behavior.

## Validation (2026-10-20)

- `cmake --build --preset native-debug -j4`: PASS (local/task0075/build-native.log).
- `cmake --build --preset godot-debug -j4`: PASS after initial new display source
  was omitted from the actual target list and then added. Final logs:
  local/task0075/build-godot-display2.log, build-godot-validation.log.
- `ctest --preset native-debug -R '^driving_(terrain_effects|particles|wheel_particles)' --output-on-failure`:
  PASS4/4 (5.53s final recorded run). Includes native counters/gates/geometry,
  original math, actual111-definition/112-emitter/13-texture resources, shared
  capacities/cleanup and two-reset x120-step exact visual-on/off trajectory parity.
- `ctest --preset godot-debug -R '^(driving_(terrain_effects|particles|particles_original|wheel_particles_assets|skids|player|backend|mover)|track_environment|track_runtime)$' --output-on-failure`:
  PASS10/10 (15.64s). Final original-resource parity test was expanded afterward;
  its final native run above passed independently.
- `ctest --preset godot-debug -R '^(driving_(particles_host|input_host|asset_host|lab_host)|track_default_projection_host|race_runtime_host|track_pipeline_parity)$' --output-on-failure`:
  PASS7/7 (38.32s). Initial/reset, preview no-overwrite,4:3/16:9/portrait and
  real-track-drive return retain default browsing projection. Existing full
  track_viewer smoke independently hits its prior empty lazy material-legend
  assertion; projection-only coverage avoids changing that unrelated expectation.
- `& ./local/tools/godot-4.7.2/Godot_v4.7.2-stable_win64_console.exe --path godot --audio-driver Dummy --resolution 960x540 --script res://scripts/debug/driving_particles_smoke.gd -- --particle-capture=D:/dev/hp2_godot/local/task0075/wheel-particles.png`:
  PASS469 active/467 visible, no gaps. Inspected screenshot shows original smoke.
- Same command with `--particle-track` and capture track-wheel-particles.png:
  PASS Track11 peak497 active/398 visible, no gaps; shared track rendering remains.
- `py -3 reverse/scripts/generate_class_templates.py --check`: PASS372 classes,
  1203 attributions,23 catalogue-only labels. No owner migration needed.
- `py -3 -m tools.oracle probe --output local/task0075/pcsx2-probe.json`:
  BLOCKED timed out (exit1). This is not an original numerical/visual comparison.
- `git diff --check`: PASS (only existing LF/CRLF normalization warnings).

Tools: CMake3.29.2, Ninja1.12.0, GCC13.2.0 MinGW, Python3.14.4,
Godot4.7.2.stable.official.ed1daf0bf, pinned godot-cpp
 e4b7c25e721ce3435a029087e3917a30aa73f06b. GPU Vulkan1.4.359/Forward+,
Intel Arc B390. Shared source review and final generated progress checks follow.

## Reproducible continuation

Continue TASK-0038 original certification only with a runnable matching Debug
scenario and equal initial/global/input/time/contact snapshots. For new recovery,
select one gap from docs/DRIVING_GAPS.md as a new scoped task: special terrain,
non-null collision damage/player consumers, Smokeable lifecycle or multi-car World.
Reproduce wheel particles with the exact GPU command above, adding --particle-track
for the shared Track11 consumer. Existing gameplay effects and unsupported GS
render domains must not be described as complete or Verified.

## Completion

- `py -3 reverse/scripts/audit_task0075_review.py --write`: PASS,10 predecessor
  identities replaced by33 explicitly reviewed bounded/host identities;153
  auxiliary files pinned, no missing/changed or unreviewed additions. Frozen
  predecessor review/scope files remain unchanged. This is source approval only.
- `py -3 -m unittest discover -s reverse/tests -p test_task0075_source_review.py`:
  PASS14 tests (5.076s), including drift/draft/promotion rejection and scoped
  historical memoization. Initial recursive audit recomputation was stopped only
  after verifying its owned PID; pass-scoped reuse reduces repeated historical
  work without preserving caches or bypassing frozen checks.
- Final `cmake --build --preset godot-debug -j4`: PASS, no work to do after
  settled source/test registration (build-godot-final-check.log).

Scope complete: default browsing projection, current gap inventory and evidenced
wheel smoke/spew closure. Original certification remains separately blocked in
TASK-0038. No unrelated task was marked complete and no commit was made.

Initial broader CTest evidence run passed5/6 and timed out in
`debug_driving_migration_rules` (reported5179.88s). Unchanged direct verbose rerun
`py -3 -m unittest discover -s reverse/tests -p test_task0044_migration.py -v`
passed12/12 in7.496s. Cause unresolved; no test was removed or weakened. The
latest successor's independent historical-recursion optimization is separately
tested and is not proof of this CTest timeout's cause.

Final bounded CTest rerun:
`ctest --preset godot-debug --timeout 60 -R '^(driving_effect_source_review_rules|driving_effect_review_contracts|class_template_coverage|class_template_generator|debug_driving_single_chain|debug_driving_migration_rules)$' --output-on-failure`
PASS6/6 in42.03s, including unchanged migration suite7.51s. This supersedes the
initial timeout as the final acceptance result, while retaining its limitation
history. Final generated-region check passed after task completion.
