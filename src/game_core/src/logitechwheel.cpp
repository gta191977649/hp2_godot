// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: exact Debug declaring owner with adopted Retail correspondence; evidence: E-0043-M01.
// Placement: E-0042-S001 exact Debug translation unit/definition marker; CPP partition follows that marker; companion header is port-side unless the header marker is recorded; no Retail ABI or layout claim.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/src/logitechwheel.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x0012B3B8; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0012b3b8.md.
void WheelDevice::Sub_0012B3B8() {
    hp2::not_implemented("0x0012B3B8", "WheelDevice::Sub_0012B3B8 (signature and full body unresolved)");
}

// ELF 0x0012BC68; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/0012bc68.md.
void WheelDevice::RecalculateDeviceNumber() {
    hp2::not_implemented("0x0012BC68", "WheelDevice::RecalculateDeviceNumber (signature and full body unresolved)");
}
} // namespace hp2::recovered

#include "game_core/port/driving/projections.hpp"
namespace hp2::debug {
// TASK-0099 E-0099-W66. Complete Debug0012B510 constructor. Sparse stores
// preserve original untouched fields/padding; no whole-struct zeroing or
// inferred connected device. Force record offsets corroborated by live types.
void Sub_0012B510(::port::driving::Memory& m,std::uint32_t address,int index){
 m.word(address,std::uint32_t(index));m.word(address+4,0xffffffff);m.word(address+8,0xffffffff);m.word(address+0x418,255);m.word(address+0x438,1);m.word(address+0x43c,0);
 for(unsigned i=0;i<8;++i)m.word(address+0x38+4*i,0);
 m.word(address+0x42c,1);m.word(address+0x430,0);m.word(address+0x440,0);
 struct Store{unsigned offset,value;bool byte;};
 const Store stores[]={
 {0x58,0x7,true}, {0x5c,0xffffffff,false}, {0x60,0x0,false}, {0x64,0x0,false},
 {0x68,0x0,false}, {0x6c,0xff,false}, {0x70,0xff,false}, {0x74,0xff,false},
 {0x78,0xff,false}, {0x94,0x8,true}, {0x98,0xffffffff,false}, {0x9c,0x0,false},
 {0xa0,0x0,false}, {0xa4,0x0,false}, {0xa8,0x0,false}, {0xac,0x0,false},
 {0xb0,0x0,false}, {0xb4,0x0,false}, {0xd0,0x3,true}, {0xd4,0x96,false},
 {0xd8,0x0,false}, {0xdc,0xff,false}, {0xe0,0x5a,false}, {0xe4,0x4b,false},
 {0xe8,0x0,false}, {0xec,0x0,false}, {0xf0,0x0,false}, {0xf4,0x0,false},
 {0xf8,0x4b,false}, {0xfc,0x0,false}, {0x10c,0x3,true}, {0x110,0xffffffff,false},
 {0x114,0x0,false}, {0x118,0x1,false}, {0x11c,0x5a,false}, {0x120,0x32,false},
 {0x124,0x0,false}, {0x128,0x0,false}, {0x12c,0x0,false}, {0x130,0x19,false},
 {0x134,0x0,false}, {0x138,0x19,false}, {0x148,0x5,true}, {0x14c,0xffffffff,false},
 {0x150,0x0,false}, {0x154,0x1,false}, {0x158,0x5a,false}, {0x15c,0x32,false},
 {0x160,0x0,false}, {0x164,0x0,false}, {0x168,0x0,false}, {0x16c,0x19,false},
 {0x170,0x0,false}, {0x174,0x19,false}, {0x1c0,0x2,true}, {0x1c4,0x12c,false},
 {0x1c8,0x0,false}, {0x1cc,0xa9,false}, {0x1d0,0x5a,false}, {0x1d4,0x14,false},
 {0x1d8,0x113,false}, {0x1dc,0x0,false}, {0x1e0,0x12,false}, {0x1e4,0x0,false},
 {0x1e8,0xd6,false}, {0x1ec,0x0,false}, {0x1fc,0x1,true}, {0x200,0x6a4,false},
 {0x204,0x0,false}, {0x208,0xff,false}, {0x210,0x5a,false}, {0x238,0x7,true},
 {0x23c,0xffffffff,false}, {0x240,0x0,false}, {0x244,0x0,false}, {0x248,0x0,false},
 {0x24c,0xff,false}, {0x250,0xff,false}, {0x254,0xff,false}, {0x258,0xff,false},
 {0x274,0x8,true}, {0x278,0xffffffff,false}, {0x27c,0x0,false}, {0x280,0x0,false},
 {0x284,0x0,false}, {0x288,0x0,false}, {0x28c,0x0,false}, {0x290,0x0,false},
 {0x294,0x0,false}, {0x2b0,0x3,true}, {0x2b4,0x0,false}, {0x2b8,0x0,false},
 {0x2bc,0x0,false}, {0x2c0,0x5a,false}, {0x2c4,0x4b,false}, {0x2c8,0x0,false},
 {0x2cc,0x0,false}, {0x2d0,0x0,false}, {0x2d4,0x0,false}, {0x2d8,0x4b,false},
 {0x2dc,0x0,false}, {0x2ec,0x3,true}, {0x2f0,0xffffffff,false}, {0x2f4,0x0,false},
 {0x2f8,0x0,false}, {0x2fc,0x5a,false}, {0x300,0x32,false}, {0x304,0x0,false},
 {0x308,0x0,false}, {0x30c,0x0,false}, {0x310,0x19,false}, {0x314,0x0,false},
 {0x318,0x19,false}, {0x328,0x5,true}, {0x32c,0xffffffff,false}, {0x330,0x0,false},
 {0x334,0x0,false}, {0x338,0x5a,false}, {0x33c,0x32,false}, {0x340,0x0,false},
 {0x344,0x0,false}, {0x348,0x0,false}, {0x34c,0x19,false}, {0x350,0x0,false},
 {0x354,0x19,false}, {0x3a0,0x2,true}, {0x3a4,0x12c,false}, {0x3a8,0x0,false},
 {0x3ac,0x0,false}, {0x3b0,0x5a,false}, {0x3b4,0x14,false}, {0x3b8,0x113,false},
 {0x3bc,0x0,false}, {0x3c0,0x12,false}, {0x3c4,0x0,false}, {0x3c8,0xd6,false},
 {0x3cc,0x0,false}
 };
 for(const auto& s:stores){if(s.byte)m.byte(address+s.offset,std::uint8_t(s.value));else m.word(address+s.offset,s.value);}
}
// Debug0012CA50 original process-static initializer, called once by owner.
void Sub_0012CA50(::port::driving::Memory& m){Sub_0012B510(m,0x2e6fc0,0);Sub_0012B510(m,0x2e7404,1);}
}
namespace hp2::debug {
// TASK-0099 E-0099-W66. Original 0x3c-byte force records. No bReset read.
void PortSetForceConditionData(::port::driving::Memory& m,int type,int port,float offset,float deadband,float negSat,float posSat,float negCoeff,float posCoeff){
 const int index=GetWheelId(m,port);if(index==2)return;
 if(type<0||type>=8)throw std::out_of_range("Original wheel force array has eight records");
 const auto record=0x2e6fc0+std::uint32_t(index)*0x444+std::uint32_t(type)*0x3c;
 const auto put=[&](unsigned off,float v){m.word(record+off,std::uint32_t(::port::driving::cvt_w_s(v)));};
 put(0x64,offset*255.f);put(0x68,deadband*255.f);
 if(type==0){put(0x6c,negSat*153.f);put(0x70,posSat*153.f);put(0x74,negCoeff*255.f);put(0x78,posCoeff*255.f);}
 else{put(0x74,negSat*153.f);put(0x78,posSat*153.f);put(0x6c,negCoeff*255.f);put(0x70,posCoeff*255.f);}
}
// Debug0012C540 reads eight FPRs and the ninth float at caller SP+0. The
// decompiler's in_stack_00000000 is fFadeLevel, not an unknown extra operand.
void PortSetForcePeriodicData(::port::driving::Memory& m,int type,int port,float magnitude,float direction,float period,float phase,float offset,float attackTime,float attackLevel,float fadeTime,float fadeLevel){
 const int index=GetWheelId(m,port);if(index==2)return;
 if(type<0||type>=8)throw std::out_of_range("Original wheel force array has eight records");
 const auto record=0x2e6fc0+std::uint32_t(index)*0x444+std::uint32_t(type)*0x3c;
 const float values[]={magnitude*127.5f,direction*255.f,period,phase,offset,attackTime,attackLevel*255.f,fadeTime,fadeLevel*255.f};
 for(unsigned i=0;i<9;++i)m.word(record+0x64+4*i,std::uint32_t(::port::driving::cvt_w_s(values[i])));
}
void PortPlayShake(::port::driving::Memory& m,int port,float magnitude){const int i=GetWheelId(m,port);if(i==2)return;const auto wheel=0x2e6fc0+std::uint32_t(i)*0x444;m.word(wheel+0x1cc,std::uint32_t(::port::driving::cvt_w_s(magnitude*255.f)));m.word(wheel+0x430,1);}
void PortSetOverallForceGain(::port::driving::Memory& m,int port,float magnitude){const int i=GetWheelId(m,port);if(i==2)return;const auto wheel=0x2e6fc0+std::uint32_t(i)*0x444;const auto value=std::uint32_t(::port::driving::cvt_w_s(magnitude*255.f));if(m.word(wheel+0x418)!=value)m.word(wheel+0x438,1);m.word(wheel+0x418,value);}
// TASK-0099 E-0099-W46. Original two-slot lookup;2 means no assigned wheel.
int GetWheelId(::port::driving::Memory& m,int port){for(int i=0;i<2;++i)if(m.signed_word(0x2e6fc4+std::uint32_t(i)*0x444)==port)return i;return 2;}
// Original pending force command fields. This queues source state; hardware
// ForceFeedbackTask consumption is separate and is not claimed as delivered.
void PortPlayCrash(::port::driving::Memory& m,int port,int duration,float magnitude,float offset){const auto i=GetWheelId(m,port);if(i==2)return;const auto wheel=0x2e6fc0+std::uint32_t(i)*0x444;m.word(wheel+0xd4,std::uint32_t(duration));m.word(wheel+0xdc,std::uint32_t(::port::driving::cvt_w_s(magnitude*255.f)));m.word(wheel+0xec,std::uint32_t(::port::driving::cvt_w_s(offset*255.f)));m.word(wheel+0x434,1);}
}

namespace hp2::debug {
// Debug0012C398; E-0112-P02. Listing proves actual envelope offsets0x6C/0x70.
// Queues source software force state; physical ForceFeedbackTask remains separate.
void PortStopForce(::port::driving::Memory& m,int port){const int index=GetWheelId(m,port);if(index!=2){const auto wheel=0x2e6fc0+std::uint32_t(index)*0x444;m.word(wheel+0x6c,0);m.word(wheel+0x70,0);}}
}
