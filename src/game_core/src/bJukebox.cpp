// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/include/bSample.h"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x00280340; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/audio/00280340.md.
void bSampleStream::SetSampleRate() {
    hp2::not_implemented("0x00280340", "bSampleStream::SetSampleRate (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include "game_core/src/bSound.hpp"

namespace hp2::recovered {
// ELF 0x00280298; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00280298.md.
void bSound::GetPath() {
    hp2::not_implemented("0x00280298", "bSound::GetPath (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include "game_core/include/bJukebox.h"

namespace hp2::recovered {
// ELF 0x002800E0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/002800e0.md.
void cJukeboxPlayer::IsDonePlaying() {
    hp2::not_implemented("0x002800E0", "cJukeboxPlayer::IsDonePlaying (signature and full body unresolved)");
}

// ELF 0x00280130; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00280130.md.
void cJukeboxPlayer::IsDoneFading() {
    hp2::not_implemented("0x00280130", "cJukeboxPlayer::IsDoneFading (signature and full body unresolved)");
}

// ELF 0x00280260; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/audio/00280260.md.
void cJukeboxPlayer::GetSampleRate() {
    hp2::not_implemented("0x00280260", "cJukeboxPlayer::GetSampleRate (signature and full body unresolved)");
}
} // namespace hp2::recovered
