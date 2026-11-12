#include "memory_availability.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
namespace port::race {
MemoryAvailability::MemoryAvailability(driving::Memory& memory,std::size_t capacity):memory_(memory){
    if(capacity>std::size_t(std::numeric_limits<int>::max()))
        throw std::invalid_argument("PC payload budget exceeds signed memory-query range");
    memory_.set_payload_budget(capacity);
}
MemoryAvailability MemoryAvailability::from_host(driving::Memory& memory){
#if defined(_WIN32)
    MEMORYSTATUSEX status{};status.dwLength=sizeof(status);
    if(!GlobalMemoryStatusEx(&status))throw std::runtime_error("GlobalMemoryStatusEx failed");
    const auto charged=memory.charged_payload_bytes();
    const auto limit=std::size_t(std::numeric_limits<int>::max());
    if(charged>limit)throw std::overflow_error("Existing PC payload exceeds signed memory-query range");
    // Query logical capacity without changing the caller's current policy.
    // Existing admission may be tighter; retaining that bound is intentional.
    auto remaining=std::min<std::uint64_t>(status.ullAvailPhys,status.ullAvailPageFile);
    remaining=std::min<std::uint64_t>(remaining,limit-charged);
    remaining=std::min<std::uint64_t>(remaining,memory.largest_admitted_payload());
    return MemoryAvailability(memory,charged+static_cast<std::size_t>(remaining));
#else
    (void)memory;
    throw std::runtime_error("Host available-memory provider is required on this platform");
#endif
}
std::size_t MemoryAvailability::capacity()const{
    const auto value=memory_.payload_budget();
    if(!value)throw std::logic_error("PC payload admission policy was disabled");
    return *value;
}
std::size_t MemoryAvailability::charged()const{return memory_.charged_payload_bytes();}
int MemoryAvailability::free_payload()const{
    const auto free=memory_.free_payload_bytes();
    if(!free||*free>std::size_t(std::numeric_limits<int>::max()))
        throw std::logic_error("PC payload admission policy was removed or enlarged");
    return static_cast<int>(*free);
}
int MemoryAvailability::largest_admitted(int mask)const{
    (void)capacity();
    const auto alignment=(mask&0x20)?64u:((mask&0x10)?32u:4u);
    const auto value=memory_.largest_admitted_payload(alignment);
    if(value>std::size_t(std::numeric_limits<int>::max()))
        throw std::logic_error("PC payload admission policy exceeds signed query range");
    return static_cast<int>(value);
}
ReplayMemoryServices MemoryAvailability::replay_services(){return {[this](int){return free_payload();}};}
}
