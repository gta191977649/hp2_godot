# TASK-0001 — Validate Ghidra toolchain and register an authorized binary

- Status: backlog
- Objective: reproducible R5900/GhidraMCP environment and exact binary identity.
- Scope: environment, read-only import checks and provenance; no gameplay reconstruction.
- Original PS2 function addresses: unknown; no candidates selected yet
- Dependencies: TASK-0000; user-supplied authorized ELF and region/build details; Java/Ghidra/R5900/MCP
- Relevant files: reverse/binaries.csv, docs/REVERSE_NOTES.md, docs/PROGRESS.md, reverse/scripts/
- GitHub issue: not created; remote not configured

## Acceptance criteria

- [ ] Record installed Ghidra/Java/R5900/MCP versions and processor/compiler specification.
- [ ] Test MCP connection with read-only project queries; record results/limitations.
- [ ] Locate user-authorized private ELF or record a concrete missing-input blocker.
- [ ] Record its SHA-256, region/version (unknown if unsupported), and provenance.
- [ ] Import privately with documented settings; confirm R5900 decoding and record limitations.
- [ ] Provide the next narrowly scoped identification task without claiming verified gameplay.

## Verification procedure

Use `Get-FileHash -Algorithm SHA256 -LiteralPath <authorized-private-ELF>`.
Record exact Ghidra import settings and read-only MCP query results. Do not
rename/analyze project objects without a scoped rationale; never put ELF/project
databases into Git. Confirm `git status` contains only shareable metadata.

## Current findings

Confirmed by file inspection during initialization: Ghidra 12.0.4 folder includes
Emotion Engine Reloaded and GhidraMCP (description 0.7.0-pre15, host version
12.0.4). Java 21 is on PATH and MCP tools are exposed to this session.
Subsequent TASK-0002 assessment confirmed read-only MCP connectivity, the active
R5900 language/default compiler, current ELF SHA-256 and loaded .text correspondence.
See docs/assessment/BINARY_REPORT.md. Exact release identity, original import
settings and complete processor/ABI correctness remain unverified. This task was
not independently executed or automatically moved to completed.

## Unresolved questions

Which exact game ELF/region/build is authorized? Which R5900 specification is
appropriate? Does installed MCP connect to the intended private project?

## Next steps

Move to active, inspect tool versions/connectivity, then identify the authorized
input. If unavailable, retain blocked status and specify exactly what is needed.

## Related Git commits

None yet.
