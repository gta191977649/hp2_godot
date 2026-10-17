// PORT-SIDE FILE. No original source path for this module was recovered from the ELF, so
// its name and location are this project's, not the original project's (TASK-0020). Do not
// read this path as evidence of where the original kept this code.
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

// TASK-0009, SLUS_203.62 SHA-256:
// 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// CopDriver/CopMover spellings are recovered allocation labels (E-0027-C01).
// Original method names/complete classes unknown; methods use ELF entry addresses.
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
    // ELF 0x001A0EA0; E-0009-01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/police/001a0ea0.md.
    // Complete local control flow; memory reading is an adapter contract.
    void Sub_001A0EA0(const PoliceMemory& memory);
private:
    CopMoverStorage& storage_;

public:
    // TASK-0028: missing entries; existing storage and bodies are preserved.
    // ELF 0x001A0DD8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/police/001a0dd8.md. Constructor entry; NOT a C++ constructor.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_001A0DD8();
    // ELF 0x001A0E88; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001a0e88.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_001A0E88();
    // ELF 0x001A0F20; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001a0f20.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_001A0F20();
    // ELF 0x001A1030; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/police/001a1030.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_001A1030();
};

class CopDriver {
public:
    explicit CopDriver(CopDriverStorage& existing_storage) : storage_(existing_storage) {}
    // ELF 0x001A2A90; E-0009-02; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/police/001a2a90.md.
    // dt in f12, object in a0; finite normal float fidelity unverified.
    bool Sub_001A2A90(float dt, PoliceMemory& memory);
private:
    CopDriverStorage& storage_;

public:
    // TASK-0028: missing entries; existing storage and bodies are preserved.
    // ELF 0x00130958; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00130958.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_00130958();
    // ELF 0x00136BB8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136bb8.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_00136BB8();
    // ELF 0x00136BC0; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136bc0.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_00136BC0();
    // ELF 0x00136BD0; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136bd0.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_00136BD0();
    // ELF 0x00136BD8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136bd8.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_00136BD8();
    // ELF 0x00136BE0; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136be0.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_00136BE0();
    // ELF 0x00136BE8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136be8.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_00136BE8();
    // ELF 0x00136C58; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136c58.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_00136C58();
    // ELF 0x00136C60; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136c60.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_00136C60();
    // ELF 0x00136CE8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136ce8.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_00136CE8();
    // ELF 0x00136CF0; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136cf0.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_00136CF0();
    // ELF 0x001A1CE8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/police/001a1ce8.md. Constructor entry; NOT a C++ constructor.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_001A1CE8();
    // ELF 0x001A1E30; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001a1e30.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_001A1E30();
    // ELF 0x001A2110; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/police/001a2110.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    // Debug reference: CopDriver::Think(float); E-0041-V01; retail ABI unconfirmed.
    void Think();
    // ELF 0x001A2B58; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/001a2b58.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    // Debug reference: CopDriver::DoSnapshot(ReplaySnapshot *); E-0041-V01; retail ABI unconfirmed.
    void DoSnapshot();
    // ELF 0x002A0550; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/002a0550.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_002A0550();
};

} // namespace hp2::recovered
