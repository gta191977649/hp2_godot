// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/src/bList.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x002503F8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/002503f8.md.
void bList::GetNode() {
    hp2::not_implemented("0x002503F8", "bList::GetNode (signature and full body unresolved)");
}

// ELF 0x00250428; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/containers/00250428.md.
void bList::TraversebList() {
    hp2::not_implemented("0x00250428", "bList::TraversebList (signature and full body unresolved)");
}

// ELF 0x00250458; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/containers/00250458.md.
void bList::AddTail() {
    hp2::not_implemented("0x00250458", "bList::AddTail (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include "game_core/inc/bSlotPool.hpp"
namespace hp2::debug {
// Debug00249A28, E-0099-W34. Original lazy pool initialization and12-byte nodes.
void bPListInit(::port::driving::Memory& m,int count){if(!m.word(0x328f1c))m.word(0x328f1c,bNewSlotPool(m,12,count,0x2b7020,16));}
// Debug00249A60, E-0099-W34.
void bPListClose(::port::driving::Memory& m){if(const auto pool=m.word(0x328f1c)){bDeleteSlotPool(m,pool);m.word(0x328f1c,0);}}
// Debug00249A88, E-0099-W34. Original default expected capacity32.
std::uint32_t bPNode::Malloc(::port::driving::Memory& m){if(!m.word(0x328f1c))bPListInit(m,32);return bOMalloc(m,m.word(0x328f1c));}
// Debug00249AB8, E-0099-W34. Original null-pool trap is an explicit invalid call.
void bPNode::Free(::port::driving::Memory& m,std::uint32_t p){if(!m.word(0x328f1c))hp2::not_implemented("0x00249AB8","original bPNode::Free null-pool trap");SlotPool(m,m.word(0x328f1c)).Free(p);}
}
#include "game_core/indep/inc/Contact.hpp"
namespace hp2::debug {
// Debug002498C0, E-0099-W38. Preserve comparator(node,current) and stable equal
// ordering. ContactTimeSorting <= advances, yielding descending BackupTime.
std::uint32_t bList::AddSorted(std::uint32_t comparator,std::uint32_t node){
 auto current=m_.word(address_);while(current!=address_){int advance;
  switch(comparator){case 0x1d13a0:advance=Contact::ContactTimeSorting(m_,node,current);break;
   default:hp2::not_implemented("0x002498C0","Unrecovered bList AddSorted comparator target");}
  if(!advance)break;current=m_.word(current);
 }
 const auto previous=m_.word(current+4);m_.word(previous,node);m_.word(current+4,node);m_.word(node+4,previous);m_.word(node,current);return node;
}
}
