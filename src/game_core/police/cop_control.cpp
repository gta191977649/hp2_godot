#include "game_core/police/cop_control.hpp"

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

void CopMover::select_type_coefficient(const PoliceMemory& memory) {
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

bool CopDriver::check_low_speed_timeout(float dt, PoliceMemory& memory) {
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
