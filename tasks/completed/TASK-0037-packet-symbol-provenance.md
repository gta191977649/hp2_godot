# TASK-0037 - Trace packet accessor original symbol provenance

- Status: completed
- Target: SLUS_203.62; executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
- Scope: Type_0011BF98 grouping and0011BF98/0011C090/0011C180/0011C248/0011C268/0011C2B8, nearby wrappers and original naming evidence.
- Preserve all prior dirty work. Read-only Ghidra; no original behavior changes.

## Acceptance

- [x] Explain native grouping versus any original class identity.
- [x] Check symbol provenance, callers, nearby functions, strings/path/assert/allocator references.
- [x] Rename only recovered-verbatim identifiers or retain address names with explicit unresolved evidence.
- [x] Update durable evidence, symbol annotations, documentation and generated checks.

Next: inspect original accessors and incoming/caller context. Do not infer class/function
spellings from purpose, proximity, generated FUN names or unrelated allocator labels.

## Findings and outcome

E-0037-N01/N02: all six queried helper symbols are DEFAULT-generated FUN addresses in
Global, consistent with the prior original-name audit. Traced component-address getters,
value wrappers and setters, UV adjustment0010C158, callback traversal00104100/001B4B50,
debug geometry0010E178 and duplicate buffers00103BF0. No original C++ membership, exact
class/function spelling, constructor at0011BF98 or source file was established.

Original eStripEntry text002AC298 has no returned exact reference. The proven64-byte strip
record and its +8 packet pointer are distinct objects; no naming bridge binds that text to
the packet accessors. DuplicatedSolid/StripEntryData/StripEntryTable label allocations/copies,
not a constructor chain for these helpers. Other Packet/Strip/VIF/Vertex leads were checked;
none justifies a rename. No inferred English identifier was promoted to original spelling.

Retained Type_0011BF98/Sub_<ADDR> and clarified the header: it is a port-created grouping,
not a recovered original class; vector/array interfaces are port projections. Added a compact
name-provenance CSV and detailed binary-specific record, linked from the six function records,
and augmented symbols evidence without changing behavior/confidence/verification status.
Read-only Ghidra output remains ignored local/task0037. No proprietary files or commits added.

## Validation and continuation

Generated class/progress checks passed. Comment/documentation-only change; no new behavioral
tests are warranted. Native vehicle_geometry_tests target rebuilt; vehicle_geometry, class_template_coverage
and progress_tables checks3/3 passed0.84s. git diff --check passed.
Exact names remain unresolved, not an unfinished rename: a matching authorized debug ELF,
link map or original diagnostic-to-address binding is required before changing spellings.
The active wheel/reflection comparison remains TASK-0035; this naming audit did not widen it.
