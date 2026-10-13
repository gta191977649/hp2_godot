// PORT-SIDE FILE. No original source path for this module was recovered from the ELF, so
// its name and location are this project's, not the original project's (TASK-0020). Do not
// read this path as evidence of where the original kept this code.
#include "game_core/port/police/cop_control.hpp"

#include <cstring>
#include <limits>

namespace hp2::recovered {
namespace {
static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559);

std::uint32_t load(const std::uint8_t* p) {
    return std::uint32_t(p[0]) | (std::uint32_t(p[1]) << 8) |
           (std::uint32_t(p[2]) << 16) | (std::uint32_t(p[3]) << 24);
}
void store(std::uint8_t* p, std::uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) p[i] = std::uint8_t(value >> (8 * i));
}
float as_float(std::uint32_t value) {
    float result;
    std::memcpy(&result, &value, sizeof(result));
    return result;
}
std::uint32_t as_bits(float value) {
    std::uint32_t result;
    std::memcpy(&result, &value, sizeof(result));
    return result;
}
} // namespace

void CopMover::Sub_001A0EA0(const PoliceMemory& memory) {
    const auto car = load(storage_.data() + 0x08);
    const auto config = memory.read_u32(car + std::uint32_t{0x0c});
    const auto type = memory.read_u32(config + std::uint32_t{0x534});
    // Disk jump table 0x002B5760 and LUI/MTC1/SWC1 return paths.
    // Preserve unknown type behavior: no write. Physical coefficient meaning unknown.
    std::uint32_t bits;
    switch (type) {
    case 4: bits = 0x41340000; break;
    case 31: bits = 0x414c0000; break;
    case 32:
    case 33: bits = 0x41700000; break;
    case 34: bits = 0x41640000; break;
    default: return;
    }
    store(storage_.data() + 0x3c, bits);
}

bool CopDriver::Sub_001A2A90(float dt, PoliceMemory& memory) {
    const auto car = load(storage_.data());
    if (!memory.query_00231258(car)) return false;
    const auto state = load(storage_.data() + 0x24);
    switch (state) {
    case 0:
    case 1:
    case 4:
        store(storage_.data() + 0x30, 0);
        return false;
    case 2:
    case 3:
        if (as_float(memory.read_u32(car + std::uint32_t{0x3c4})) < 2.0f) {
            // Assembly LWC1/ADD.S/SWC1 confirms float bits, correcting Ghidra's
            // misleading int-to-float and float-to-int casts for this field.
            const float timer = as_float(load(storage_.data() + 0x30)) + dt;
            store(storage_.data() + 0x30, as_bits(timer));
            return 1.0f < timer;
        }
        store(storage_.data() + 0x30, 0);
        return false;
    default:
        return false;
    }
}

} // namespace hp2::recovered

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x00130958; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00130958.md.
void CopDriver::Sub_00130958() {
    hp2::not_implemented("0x00130958", "CopDriver::Sub_00130958 (signature and full body unresolved)");
}

// ELF 0x00136BB8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136bb8.md.
void CopDriver::Sub_00136BB8() {
    hp2::not_implemented("0x00136BB8", "CopDriver::Sub_00136BB8 (signature and full body unresolved)");
}

// ELF 0x00136BC0; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136bc0.md.
void CopDriver::Sub_00136BC0() {
    hp2::not_implemented("0x00136BC0", "CopDriver::Sub_00136BC0 (signature and full body unresolved)");
}

// ELF 0x00136BD0; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136bd0.md.
void CopDriver::Sub_00136BD0() {
    hp2::not_implemented("0x00136BD0", "CopDriver::Sub_00136BD0 (signature and full body unresolved)");
}

// ELF 0x00136BD8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136bd8.md.
void CopDriver::Sub_00136BD8() {
    hp2::not_implemented("0x00136BD8", "CopDriver::Sub_00136BD8 (signature and full body unresolved)");
}

// ELF 0x00136BE0; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136be0.md.
void CopDriver::Sub_00136BE0() {
    hp2::not_implemented("0x00136BE0", "CopDriver::Sub_00136BE0 (signature and full body unresolved)");
}

// ELF 0x00136BE8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136be8.md.
void CopDriver::Sub_00136BE8() {
    hp2::not_implemented("0x00136BE8", "CopDriver::Sub_00136BE8 (signature and full body unresolved)");
}

// ELF 0x00136C00; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136c00.md.
void CopDriver::Sub_00136C00() {
    hp2::not_implemented("0x00136C00", "CopDriver::Sub_00136C00 (signature and full body unresolved)");
}

// ELF 0x00136C08; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136c08.md.
void CopDriver::Sub_00136C08() {
    hp2::not_implemented("0x00136C08", "CopDriver::Sub_00136C08 (signature and full body unresolved)");
}

// ELF 0x00136C10; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136c10.md.
void CopDriver::Sub_00136C10() {
    hp2::not_implemented("0x00136C10", "CopDriver::Sub_00136C10 (signature and full body unresolved)");
}

// ELF 0x00136C20; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136c20.md.
void CopDriver::Sub_00136C20() {
    hp2::not_implemented("0x00136C20", "CopDriver::Sub_00136C20 (signature and full body unresolved)");
}

// ELF 0x00136C30; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136c30.md.
void CopDriver::Sub_00136C30() {
    hp2::not_implemented("0x00136C30", "CopDriver::Sub_00136C30 (signature and full body unresolved)");
}

// ELF 0x00136C38; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136c38.md.
void CopDriver::Sub_00136C38() {
    hp2::not_implemented("0x00136C38", "CopDriver::Sub_00136C38 (signature and full body unresolved)");
}

// ELF 0x00136C40; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136c40.md.
void CopDriver::Sub_00136C40() {
    hp2::not_implemented("0x00136C40", "CopDriver::Sub_00136C40 (signature and full body unresolved)");
}

// ELF 0x00136C50; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136c50.md.
void CopDriver::Sub_00136C50() {
    hp2::not_implemented("0x00136C50", "CopDriver::Sub_00136C50 (signature and full body unresolved)");
}

// ELF 0x00136C58; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136c58.md.
void CopDriver::Sub_00136C58() {
    hp2::not_implemented("0x00136C58", "CopDriver::Sub_00136C58 (signature and full body unresolved)");
}

// ELF 0x00136C60; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136c60.md.
void CopDriver::Sub_00136C60() {
    hp2::not_implemented("0x00136C60", "CopDriver::Sub_00136C60 (signature and full body unresolved)");
}

// ELF 0x00136C68; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136c68.md.
void CopDriver::Sub_00136C68() {
    hp2::not_implemented("0x00136C68", "CopDriver::Sub_00136C68 (signature and full body unresolved)");
}

// ELF 0x00136C70; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136c70.md.
void CopDriver::Sub_00136C70() {
    hp2::not_implemented("0x00136C70", "CopDriver::Sub_00136C70 (signature and full body unresolved)");
}

// ELF 0x00136CE8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136ce8.md.
void CopDriver::Sub_00136CE8() {
    hp2::not_implemented("0x00136CE8", "CopDriver::Sub_00136CE8 (signature and full body unresolved)");
}

// ELF 0x00136CF0; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136cf0.md.
void CopDriver::Sub_00136CF0() {
    hp2::not_implemented("0x00136CF0", "CopDriver::Sub_00136CF0 (signature and full body unresolved)");
}

// ELF 0x00136D00; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136d00.md.
void CopDriver::Sub_00136D00() {
    hp2::not_implemented("0x00136D00", "CopDriver::Sub_00136D00 (signature and full body unresolved)");
}

// ELF 0x001A1CE8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/police/001a1ce8.md.
void CopDriver::Sub_001A1CE8() {
    hp2::not_implemented("0x001A1CE8", "CopDriver::Sub_001A1CE8 (signature and full body unresolved)");
}

// ELF 0x001A1E30; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001a1e30.md.
void CopDriver::Sub_001A1E30() {
    hp2::not_implemented("0x001A1E30", "CopDriver::Sub_001A1E30 (signature and full body unresolved)");
}

// ELF 0x001A2110; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/police/001a2110.md.
void CopDriver::Sub_001A2110() {
    hp2::not_implemented("0x001A2110", "CopDriver::Sub_001A2110 (signature and full body unresolved)");
}

// ELF 0x001A2B58; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001a2b58.md.
void CopDriver::Sub_001A2B58() {
    hp2::not_implemented("0x001A2B58", "CopDriver::Sub_001A2B58 (signature and full body unresolved)");
}

// ELF 0x001C0100; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001c0100.md.
void CopDriver::Sub_001C0100() {
    hp2::not_implemented("0x001C0100", "CopDriver::Sub_001C0100 (signature and full body unresolved)");
}

// ELF 0x001C0140; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001c0140.md.
void CopDriver::Sub_001C0140() {
    hp2::not_implemented("0x001C0140", "CopDriver::Sub_001C0140 (signature and full body unresolved)");
}

// ELF 0x001C0160; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001c0160.md.
void CopDriver::Sub_001C0160() {
    hp2::not_implemented("0x001C0160", "CopDriver::Sub_001C0160 (signature and full body unresolved)");
}

// ELF 0x001C0198; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001c0198.md.
void CopDriver::Sub_001C0198() {
    hp2::not_implemented("0x001C0198", "CopDriver::Sub_001C0198 (signature and full body unresolved)");
}

// ELF 0x0023A178; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0023a178.md.
void CopDriver::Sub_0023A178() {
    hp2::not_implemented("0x0023A178", "CopDriver::Sub_0023A178 (signature and full body unresolved)");
}

// ELF 0x0023A198; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0023a198.md.
void CopDriver::Sub_0023A198() {
    hp2::not_implemented("0x0023A198", "CopDriver::Sub_0023A198 (signature and full body unresolved)");
}

// ELF 0x0023A1C8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0023a1c8.md.
void CopDriver::Sub_0023A1C8() {
    hp2::not_implemented("0x0023A1C8", "CopDriver::Sub_0023A1C8 (signature and full body unresolved)");
}

// ELF 0x0023A2F8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0023a2f8.md.
void CopDriver::Sub_0023A2F8() {
    hp2::not_implemented("0x0023A2F8", "CopDriver::Sub_0023A2F8 (signature and full body unresolved)");
}

// ELF 0x0023A300; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0023a300.md.
void CopDriver::Sub_0023A300() {
    hp2::not_implemented("0x0023A300", "CopDriver::Sub_0023A300 (signature and full body unresolved)");
}

// ELF 0x0023A3D0; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0023a3d0.md.
void CopDriver::Sub_0023A3D0() {
    hp2::not_implemented("0x0023A3D0", "CopDriver::Sub_0023A3D0 (signature and full body unresolved)");
}

// ELF 0x002A0550; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/002a0550.md.
void CopDriver::Sub_002A0550() {
    hp2::not_implemented("0x002A0550", "CopDriver::Sub_002A0550 (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x001A0DD8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/police/001a0dd8.md.
void CopMover::Sub_001A0DD8() {
    hp2::not_implemented("0x001A0DD8", "CopMover::Sub_001A0DD8 (signature and full body unresolved)");
}

// ELF 0x001A0E88; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001a0e88.md.
void CopMover::Sub_001A0E88() {
    hp2::not_implemented("0x001A0E88", "CopMover::Sub_001A0E88 (signature and full body unresolved)");
}

// ELF 0x001A0F20; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001a0f20.md.
void CopMover::Sub_001A0F20() {
    hp2::not_implemented("0x001A0F20", "CopMover::Sub_001A0F20 (signature and full body unresolved)");
}

// ELF 0x001A1030; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/police/001a1030.md.
void CopMover::Sub_001A1030() {
    hp2::not_implemented("0x001A1030", "CopMover::Sub_001A1030 (signature and full body unresolved)");
}
} // namespace hp2::recovered
