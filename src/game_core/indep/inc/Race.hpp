// TASK-0042 E-0042-S001: exact Debug source marker(s): ../indep/inc/Race.hpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class TimeAroundTrackTable {
public:
    // No member address is confirmed; no speculative members are declared.

public:
    // TASK-0028: missing entries; existing storage and bodies are preserved.
    // ELF 0x00146AE8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00146ae8.md.
    // Original Debug reference: TimeAroundTrackTable::GetTimeTableEntry(float, float *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetTimeTableEntry();
};
} // namespace hp2::recovered

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class RacingCar {
public:
    // ELF 0x001493B8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001493b8.md.
    // Original Debug reference: RacingCar::RacingCar(void); constructor/destructor entry is not a native C++ lifetime operation.
    // Port-side void/no-argument placeholder; original signature unknown.
    void Sub_001493B8();
};
} // namespace hp2::recovered

#include "game_core/port/driving/object_view.hpp"
#include "game_core/port/track/route_data.hpp"
#include <string>
namespace port::race {struct PhysicsConstructionServices;}
namespace port::race {struct FinishCarPointer {std::uint32_t value;};}
namespace hp2::debug {
struct Timer;
class TimeAroundTrackTable:public ::port::driving::ObjectView {
public:using ObjectView::ObjectView;
 void Sub_00146590(int car_class,const ::port::track::RouteData&); //E-0099-W56.
 void Sub_00149340(int flags);
 std::uint32_t GetTimeTableEntry(float lap_number,float* seconds=nullptr); //001468D8
 float GetTimeAroundTrack(float lap_number); //00146980
};
struct BoundaryCrossingInfo {int CrossedForwards=0,CrossedBackwards=0;std::int32_t TimeSinceCrossing=0;}; // Port decoded carrier, not original ABI.
class RacingCar : public ::port::driving::ObjectView {
public:
 using ObjectView::ObjectView;
 void DoSnapshot(std::uint32_t snapshot); //E-0099-W74, original entry documented at definition.

 // Debug00149230,001469A0,00146A78; E-0099-W34. Explicit lifetime/Init.
 void Sub_00149230();void Init(std::uint32_t car);void UnInit();
 // Debug00146C48,00146DC0,001473C0; E-0099-W34.
 int CheckBoundaryCrossing(BoundaryCrossingInfo&,float distance,float boundary);
 void UpdateLapCounts();std::uint32_t GetPlayer()const;
 std::uint32_t GetLastFinishLineCrossingTime()const; //00147360, E-0099-W70.
 std::uint32_t GetLastLapTime()const; //00147378.
 std::uint32_t GetBestLapTime()const; //00147390.
 void ExtrapolateRaceTime(int laps,std::uint32_t start_time); //001471E8.
 void FakeStartlineCrossing(); //00146BF8, E-0099-W56.
};
class Race : public ::port::driving::ObjectView {
public:
 using ObjectView::ObjectView;
 void DoSnapshot(std::uint32_t snapshot); //E-0099-W74, original entry documented at definition.

 // Debug001473F0/001474E8/00147800/00147928; E-0099-W34.
 void Sub_001473F0(const ::port::track::RouteData&);
 bool CanAICarsStartDriving()const; //Debug00148E50, E-0099-W47.
 void Sub_001474E8(int in_chrg);
 int AddBoundaryPosition(const std::string&,std::array<float,2>&,float,const ::port::track::RouteData&);
 void FigureOutBoundaries(const ::port::track::RouteData&);
 bool GracePeriodIsActive(std::uint32_t car)const; // Debug00149088, E-0099-W37.
 // Debug00147B60/00147C90, E-0099-W34.
 float GetLapNumberExact(float percent_distance);
 void AddRacingCar(std::uint32_t car);
 void AddRacingCars(); //00147BC8, E-0099-W56.
 void SortRacingCarsByLeadership(); //00148020.
 void SetRaceStage(int stage,::port::race::PhysicsConstructionServices&); //00147CF8.
 void Init(int stage,const ::port::track::RouteData&,::port::race::PhysicsConstructionServices&); //00147768.
 void SetupGhostCars(); //001483E8, genuine original empty body.
 void Update(::port::race::PhysicsConstructionServices&,const ::port::track::RouteData&); //00148850, E-0099-W70.
 void UpdateGhostCar(); //00148718, constructor-null ghost component.
 void MaybeSaveGhostCarData(std::uint32_t racing_car); //001482F8.
 void MonitorLeadership(); //00148128.
 void CalculateSplitTime(Timer&,std::uint32_t first,std::uint32_t second); //001485C8,E-0110-P01.
 int GetNextFinishRank()const; //00148FE0.
 std::uint32_t GetFinisher(int rank)const; //00148F98.
 void FinishCar(::port::race::FinishCarPointer,int rank,bool blink,int reason,const ::port::track::RouteData&); //00149028,E-0112-COP03.
 void FinishCar(int index,int rank,bool blink,int reason,const ::port::track::RouteData&); //001483F8.
 // Debug00148D40/00148DD0; E-0110-F01. Original post-finish finalization;
 // SimulateRaceFinish is the recovered name, not a host synthetic outcome.
 void SimulateRaceFinish(const ::port::track::RouteData&);
 void NotifyTheRaceIsOver();
 std::uint32_t GetRacingCar(std::uint32_t car)const; //00148F30,E-0110-F01.
 int GetCountdownNumberToDisplay()const; //00147EC8, E-0099-W62.
 float GetCountdownProgress()const; //00147F90.
 int AreAllPlayersFinishedRacing()const; //00148E88.
 std::uint32_t GetMostRecentlyFinishedPlayer()const; //00148ED0.
};
}
