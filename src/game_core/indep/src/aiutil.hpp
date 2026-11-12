// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once
#include "game_core/port/driving/object_view.hpp"
#include "game_core/port/track/route_data.hpp"
namespace hp2::debug {
// Debug001C7D98; E-0100-CM01, original aiutil.cpp.
int GetNumWheelsOnGround(const ::port::driving::Memory&,std::uint32_t car);
class DriveTargetRenderer:public ::port::driving::ObjectView {public:using ObjectView::ObjectView;void SetTargetToRender(std::uint32_t target);void DestoryTargetMarkers();}; //Debug001C8618/W47;001C85B0/W89.
float GetCurrentCurvatureOfDrivingLine(::port::driving::Memory&,std::uint32_t location); //001C9B90,W95.
float CalculateMaxDeceleration(::port::driving::Memory&,std::uint32_t car,float speed); //001C9F48,E-0110-F11.
float CalculateMaxAcceleration(::port::driving::Memory&,std::uint32_t car,float speed); //Debug001C9E20, E-0099-W47
bool CanICatchThisCar(::port::driving::Memory&,std::uint32_t nearby,std::uint32_t car,float maximum,bool debug_print); //Debug001C9FA8
// Debug001C7D78/001C7F88; E-0099-W37, original aiutil.cpp.
float CalculateLookAheadDistance(const ::port::driving::Memory&,std::uint32_t car);
bool IsPointAheadOfCar(const ::port::driving::Memory&,const std::array<float,2>&,std::uint32_t car,float cosine);
void HighlightDrivingLine(::port::driving::Memory&,std::uint32_t location,int driver); //Debug001C8468
bool DoWeCareAboutThisCar(const ::port::driving::Memory&,std::uint32_t nearby_car,std::uint32_t nearby_cars); //Debug001C9DB8
void StraightLinePredict(::port::driving::Memory&,std::uint32_t car,std::uint32_t target,std::uint32_t port_stack_vector_lane); //Debug001CA4A8
// Debug001CA118/001CA1A8, E-0099-W47. XYZ-only projection: original pad is
// unknown and unconsumed by PsychoDriver.CalcSteerToPoint, which stores XY only.
std::array<float,3> SteerStraightVector3D(::port::driving::Memory&,std::uint32_t car,float distance);
std::array<float,3> SteerStraightVelocity3D(::port::driving::Memory&,std::uint32_t car,float distance);
// E-0099-W17. Semantic logical-pointer carrier; host ABI is not original.
struct NearestRoutePointInfo {static void DoSnapshot(::port::driving::Memory&,std::uint32_t,std::uint32_t); /*001CA430,W74*/ std::uint32_t Route=0,Point=0;bool IsValid(const ::port::driving::Memory&)const;}; //Debug001CA3D8, W56.
unsigned GuessTargetDestinations(::port::driving::Memory&,NearestRoutePointInfo*,unsigned,std::uint32_t,std::uint32_t,float,bool); //001C8B88,E-0110-T01.
void GetNextRoutePoint(::port::driving::Memory&,NearestRoutePointInfo&); //001C8978, E-0099-W60.
void GetNextRoutePointAhead(::port::driving::Memory&,NearestRoutePointInfo&); //001C8A88.
enum RoutesToSearch {ALL_RACING_ROUTES=0,ALL_TRAFFIC_ROUTES=1,FORWARD_TRAFFIC_ROUTES=2,ONCOMING_TRAFFIC_ROUTES=3};
// Debug001C87B8, E-0099-W17. Output is untouched when no route is selected.
bool GetNearestRoutePoint(::port::driving::Memory&,const ::port::track::RouteData&,NearestRoutePointInfo&,
                         const std::array<float,2>&,RoutesToSearch,int search_flags,float dat);
}

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class NearestRoutePointInfo {
public:
    // ELF 0x001CD408; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001cd408.md.
    // Original Debug reference: NearestRoutePointInfo::IsValid(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void IsValid();
};
} // namespace hp2::recovered

namespace hp2::debug {
// Debug001C8208 E-0099-W61; raw source polygon, finite ordered comparison domain.
std::int64_t IsPointInPoly(::port::driving::Memory&,const std::array<float,2>& point,std::uint32_t points,std::int64_t count);
}

namespace hp2::debug {
// Original001CA210; W62. Player driving-style getter001CA6F8 is emitted here.
void SetAIDriverModesFromPlayers(::port::driving::Memory&);
}
