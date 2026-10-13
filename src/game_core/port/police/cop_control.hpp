// PORT-SIDE FILE. No original source path for this module was recovered from the ELF, so
// its name and location are this project's, not the original project's (TASK-0020). Do not
// read this path as evidence of where the original kept this code.
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

// TASK-0009, SLUS_203.62 SHA-256:
// 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Analysis-purpose API names; original identifiers/complete classes unknown.
// Views of existing PS2 storage, not reconstructed object constructors/inheritance.
namespace hp2::recovered {

class PoliceMemory {
public:
    virtual ~PoliceMemory() = default;
    virtual std::uint32_t read_u32(std::uint32_t address) const = 0;
    // Opaque external call at 0x00231258: true means its return was nonzero.
    // The adapter must preserve its side effects; behavior is not implemented here.
    virtual bool query_00231258(std::uint32_t car_address) = 0;
};

using CopMoverStorage = std::array<std::uint8_t, 0x80>;
using CopDriverStorage = std::array<std::uint8_t, 0x4c>;

class CopMover {
public:
    explicit CopMover(CopMoverStorage& existing_storage) : storage_(existing_storage) {}
    // 0x001A0EA0: complete local control flow; memory reading is an adapter contract.
    void select_type_coefficient(const PoliceMemory& memory);
private:
    CopMoverStorage& storage_;
};

class CopDriver {
public:
    explicit CopDriver(CopDriverStorage& existing_storage) : storage_(existing_storage) {}
    // 0x001A2A90: dt in f12, object in a0; finite normal float fidelity unverified.
    bool check_low_speed_timeout(float dt, PoliceMemory& memory);
private:
    CopDriverStorage& storage_;
};

} // namespace hp2::recovered
