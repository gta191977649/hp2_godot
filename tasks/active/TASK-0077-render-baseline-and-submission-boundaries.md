# TASK-0077 - Render baseline and submission boundaries

- Status: active
- Authorization: implement the Fukami-inspired plan, starting with P0 only. P1 submission boundaries, P2 depth source lifetime, and P3 build organization follow independent acceptance; they are not implemented in this phase.
- Scope: port-side benchmark sampling/export separation, explicit environment-capture policy, immutable run identities, A/A controls and first-divergence reports. No recovered code, physics, renderer defaults, or native compilation changes.
- Baseline: existing dirty work preserved; exact edited inputs and game_core hashes saved under ignored local/task0077/baseline. Isolated projects use a copied current TASK0076 DLL; never replace a loaded workspace DLL.

## Acceptance

- [ ] Bounded Profile collection stops at its limit without snapshot/export inside benchmark measurement, including long stress captures; exports remain observable and errors invalidate the run.
- [ ] Record source/DLL/engine/build-recipe identity before and after execution; reject mutation or missing identity rather than claiming reproducibility.
- [ ] Default environment capture stays off; explicit opt-in is a distinct scenario with actual effects reported.
- [ ] A/A controls precede candidate comparison; diagnostics identify the first frame/view/command difference where evidence exists, and never fabricate unavailable command detail or GPU times.
- [ ] Relevant asset-free and engine contracts pass; run sequential isolated route controls and report exact scope and limitations.
- [ ] Source protection, task/report/log/architecture decisions and generated progress updated.

## Findings and next action

E-0077-P01: ProfileDebug._process currently calls export_profile at its limit; synchronous JSON/hash/file work overlaps the final measured frame. track_performance._scenario_settings assumes environment=true for non-default scenarios even though TASK0076 defaults it off. Preserve legacy records as counterevidence; new controls must explicitly bind the current policy.

Next: implement held export, explicit capture configuration and identity/control reporting; validate without changing recovered source or optimization defaults. Original runtime equivalence and 60FPS are not acceptance claims for this tooling task.

## Related commits

## Task switch checkpoint (2026-10-21)

The user switched priority to concise automatic progress (TASK0078). P0 source
changes and initial contracts remain; the sequential graphical matrix was
interrupted and contains failed/incomplete runs. Completed process/identity
checks do not establish A/A acceptance. Preserve local/task0077/matrix-status.json
and raw logs; do not promote P0 completion or start P1-P3 from these data. No
owned Godot/matrix process remained at the TASK0078 startup check. Further P0
comparison/recovery is deferred; this new documentation task launches no Godot.

None created.
