// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once
#include "game_core/port/driving/memory.hpp"
#include <functional>

namespace port::race {
// Explicit bindings of original frontend/SkipFE globals; no profile ABI overlay.
struct DefaultSettings {
    bool skip_frontend=false,english_language=true;
    std::uint8_t pov_index=1,rear_view=1,speedo_mode=0,jump_camera=1,zone_camera=1;
};
using SkinLookup=std::function<void(int,int,int)>;
}
namespace hp2::debug {
// E-0098-RP01. Logical PS2 storage projection, not a host ABI or a second world.
class RaceParameters {
public:
    RaceParameters(::port::driving::Memory& memory,std::uint32_t address):memory_(memory),address_(address){}
    void InitWithDefaults(const ::port::race::DefaultSettings&);
    void AddDriverInfo(std::uint32_t,const ::port::race::SkinLookup&);
    void SetDriverInfo(std::uint32_t,int);
    std::uint32_t GetDriverInfo(int) const;
    std::uint32_t GetDriverInfoByDriverNumber(int) const;
    std::uint32_t EliminateDriver(int,int);
    void ClearEliminatedDrivers();
    int GetNumEliminated() const;
    void ResetStartingPositions();
    bool IsFreeRun() const;
    int GetLocationNumber() const;
    int GetLocationIndex() const;
    void ChooseHudTextures();
private:
    ::port::driving::Memory& memory_;
    std::uint32_t address_;
};
struct tGlobalDisplaySettings {
    static void ResetToDefaults(::port::driving::Memory&,std::uint32_t,const ::port::race::DefaultSettings&);
};
}

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class RaceParameters {
public:
    // ELF 0x001EDC50; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001edc50.md.
    // Original Debug reference: RaceParameters::ClearEliminatedDrivers(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void ClearEliminatedDrivers();
    // ELF 0x001EDCD0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001edcd0.md.
    // Original Debug reference: RaceParameters::GetNumEliminated(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetNumEliminated();
    // ELF 0x001EDDB8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001eddb8.md.
    // Original Debug reference: RaceParameters::GetLocationNumber(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetLocationNumber();
    // ELF 0x001EE390; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001ee390.md.
    // Original Debug reference: RaceParameters::IsFreeRun(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void IsFreeRun();
};
} // namespace hp2::recovered

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class cPlayerSettings {
public:
    // ELF 0x001EE1F8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001ee1f8.md.
    // Original Debug reference: cPlayerSettings::ToggleGauges(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void ToggleGauges();
    // ELF 0x001EE230; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001ee230.md.
    // Original Debug reference: cPlayerSettings::ToggleLapInfo(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void ToggleLapInfo();
    // ELF 0x001EE258; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001ee258.md.
    // Original Debug reference: cPlayerSettings::CycleSplitTime(int); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void CycleSplitTime();
    // ELF 0x001EE2B0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001ee2b0.md.
    // Original Debug reference: cPlayerSettings::ToggleRacePosition(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void ToggleRacePosition();
    // ELF 0x001EE2D8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001ee2d8.md.
    // Original Debug reference: cPlayerSettings::ToggleLookback(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void ToggleLookback();
    // ELF 0x001EE300; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001ee300.md.
    // Original Debug reference: cPlayerSettings::CycleAlternateCamera(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void CycleAlternateCamera();
};
} // namespace hp2::recovered

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class tGlobalDisplaySettings {
public:
    // ELF 0x001EE120; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001ee120.md.
    // Original Debug reference: tGlobalDisplaySettings::CycleLeaderboardMode(int); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void CycleLeaderboardMode();
};
} // namespace hp2::recovered
