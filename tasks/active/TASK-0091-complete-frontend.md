# TASK-0091 - Complete Retail frontend

- Status: active
- Request: recover every original frontend screen, including Jukebox, pre-race setup,
  results/rewards/records, pause and replay interfaces. Do not enter a race.
- Authority: SLUS_203.62 US PS2, SHA-256
  7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2;
  executable revision unknown. Debug names require reviewed Retail bridges.
- Process: one implementation subtask at a time; current implementation TASK-0092 corrective music audit.
  TASK-0092 production integration landed in the working tree; original comparison remains open.
- Evidence: E-0091-* for the inventory; E-0092-* for Jukebox recovery.
- Tools: Ghidra MCP only for binary analysis; the user explicitly excludes computer-use.

## Acceptance criteria

- [ ] Inventory screens, popups and state-dependent variants from registrations, callers and assets.
- [ ] Complete common lifecycle, rendering, navigation, input, animation and media dependencies.
- [ ] Complete Jukebox and all startup/options/profile/help/credits interfaces.
- [ ] Complete mode/event/car/track/setup/reward-preview interfaces.
- [ ] Complete results/reward/records/pause/replay interfaces with isolated host fixtures.
- [ ] Preserve race setup in a port-side launch request; report the race boundary and allow Back.
- [ ] Exercise every supported screen route without a reachable untranslated frontend path.
- [ ] Native, Godot, real-asset and windowed validation recorded.
- [ ] Matched original-runtime comparisons recorded; missing comparisons remain incomplete.

## Findings and next action

The existing 38 registration rows and 41 authored documents are seeds, not proof of
complete coverage. FrontEndRuntime dispatches activation for only a subset of them.
Existing JBC decoding/scheduling tests do not implement JukeBoxMusicScreen.

TASK-0092 Jukebox, TASK-0093 Audio and TASK-0094 Display/HUD/calibration are
connected in the working tree with bounded PC tests; original comparison remains
open. Continue TASK-0095 NameEntry keyboard, then controller/profile/help/credits,
race setup/car/track and results/pause/replay. Their normal paths remain incomplete.
No Verified promotion and no commit requested.

## Approved combined frontend/career plan,2026-10-28

Complete frontend plus single-player Ultimate Racer is now approved. The active
implementation scope is TASK-0098 event/production selection; unrelated Retail
pages remain queued here with existing criteria unchanged. The no-race boundary
in this Retail task does not restrict the already authorized Debug TASK-0096.
reverse/frontend/closure-routes.csv is the reproducible63-registration/76-document
baseline (90 route/unknown rows), not an exhaustive state audit or behavior pass.
Run `py -3 reverse/scripts/audit_frontend_closure.py --check`. Every final
window/original acceptance remains pending until evidenced. See
reverse/task0098/production-entry.md for current entry implementation and gaps.

## Approved combined frontend/career plan,2026-10-28

Complete frontend plus single-player Ultimate Racer is now approved. The active
implementation scope is TASK-0098 event/production selection; unrelated Retail
pages remain queued here with existing criteria unchanged. The no-race boundary
in this Retail task does not restrict the already authorized Debug TASK-0096.
reverse/frontend/closure-routes.csv is the reproducible63-registration/76-document
baseline (90 route/unknown rows), not an exhaustive state audit or behavior pass.
Run `py -3 reverse/scripts/audit_frontend_closure.py --check`. Every final
window/original acceptance remains pending until evidenced. See
reverse/task0098/production-entry.md for current entry implementation and gaps.

2026-10-28 TASK-0098 production entry milestone: real Main Menu -> first
Ultimate Racer delivery -> loaded Select Car -> Back -> reenter has now passed
the GPU/script route (three cycles, swatch input and visible vehicle checks).
See reverse/task0098/production-roundtrip.md for the final run/commands and
remaining unsupported paths. This supersedes the prior blocked001F6A28 entry
report only for that bounded route. Full selection, all events, race closure,
Release performance and original certification remain incomplete.
