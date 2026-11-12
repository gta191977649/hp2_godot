#pragma once
// Port companion declaration for original coputil.cpp, E-0099-W119.
#include "game_core/port/driving/memory.hpp"
namespace hp2::debug {float GetTopSpeed(::port::driving::Memory&,std::uint32_t car); /*0022B958*/}
namespace hp2::debug {bool CarInRecentCollision(::port::driving::Memory&,std::uint32_t car); /*0022B878,E-0110-P01*/}
namespace hp2::debug {std::uint32_t CarCausedRecentCollision(::port::driving::Memory&,std::uint32_t); /*0022B738,E-0100-PA01*/}
namespace hp2::debug {bool DidTheseCarsCollide(::port::driving::Memory&,std::uint32_t first,std::uint32_t second); /*0022B888,E-0100-PU03*/}
namespace hp2::debug {bool CanTeleport(::port::driving::Memory&,std::uint32_t car,const std::array<float,2>& destination); /*0022B610,E-0100-CT01*/}
