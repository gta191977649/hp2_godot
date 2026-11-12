// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/src/bMemory.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x002572D8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/002572d8.md.
void MemoryPool::GetAmountFree() {
    hp2::not_implemented("0x002572D8", "MemoryPool::GetAmountFree (signature and full body unresolved)");
}

// ELF 0x00257310; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/00257310.md.
void MemoryPool::GetLargestFreeBlock() {
    hp2::not_implemented("0x00257310", "MemoryPool::GetLargestFreeBlock (signature and full body unresolved)");
}
} // namespace hp2::recovered

// E-0099-W62, exact source src/bMemory.cpp. Raw original pool statistics only;
// PC admission is separately owned by port::race::MemoryAvailability.
#include <cstring>
namespace hp2::debug {
std::int32_t MemoryPool::GetAmountFree()const{
 std::uint32_t total=0;auto node=m_.word(address_);
 if(!node)hp2::not_implemented("0x002506A8","original MemoryPool free-list startup has not run");
 for(;node!=address_;node=m_.word(node))total+=m_.word(node+8);
 std::int32_t result;std::memcpy(&result,&total,4);return result;
}
std::int32_t MemoryPool::GetLargestFreeBlock()const{
 std::int32_t largest=0;auto node=m_.word(address_);
 if(!node)hp2::not_implemented("0x002506E0","original MemoryPool free-list startup has not run");
 for(;node!=address_;node=m_.word(node)){const auto size=m_.signed_word(node+8);if(largest<size)largest=size;}return largest;
}
std::int32_t bCountFreeMemory(::port::driving::Memory& m,int mask){return MemoryPool(m,0x329048+8*(mask&7)).GetAmountFree();}
std::int32_t bLargestMalloc(::port::driving::Memory& m,int mask){
 const auto largest=MemoryPool(m,0x329048+8*(mask&7)).GetLargestFreeBlock();
 const int alignment=(mask&0x20)?64:((mask&0x10)?32:4);
 // Original SUBU wraps before signed comparison.
 std::uint32_t bits=static_cast<std::uint32_t>(largest)-8u-static_cast<unsigned>(alignment-4);
 std::int32_t value;std::memcpy(&value,&bits,4);return value>15?value:0;
}
}
