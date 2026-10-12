# TASK-0009 — Whole-binary function and class recovery

- Status: active; whole-binary recovery is not yet complete.
- Request: recover every function, correct names and classes. Original names absent
  from the stripped ELF cannot be guaranteed; evidence-backed purpose names remain
  a separate category. Unknown identifiers/types remain unknown.
- Binary: SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2;
  USA/NTSC inferred, executable revision unknown, boot VER 1.00.
- Scope: full 7,544-entry recovery inventory and resumable pseudocode extraction;
  class-name evidence inventory; police class layout/ABI investigation as first
  concrete recovery unit. Preserve prior outputs and unrelated TASK-0005.
- Acceptance: all entries tracked with evidence, separate actual/current/proposed
  names and native-code status; each decompilation failure/body blocker retained;
  class association requires constructor/vtable/reference evidence; implemented
  functions need tests and explicit original-comparison limitations. No Verified
  promotion without original traces. No anonymous bulk renaming to fabricate coverage.
- Dependencies: live Ghidra MCP, original authorized ELF, GCC/Python; unknown types,
  damaged boundaries and missing original execution oracle remain blockers for
  complete readable/native behavior recovery.
- This turn's deliverables: complete inventory/export pass and first supported
  class-layout/helper artifacts, durable continuation and validation report.
- No related commits yet.

## First-pass findings and checks — 2026-10-11

- User confirmed only the existing ELF is available; no symbol/debug original-name source.
- Full 7,544-row inventory; 7,516 pseudocode exports + 28 boundary blockers,
  no decompile failures. Before/after metadata and ELF SHA-256 unchanged.
- 118 surviving class/method text hints / 49 distinct class texts, explicitly
  unassociated candidates. CopDriver/CopMover partial layout records and storage views added.
- Two native local bodies recovered: 0x001A0EA0 coefficient selection and
  0x001A2A90 low-speed timeout. Opaque 0x00231258 contract and float/oracle gaps
  remain unresolved. Tested local scope, never Verified.
- Raw Move flow candidate 0x001A1030–0x001A1BCB reaches 743 words and restores
  the entry stack frame at the return delay slot. Isolated candidate generation
  succeeds (7,517 translations + 27 placeholders); Move is 149,170 bytes of
  instruction translation. No live Ghidra mutation or readable native Move yet.
- `py -3 -m unittest discover -s reverse/scripts/tests -v`: 8/8 passed (five
  checkpoint safety tests, three raw topology tests). No game equivalence claim.
- `cmake --preset native-debug`, `cmake --build --preset native-debug`,
  `ctest --preset native-debug`: configure/build passed, CTest 4/4 passed.
  Standalone GCC `-std=c++17 -Wall -Wextra -Wpedantic -Werror -fno-fast-math`
  compilation and helper tests passed. Native helper coverage is in FULL_RECOVERY.md.
- Candidate generation config/executable/log, per-file export hashes and exact
  syntax-check argument arrays retained privately; scripts checked into project
  memory. GCC 13.2.0, Python 3.14.4, CMake 3.29.2, Ghidra 12.0.4.
- Existing staged submodule files preserved; no Git commit or restricted staging.

## Unmet acceptance and continuation

Every native function, original name, complete class/hierarchy, exact signatures
and original behavior are NOT restored. Task stays active. No automatic bulk
renaming or fake class implementation to fill the inventory. Read
`docs/FULL_RECOVERY.md`, then continue TASK-0009 with Move R5900/pcode/ABI audit,
virtual dispatch and car/terrain/vector call contracts; audit 0x001A0E88 tail call
before defining it. Trace each class-text hint to actual constructors/vtables and
callers. Add native bodies with tests, preserving unknown originals and boundary
blockers. Original execution traces are required before Verified promotion.

- `py -3 reverse/scripts/catalog_full_recovery.py` and `py -3 reverse/scripts/validate_full_recovery.py`: passed exhaustive address/file/hash checks and seven candidate C++ syntax probes, including full Move and registration. Both catalogs resolve actual generated filenames by address rather than guessing FUN_/sub_ spelling. No link/boot/equivalence claim.
