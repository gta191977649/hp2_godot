// TASK-0042 E-0042-S001: exact Debug source marker(s): inc/bSlotPool.hpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// PORT-SIDE template file; destination/header extension is not original-file evidence.
#pragma once
#include "game_core/port/driving/memory.hpp"

namespace hp2::debug {
// E-0099-W05. Debug SlotPool memory view; independent of Retail placeholders.
enum SlotPoolFlags : std::uint32_t {
    SLOTPOOL_FLAG_OVERFLOW_IF_FULL=1,SLOTPOOL_FLAG_ZERO_ALLOCATED_MEMORY=2,
    SLOTPOOL_FLAG_WARN_IF_OVERFLOW=4,SLOTPOOL_FLAG_WARN_IF_NONEMPTY_DELETE=8
};
class SlotPool {
    ::port::driving::Memory& memory_;std::uint32_t address_;
public:
    SlotPool(::port::driving::Memory& memory,std::uint32_t address):memory_(memory),address_(address){}
    // Debug00251110; E-0099-W05. The low-level pool does not round SlotSize.
    static std::uint32_t NewSlotPool(::port::driving::Memory&,int,int,std::uint32_t,int);
    // Debug002511C8/00251238/002512F8; E-0099-W05.
    void FlushSlotPool();void ExpandSlotPool(int);void CleanupExpandedSlotPools();
    // Debug00251500/002515D0/00251608; E-0099-W05.
    std::uint32_t Malloc();std::uint32_t FastMalloc();void Free(std::uint32_t);
};
// Debug002517F0; E-0099-W05. Explicit original static initialization, not automatic
// initialization inside a consumer such as World or TrackGraph.
void Sub_002517F0(::port::driving::Memory&,std::uint32_t manager=0x316e48);
// Debug00250FE0/00251880 and00251018/00251918; E-0099-W05.
std::uint32_t bNewSlotPool(::port::driving::Memory&,int,int,std::uint32_t,int);
void bDeleteSlotPool(::port::driving::Memory&,std::uint32_t);
// Debug00251050; E-0099-W05. Malloc (including zero/overflow), not FastMalloc.
std::uint32_t bOMalloc(::port::driving::Memory&,std::uint32_t);
// Debug002510F8, E-0099-W57. Original32-bit total-minus-allocated result.
std::int32_t bCountFreeSlots(const ::port::driving::Memory&,std::uint32_t);
// Debug002510E0, E-0099-W83. Original exact equality, not >= or free-list state.
int bIsSlotPoolFull(const ::port::driving::Memory&,std::uint32_t);
}

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: qualified_string; evidence: E-0019-N03.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
// Diagnostic text (not a callable signature): SlotPool::CleanupExpandedSlotPools() [0x002C0450]
namespace hp2::recovered {
class SlotPool {
public:
    // ELF 0x00257F28; E-0019-N03; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/memory/00257f28.md.
    // Port-side void/no-argument placeholder; original signature unknown.
    void CleanupExpandedSlotPools();

public:
    // TASK-0028: missing entries; existing storage and bodies are preserved.
    // ELF 0x00257DF8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00257df8.md.
    // Original Debug reference: SlotPool::FlushSlotPool(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void FlushSlotPool();
    // ELF 0x00258200; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00258200.md.
    // Original Debug reference: SlotPool::FastMalloc(void); exact original method spelling; void/no-argument host gap interface only.
    // Port-side void/no-argument placeholder; original signature unknown.
    void FastMalloc();
};
} // namespace hp2::recovered
