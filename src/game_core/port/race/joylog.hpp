#pragma once
// Port declarations of original static Joylog operations; no native PS2 ABI.
#include "game_core/port/race/debug_transport.hpp"
namespace hp2::debug {
class Joylog {
public:
 static void Init(::port::driving::Memory&); //00181AB0, E-0099-W49.
 static std::uint32_t GetData(::port::driving::Memory&,int bits,int channel,const ::port::race::MonitorSyscall& ={}); //001818A0.
 static void AddData(::port::driving::Memory&,std::int32_t,int bits,int channel,const ::port::race::MonitorSyscall& ={}); //001819E0.
};
}
