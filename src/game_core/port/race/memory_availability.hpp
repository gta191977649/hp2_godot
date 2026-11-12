#pragma once
// PC-only admission policy for the logical Memory allocator. Not PS2 mallinfo,
// allocator overhead, fragmentation, or a promise that host allocation succeeds.
#include "game_core/port/driving/memory.hpp"
#include "replay_memory.hpp"
#include <cstddef>
namespace port::race {
class MemoryAvailability {
public:
    // Capacity includes already charged owned payload; imported image regions
    // are excluded. The caller owns the choice of this actual PC budget.
    MemoryAvailability(driving::Memory&,std::size_t capacity);
    // Windows available physical/commit snapshot, bounded by signed return and
    // logical-address limits. Other platforms require an explicit capacity.
    static MemoryAvailability from_host(driving::Memory&);
    MemoryAvailability(const MemoryAvailability&)=delete;
    MemoryAvailability& operator=(const MemoryAvailability&)=delete;
    std::size_t capacity()const;
    std::size_t charged()const;
    int free_payload()const;
    int largest_admitted(int alignment_mask)const;
    // The returned callback borrows this object; it must outlive the consumer.
    ReplayMemoryServices replay_services();
private:
    driving::Memory& memory_;
};
}
