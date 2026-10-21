// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/port/driving/object_view.hpp"
namespace hp2::recovered {
class CarState : public port::driving::ObjectView {
public:
 using ObjectView::ObjectView;
 // Debug001BD6C0; E-0081-L01; reverse/task0081/lifecycle.md. Full reset leaf;
 // position, distance counters, DriverMode and configuration pointers survive.
 void ResetCar();
public:
    // ELF 0x001C0780; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001c0780.md.
    // Original Debug reference: CarState::ResetCarForSound(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void ResetCarForSound();
    // Debug001BD938; E-0079-S01; reverse/task0079/driving-chain.md.
    // Debug ObjectView binding; resets crossed terrain at the frame boundary.
    void BeginTimestep();
    // ELF 0x001C0888; E-0042-C135; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/001c0888.md.
    // Original Debug reference: CarState::GetYaw(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    float GetYaw();
    // ELF 0x001C08D0; E-0042-C136; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/001c08d0.md.
    // Original Debug reference: CarState::GetYawAlways(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    float GetYawAlways();
    // ELF 0x001C0CF8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/001c0cf8.md.
    // Original Debug reference: CarState::SetWheelTouchingGround(int, char); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void SetWheelTouchingGround(std::int32_t,char);
    // ELF 0x001C0D48; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/driving/001c0d48.md.
    // Original Debug reference: CarState::SetWheelTerrain(int, TerrainType); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void SetWheelTerrain(std::int32_t,std::int32_t);
    // ELF 0x001C0D78; E-0042-U144; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/vehicle/001c0d78.md.
    // Original Debug reference: CarState::CalculateWheelMatrices(int); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void CalculateWheelMatrices();
 void CalculateWheelMatrices(std::int32_t);
};
} // namespace hp2::recovered
