// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/indep/src/speech/nfsfsm.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x0020D8C0; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0020d8c0.md.
void nfsFsm::Sub_0020D8C0() {
    hp2::not_implemented("0x0020D8C0", "nfsFsm::Sub_0020D8C0 (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include "game_core/indep/src/Speech/actions.hpp"

namespace hp2::recovered {
// ELF 0x0020E438; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0020e438.md.
void rotorChatterAction_t::executeAction() {
    hp2::not_implemented("0x0020E438", "rotorChatterAction_t::executeAction (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x0020E3E8; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0020e3e8.md.
void rotorLocationAction_t::executeAction() {
    hp2::not_implemented("0x0020E3E8", "rotorLocationAction_t::executeAction (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x0020E3C8; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0020e3c8.md.
void rotorSpikeFailedAction_t::executeAction() {
    hp2::not_implemented("0x0020E3C8", "rotorSpikeFailedAction_t::executeAction (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {
// ELF 0x0020E410; E-0041-V01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0020e410.md.
void rotorTrafficAction_t::executeAction() {
    hp2::not_implemented("0x0020E410", "rotorTrafficAction_t::executeAction (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include "game_core/port/race/cop_speech.hpp"
#include "game_core/port/race/cop_speech_data.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
namespace port::race {
namespace {
std::uint64_t speech_count(const port::driving::Memory& m,std::uint32_t at){return m.word(at)|(std::uint64_t(m.word(at+4))<<32);}
void speech_count(port::driving::Memory& m,std::uint32_t at,std::uint64_t n){m.word(at,std::uint32_t(n));m.word(at+4,std::uint32_t(n>>32));}
// Debug00207130/00207248/002072D8, E-0099-W16. Actual fragmented-vector
// growth, including original array cookie and default transition vtable stores.
void speech_transition(port::driving::Memory& m,std::uint32_t object,const speech_data::Transition& t,
                       std::vector<std::uint32_t>& allocations){
 const auto state=object+t.from;auto size=speech_count(m,state+8),capacity=speech_count(m,state+0x10);
 if(size==capacity){const auto desired=std::max<std::uint64_t>(1,size*2),extra=desired-capacity;
  if(extra>std::uint64_t(std::numeric_limits<int>::max()-16)/20)throw std::overflow_error("Original speech transition allocation exceeds host domain");
  const auto node=m.allocate(0x18);allocations.push_back(node);
  const auto array=m.allocate(std::uint32_t(extra)*20+0x10);allocations.push_back(array);m.word(array,std::uint32_t(extra));
  const auto data=array+0x10;const auto tail=m.word(state+4);m.word(node,0);m.word(node+4,data);
  const auto first=tail?speech_count(m,tail+8)+speech_count(m,tail+0x10):0;
  speech_count(m,node+8,first);speech_count(m,node+0x10,extra);
  for(std::uint64_t i=0;i<extra;++i){const auto at=data+std::uint32_t(i)*20;m.word(at,0);m.word(at+4,0);m.word(at+0x10,0x2b30d8);}
  if(tail)m.word(tail,node);else m.word(state,node);m.word(state+4,node);speech_count(m,state+0x10,desired);
 }
 speech_count(m,state+8,size+1);
 // Debug00207288 index traversal, after original size increment.
 auto node=m.word(state);
 while(node){const auto first=speech_count(m,node+8),count=speech_count(m,node+0x10);
  if(first<=size&&size-first<count){const auto at=m.word(node+4)+std::uint32_t(size-first)*20;
   m.word(at,t.event);m.word(at+4,object+t.to);m.word(at+8,object+t.condition);m.word(at+0xc,object+t.action);return;}
  node=m.word(node);
 }
 throw std::logic_error("Original speech fragmented-vector index is not present");
}
}
// Debug00207630; E-0099-W16. Every captured constructor statement is covered:
//25 state constructors,93 virtual-field stores and92 ordered transitions.
// Conditions/actions are original memory objects; executing their still-missing
// event/audio paths is not claimed by this constructor projection.
void construct_speech_fsm(port::driving::Memory& m,std::uint32_t at,std::vector<std::uint32_t>& allocations){
 m.word(at,0);m.word(at+4,0);m.word(at+8,0x2b3048); // fsm00206DC0.
 m.word(at+8,0x2b3100);
 for(auto offset:speech_data::states){const auto state=at+offset;m.word(state,0);m.word(state+4,0);
  speech_count(m,state+8,0);speech_count(m,state+0x10,0);m.word(state+0x18,0x2b30a8);}
 for(const auto& store:speech_data::stores)m.word(at+store.offset,store.value);
 for(const auto& transition:speech_data::transitions)speech_transition(m,at,transition,allocations);
 m.word(at+4,at+0x10);
}
}
