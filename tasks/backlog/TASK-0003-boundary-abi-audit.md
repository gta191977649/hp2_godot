# TASK-0003 — Audit assessment boundaries and ABI

- Status: backlog
- Objective: establish trustworthy bodies/contracts for damaged and highly connected pilot entries.
- Scope: read-only disassembly/CFG/caller/ABI audit first; any Ghidra repair must be explicitly documented and reversible.
- Original addresses: 0x0019a4b8, 0x001ebd20, 0x002577c0, 0x00282fc0
- Binary SHA-256: 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2
- Dependencies: TASK-0002, private ELF and connected hp2_ps2 project
- Relevant files: docs/assessment/BINARY_REPORT.md, FUNCTION_INVENTORY.csv, PILOT_TASKS.md, reverse/functions/
- GitHub issue: not created

## Acceptance criteria

- [ ] Reconcile one-byte bodies at camera/load entries with actual instructions, callers and decompiler reachability.
- [ ] Document entry/chunks and shared-block evidence; do not infer bodies from min/max spans.
- [ ] Audit allocation candidate return ABI and the imported __stdcall label on _Exit.
- [ ] Record GP/register/stack/return uncertainties and supported contracts with provenance.
- [ ] If repairs occur, record before/after snapshots and rerun affected metrics; retain historical assessment.

## Verification procedure

Read-only listing/reference/decompile queries and source ELF bytes first. Confirm
hash and correct program. No full auto-analysis or mass renaming. A scoped repair
must explain evidence and demonstrate stable affected CFG/caller results afterward.

## Current findings

Two named entries have one-byte bodies; one has no CFG, the other only a partial
block. 0x002577c0 has 255 static callers and a void-looking decompilation despite
callers consuming a result. _Exit's __stdcall metadata is not established R5900 ABI.

## Unresolved questions

Exact body boundaries, legitimate shared chunks, previous analysis provenance and calling contracts.

## Next steps

Start with 0x001ebd20 and 0x0019a4b8; publish a bounded evidence report, then audit shared ABI.

## Related Git commits

None.
