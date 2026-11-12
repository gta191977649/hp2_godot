// TASK-0099 E-0099-W49. Original indep/src/joylog.cpp corroborated by MCP FDR/SYMR.
#include "game_core/port/race/joylog.hpp"
#include "game_core/indep/src/brepeat.hpp"
#include "game_core/port/not_implemented.hpp"
namespace hp2::debug {
// Debug00181AB0, E-0099-W49.
void Joylog::Init(::port::driving::Memory& m){if(m.word(0x328f18)==1&&m.word(0x326f24))hp2::not_implemented("0x001E7A40","Joylog initialization requires the actual synchronous Codeine buffer query/config transport");}
// Debug001818A0, E-0099-W49.
std::uint32_t Joylog::GetData(::port::driving::Memory& m,int bits,int channel,const ::port::race::MonitorSyscall& monitor){const auto repeat=m.word(0x3278b4);if(!repeat)return 0;const auto data=bRepeat(m,repeat).RemoveData(bits,channel);if(m.word(repeat+0x28)){bRepeat(m,repeat).TellCodeineWeAreFinishedRemovingData(monitor);m.word(0x3278b4,0);m.word(0x3278ac,0);hp2::not_implemented("0x00249EB8","original bBreak trap after Joylog removal error");}else if(m.signed_word(repeat+0x20)<=m.signed_word(repeat+0x24)){bRepeat(m,repeat).TellCodeineWeAreFinishedRemovingData(monitor);m.word(0x3278b4,0);m.word(0x3278ac,0);}return std::uint32_t(data);}
// Debug001819E0, E-0099-W49.
void Joylog::AddData(::port::driving::Memory& m,std::int32_t data,int bits,int channel,const ::port::race::MonitorSyscall& monitor){const auto repeat=m.word(0x3278b4);if(!repeat||!m.word(0x3278b0))return;bRepeat(m,repeat).AddData(data,bits,channel,monitor);if(std::int32_t(m.word(repeat+0x14)*8-0x44)<=m.signed_word(repeat+8))m.word(0x3278b0,0);m.word(repeat+0x2c,m.word(0x327ce0));}
}
