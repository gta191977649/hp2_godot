// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once
#include "game_core/port/driving/memory.hpp"
#include <functional>
#include <optional>
namespace port::race {
// Original route-manager lookup: nullopt means an actually absent route, not an unbound provider.
using RoutePointCount=std::function<std::optional<std::uint32_t>(std::uint32_t)>;
}
namespace hp2::debug {
class TrafficTeleporter {
public:
    TrafficTeleporter(::port::driving::Memory&,std::uint32_t,const ::port::race::RoutePointCount&);
    void FillInTotals(std::uint32_t array,std::uint32_t route_start,const ::port::race::RoutePointCount&);
    void SetThinkTime();
private:
    ::port::driving::Memory& memory_;
    std::uint32_t address_;
};
}

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): TrafficTeleporter::DoTimestep() [0x002BCDC0]
namespace hp2::recovered {
class TrafficTeleporter {
public:
    // No member address is confirmed; no speculative members are declared.
};
} // namespace hp2::recovered
