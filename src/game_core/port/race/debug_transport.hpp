#pragma once
// Port platform binding for original optional PS2 development-monitor calls.
// TASK-0099 E-0099-W30. This never creates a network connection itself.
#include "game_core/port/driving/memory.hpp"
#include <functional>
namespace port::race {
using MonitorSyscall=std::function<void(std::uint32_t,std::uint64_t,std::uint64_t)>;
}
namespace hp2::debug {
void bSendPacket(::port::driving::Memory&,std::uint32_t,int,std::uint32_t,int,const ::port::race::MonitorSyscall& = {}); //00256350
void bFunkCallASync(::port::driving::Memory&,std::uint32_t,std::uint8_t,std::uint32_t,int,const ::port::race::MonitorSyscall& = {}); //002563F0
void bRefreshTweaker(::port::driving::Memory&); //00256670, E-0099-W120.
void bfSendString(::port::driving::Memory&,std::uint32_t); //00254378
void espSetAttributeInt(::port::driving::Memory&,std::uint32_t,std::uint32_t,int,const ::port::race::MonitorSyscall& = {}); //00255950
std::uint32_t bAddCodeineInventory(::port::driving::Memory&,std::uint32_t,std::uint32_t,const ::port::race::MonitorSyscall& = {}); //00256740
std::uint32_t bAddCodeineInventory(::port::driving::Memory&,std::uint32_t,std::uint32_t,std::uint32_t,const ::port::race::MonitorSyscall& = {}); //00256768
void bRemoveCodeineInventory(::port::driving::Memory&,std::int32_t,const ::port::race::MonitorSyscall& = {}); //00256838
}
