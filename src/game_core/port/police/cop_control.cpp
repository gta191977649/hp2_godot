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

















// ELF 0x00136C58; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136c58.md.
void CopDriver::Sub_00136C58() {
    hp2::not_implemented("0x00136C58", "CopDriver::Sub_00136C58 (signature and full body unresolved)");
}

// ELF 0x00136C60; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136c60.md.
void CopDriver::Sub_00136C60() {
    hp2::not_implemented("0x00136C60", "CopDriver::Sub_00136C60 (signature and full body unresolved)");
}





// ELF 0x00136CE8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136ce8.md.
void CopDriver::Sub_00136CE8() {
    hp2::not_implemented("0x00136CE8", "CopDriver::Sub_00136CE8 (signature and full body unresolved)");
}

// ELF 0x00136CF0; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136cf0.md.
void CopDriver::Sub_00136CF0() {
    hp2::not_implemented("0x00136CF0", "CopDriver::Sub_00136CF0 (signature and full body unresolved)");
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
// TASK-0041 E-0041-V01; debug 0x0019F980: CopDriver::Think(float); retail 0x001A2110.
void CopDriver::Think() {
    hp2::not_implemented("0x001A2110", "CopDriver::Think (signature and full body unresolved)");
}

// ELF 0x001A2B58; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001a2b58.md.
// TASK-0041 E-0041-V01; debug 0x001A03B8: CopDriver::DoSnapshot(ReplaySnapshot *); retail 0x001A2B58.
void CopDriver::DoSnapshot() {
    hp2::not_implemented("0x001A2B58", "CopDriver::DoSnapshot (signature and full body unresolved)");
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
