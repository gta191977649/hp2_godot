# TASK-0023 - PCSX2 original-behaviour comparison

- Status: active (not started)
- Objective: a recorded, reproducible comparison against the original running under PCSX2, so
  that a symbol can be promoted to `Verified` over a stated scope for the first time.
- Scope: included - PCSX2 identity and settings, a documented capture procedure, and one scoped
  comparison: screen 39's node positions, the selected node's double scale, connector geometry
  and camera framing. Excluded - broad or automated frame diffing, gameplay, and anything that
  would require committing a BIOS, an ISO or a memory-card image.
- Original PS2 addresses: not applicable; this is comparison infrastructure.
- Dependencies: TASK-0021 (landed) and TASK-0022 (the camera easing, which changes what a
  comparison would show). Unblocks the `Verified` status AGENT.md R1 defines, and the long
  stalled TASK-0004.
- Relevant files: docs/EVENT_TREE.md, docs/PROGRESS.md, tools/ (a capture helper, if one is
  needed), reverse/symbols.csv (status promotions)
- GitHub issue: not created

## Acceptance criteria

- [ ] PCSX2's exact version and the BIOS identity are recorded, without committing either.
- [ ] A procedure another session can follow from the repository alone: how to boot, how to
      reach screen 39, how to capture, and what to compare against.
- [ ] One scoped comparison performed and recorded, with its differences listed rather than
      summarised away.
- [ ] Any `Verified` promotion names its scope in reverse/symbols.csv and in the function record.
- [ ] No BIOS, ISO, memory-card image or capture enters Git (AGENT.md R7).

## Verification procedure

The task is itself a verification procedure. Its own check is that a second person, given only
this repository, can repeat the comparison and get the same recorded differences.

## Current findings

- PCSX2 is installed at `D:\Game\pcsx2`, a portable Qt build: `pcsx2-qt.exe`, `bios/`,
  `cheat/`, D3D12 shader cache. There is **no `memcards/` directory**, so there is no
  pre-existing HP2 save on this machine -- consistent with TASK-0021 running on a zeroed profile.
- Earlier project state recorded PCSX2 as "not located in scoped tool checks"; that is now out of
  date and docs/PROGRESS.md's environment table should be corrected when this task starts.

## Unresolved questions

1. Whether a `settings.db` this port writes is accepted by PCSX2 or by a real PS2.
2. What tolerance a screenshot comparison should use. A pixel-exact match is not expected across
   a different renderer, so the comparison has to state what it is comparing -- positions and
   proportions, not pixels -- before it is run, not after.

## Next steps

1. Record PCSX2's version and BIOS identity.
2. Write the capture procedure.
3. Run the screen 39 comparison once TASK-0022's camera work has landed, since the easing
   changes what the frame looks like mid-transition.

## Related Git commits

None yet. Record actual commit hashes only after creation.

## Completion evidence

Not started.
