// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
namespace hp2::recovered {
class Timer {
public:
    // ELF 0x001A3B28; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/frontend/001a3b28.md.
    // Original Debug reference: Timer::GetHoursMinsSeconds(int *, int *, int *, int *); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void GetHoursMinsSeconds();
    // ELF 0x001A3CF0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001a3cf0.md.
    // Original Debug reference: Timer::CountDown(float); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void CountDown();
};
} // namespace hp2::recovered

#include "game_core/port/driving/memory.hpp"
namespace port::driving::native {struct ScheduleServices;}
namespace hp2::recovered {
// TASK-0044 E-0044-P05. Global-context Memory is an explicit port binding.
float GetRealTimeElapsedFromQuantized(port::driving::Memory&,std::int32_t);
std::int32_t GetQuantizedRealTimeElapsed(float);
void PrepareRealTimestep(port::driving::Memory&,float,port::driving::native::ScheduleServices&);
void AdvanceRealTime(port::driving::Memory&);void ResetWorldTime(port::driving::Memory&);
void PrepareWorldTimestep(port::driving::Memory&,float);void AdvanceWorldTime(port::driving::Memory&);
}
