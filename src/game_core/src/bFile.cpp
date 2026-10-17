// TASK-0042 E-0042-S001: exact Debug source marker(s): src\bFile.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// Original translation unit: `src\bFile.cpp` (ELF 0x002BF970; E-0029-P09).
// Directory binding through 0x00253828; see reverse/classes/source-path-bindings.json.
// This per-class file is a port-side partition, not the original translation unit or header.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: confirmed original directory (E-0029-P09); per-class basename/header extension port-side.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): bFile::Filename [0x002BF938]
#include "game_core/inc/bFile.hpp"

#include "game_core/src/bFile.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x00253A40; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00253a40.md.
void AsyncChunkPri::Sub_00253A40() {
    hp2::not_implemented("0x00253A40", "AsyncChunkPri::Sub_00253A40 (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x002538B8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/002538b8.md.
void AsyncEntry::Sub_002538B8() {
    hp2::not_implemented("0x002538B8", "AsyncEntry::Sub_002538B8 (signature and full body unresolved)");
}
} // namespace hp2::recovered
