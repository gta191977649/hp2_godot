// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\choppersound.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/ChopperSound.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {




// ELF 0x00214D68; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00214d68.md.
void ChopperSound::Sub_00214D68() {
    hp2::not_implemented("0x00214D68", "ChopperSound::Sub_00214D68 (signature and full body unresolved)");
}

// ELF 0x00214DF0; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00214df0.md.
void ChopperSound::Sub_00214DF0() {
    hp2::not_implemented("0x00214DF0", "ChopperSound::Sub_00214DF0 (signature and full body unresolved)");
}

// ELF 0x00214F30; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00214f30.md.
// TASK-0041 E-0041-V01; debug 0x0020FB28: ChopperSound::Update(float, unsigned short, float); retail 0x00214F30.
void ChopperSound::Update() {
    hp2::not_implemented("0x00214F30", "ChopperSound::Update (signature and full body unresolved)");
}

// ELF 0x00215000; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00215000.md.
// TASK-0041 E-0041-V01; debug 0x0020FBF8: ChopperSound::Mute(int); retail 0x00215000.
void ChopperSound::Mute() {
    hp2::not_implemented("0x00215000", "ChopperSound::Mute (signature and full body unresolved)");
}

// ELF 0x00215028; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00215028.md.
// TASK-0041 E-0041-V01; debug 0x0020FC20: ChopperSound::DeactivateSound(void); retail 0x00215028.
void ChopperSound::DeactivateSound() {
    hp2::not_implemented("0x00215028", "ChopperSound::DeactivateSound (signature and full body unresolved)");
}

// ELF 0x00215050; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00215050.md.
// TASK-0041 E-0041-V01; debug 0x0020FC48: ChopperSound::ActivateSound(void); retail 0x00215050.
void ChopperSound::ActivateSound() {
    hp2::not_implemented("0x00215050", "ChopperSound::ActivateSound (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include "game_core/indep/inc/Sound3d.hpp"
#include "game_core/indep/src/sound.hpp"
#include "game_core/src/bSample.hpp"
namespace hp2::debug {
// Debug0020F960, E-0099-W94. Actual shared source sample; no muted substitute.
void ChopperSound::Sub_0020F960(std::uint32_t chopper){Sound3d(m_,address_).Sub_00203980();m_.word(address_+0x20,chopper);m_.word(address_+0x1c,0x2b3e48);if(m_.word(0x326f40)){const auto sample=GetInGameSample(m_,0x2b3e00);m_.word(address_+0x24,sample);if(!sample)hp2::not_implemented("0x0020F960","original ChopperEngine sample is absent from actual InGame region");bSample object(m_,sample);object.SetCategory(0);if(object.IsPlaying())object.Stop();}m_.word(address_+0x28,0);m_.word(address_+0x2c,0);}
// Debug0020F9E8, E-0099-W94. Source dtor delegates directly to Sound3d.
void ChopperSound::Sub_0020F9E8(int flags){m_.word(address_+0x1c,0x2b3e48);Sound3d(m_,address_).Sub_00204060(flags);}
// Debug0020FC20, E-0099-W98; exact choppersound.cpp definition owner.
void ChopperSound::DeactivateSound(){if(m_.word(0x326f40))if(const auto sample=m_.word(address_+0x24))bSample(m_,sample).Stop();}
namespace sound {
// Debug0020FC90, E-0099-W94; exact choppersound.cpp definition owner.
std::uint32_t CreateChopperSound(::port::driving::Memory& m,std::uint32_t chopper){const auto object=m.allocate(0x30);ChopperSound(m,object).Sub_0020F960(chopper);return object;}
}
}
