// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\trafficteleporter.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): TrafficTeleporter::DoTimestep() [0x002BCDC0]
#include "game_core/TrafficTeleporter.hpp"

#include "game_core/port/not_implemented.hpp"
namespace hp2::debug {
// Debug0021CFF0, E-0099-W02. Actual route totals and original constructor-only minimum20.
TrafficTeleporter::TrafficTeleporter(::port::driving::Memory& memory,std::uint32_t address,
    const ::port::race::RoutePointCount& routes):memory_(memory),address_(address){
    if(!routes)hp2::not_implemented("0x0021CFF0","original loaded-route provider missing");
    FillInTotals(address_+8,50,routes);FillInTotals(address_+0x30,60,routes);SetThinkTime();
    if(memory_.scalar(address_)<20)memory_.scalar(address_,20);
    memory_.word(address_+4,0);
}
// Debug0021D778, E-0099-W02. Unsigned cumulative totals; missing routes preserve prior total.
void TrafficTeleporter::FillInTotals(std::uint32_t array,std::uint32_t start,const ::port::race::RoutePointCount& routes){
    if(!routes)hp2::not_implemented("0x0021D778","original loaded-route provider missing");
    std::uint32_t total=0;
    for(unsigned i=0;i<10;++i){const auto route=start+i;
        if(static_cast<std::int32_t>(route)>=0){const auto count=routes(route);if(count)total+=*count;}
        memory_.word(array+4*i,total);
    }
}
// Debug0021D700, E-0099-W02. Subsequent setter calls do not apply constructor's minimum.
void TrafficTeleporter::SetThinkTime(){memory_.scalar(address_,(1.f-memory_.scalar(0x311f78+0x10))*15.f+1.f);}
}
