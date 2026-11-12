// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class Collision {
public:
    // ELF 0x0021EB20; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0021eb20.md.
    // Original Debug reference: Collision::IsTwoCarCollision(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void IsTwoCarCollision();
    // ELF 0x0021EB48; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0021eb48.md.
    // Original Debug reference: Collision::GetCollidingCars(Car **, Car **); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetCollidingCars();
    // ELF 0x0021EBD8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0021ebd8.md.
    // Original Debug reference: Collision::IsSmokeableInvolved(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void IsSmokeableInvolved();
};
} // namespace hp2::recovered

#include "game_core/port/driving/object_view.hpp"
namespace hp2::debug {
// Debug002191D8/00219208, E-0099-W38; original collision.cpp.
void InitCollision(::port::driving::Memory&);
void CloseCollision(::port::driving::Memory&);
class Collision : public ::port::driving::ObjectView {
public:
 using ObjectView::ObjectView;
 // Debug00219230/00219308; explicit original pool-owned lifecycle.
 void Sub_00219230(std::uint32_t witness,std::uint32_t contact_point);
 void Sub_00219308(int flags);
 int GetTerrainType() const; //00219698, E-0099-W38
 bool IsTwoCarCollision() const; //00219730
 void GetCollidingCars(std::uint32_t& first,std::uint32_t& second) const; //00219758
 bool IsSmokeableInvolved() const; //002197E8
 bool IsPlayerInvolved() const; //002197A0
 void DoSnapshot(std::uint32_t); //00219370 E-0099-W76.
 int Update(float timestep); //00219650
 void End(); //00219690
 void AddImpulse(std::uint32_t point,std::uint32_t impulse,float scrape_speed); //00219400, E-0099-W38
};
}
