#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace hp2::recovered {
// TASK-0041 E-0041-R13..R18: exact names and membership from the authorized
// SHA256-815b60c7... debug reference, independently matched to retail entries.
// The former Type_0011BF98 grouping combined two original class scopes.
// 0011BF98 is eStripDataE::GetElementXYZ, not a constructor.
// TASK-0042: original translation unit src/ecstasy/estripdatae.cpp is recovered.
// Companion estripdatae.hpp is a port-side header partition; original header unknown.
// Native ABI/layout remain unconfirmed; E-0042-S001 source evidence is in
// reverse/naming/task0042-source-ownership.csv (Debug ECOFF/MAP reference).
// std::vector/std::array interfaces and bounds exceptions are port-side projections.
// SLUS_203.62; executable revision unknown. SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// E-0033-G02: reverse/structures/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/vehicle/geometry.md.
class eStripData {
public:
    // ELF 0x0011C248; E-0041-R16: byte at packet +4.
    static std::uint8_t GetNumVerts(const std::vector<std::uint8_t>&, std::size_t);
    // ELF 0x0011C2B8; E-0041-R18; calls eStripDataE::GetElementXYZ at 0011BF98.
    static std::array<float, 3> GetXYZ(const std::vector<std::uint8_t>&, std::size_t, std::size_t);
    // ELF 0x0011C268; E-0041-R17; calls eStripDataE::GetElementST0 at 0011C090.
    static std::array<float, 2> GetST0(const std::vector<std::uint8_t>&, std::size_t, std::size_t);
};
class eStripDataE {
public:
    // ELF 0x0011C180; E-0041-R15. Original address calculation is projected as
    // a raw 16-bit load; this vector/static interface is not the original ABI.
    static std::uint16_t GetElementRGBA(const std::vector<std::uint8_t>&, std::size_t, std::size_t);
};
}
