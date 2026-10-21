// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\realdriver.cpp.
// TASK-0046 typed entries below use Debug NFSHP2.ELF SHA-256
// 815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
// Historical unrelated Retail gaps retain their own provenance below.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/indep/inc/RealDriver.hpp"

#include "game_core/port/not_implemented.hpp"
#include "game_core/indep/inc/Table.hpp"
#include "game_core/indep/src/carstate.hpp"
#include "game_core/port/driving/projections.hpp"
#include <cmath>
#include "game_core/port/driving/horn.hpp"

namespace port::driving {
// Debug0018B788/0018B7C8/002039C0; E-0082-H01. Explicit native-to-host
// activation projection; does not emulate guest voice allocation or SPU hardware.
void set_horn_activation(Memory& m,std::uint32_t sound,bool active){
 if(m.word(0x326f40)==0||m.word(0x327980)==0)return;
 m.word(m.word(sound+0x34),active?1u:0u);
}
// Debug0018B808; E-0082-H01.
bool horn_activation(const Memory& m,std::uint32_t sound){
 return m.word(0x326f40)!=0&&m.word(0x327980)!=0&&m.word(m.word(sound+0x34))!=0;
}
}

namespace hp2::recovered {
// Debug001FEE80; E-0046-C01. Listing is an unconditional return, no side effects.
void RealDriverAddNearbyDebugTopology(port::driving::Memory&,std::uint32_t){}
// Debug0013AB98; E-0046-C01. DriveTarget empty ctor/Clear path is inlined here;
// nonempty route clearing is not reached by the constructor and is not claimed.
void RealDriver::Sub_0013AB98(std::uint32_t car){
 for(std::uint32_t i=0;i<0x210;++i)m_.byte(address_+i,0);
 // Debug00216528 ->002167C0 empty target: LastIndexToCheckForDrivability=-1.
 m_.byte(address_+0x204,0xff);
 m_.half(address_+0x210,0);for(auto o:{0x214,0x218,0x21c,0x228})m_.word(address_+o,0);
 m_.word(address_+0x220,1);m_.word(address_+0x230,car);m_.word(address_+0x234,0xffffffffu);
 m_.word(address_+0x278,0);m_.word(address_+0x238,0);m_.word(address_+0x22c,0x2a6e60);ResetJoyValues();
 m_.word(address_+0x23c,1);m_.word(address_+0x220,0xffffffffu);
 for(auto o:{0x240,0x244,0x248,0x24c,0x27c,0x278})m_.word(address_+o,0);
}
// Debug0013C750; E-0046-C01. Exact Table ctor operands from listing.
void Sub_0013C750(port::driving::Memory& m){
 Table(m,0x2e8068).Sub_0018D810(0x2e7fd8,10,0,160);
 Table(m,0x2e8080).Sub_0018D810(0x2e8000,10,0,160);
 Table(m,0x2e8098).Sub_0018D810(0x2e8028,10,0,160);
 Table(m,0x2e80b0).Sub_0018D810(0x2e8050,6,0,1);
 Table(m,0x2e80f0).Sub_0018D810(0x2e80c8,9,0,256);
 Table(m,0x2e8120).Sub_0018D810(0x2e8108,5,0,1);
 Table(m,0x2e8150).Sub_0018D810(0x2e8138,6,0,1);
}
// Debug0013B198; E-0046-C01. CurrentGear is Car+3c8, not DesiredGear.
void RealDriver::SetManualTransmission(int type){
    if(type==1){m_.word(address_+0x24c,1);m_.word(address_+0x220,m_.word(m_.word(address_+0x230)+0x3c8));return;}
    m_.word(address_+0x220,0xffffffffu);m_.word(address_+0x24c,type==0?0:2);
}
// Debug0013B168; E-0046-C01. Checked RealDriver SetLowGear vtable delta0.
void RealDriver::DisableStartLineState(){m_.word(address_+0x23c,0);SetLowGear();}
// Debug0013BC90; E-0046-C01. Original callback param is an event value, not a level poll.
void RealDriver::JoyHandler_ShiftUp(int,int,std::uint8_t param){
 if(!param)return;
 const auto car=m_.word(address_+0x230);const auto mode=m_.word(address_+0x24c);const auto gear=m_.signed_word(address_+0x220);
 const auto top=m_.signed_word(m_.word(car+0xc)+0x288)+1;
 if(mode==1){if(gear!=0&&gear<top)m_.word(address_+0x220,gear+1);}
 else if(mode==2){if(m_.word(car+0x24)!=4){if(gear==-2)m_.word(address_+0x224,m_.word(address_+0x224)+1);else{m_.word(address_+0x220,0xfffffffeu);const auto current=m_.signed_word(car+0x3c8);m_.word(address_+0x224,current<top?current+1:current);}}}
 else if(m_.word(car+0x18))hp2::not_implemented("0x0013BC90","Player::RerouteJoyEvent dependency is not bound");
}
// Debug0013BD70; E-0046-C01. Manual downshift stops at NEUTRAL1.
void RealDriver::JoyHandler_ShiftDown(int,int,std::uint8_t param){
 if(!param)return;
 const auto car=m_.word(address_+0x230);const auto mode=m_.word(address_+0x24c);const auto gear=m_.signed_word(address_+0x220);
 if(mode==1){if(gear!=0&&1<gear)m_.word(address_+0x220,gear-1);}
 else if(mode==2){if(m_.word(car+0x24)!=4){if(gear==-2)m_.word(address_+0x224,m_.word(address_+0x224)-1);else{m_.word(address_+0x220,0xfffffffeu);const auto current=m_.signed_word(car+0x3c8);m_.word(address_+0x224,current<3?current:current-1);}}}
 else if(m_.word(car+0x18))hp2::not_implemented("0x0013BD70","Player::RerouteJoyEvent dependency is not bound");
}
// Debug0013B948; E-0046-C01. Constants are read from this Debug image.
void RealDriver::DoThrottleBrakeReverse(float t,port::driving::PlayerServices& services){
 const auto s=address_,car=m_.word(s+0x230);const float speed=m_.scalar(car+0x3c0);float throttle=0;
 const float timer=m_.scalar(s+0x244)-t;m_.scalar(s+0x244,timer);
 if(0<timer&&m_.scalar(0x324eac)<std::fabs(speed))m_.scalar(s+0x21c,1);
 else{m_.word(s+0x244,0);m_.word(s+0x21c,m_.word(s+0x274));throttle=m_.scalar(s+0x25c);}
 float brake=m_.scalar(s+0x264),reverse;
 if(m_.scalar(0x324eb0)<brake)m_.word(s+0x27c,1);
 else if(m_.scalar(0x327188)<speed)m_.word(s+0x27c,0);
 if(m_.scalar(0x327180)<speed)reverse=0;
 else{reverse=brake;if(speed<m_.scalar(0x327184)||(m_.word(s+0x27c)!=0&&speed<m_.scalar(0x327188))){brake=throttle;throttle=0;}else brake=0;}
 reverse=reverse+reverse;if(1<reverse)reverse=1;m_.scalar(s+0x218,brake);
 if(.5f<m_.scalar(s+0x21c)&&((m_.word(0x327d10)+1)&7)==0&&10<m_.scalar(car+0x3c4)){
  if(!services.RegisterInterestingEvent)hp2::not_implemented("0x0013E070","Replay interesting-event service absent");
  services.RegisterInterestingEvent(m_,1,"EBrake");
 }
 if(throttle+m_.scalar(0x324eb4)<reverse){m_.scalar(s+0x214,reverse);m_.word(s+0x220,0);}
 else{m_.scalar(s+0x214,throttle);if(m_.word(s+0x220)==0)SetLowGear();}
 m_.word(car+0x3cc,m_.word(s+0x214));
}
// Debug0013B438; E-0046-C01. Keep original range, yaw, rate and bAngle conversion.
void RealDriver::DoSteering(float t,int port,port::driving::PlayerServices& services){
 const auto s=address_,car=m_.word(s+0x230),state=car+0x40;
 if(!services.IsJoystickTypeWheel)hp2::not_implemented("0x0011D720","Joystick type service absent");
 float angle=static_cast<float>(m_.half(s+0x210))*0.0054931640625f;if(180<=angle)angle=angle-360;
 float yaw=CarState(m_,state).GetYaw()*m_.scalar(0x324e98);const float abs_yaw=std::fabs(yaw);
 const auto v=m_.vector(state+0x30);float length=v[0]*v[0];length=length+v[1]*v[1];length=length+v[2]*v[2];length=std::sqrt(length);
 float range=Table(m_,services.IsJoystickTypeWheel(port)?0x2e8068:0x2e8080).GetValue(m_.scalar(car+0x3c0));
 if(m_.word(s+0x248)==0){if(7<abs_yaw&&m_.scalar(0x327178)<=length)m_.word(s+0x248,1);}
 else if(abs_yaw<5||length<m_.scalar(0x327178))m_.word(s+0x248,0);
 const float input=m_.scalar(s+0x254);
 if(yaw*input<0&&(m_.word(s+0x248)!=0||range<abs_yaw)){
  const float coefficient=services.IsJoystickTypeWheel(port)?1:Table(m_,0x2e8120).GetValue(std::fabs(input));m_.scalar(0x327150,coefficient);
  range=abs_yaw*coefficient;if(coefficient*45<range)range=coefficient*45;
 }
 const float normalized=angle/range;float previous=Table(m_,0x2e80b0).InverseLookup(std::fabs(normalized));if(normalized<0)previous=-previous;
 float delta=input-previous;float rate=m_.scalar(0x327170)*Table(m_,0x2e8098).GetValue(m_.scalar(car+0x3c0));
 if((0<previous&&input<previous)||(previous<0&&previous<input))rate=rate*m_.scalar(0x327174);
 float limit=rate*t;if(limit<m_.scalar(0x324e9c))limit=m_.scalar(0x324e9c);
 if(limit<delta)delta=limit;else if(delta<-limit)delta=-limit;
 const float next=previous+delta;float output=Table(m_,0x2e80b0).GetValue(std::fabs(next))*range;if(next<0)output=-output;
 output=output*m_.scalar(m_.word(car+0xc)+0x300);output=output*65536;
 // Debug0013B78C CVT.W.S precedes0013B798 integer DIV by360.
 m_.half(s+0x210,static_cast<std::uint16_t>(port::driving::cvt_w_s(output)/360));
}
// Debug0013B200; E-0046-C01. Unrelated player/nitro paths remain explicit gaps.
void RealDriver::Drive(float t,port::driving::PlayerServices& services){
 if(!services.MaybeAddTrackRoutePoint)hp2::not_implemented("0x0013B200","Route diagnostic service absent");
  services.MaybeAddTrackRoutePoint(m_,m_.word(address_+0x230));
 if(!services.RealDriverAddNearbyDebugTopology)hp2::not_implemented("0x001FEE80","Topology diagnostic service absent");
  services.RealDriverAddNearbyDebugTopology(m_,m_.word(address_+0x230));
 if(!services.IsJoystickPluggedIn)hp2::not_implemented("0x0013B200","Joystick presence service absent");
 const auto port=m_.signed_word(address_+0x234);if(!services.IsJoystickPluggedIn(port))ResetJoyValues();
 DoThrottleBrakeReverse(t,services);DoSteering(t,port,services);
 if(m_.word(address_+0x240)!=0){m_.word(address_+0x25c,0);m_.scalar(address_+0x264,1);}
 if(m_.word(address_+0x23c)!=0)m_.scalar(address_+0x218,1);
 if(m_.word(m_.word(address_+0x230)+0x18)!=0)hp2::not_implemented("0x0013B200","Player nitro/radial-blur path is outside the playerless test fixture");
}
}

namespace hp2::recovered {
// ELF 0x00130958; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00130958.md.
void RealDriver::Sub_00130958() {
    hp2::not_implemented("0x00130958", "RealDriver::Sub_00130958 (signature and full body unresolved)");
}

















// ELF 0x00136C58; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136c58.md.
void RealDriver::Sub_00136C58() {
    hp2::not_implemented("0x00136C58", "RealDriver::Sub_00136C58 (signature and full body unresolved)");
}

// ELF 0x00136C60; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136c60.md.
void RealDriver::Sub_00136C60() {
    hp2::not_implemented("0x00136C60", "RealDriver::Sub_00136C60 (signature and full body unresolved)");
}





// ELF 0x00136CE8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136ce8.md.
void RealDriver::Sub_00136CE8() {
    hp2::not_implemented("0x00136CE8", "RealDriver::Sub_00136CE8 (signature and full body unresolved)");
}

// ELF 0x00136CF0; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136cf0.md.
void RealDriver::Sub_00136CF0() {
    hp2::not_implemented("0x00136CF0", "RealDriver::Sub_00136CF0 (signature and full body unresolved)");
}

// ELF 0x00136D00; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/00136d00.md.


// ELF 0x0013AD90; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0013ad90.md.
void RealDriver::Sub_0013AD90() {
    hp2::not_implemented("0x0013AD90", "RealDriver::Sub_0013AD90 (signature and full body unresolved)");
}

// ELF 0x0013AE58; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0013ae58.md.
void RealDriver::Sub_0013AE58() {
    hp2::not_implemented("0x0013AE58", "RealDriver::Sub_0013AE58 (signature and full body unresolved)");
}

// ELF 0x0013AEB8; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0013aeb8.md.
// TASK-0041 E-0041-V01; debug 0x0013ACC0: RealDriver::DoSnapshot(ReplaySnapshot *); retail 0x0013AEB8.
void RealDriver::DoSnapshot() {
    hp2::not_implemented("0x0013AEB8", "RealDriver::DoSnapshot (signature and full body unresolved)");
}

// ELF 0x0013B080; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0013b080.md.
// TASK-0041 E-0041-V01; debug 0x0013AE88: RealDriver::AddJoyHandlers(void); retail 0x0013B080.
void RealDriver::AddJoyHandlers() {
    hp2::not_implemented("0x0013B080", "RealDriver::AddJoyHandlers (signature and full body unresolved)");
}

// ELF 0x0013B250; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0013b250.md.
// TASK-0041 E-0041-V01; debug 0x0013B058: RealDriver::RemoveJoyHandlers(void); retail 0x0013B250.
void RealDriver::RemoveJoyHandlers() {
    hp2::not_implemented("0x0013B250", "RealDriver::RemoveJoyHandlers (signature and full body unresolved)");
}

// Debug0013B1D8; E-0046-C01; recovered SetLowGear, not a Retail ABI.
void RealDriver::SetLowGear() {
    // Debug0013B1D8; E-0046-C01. FIRST_GEAR2 / AUTOMATIC-1.
    m_.word(address_+0x220,m_.word(address_+0x24c)==1?2:0xffffffffu);
}


// Debug0013C220; E-0082-H01; reverse/task0082/horn.md. Historical Retail
// template0013C420 is superseded in this canonical Debug owner.
void RealDriver::StartHonkHorn() {
    // Debug0013C220; E-0082-H01. HandyDebuggingFunction001E7198 is empty.
    const auto car=m_.word(address_+0x230),sound=m_.word(car+0x850);
    if(m_.word(m_.word(car+0xc)+0x538)!=1&&sound)
        port::driving::set_horn_activation(m_,sound,true);
}

// Debug0013C260; E-0082-H01; reverse/task0082/horn.md. Historical Retail
// template0013C460 is superseded in this canonical Debug owner.
void RealDriver::StopHonkHorn() {
    // Debug0013C260; E-0082-H01.
    const auto car=m_.word(address_+0x230),sound=m_.word(car+0x850);
    if(m_.word(m_.word(car+0xc)+0x538)!=1&&sound)
        port::driving::set_horn_activation(m_,sound,false);
}

// ELF 0x0013CAE0; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0013cae0.md.
// TASK-0041 E-0041-V01; debug 0x0013C8E0: RealDriver::GetDriverNumber(void); retail 0x0013CAE0.
void RealDriver::GetDriverNumber() {
    hp2::not_implemented("0x0013CAE0", "RealDriver::GetDriverNumber (signature and full body unresolved)");
}

// ELF 0x0013CAF0; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/0013caf0.md.
// TASK-0041 E-0041-V01; debug 0x0013C8F0: RealDriver::SetDesiredSpeed(float, float, bool); retail 0x0013CAF0.
void RealDriver::SetDesiredSpeed() {
    hp2::not_implemented("0x0013CAF0", "RealDriver::SetDesiredSpeed (signature and full body unresolved)");
}

// ELF 0x002A0550; E-0027-C01; E-0028-C01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/unknown/002a0550.md.
void RealDriver::Sub_002A0550() {
    hp2::not_implemented("0x002A0550", "RealDriver::Sub_002A0550 (signature and full body unresolved)");
}
} // namespace hp2::recovered

namespace hp2::recovered {

// Debug0013BF58; E-0082-H01; reverse/task0082/horn.md.
void RealDriver::JoyHandler_HonkHorn(int,int,std::uint8_t param){
 const auto car=m_.word(address_+0x230),config=m_.word(car+0xc);
 if(!param){StopHonkHorn();if(m_.word(config+0x538)!=1)m_.word(car+0x830,0);return;}
 StartHonkHorn();
 // Original001C2F48 null editor branch. No editor is installed by this fixture.
 if(m_.word(0x3281d4)!=0)hp2::not_implemented("0x001C2088","TrackRouteEditor::CancelTuningDrivingLines");
 if(m_.word(config+0x538)==1){
  if(m_.word(0x3121ec)!=0)hp2::not_implemented("0x0013BF58","BeACopToggleSirenAndLights requires police ownership");
  const bool on=m_.word(car+0x808)==0;m_.word(car+0x834,on?1:0);m_.word(car+0x808,on?2:0);
 }else m_.word(car+0x830,1);
}
// Debug0013B368; E-0046-C01; ResetJoyValues.
void RealDriver::ResetJoyValues() {
    // Debug0013B368; E-0046-C01. Byte stores preserve original padding.
    m_.byte(address_+0x250,0x80);m_.byte(address_+0x251,0x80);
    for(auto offset:{0x258,0x259,0x25a,0x260,0x261,0x262,0x268,0x269,0x270,0x271})m_.byte(address_+offset,0);
    for(auto offset:{0x254,0x25c,0x264,0x26c,0x274})m_.word(address_+offset,0);
}
} // namespace hp2::recovered
