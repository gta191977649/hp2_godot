// TASK-0042 E-0042-S001: exact Debug source marker(s): ../indep/inc/Sound3d.hpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring class with adopted retail correspondence; evidence: E-0042-S001.
// Placement: TASK-0042 E-0042-S001; exact Debug definition source/header markers where recorded; unmatched companion remains port-side; retail ABI unconfirmed.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class Sound3d {
public:
    // ELF 0x00208B28; E-0041-V01; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00208b28.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Activate();
    // ELF 0x00208C78; E-0041-V01; E-0042-S001; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00208c78.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void UpdateCamRelativeDistAndVelocity();

public:
    // TASK-0028: missing entries; existing storage and bodies are preserved.
    // ELF 0x00208FE0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00208fe0.md.
    // Original Debug reference: Sound3d::AzimuthToPan(unsigned short); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void AzimuthToPan();
    // ELF 0x002091A0; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/002091a0.md.
    // Original Debug reference: Sound3d::~Sound3d(void); constructor/destructor entry is not a native C++ lifetime operation.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_002091A0();
};
} // namespace hp2::recovered

#include "game_core/port/driving/object_view.hpp"
namespace hp2::debug {
class Sound3d : public ::port::driving::ObjectView {
public:
 using ObjectView::ObjectView;
 void Sub_00203980();void Sub_00204060(int); // E-0099-W27.
 void Activate(bool);void ActivateSound();void DeactivateSound();
 static float GetDistToClosestCamera(::port::driving::Memory&,std::uint32_t); //00203A30 E-0099-W40
 static std::uint16_t ComputeAzimuth(::port::driving::Memory&,std::uint32_t); //00203C90
 static float ComputeVolume(::port::driving::Memory&,std::uint32_t,float); //00203E20
 void UpdateCamRelativeDistAndVelocity(std::uint32_t position); //00203B10 E-0099-W40.
 void Update(std::uint32_t position,std::uint32_t envelope); //00203B88.
};
}

namespace hp2::debug {
// Debug001365F0,E-0099-W106; original smooth<float> inline definition owner.
inline float smooth(float curr,float target,float delta){float next=curr+delta;if(target<=next){next=curr-delta;if(next<=target)next=target;}return next;}
}
