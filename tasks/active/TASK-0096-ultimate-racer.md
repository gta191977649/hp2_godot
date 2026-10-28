# TASK-0096 - Complete single-player Ultimate Racer lifecycle

- Status: active
- User authorization:2026-10-26, approved implementation plan. Actual racing is
  authorized for this scope, superseding TASK-0091's no-race boundary here only.
- Behavior authority: Debug NFSHP2.ELF, SHA-256
  815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b;
  executable revision/date not independently verified. Covers required frontend,
  car selection, race, World, AI, police, outcome and progression.
- Existing Retail frontend/assets/profile authority remains separately identified:
  SLUS_203.62 SHA7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
  Never reinterpret one version's storage as the other's.
- Current implementation subtask: TASK-0098 production Event -> selection entry, per the approved full-closure plan2026-10-28. TASK-0099 World work is retained and deferred until selection roundtrip acceptance.
  TASK-0097 inventory/version foundation completed; full mode remains incomplete.
- Preserve all existing uncommitted user work; no staging/commit requested.

## Required end-to-end behavior

Main menu -> Hot Pursuit/Ultimate Racer event tree -> original vehicle/skin and
race preparation interfaces -> original loading/World -> player/opponents/traffic/
police as configured -> original success/failure -> results/rewards/unlocks/private
save -> retry or event tree. All32 tree records must be classified, including
special31/32. First event closure is only a milestone, never the final deliverable.
Original reachable pause/retry/abandon/replay routes belong to this scope. Two-player
branches and unrelated frontend pages are excluded from this single-player task.

## Ordered subtasks

1. TASK-0097: Debug call/dependency inventory, all-event matrix, version adapter.
2. TASK-0098: full original car/skin selection and event-bound preparation.
3. TASK-0099: real World initialization, participants, routes, collisions and race lifecycle.
4. TASK-0100: original police/pursuit orchestration and configured mechanisms.
5. TASK-0101: race rules/HUD/camera/audio and reachable pause/replay controls.
6. TASK-0102: real outcome/results/rewards/profile persistence and full-tree progression.
7. TASK-0103: final integrated behavior/performance/lifetime acceptance.

Performance runs through every subtask, not only TASK-0103. Independent read-only
investigations may run in parallel under REA skill; implementation scope stays single.

## Architecture and performance contract

Recovered logic stays in original source ownership, Godot-free C++. Port-owned
RaceLaunchRequest/RaceSession/RaceSnapshot/RaceOutcome provide versioned semantic
handoff, lifecycle ownership, compact reusable publication and single outcome commit.
Godot handles input devices, rendering, audio and host resource lifetime. Generation
keys isolate stale load results. Never launch partial races or write synthetic outcomes.

Target current machine Vulkan/Forward+1920x1080 stable60FPS. Retain original clocks,
RNG/order/counts/rules. Measure memory copying/allocations/transfer first; replace
per-car full-image transactions with shared-world dirty-page/write-barrier transactions
only with rollback and state equivalence tests. Reuse bounded resources and submit
changes only. Engine particles/lights/instancing require demonstrated original behavior
and visual ordering compatibility. No generic Godot vehicle physics/navigation AI.

## Acceptance

- [ ] All32 event records, goals, cars, tracks, pursuit and reward/unlock dependencies audited.
- [ ] Real original car-selection UI, restrictions/readiness/skin/cancel/reentry work.
- [ ] World/race/player/opponents/traffic/police are actual recovered runtime objects.
- [ ] Every original reachable single-player event type and police mechanism works.
- [ ] Countdown/run/success/failure/pause/retry/abandon/replay/result/return routes work.
- [ ] Outcome/reward/profile commit uses real state, is idempotent and survives restart.
- [ ] No normal-scope frontend/gameplay NotImplemented paths remain.
- [ ] Native/Godot builds, original-data regressions and window interaction pass.
- [ ] Matched original Debug runtime traces and interface/media comparisons recorded.
- [ ] Release1080p uncapped/VSync-off runs:120 warmup+600 samples,3 repeats,
  all event types and worst original reachable pursuit; steady P99<=16.67ms,
  no running-frame spikes>33.33ms; loading separately measured; missing GPU timers explicit.
- [ ]30 retries/10 frontend roundtrips restore warm resource/RID/audio/worker baseline,
  without continuing memory growth or stale callbacks.

## Evidence and continuation

Only original authorized binaries via read-only MCP and configured original assets
establish behavior. Exact symbols/addresses/types/calls/static bits retained; unknown
paths throw at their address. PC tests do not establish Verified. No proprietary
ELF/assets/database/save/captures in Git. Missing original-runtime prerequisites
remain explicit; independent implementation continues without invented evidence.

Inventory TASK-0097 is complete. Current implementation is TASK-0099 after the2026-10-28 scheduling revision; native selection core
is tested, original selection presentation/production entry and subsequent World/
police/rules/outcome stages remain incomplete. No playable career is claimed.
Update task findings, evidence records/symbols, decisions and
PROGRESS_LOG; regenerate PROGRESS.md with py -3 reverse/scripts/generate_progress.py.

2026-10-26 continuation: TASK-0098 now has native selector Tick loading/fade
orchestration, locked-car presentation/description priority and preserved native
packet stage ranges. These are bounded dependencies, not a formal selection entry
or complete loading lifecycle. See the TASK-0098 checkpoint and its exact next
action. All parent end-to-end, World/pursuit/outcome and original/performance
acceptance checkboxes remain open.

Renewed one-delivery request2026-10-26: implementation remains dependency-ordered
in active TASK-0098. Native mutable frontend effects/nebula/shared budgets and
normalization now have bounded regressions; compositor counterevidence adds the
stage5 shared-alpha flush. The five requested end-to-end deliverables remain
incomplete. Read-only World/police/real-outcome/save prerequisites are retained in
reverse/task0097/lifecycle-continuation.md E-0096-C01; they are not implementations.

2026-10-27 continuation: TASK-0098 adds a tested native page lifetime composition,
preload/info/group ownership and shared-alpha/Flailer-depth preparation. Actual
host resource leases/shared-depth GPU executor and formal event entry are still
unbound.13 scoped regressions and normal Godot DLL build passed at the checkpoint;
final commands/results are in TASK-0098. No parent acceptance box is newly checked.

2026-10-28 user-approved first-event priority: native launch baseline/event/13participants/StartRace-to-GameFlow now has bounded original-data tests, with clearly labelled text/profile/flow fixtures. Current implementation advances to TASK-0099 World/participants rather than further isolated presentation work. This revises scheduling, not behavior/acceptance: TASK-0098 stays incomplete for actual frontend/private-profile services and no original/runtime/performance box is checked. First event full roundtrip remains unfinished; no synthetic outcome or playable-race claim.

## Approved full-closure execution,2026-10-28

User approved full Retail frontend plus complete single-player Ultimate Racer,
including all32 events, actual production routes and matched original behavior.
Scheduling supersedes the earlier first-World-first revision: current implementation
returns to TASK-0098 Event -> production selection -> cancel/reentry, then0099..0103.
TASK-0091 retains unrelated Retail pages. No existing acceptance is waived.

Current bounded scope: reconcile the63-row evidence inventory with the38-row runtime
registry; recover Debug tTreeDetails StartEvent/HandleMessages and the exact entry
writes, history and multitrack continuation. Required host bindings must be explicit.
Tests distinguish local orchestration/service fixtures from production/window and
original-runtime acceptance. Keep the actual event route throwing until real page
services exist. No claim of an operational selection page from registry/core work.

Environment: CMake3.29.2, Python3.14.4, pinned godot-cpp
e4b7c25e721ce3435a029087e3917a30aa73f06b. Readonly Ghidra MCP responds;
no PCSX2 process or listening21512/28011 port observed. Original comparison is open.
Existing uncommitted changes are preserved. No staging or commit requested.

2026-10-28 TASK-0098 production entry milestone: real Main Menu -> first
Ultimate Racer delivery -> loaded Select Car -> Back -> reenter has now passed
the GPU/script route (three cycles, swatch input and visible vehicle checks).
See reverse/task0098/production-roundtrip.md for the final run/commands and
remaining unsupported paths. This supersedes the prior blocked001F6A28 entry
report only for that bounded route. Full selection, all events, race closure,
Release performance and original certification remain incomplete.
