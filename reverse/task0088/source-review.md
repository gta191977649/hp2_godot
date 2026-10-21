# TASK-0088 original driving POV source successor

E-0088-S01. Exact scope covers bounded native POV selection/cycling, bumper and
in-car pose metadata, original per-view projection, committed bridge APIs, host
V cycle and held B lookback, temporary mouse overlay and input cleanup. Behavior
evidence E-0088-C01 and host-camera.md remain separate from this source review.
Player::CycleDriveCameraPOVIndex00176EE0 and Update0015C2B0 anchor the native
slices; guest dashboard drawing remains an explicit RenderDash0015D89C throwing
boundary. No original whole-entry stub transition is allowed. Original-runtime
comparison is absent; no benchmark or Verified claim is made.

Frozen TASK0087 JSON is immutable. Historical identity/auxiliary snapshots
continue enforcing its unchanged validate rules and evidence checks. This task's
exact source slots preserve all unrelated function bodies, signatures and stub
classifications. Inherited auxiliary files retain their pins except the explicit
thirteen replacements; the previously unpinned driving input smoke fixture and
new audit/test/evidence note receive explicit new whole-file pins. Review JSON
cannot widen the scope fixed in the audit code. Generation uses the validated
immutable baseline once; adversarial tests reuse their one computed inventory.

## Final validation

Python3.14.4, CMake3.29.2.
- `py -3 -m unittest discover -s reverse/tests -p test_task0088_source_review.py`:7/7 passed264.287s.
- `ctest --preset native-debug -R '^existing_body_preservation$' --output-on-failure`:1/1 passed261.06s.
Exact scope:3 replaced functions,4 additions;13 modified inherited auxiliary
pins and4 additions. No original whole-entry stub transitions. Generation used
the already validated historical baseline and one live inventory capture.
After these checks, only this validation prose and its manifest hash changed;
metadata consistency was checked with the same verified immutable inventory.
No benchmark or original-runtime equivalence claim is made.
