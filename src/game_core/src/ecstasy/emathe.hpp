#pragma once
// Port-side declarations for original emathe.cpp shared-Memory leaves.
#include "game_core/port/driving/memory.hpp"
namespace hp2::debug {
void Sub_0011C2E8(::port::driving::Memory&); // E-0099-W50, original eUnitVector4 static initializer.
void eIdentity(::port::driving::Memory&,std::uint32_t matrix); //0011B558.
void eMathInit(::port::driving::Memory&); //0011B4E0; distinct original identity/zero globals.
// Explicit native containers for original vector/matrix operands; no PS2 ABI.
void eMulVector(::port::driving::Memory&,std::array<float,4>&,const std::array<float,16>&,const std::array<float,4>&); //0011B6A8 vector3, actual eUnitVector4.W.
void eMulVector(std::array<float,4>&,const std::array<float,16>&,const std::array<float,4>&); //0011B668 vector4.
void eTransposeMatrix(std::array<float,16>&,const std::array<float,16>&); //0011BEA0.
void eInvertTransformationMatrix(std::array<float,16>&,const std::array<float,16>&); //0011BF28.
void eCreateLookAtMatrix(std::array<float,16>&,const std::array<float,4>& eye,const std::array<float,4>& center,const std::array<float,4>& up); //0011B8C8.
}
