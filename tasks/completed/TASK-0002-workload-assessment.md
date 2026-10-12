# TASK-0002 — PS2 port workload assessment

- Status: completed (static assessment snapshot; pilots not executed)
- Objective: evidence-grounded inventory, risk analysis, stratified pilot and effort model.
- Scope: read-only assessment of supplied SLUS_203.62 and existing Ghidra analysis; no game implementation.
- Original PS2 addresses: to be collected with ELF SHA-256
- Dependencies: TASK-0000; installed Ghidra/R5900/GhidraMCP; existing hp2_ps2 project
- Relevant files: docs/assessment/, reverse/binaries.csv, docs/PROGRESS.md
- GitHub issue: not created

## Acceptance criteria

- [x] Record binary identity, tool configuration, measured inventory and limitations.
- [x] Analyze subsystem/complexity evidence without presenting hypotheses as confirmed.
- [x] Select 10–20 varied pilot functions with real addresses, dependencies and validation requirements.
- [x] Provide optimistic/baseline/pessimistic model, separating observations and assumptions.
- [x] Create six requested reports, actionable tasks, and update progress.

## Verification procedure

Hash ELF, inspect current Ghidra metadata/functions/call graphs/decompilation,
validate CSV coverage and consistency, preserve proprietary inputs outside Git.

## Current findings

MCP connected to hp2_ps2; full export contains 7544 unique entries, 17162 static
call pairs and 400582 decoded .text instructions. Loaded .text matched the disk
ELF byte-for-byte. Source SHA-256 is recorded in docs/assessment/BINARY_REPORT.md.
Twenty pilot functions were selected; 21 distinct functions were decompile-probed,
all returned output, and 3 outputs contain warnings. No gameplay reconstruction
or original verification was performed. These are observations, not discovery guarantees.

## Unresolved questions

Function bodies/ABI need review (26 empty CFGs and named one-byte bodies).
Subsystem identification remains mostly unknown. Original oracle, exact binary
release matching and actual reconstruction/verification times are unresolved.
Global decompile success/failure totals remain unknown: only 21 entries were probed.

## Next steps

Execute TASK-0003 boundary/ABI audit, then TASK-0004 original comparison setup.
Execute scoped TASK-P001–TASK-P020 pilots after prerequisites; do not infer a
full-game schedule from uncalibrated scenario rates or a single successful helper.

## Related Git commits

None.

## Completion evidence — 2026-10-10

- Created all six requested reports plus evidence/metadata files and reproducible summarizer.
- Summarizer asserted 7544 unique matching function/graph records, complete .text
  capture, exact disk-byte agreement, and 20 distinct pilot selections.
- Final validation confirmed 7544 CSV rows, 17162 distinct edges, all 20 matching
  task files, 21 output-returned unverified probes, consistent strata and all scenario formulas.
- All raw listing/decompilation/graph captures matched ignored local/ paths.
- Final Ghidra metadata still reports 7544 functions and 11235 symbols; undo history
  remained empty. No symbol/type/analysis/save mutation requested.
- No C++ source changed; native/Godot build tests were not rerun for metadata work.
- No PCSX2 execution, reconstruction timings, asset conversion or gameplay
  verification performed. These future tasks remain unfinished in backlog.
- No Git commit, remote publishing or issue creation performed.
