# TASK-0006 — HP2 PS2Recomp 0.4 trial

- Status: completed (generation trial only; full baseline and equivalence remain unresolved)
- Scope: build the supplied D:/dev/PS2Recomp-0.4 tool, export existing Ghidra function boundaries read-only, generate private C++ under ps2_decomp, and assess output limitations. No recovered core integration.
- Binary: D:/ps2_game/SLUS_203.62; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2; executable revision unknown, USA/NTSC inferred, boot VER 1.00.
- Acceptance: record tool/dependency identities; reproducible Ghidra map/config; execute recompiler; count output and diagnostics; document compile/behavior validation limitations and continuation.
- Protection: ps2_decomp is ignored; preserve supplied tool sources, Ghidra annotations, existing TASK-0005 and all staged/untracked user files.
- Initial findings: upstream exporter uses GUI prompts and name-based skip/stub heuristics; use read-only MCP boundary export with empty skip/stub and no instruction patches for the first trial.
- Related commits: none.

## Findings and acceptance evidence

All trial criteria met: source fingerprints and dependency commits recorded;
read-only 7,544-function map/config exported; tool built and executed; diagnostic
run exited 0 with 7,516 translations plus 28 explicit placeholders; artifact
assertions and five syntax probes passed. See docs/PS2RECOMP_TRIAL.md and evidence
E-0006-001 through E-0006-005. Baseline was terminated near 6 GB memory due to
the 475,968-byte envelope at 0x0016FA80. Private partial files preserved.
No full link, runtime boot or original comparison was performed; no Verified
promotion. No behavior tests were added because no recovered behavior was implemented.

Next: TASK-0003 boundary/ABI audit, adding the 0x0016FA80 envelope inspection.
Reproduction: py -3 reverse/scripts/export_ps2recomp_mcp.py, then prepare_ps2recomp_triage.py,
then local/ps2recomp-build/ps2xRecomp/ps2_recomp.exe ps2_decomp/hp2-triage.toml.
