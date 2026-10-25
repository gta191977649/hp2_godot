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
- Current implementation subtask: TASK-0098 original event-bound selection.
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

Inventory TASK-0097 is complete. Continue active TASK-0098; native selection core
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
