// Debug0021DBC0,E-0100-PA01; original ..\indep\src\speech\conversions.cpp.
#include "conversions.hpp"
namespace hp2::debug {
std::int32_t Speech::ConvertCarTypeToDescription(::port::driving::Memory& m,int type){return type>=0&&type<0x43?m.signed_word(0x3138c0+4*unsigned(type)):0;}
}

#include "game_core/port/not_implemented.hpp"
namespace hp2::debug {
// Debug0021DBF0; E-0112-R08, imported source table3139D0, original0..66 gate.
int Speech::ConvertCarTypeToUnlockSpeech(const ::port::driving::Memory& m,int car){return car>=0&&car<0x43?m.signed_word(0x3139d0+4*unsigned(car)):0;}
// Debug0021DC20/0021DC40; E-0112-R08. Actual table reads; invalid source
// indices remain explicit unsupported memory domains rather than fake flags.
int Speech::ConvertEventNumber1To20(const ::port::driving::Memory& m,int event){if(event<1||event>20)hp2::not_implemented("0x0021DC20","original event1..20 speech index outside source table");return m.signed_word(0x313ae0+4*unsigned(event-1));}
int Speech::ConvertEventNumber21ToFinal(const ::port::driving::Memory& m,int event){if(event<21||event>33)hp2::not_implemented("0x0021DC40","original event21..final speech index outside source table");return m.signed_word(0x313b30+4*unsigned(event-21));}
// Debug0021DD08; E-0112-R08, exact original switch and speech enum values.
int Speech::ConvertTrackNumberToUnlockSpeech(int track){switch(track){case 11:case 14:return 32768;case 12:case 15:return 32;case 13:case 16:return 65536;case 21:case 24:return 8;case 22:case 25:return 16;case 23:case 26:return 2048;case 31:case 34:return 4096;case 32:case 35:return 8192;case 33:case 36:return 16384;case 41:case 44:return 256;case 42:case 45:return 2;case 43:case 46:return 131072;case 61:case 64:return 4;case 62:case 65:return 512;case 63:case 66:return 1024;default:return 0;}}
}
