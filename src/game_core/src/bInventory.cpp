// Original source src\bInventory.cpp, freshly recovered ECOFF owner; E-0099-W30.
#include "game_core/port/race/debug_transport.hpp"
#include <stdexcept>
#include <string>
namespace {
std::string inventory_string(::port::driving::Memory& m,std::uint32_t address){std::string s;for(unsigned i=0;;++i){if(i==4096)throw std::out_of_range("Unterminated Codeine inventory string");const auto c=m.byte(address+i);if(!c)return s;s.push_back(static_cast<char>(c));}}
std::uint32_t inventory_hash(const std::string& s,std::uint32_t seed){for(unsigned char c:s)seed=seed*33+c;return seed;}
}
namespace hp2::debug {
// Debug00256838, E-0099-W30. Original handle removal uses a four-byte payload.
void bRemoveCodeineInventory(::port::driving::Memory& m,std::int32_t handle,const ::port::race::MonitorSyscall& call){
 ::port::driving::Memory::Temporary storage(m,16);m.word(storage.address(),static_cast<std::uint32_t>(handle));
 bFunkCallASync(m,0x329080,'[',storage.address(),4,call);
}
// Debug00256740 ->00256768. Exact required caller forwarding, no fake handle.
std::uint32_t bAddCodeineInventory(::port::driving::Memory& m,std::uint32_t name,std::uint32_t columns,const ::port::race::MonitorSyscall& call){return bAddCodeineInventory(m,0,name,columns,call);}
// Debug00256768, E-0099-W30. Packet strings/limit and seeded comma hash follow
// loaded original constants329078/329080/329088, independently captured.
std::uint32_t bAddCodeineInventory(::port::driving::Memory& m,std::uint32_t handle,std::uint32_t name,std::uint32_t columns,const ::port::race::MonitorSyscall& call){
 const auto n=inventory_string(m,name),c=inventory_string(m,columns);
 if(n.size()+c.size()+5<1024){
  ::port::driving::Memory::Temporary storage(m,1024);const auto packet=storage.address();m.word(packet,handle);
  const auto text=n+'\t'+c;for(unsigned i=0;i<text.size();++i)m.byte(packet+4+i,static_cast<std::uint8_t>(text[i]));m.byte(packet+4+static_cast<unsigned>(text.size()),0);
  bFunkCallASync(m,0x329080,'Z',packet,static_cast<int>(text.size()+5),call);
 }
 return inventory_hash(n,handle?inventory_hash(inventory_string(m,0x329088),handle):0xffffffffu);
}
}
