#pragma once
// Port companion declaration; original conversions.cpp definition proven by
// Debug ECOFF FDR149/SYMR0x83B720, E-0100-PA01. No original ABI claim.
#include "game_core/port/driving/memory.hpp"
namespace hp2::debug {class Speech {public:
 static std::int32_t ConvertCarTypeToDescription(::port::driving::Memory&,int);
 static int ConvertCarTypeToUnlockSpeech(const ::port::driving::Memory&,int); //0021DBF0,E-0112-R08.
 static int ConvertEventNumber1To20(const ::port::driving::Memory&,int); //0021DC20.
 static int ConvertEventNumber21ToFinal(const ::port::driving::Memory&,int); //0021DC40.
 static int ConvertTrackNumberToUnlockSpeech(int); //0021DD08.
};}
