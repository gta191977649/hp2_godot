# TASK-0015 — Front-end activation and highlight animation

- Status: active
- Scope: continue TASK-0014's E-0015-01..10 handoff; wire confirmed main-menu actions,
  registration lookup, Options actions and recovered highlight pulse into the host.
- Binary: SLUS_203.62 (US PS2), SHA-256
  7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2; executable revision unknown.
- Preserve all existing uncommitted user/previous-agent changes. No commit requested.
- Acceptance: native action/boundary tests; native and bridge builds; headless main-menu
  transition and live widget animation checks; stable function records and symbols.
- Unknown behavior must throw; no fabricated localisation, scrolling paths, or screen handlers.
- Original comparison remains required before Verified; no reference capture supplied.
- Next: inspect Options exit animator and Back routing read-only, implement the confirmed
  dependency closure or document the exact unresolved boundary.


## Results and acceptance

- [x] Main actions + immutable registrations wired, preserving DAT_002FC070 and class identity.
- [x] Main pulse/bar/frame rendering integration; animation smoke covers pulse/repeat/retarget.
- [x] Options direct and recovered exit-animation actions; NameEntry mapping resolved.
- [x] Main denies Back; Options permits Back and denies during exit.
- [x] Action/registry and exit curve/group/boundary tests; native and bridge CTest 11/11.
- [x] Activation/animation/all-screen smoke; commands and versions in docs/PROGRESS.md.
- [ ] Display transition: 001EE6C0 source setting blocks not represented; explicit throw remains.
- [ ] Original comparison; unavailable in this session, all code remains unverified.

Evidence: E-0015-11/12 (action slices), E-0015-REG-01 (37 registrations), E-0015-21
(exit dependencies/Back/snapshot). Existing E-0015-01..10 retain handoff findings.
Task stays active for the two open criteria. No related commit exists.
Next session: trace 002FC194/0030CDCC/0030D49C runtime state for Display, then run
`./tools/run_frontend.ps1 -Build -ActivationSmoke` and `-AnimationSmoke`. Other destination
handlers remain explicit NotImplemented and require separately scoped recovery.

Index audit: TASK-0015 addresses unique; all record paths exist. Pre-existing duplicate
001680E8 (engine/frontend E-0010/E-0011 rows) preserved as unrelated debt.
