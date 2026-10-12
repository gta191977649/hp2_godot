# TASK-0008 â€” Locate HP2 police AI and movement control

- Status: completed (read-only map scope)
- Scope: read-only Ghidra MCP/string/caller/vtable investigation of CopDriver/CopMover; export private pseudocode and link existing generated C++; evidence-backed function map and reconstruction limitations. No gameplay implementation or broad renaming.
- Binary: sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2; USA/NTSC inferred; executable revision unknown, boot VER 1.00.
- Acceptance: identify constructor and likely Think/Move entry points with direct evidence; distinguish driver decisions/movement/world orchestration; preserve private exports and tool/hash provenance; document exact reproduction and unresolved ABI/type/oracle prerequisites.
- Initial anchors: CopDriver label 0x002AE4E8 referenced from controller mode dispatcher; CopMover 0x002AE560 from movement-mode dispatcher; Think/Move diagnostic strings may lack preexisting Ghidra xrefs.
- Related commits: none; preserve current names, previous outputs and user files.

## Findings and acceptance evidence

- Constructor and virtual entry chains captured; docs/POLICE_AI_MAP.md and E-0008-01 through E-0008-05 distinguish driver decisions and mover execution.
- World orchestration unresolved and explicitly recorded; coarse raw-scan false positive rejected through register overwrite evidence.
- Eight durable function records and symbols index entries; original/recovered identifiers unknown for unapplied candidates. No Verified promotion.
- Private JSON/pseudocode exports retained with artifact hashes in validation.json; no Ghidra database or restricted binary added.
- `py -3 reverse/scripts/validate_police_ai.py`: passed eight live metadata matches, unchanged ELF SHA-256 and seven C++ syntax probes; Move one-byte body/TODO check blocked. GCC 13.2.0; Python 3.14.4; exact arguments in private ledger.
- `py -3 reverse/scripts/inspect_police_ai.py <addresses>` read-only exports; query for 0x001A0E88 failed because no function is defined, gap listing retained. No boundary repair inferred.
- No reconstructed behavior tests: this task implemented no game behavior. No link/boot/original comparison. Naming/rollback tools untouched.
- No commits made; initial staged submodule files and unrelated active TASK-0005 preserved.

## Continuation

Read docs/POLICE_AI_MAP.md; create a scoped police boundary/ABI audit for
0x001A1030 and 0x001A0E88 with the same ELF and connected Ghidra MCP. Establish
return/branch paths and virtual dispatch before repairs or readable C++ recovery.
The map task is complete; behavioral reconstruction and boundary repair remain
separate unresolved work, not claimed complete by this status.
