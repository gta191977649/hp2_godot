# TASK-0007 — Evidence-backed HP2 function naming

- Status: completed (bounded naming batch; remaining semantics/ABI/original equivalence unresolved)
- Date: 2026-10-11 (Australia/Sydney)
- Scope: screen all 7,544 functions; inspect original-symbol/debug evidence, existing 20 pilots, library/system candidates; rename only high-confidence default names, save rollback records, regenerate private named output.
- Binary: sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2; USA/NTSC inferred; executable revision unknown, boot VER 1.00.
- Dependencies: TASK-0002/TASK-0006, authorized ELF, connected existing Ghidra project, built PS2Recomp.
- Constraints: preserve existing custom names and body boundaries; no analysis rerun, prototype changes, gameplay changes, Verified promotion or unrelated TASK-0005 changes. Preserve prior generated files; 28 exclusions remain address-based.
- Acceptance: full-population screening and 20-pilot disposition; private pre/post snapshots and rollback map; evidence for every rename; metadata/records updated; named code generated; synthetic rename/rollback tests and representative syntax checks; unchanged ELF hash/bounds; durable report and continuation.
- Initial blockers: damaged camera/FlightAreas entries and 0x0016FA80 envelope remain blocked for naming.
- Related commits: none.

## Findings and acceptance evidence

- Full 7,544-row screening and all 20 pilot dispositions recorded in reverse/naming/.
- ELF symbol/debug audit found no original symbol source; scoped local search found no alternate symbol files.
- 16 reviewed purpose names applied plus 11 ELF-confirmed automatic jump aliases; existing custom names preserved.
- Before/after snapshots, affected symbol metadata, journal and 27-address rollback map retained privately; Ghidra saved.
- Per-function records and symbols index updated without Verified status.
- Named generation exited 0: 7,516 translations + original 28 address-based placeholders; earlier outputs preserved.
- 11 synthetic safety/rollback tests passed; 20 syntax probes passed; ELF hash/all entry envelopes/namespaces/calling conventions unchanged.
- Initial alias-rendering postcondition failed before save; actual alias behavior was inspected, expectation/tests corrected and --finalize succeeded. See docs/FUNCTION_NAMING.md.
- Name rollback does not restore DEFAULT provenance/source type via available API; snapshots retain it. Live alias rollback/full DB restore not validated.
- No full link/boot or original comparison; no recovered behavior implemented, so behavior tests were not invented.

Evidence: E-0007-001 through E-0007-005, E-0007-N01–N16 and deferred E-0007-P-ADDRESS.
Exact commands/results/tool versions and continuation: docs/FUNCTION_NAMING.md.
Related commits: none. Existing staged/untracked user files preserved.

## Next action
Read TASK-0003 and audit 0x0016FA80, 0x0019A4B8, 0x001EBD20 and 0x002577C0.
Keep remaining name candidates deferred until boundaries/ABI/purpose evidence improves.
