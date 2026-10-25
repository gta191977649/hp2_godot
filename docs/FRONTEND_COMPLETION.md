# Complete frontend work - TASK-0091

The requested deliverable covers all Retail interfaces, including Jukebox,
pre-race settings, results/rewards/records, pause and replay. Racing is excluded:
the future port launch boundary must preserve recovered setup and allow Back.
Results/pause/replay require isolated host fixtures rather than fabricated races.

## Current state

This work is incomplete. Production Jukebox metadata/rows/preview/Back music and
conditional private Autosave are connected. Audio sliders/output/defaults and
Display/HUD/calibration now have native state and real-asset route coverage.
NameEntry keyboard/profile controls and original render bucket phases are also
connected. TASK-0095 is current; TASK-0092..0094 await original comparisons. No scope has newly been marked Verified. Rendering a
registered document is not evidence that its behavior is complete.

The expanded census is in `reverse/frontend/task0091-coverage.csv`:63 registration
rows, with FRONTB41 and INGAMEB35 authored documents relevant. Static registration
is not implementation or parity. Read `reverse/frontend/task0091-investigation.md`
for new registrations, corrected gameflow indexing and remaining dependencies.
Do not use a row count as a completeness gate.

## Work order and acceptance

1. Close remaining TASK-0092 hardware/error-path and original-comparison criteria;
   production metadata, controls, audio and private save integration are implemented.
2. Create separately scoped tasks for common rendering/lifecycle gaps and remaining
   startup/options/profile/help/credits screens, preserving existing task evidence.
3. Recover mode/event/car/track/setup/reward routes and the explicit race boundary.
4. Recover results/rewards/records/pause/replay and their independent test fixtures.
5. Check the complete graph against registration callers, asset documents and
   runtime variants; validate every edge and relevant state in native/Godot tests.
6. Record matched original comparisons per scope. Uncompared scopes stay incomplete.

## Tool constraint and continuation

The user requests Ghidra MCP and excludes computer-use. Use only read-only binary
queries. Current 001F0D10 entry bytes are undefined in Ghidra; its interior listing
and jump table remain readable. Do not mutate the database to create a function.
Reproduce the SHA-bound Jukebox evidence using
`py -3 reverse/scripts/inspect_jukebox_screen.py --elf
(Get-Content config/ps2_elf.local.json | ConvertFrom-Json).elf`.

Next: finish TASK-0095 common Help dependency and original comparison. Keyboard
rendering now uses evidenced final bucket phases and reverse-authored ties. Remaining profile/controller,
credits/help, race setup/car/track, launch boundary, endgame/pause/replay are open.

##2026-10-26 corrective audio audit

E-0092-J08 corrects a production regression: per-frame music polling previously
started idle BGM without an original start call. Poll/completion and explicit
001AEAB0 start requests are now separate. Silence before explicit startup, after
manual second-intro skip and after preview stop is regression-tested natively
and through Godot. Static Title after natural intro completion still starts music
as the Retail code specifies. Full audio parity remains unverified. See
reverse/frontend/task0092-music-audit.md; TASK-0092 is current corrective scope.
