# TASK-0019 — Original function identifier recovery

Status: completed scoped investigation, 2026-10-13. RenderingServer deferred by user.
Target US PS2 SLUS_203.62, revision unknown; SHA-256
7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.

Scope: inspect original names, especially 002490D8/00248C60. Read-only Ghidra;
no guessed names, database mutations, runtime reconstruction or rendering migration.

Acceptance results:
- E-0019-N02: 7,544 unique function-symbol entries classified by source.
- E-0019-N03: 118 qualified text matches checked at whole C-string boundaries;
  five original methods supported by diagnostics, references and MIPS instructions.
  Adopted in reverse/symbols.csv with unknown signatures/ABI and Decompiled status.
- E-0019-N01: requested animation names remain unknown; 00248C60 shared by
  entrance/exit common lists. Allocation labels do not prove canonical class names.
  options.hpp comments clarify that current membership is port encapsulation.
- No existing native implementation of the five named methods exists to rename.
- Five adopted addresses are unique and all five record paths exist. The index
  already contains two rows for 001680E8 from earlier tasks; preserved as unrelated
  naming debt, so whole-index uniqueness does not pass. git diff --check passed.
  Runtime tests omitted because only metadata/comments changed; no equivalence claim.

Records: docs/FUNCTION_NAME_RECOVERY.md, docs/ANIMATION_IDENTIFIER_AUDIT.md,
reverse/naming/task0019-*.csv and per-address function records.
No commit or staging. Preserve unrelated files and incomplete TASK-0018 work.
Next: investigate computed references for 111 remaining qualified string matches;
retain address identifiers unless original spelling/address association is proven.
