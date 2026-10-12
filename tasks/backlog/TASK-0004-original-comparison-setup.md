# TASK-0004 — Establish original comparison oracle

- Status: backlog
- Objective: reproducible original invocation and capture for the representative pilot.
- Scope: PCSX2/tool version/settings and trace setup, followed by one bounded reference capture; no full port.
- Original address: 0x00250340 for the first byte-fill candidate; invocation mechanism not established
- Binary SHA-256: 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2
- Dependencies: private original media/ELF; available emulator/debugging setup; TASK-0002
- Relevant files: docs/assessment/PILOT_TASKS.md, WORKLOAD_ESTIMATE.md, reverse/functions/, ignored private/ or local/ captures
- GitHub issue: not created

## Acceptance criteria

- [ ] Locate/detect emulator; record exact version, settings, BIOS/media identity where applicable, and limitations.
- [ ] Establish repeatable initial state/input and a supported invocation/capture method.
- [ ] Capture outputs/side effects for original byte-fill candidate normal/boundary cases.
- [ ] Record private capture hashes and shareable reproduction metadata; confirm repeatability.
- [ ] Define comparison tolerances and an explicit blocker if arbitrary original invocation cannot be achieved.

## Verification procedure

Repeat the same original trace/capture and compare outputs. Keep proprietary
BIOS/media/saves private. Do not invent function outputs or treat host-only tests
as the original oracle. Record whether the original function was directly invoked
or observed in actual gameplay; these offer different coverage.

## Current findings

Static assessment has no original-execution traces or PCSX2 comparisons. Scoped
PATH/standard install/WinGet/D:/ps2_game checks did not locate PCSX2; this does not
prove it is absent elsewhere. Native/Godot infrastructure smoke passed previously.

## Unresolved questions

Emulator location, supported debug/capture method, initial state and licensing/provenance of supplied reference inputs.

## Next steps

Locate comparison tools and prove a repeatable reference capture before executing pilot TASK-P010.

## Related Git commits

None.
