#pragma once
// Port-owned original-address -> native whole-callee dispatch and clock binding.
// TASK-0099 E-0099-W29. No synthetic event generation or default handler/ticker.
#include "game_core/port/driving/memory.hpp"
#include <functional>
#include <map>
namespace port::race {
struct EventDispatch {
 using Handler=std::function<void(::port::driving::Memory&,std::uint32_t event)>;
 std::map<std::uint32_t,Handler> handlers;
 // Original bGetTicker0024A340 raw uint32 Count units. Debug frequency scalar
 //00326254 is300000000 ticks/sec; differences wrap unsigned32 then convert to ms.
 std::function<std::uint32_t()> ticker;
};
struct EventResources {
 ::port::driving::Memory* owner=nullptr;
 std::vector<std::uint32_t> allocations,chunks,packs;
 bool loaded=false;
};
// Explicit reviewed Retail version1 resource -> Debug loader/consumer binding.
EventResources load_retail_event_resources(::port::driving::Memory&,const std::vector<std::uint8_t>&);
void unload_event_resources(::port::driving::Memory&,EventResources&);
}
namespace hp2::debug {
// Debug NFSHP2.ELF815b...; E-0099-W29; original ../indep/src/eventmanager.cpp.
void Sub_001B7DE0(::port::driving::Memory&); // Original static list constructors.
void Sub_001B7DA0(::port::driving::Memory&); // Original static list destructors.
void emEventManagerInit(::port::driving::Memory&); //001B7490
void emEventManagerClose(::port::driving::Memory&); //001B74E0; globals stay untouched.
int LoaderEventManager(::port::driving::Memory&,std::uint32_t chunk); //001B7518
int UnloaderEventManager(::port::driving::Memory&,std::uint32_t chunk); //001B7660
int emAddHandler(::port::driving::Memory&,std::uint32_t original_function,std::uint32_t stream_mask); //001B76E8
void emRemoveHandler(::port::driving::Memory&,std::uint32_t original_function); //001B77C0
std::uint32_t emAddEvent(::port::driving::Memory&,std::uint32_t id); //001B7848
void emRemoveEvent(::port::driving::Memory&,std::uint32_t event); //001B78D0
void emLockEvent(::port::driving::Memory&,std::uint32_t event); //001B7918
void emUnlockEvent(::port::driving::Memory&,std::uint32_t event); //001B7928
void emProcessAllEvents(::port::driving::Memory&,const ::port::race::EventDispatch&); //001B7938
//001B7B10. Null or actual original TriggerEventArray of at most40 events +null.
std::uint32_t emTriggerEvents(::port::driving::Memory&,const std::array<float,3>&);
void CarReflectEventHandler(::port::driving::Memory&,std::uint32_t); //00130B70 E-0099-W64; actual nullable Car dispatch.
}
