# TASK-0019 — Recoverable original function names

2026-10-13, read-only Ghidra MCP. Target US PS2 SLUS_203.62, revision unknown,
SHA-256 `7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2`.
The original names can be recovered for a limited subset. A Ghidra label,
allocation/debug description, or nearby class name alone is insufficient.

## Ghidra symbol provenance — E-0019-N02

Fresh paginated function-symbol inventory contains 7,544 unique entry addresses:

| Ghidra source | Count | What it establishes |
|---|---:|---|
| DEFAULT | 7,461 | Generated FUN_/thunk names; original identifiers unknown |
| ANALYSIS | 64 | Recognized SDK-like names produced by analysis; source tag alone is not original symbol evidence |
| USER_DEFINED | 18 | Earlier HP2_* purpose labels; analyst inventions, not recovered original identifiers |
| IMPORTED | 1 | `entry` at00100008; imported loader entry designation, not a recovered game method name |

All 75 DEFAULT pages were consumed with page_size100; no duplicated entry address
exists across the four source categories. The public inventory is
[function-symbol-origins.csv](../reverse/naming/task0019-function-symbol-origins.csv).
The source column explains provenance in the database; it does not certify that
the binary contained that textual name. Earlier HP2_* labels must not be adopted
as original names simply because they appear in decompiled calls.

Prior ELF assessment reports no .symtab/.dynsym/DWARF debug sections and an
empty .mdebug.eabi64 section. Fresh Ghidra memory-block inventory likewise exposes
no loaded debug-symbol table. This is a stripped-target limitation, not proof
that every original name is absent from diagnostic text.

`analyze list_rtti` returns no recognized classes. However byte reads at002C7090
show legacy C++ runtime type-info text (`__class_type_info`, `__si_type_info`,
`__user_type_info`, `bad_typeid`, `bad_cast`, `type_info`, `bad_alloc`, etc.).
Therefore the empty tool result must not be described as absence of all RTTI.
The bounded RTTI search has not identified game-class type-info records that
provide method names. A vtable identifies slots/targets, but without associated
name-bearing metadata it cannot reveal a method's original spelling.

## Five original method mappings — E-0019-N03

The earlier TASK-0016 sweep found 118 nonoverlapping qualified-name text matches.
It queried references to the match offset. That misses names embedded after a
prefix such as `WARNING:`: code references the beginning of the whole C string.
This pass locates the preceding NUL, queries that full-string start, and follows
positive references into their containing function and MIPS argument setup.
Seven text matches have full-string references. Five map to methods; two are
rejected because their syntactic role is different.

| Function entry | Recovered original identifier | Diagnostic string start | Instruction anchor |
|---|---|---|---|
|0012E6C8|`DemoDiscManager::Init`|002ADA80|0012E700 LUI a0 +0012E704 ADDIU;0012E708 print call|
|0012EB00|`DemoDiscManager::SetEndReason`|002ADDC8|0012EB24 LUI a0;0012EB28 print call with0012EB2C delay-slot ADDIU|
|00257F28|`SlotPool::CleanupExpandedSlotPools`|002C0450|00258038 LUI s4;002580F0 ADDIU a0,s4;002580F8 print call|
|002584B0|`SlotPoolManager::NewSlotPool`|002C04B0|002584FC LUI a0;00258504 print call with00258508 delay-slot ADDIU|
|00258548|`SlotPoolManager::DeleteSlotPool`|002C0510|0025856C LUI a0;00258574 print call with00258578 delay-slot ADDIU|

Each print target is00251150. Decompilation independently establishes the
containing entry and the operation described by its self-diagnostic: demo
initialization/end-reason update, cleanup of expanded pools, creating or deleting
pool objects. Original identifier confidence is high for these five mappings.
The logging parentheses are not a recovered complete parameter signature.
No implementation or equivalence status is implied by recovering a name.

Two important counterexamples:

- `SlotPoolManager::SlotPoolManager()` appears in the same warning as NewSlotPool
  as the initialization prerequisite. It does not name the containing function
  at002584B0; no constructor mapping is adopted.
- `bFile::Filename` occurs inside an assertion about filename capacity. It is a
  field expression. The referencing function00253828 is not renamed to that text.

The explicit adoption rows are
[original-name-mappings.csv](../reverse/naming/task0019-original-name-mappings.csv);
the complete scoped follow-up is
[qualified-string-starts.csv](../reverse/naming/task0019-qualified-string-starts.csv).
All five have per-address records under reverse/functions. These five names are adopted in reverse/symbols.csv. None has a native C++
implementation to rename; signatures and calling conventions remain unknown. The Ghidra database remains unchanged.

## What remains unknown

111 qualified-text matches have no reported full-string reference in this sweep.
That includes many profiler-style World/Car/AI/Chopper method names. It does not
prove the absence of computed references or a method in the binary, but does not
justify renaming an address from nearby text or a plausible behavior match.

For requested animation helpers002490D8/00248C60, recovering a surrounding class
or allocation label would improve descriptive documentation. It cannot by itself
prove those helpers' method names. Retain `Sub_<ADDR>` until a direct original
method identifier/address association is recovered. The focused animation-owner
investigation is separate from this symbol/qualified-diagnostic inventory.

## Reproduction and validation

MCP symbols list, symbol_typeFUNCTION, each of source_typeIMPORTED/USER_DEFINED/
ANALYSIS/DEFAULT, paginating until no next_cursor. Memory search canonical
00100000..003358F0 for qualified-name text and RTTI markers; exclude mirrored RAM
aliases. Locate whole C-string boundaries using memory-read bytes, query references
to the true start, then inspect decompile and listing including MIPS delay slots.
Raw tool outputs are private under ignored `local/task0019/naming/`.

Metadata checks passed:7,544 unique function addresses, category counts add up,
118 nonoverlapping qualified matches, seven positive whole-string matches,
five method mappings with instruction proof. No runtime tests are appropriate
for this read-only naming investigation; no reconstructed behavior changed here.
No original/PCSX2 comparison or Verified promotion.
