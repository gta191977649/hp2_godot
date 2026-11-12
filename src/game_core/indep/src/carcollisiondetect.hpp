// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once
#include "game_core/port/driving/object_view.hpp"
#include <optional>
namespace hp2::debug {
// Debug001E3048/001E37C0/001E3880; E-0099-W37, original SAT helpers.
int IntervalOverlap(float a,float b,float c,float d,float& low,float& high);
void TimeIntervalForSeparatingAxis(const ::port::driving::Memory&,float velocity,float translation,float sum,float& low,float& high);
float WhenWillOverlapOccur2D(const ::port::driving::Memory&,std::uint32_t a,std::uint32_t b,const std::array<float,2>& translation,const std::array<float,2>& velocity);
class CarCollisionPredict : public ::port::driving::ObjectView {
public:
 using ObjectView::ObjectView;
 bool IsThisCarStoppedDirectlyInFrontOfMe(std::uint32_t nearby_car,std::uint32_t nearby_cars)const; //Debug001E25C0
 void MakeOBBsForDriveTarget(std::uint32_t target,std::uint32_t obbs,std::uint32_t car,float exaggeration); //Debug001E2DE8
 bool WillIHitThisCar(std::uint32_t target,std::uint32_t location,std::uint32_t my_car,std::uint32_t other_car,float maximum,float& impact,std::uint32_t port_stack_vector_lane); //Debug001E2650
 // Debug001E2410, E-0099-W37. Original nearby filtering and full moving
 // prediction; unknown temporary W operand is required only when used.
 std::uint32_t WhoWillThisDriveTargetHit(std::uint32_t target,std::uint32_t nearby,std::uint32_t location,float maximum,float& impact,std::optional<std::uint32_t> port_stack_vector_lane=std::nullopt);
};
}

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class CarCollisionPredict {
public:
    // ELF 0x001E6010; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001e6010.md.
    // Original Debug reference: CarCollisionPredict::IsThisCarStoppedDirectlyInFrontOfMe(NearbyCar *, NearbyCarList *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void IsThisCarStoppedDirectlyInFrontOfMe();
};
} // namespace hp2::recovered
