// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once
#include "game_core/port/driving/object_view.hpp"
namespace hp2::debug {
class DrivingLocation : public ::port::driving::ObjectView {
public:
 using ObjectView::ObjectView;
 void DoSnapshot(std::uint32_t snapshot); //00233A38, W74.
 // Debug002338F8/00233918/00233928/002339C8, E-0099-W34.
 void SetRoute(std::uint32_t route,std::uint32_t point,int line,bool relaxed);
 void SetRoutePoint(std::uint32_t point); //Debug00233910, W47.
 void SetDrivingLine(int line,bool relaxed);
 void SetRelaxedSwitch(bool);
 bool IsClear()const;
 // Debug00233940/00233990, E-0099-W37.
 float GetNumSecondsLeftInRelaxedSwitch()const;
 bool AreWeInARelaxedSwitch()const;
};
}

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class DrivingLocation {
public:
    // ELF 0x00239C90; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00239c90.md.
    // Original Debug reference: DrivingLocation::DoesThisRoutePointMatchThisRoute(TrackRoute *, TrackRoutePoint *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void DoesThisRoutePointMatchThisRoute();
};
} // namespace hp2::recovered
