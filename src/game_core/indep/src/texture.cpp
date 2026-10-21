// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\texture.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/indep/inc/Texture.hpp"

#include "game_core/indep/src/texture.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// Retail001ABF48 / Debug001A95C8; E0056T01, reverse/task0056/textures.md.
std::optional<std::uint32_t> GetTextureInfo(std::uint32_t name,bool return_default,
    const std::set<std::uint32_t>& loaded,const std::set<std::uint32_t>& targets) {
    if(loaded.count(name)||targets.count(name))return name;
    if(!return_default)return std::nullopt;
    auto fallback=GetTextureInfo(0x19006835,false,loaded,targets);
    if(!fallback)hp2::not_implemented("0x001ABF48","GetTextureInfo: original DEFAULTTEXTURE absent, bBreak path");
    return fallback;
}
// ELF 0x001AC3F0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/vehicle/001ac3f0.md.
void TexturePack::GetLoadedTexture() {
    hp2::not_implemented("0x001AC3F0", "TexturePack::GetLoadedTexture (signature and full body unresolved)");
}

// ELF 0x001AC468; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001ac468.md.
void TexturePack::GetTexture() {
    hp2::not_implemented("0x001AC468", "TexturePack::GetTexture (signature and full body unresolved)");
}

// ELF 0x001AC4B0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001ac4b0.md.
void TexturePack::GetNthTexture() {
    hp2::not_implemented("0x001AC4B0", "TexturePack::GetNthTexture (signature and full body unresolved)");
}

// ELF 0x001AC780; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001ac780.md.
void TexturePack::FindLoadedTextureData() {
    hp2::not_implemented("0x001AC780", "TexturePack::FindLoadedTextureData (signature and full body unresolved)");
}

// ELF 0x001AC7D0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001ac7d0.md.
void TexturePack::FindNotYetLoadedNeighbourTextureData() {
    hp2::not_implemented("0x001AC7D0", "TexturePack::FindNotYetLoadedNeighbourTextureData (signature and full body unresolved)");
}

// ELF 0x001AD008; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001ad008.md.
void TexturePack::GetLoadedSize() {
    hp2::not_implemented("0x001AD008", "TexturePack::GetLoadedSize (signature and full body unresolved)");
}
} // namespace hp2::recovered
