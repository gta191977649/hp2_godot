# TASK-0026 — Make docs/PROGRESS.md a tracker instead of a transcript

- Status: completed (restructure and generator delivered; the task files' own Status-line
  inconsistency is recorded as a follow-up, not fixed here)
- Objective: `docs/PROGRESS.md` answers "where does recovery stand, what is blocking it, what is
  next" in one screen, with every number reproducible from repository records.
- Scope: included — split the 891-line chronological narrative into a tracker and a log, add a
  generator for the tables, add a staleness test, point `AGENTS.md` at the new structure.
  Excluded — changing any recovery finding, editing the narrative's content, normalising the
  `Status:` lines inside the task files themselves.
- Original PS2 addresses: not applicable. This task touches documentation and tooling only; no
  ELF was read and no recovery claim was added, changed or removed.
- Dependencies: `reverse/symbols.csv`, `reverse/structures/`, `reverse/binaries.csv`,
  `docs/assessment/FUNCTION_INVENTORY.csv`, `tasks/{active,backlog,completed}/`, and the
  `hp2::not_implemented` call sites in `src/`.
- Relevant files: `docs/PROGRESS.md`, `docs/PROGRESS_LOG.md`,
  `reverse/scripts/generate_progress.py`, `CMakeLists.txt`, `AGENTS.md`.
- GitHub issue: not created

## Acceptance criteria

- [x] `docs/PROGRESS.md` leads with status, blockers, validation and the exact next action, and
      keeps the four things `AGENTS.md`'s completion protocol step 4 requires.
- [x] Every count and percentage in it is generated from repository records, not typed.
- [x] The old narrative survives in full; nothing was deleted, including retracted claims
      (AGENT.md R6).
- [x] A stale tracker fails a test rather than silently misleading the next session.
- [x] `AGENTS.md` tells a future session which file to read and which not to hand-edit.

## Verification procedure

```
py -3 reverse/scripts/generate_progress.py --check
ctest --preset native-debug -R '^progress_tables$' --output-on-failure
ctest --preset native-debug --output-on-failure
```

The generator is read-only with respect to everything except the generated regions of
`docs/PROGRESS.md`. It never touches the ELF, Ghidra or the asset tree.

## Current findings

The reference the user pointed at, `D:\dev\gta-reversed\docs\ReversedClasses.md`, is generated
from `hooks.json` on every hook change, and says so in its own header. That is the property
worth copying, more than its exact layout: its percentages are reproducible, and its
disclaimers state plainly that the number is a rough estimate of documented-and-reimplemented
functions rather than of correctness.

Three things this repository has that the reference does not, and that the tracker therefore
shows:

- **A status ladder with an empty top rung.** `Verified` requires comparison against the
  original (AGENT.md R1) and nothing has it. Showing the rung at 0 is more informative than
  leaving it out, because it states the project's central limitation in the same table as its
  progress.
- **An explicit frontier.** AGENT.md R3 makes every untranslated recovered path call
  `hp2::not_implemented(elf_address, what)`, so the call sites are an exact, machine-readable
  work queue. 86 sites across 62 distinct ELF addresses at the time of writing.
- **A structure-record count next to the function count.** 4 against 368. That ratio is the
  honest shape of the gap and is easy to overlook when only function progress is reported.

No per-subsystem percentage is shown, deliberately: `FUNCTION_INVENTORY.csv` classifies 7525 of
its 7544 rows as `Unknown`, so any per-subsystem denominator would be invented (AGENT.md R1).

## Unresolved questions

- The task files' `Status:` lines are inconsistent: some start with `- `, some do not, and
  TASK-0017/0018/0020 put a whole paragraph of justification there. The generator reads across
  continuation lines and shortens at a word boundary, which works, but normalising the task
  files themselves would be better and is not done here.
- `TASK-0019`'s file is in `tasks/completed/` with a `Status:` line that has no leading `- `;
  the same normalisation would cover it.
- Two structure records still sit directly in `reverse/structures/` rather than under
  `<binary-id>/<subsystem>/` as AGENT.md R6 specifies. The generator reports the count rather
  than hiding it; moving them is a separate change.
- The headline denominator counts library and CRT functions the port will never translate. A
  more meaningful denominator needs the inventory's `subsystem` column populated, which is
  TASK-0009 work.

## Next steps

1. Normalise `Status:` lines across `tasks/**` so the board's Note column is short everywhere.
2. Move the two loose structure records into the `<binary-id>/<subsystem>/` layout.
3. When `TASK-0009` classifies inventory rows by subsystem, add per-subsystem denominators to
   the generated stats.

## Related Git commits

None yet. Record actual commit hashes only after creation.

## Completion evidence

2026-10-13. CMake 3.29.2, Ninja 1.12.0, GCC 13.2.0 (Strawberry), Windows 11.

- `py -3 reverse/scripts/generate_progress.py --check` — `PASS docs/PROGRESS.md generated tables
  are current`.
- `ctest --preset native-debug --output-on-failure` — **25/25 passed**, the new `progress_tables`
  test included.
- `docs/PROGRESS.md` went from 891 lines of chronological narrative to a tracker whose tables
  regenerate; `docs/PROGRESS_LOG.md` holds the narrative verbatim with a generated contents list
  added at the top.

Limits: this task changed no recovery finding and establishes nothing about the original game.
The generator reads repository files only.
