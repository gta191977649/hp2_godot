#pragma once
#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace hp2::recovered {
// Address-anchored packet accessor grouping; no original class ownership claimed.
// SLUS_203.62 SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// E-0033-G02: reverse/structures/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/vehicle/geometry.md.
class Type_0011BF98 {
public:
    // ELF 0x0011C248: byte at packet +4.
    static std::uint8_t Sub_0011C248(const std::vector<std::uint8_t>&, std::size_t);
    // ELF 0x0011BF98 and 0x0011C2B8: packed position component access.
    static std::array<float, 3> Sub_0011C2B8(const std::vector<std::uint8_t>&, std::size_t, std::size_t);
    // ELF 0x0011C090 and 0x0011C268: paired UV component access.
    static std::array<float, 2> Sub_0011C268(const std::vector<std::uint8_t>&, std::size_t, std::size_t);
    // ELF 0x0011C180: color address calculation, projected as raw 16-bit load.
    static std::uint16_t Sub_0011C180(const std::vector<std::uint8_t>&, std::size_t, std::size_t);
};
}
