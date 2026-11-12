// TASK-0099 E-0099-W42. Original src/joysticke.cpp ownership confirmed by MCP FDR/SYMR.
#include "game_core/port/race/physics_construction.hpp"
namespace hp2::debug {
// Debug0011D720, E-0099-W42. Original wheel type bytes21/22/35 only.
bool IsJoystickTypeWheel(::port::driving::Memory& m,int port){const auto type=GetJoystickType(m,port);return type==21||type==22||type==35;}
}
