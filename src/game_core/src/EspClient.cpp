// Original source src\EspClient.cpp, freshly recovered ECOFF owner; E-0099-W30.
#include "game_core/port/race/debug_transport.hpp"
#include <stdexcept>
namespace hp2::debug {
// Debug00254378. String padding is skipped, never filled; null is the FF word.
void bfSendString(::port::driving::Memory& m,std::uint32_t string){
 const auto current=m.word(0x329068);
 if(!string){m.word(current,0xffffffffu);m.word(0x329068,current+4);return;}
 unsigned i=0;for(;;++i){if(i==4096)throw std::out_of_range("Unterminated Espresso string");const auto c=m.byte(string+i);m.byte(current+i,c);if(!c)break;}
 m.word(0x329068,current+((i+4u)&0xfffffffcu));
}
// Debug00255950. Even with inactive transport this packet/global work is real.
void espSetAttributeInt(::port::driving::Memory& m,std::uint32_t object,std::uint32_t name,int value,const ::port::race::MonitorSyscall& call){
 m.word(0x329064,0x317f40);m.word(0x32906c,0x318340);m.word(0x329068,0x317f44);m.word(0x317f40,object);
 bfSendString(m,name);const auto current=m.word(0x329068);m.word(current,static_cast<std::uint32_t>(value));m.word(0x329068,current+4);
 if(current+4>m.word(0x32906c))throw std::out_of_range("Original Espresso packet buffer assertion");
 bFunkCallASync(m,0x2b8288,0x26,m.word(0x329064),static_cast<int>(m.word(0x329068)-m.word(0x329064)),call);
}
}
