// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/indep/src/movieplayer/ringbuffer.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x002393A0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/002393a0.md.
void RingBuffer::GetLastFrame() {
    hp2::not_implemented("0x002393A0", "RingBuffer::GetLastFrame (signature and full body unresolved)");
}

// ELF 0x002395A0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/002395a0.md.
void RingBuffer::ContinuousAvail() {
    hp2::not_implemented("0x002395A0", "RingBuffer::ContinuousAvail (signature and full body unresolved)");
}

// ELF 0x00239A58; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00239a58.md.
void RingBuffer::GetFrame() {
    hp2::not_implemented("0x00239A58", "RingBuffer::GetFrame (signature and full body unresolved)");
}

// ELF 0x00239B48; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00239b48.md.
void RingBuffer::ReleaseFrame() {
    hp2::not_implemented("0x00239B48", "RingBuffer::ReleaseFrame (signature and full body unresolved)");
}
} // namespace hp2::recovered
