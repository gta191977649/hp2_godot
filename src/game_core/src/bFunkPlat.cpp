// Original source src\bFunkPlat.cpp, freshly recovered ECOFF owner; E-0099-W30.
#include "game_core/port/race/debug_transport.hpp"
#include "game_core/port/not_implemented.hpp"
#include <stdexcept>
namespace hp2::debug {
// Debug00256670, E-0099-W120; original bFunkPlat.cpp definition. Preserve the
// existing platform/monitor boundary; no transport callback is fabricated.
void bRefreshTweaker(::port::driving::Memory& m){bFunkCallASync(m,0x329070,0x28,0,0);}
// Debug00256350. Monitor operations are required only on actual platform type1.
void bSendPacket(::port::driving::Memory& m,std::uint32_t header,int header_size,std::uint32_t data,int data_size,const ::port::race::MonitorSyscall& call){
 if(m.word(0x328f18)!=1)return;
 if(!call)hp2::not_implemented("0x00256350","PS2 development monitor syscall transport is not bound");
 call(0,0,0);
 call(2,(std::uint64_t(data)<<32)|header,(std::uint64_t(static_cast<std::uint32_t>(data_size))<<32)|static_cast<std::uint32_t>(header_size));
}
// Debug002563F0. Other platforms/oversize packets return before reading strings
// or building the original eight-byte monitor header. No assumed connection test.
void bFunkCallASync(::port::driving::Memory& m,std::uint32_t server,std::uint8_t function,std::uint32_t data,int size,const ::port::race::MonitorSyscall& call){
 if(m.word(0x328f18)!=1 || size>=0x7e1)return;
 if(size<0)throw std::out_of_range("Negative original monitor packet size");
 std::uint32_t hash=0xffffffffu;unsigned i=0;
 for(;m.byte(server+i);++i){if(i==4096)throw std::out_of_range("Unterminated monitor server name");hash=hash*33+m.byte(server+i);}
 ::port::driving::Memory::Temporary storage(m,8);const auto header=storage.address();
 m.word(header,0);m.byte(header+1,function);m.half(header+2,static_cast<std::uint16_t>(size+8));m.word(header+4,hash);
 bSendPacket(m,header,8,data,size,call);
}
}
