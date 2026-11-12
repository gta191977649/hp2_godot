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
#include "game_core/port/race/control_construction.hpp"
#include "game_core/ReplayManager.hpp"
#include "game_core/port/race/car_render_resources.hpp"
#include "game_core/indep/inc/Player.hpp"

#include "game_core/port/not_implemented.hpp"
#include "game_core/indep/inc/Table.hpp"
#include "game_core/indep/src/carstate.hpp"
#include "game_core/port/driving/projections.hpp"
#include <cmath>
#include "game_core/port/driving/horn.hpp"
#include "game_core/indep/inc/Sound3d.hpp"

namespace port::driving {
// Debug0018B788/0018B7C8/002039C0; E-0082-H01/E-0099-W40.
// Actual virtual activation: original Horn/Siren targets own real queued voices.
void set_horn_activation(Memory& m,std::uint32_t sound,bool active){
 if(m.word(0x326f40)==0||m.word(0x327980)==0)return;
 hp2::debug::Sound3d(m,m.word(sound+0x34)).Activate(active);
}
// Debug0018B808; E-0082-H01.
bool horn_activation(const Memory& m,std::uint32_t sound){
 return m.word(0x326f40)!=0&&m.word(0x327980)!=0&&m.word(m.word(sound+0x34))!=0;
}
}

namespace hp2::recovered {
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
 (void)services; // Compatibility carrier; replay is the original direct game call.
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
  const auto replay=m_.word(0x3271a8);if(!replay)hp2::not_implemented("0x0013E070","actual ReplayManager owner is not bound");
  ReplayManager(m_,replay).RegisterInterestingEvent(1,"EBrake");
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
// Debug0013B200, E-0099-W63. Original game callees are direct recovered calls;
// only primitive joystick hardware queries enter through the port environment.
void RealDriver::Drive(float t,port::driving::PlayerServices& services){
 const auto car=m_.word(address_+0x230);hp2::debug::MaybeAddTrackRoutePoint(m_,car+0x60,car+0x70,m_.scalar(car+0x68),0,m_.signed_byte(m_.word(car+0xc)+0xe6));
 hp2::debug::RealDriverAddNearbyDebugTopology(m_,car+0x520,car+0x60);
 if(!services.IsJoystickPluggedIn)hp2::not_implemented("0x0013B200","Joystick presence service absent");
 const auto port=m_.signed_word(address_+0x234);if(!services.IsJoystickPluggedIn(port))ResetJoyValues();
 DoThrottleBrakeReverse(t,services);DoSteering(t,port,services);
 if(m_.word(address_+0x240)!=0){m_.word(address_+0x25c,0);m_.scalar(address_+0x264,1);}
 if(m_.word(address_+0x23c)!=0)m_.scalar(address_+0x218,1);
 if(const auto player=m_.word(m_.word(address_+0x230)+0x18)){const auto number=m_.signed_word(player+0x10);if(NitroActive()&&!NitroWasActive()){::port::race::RadialBlurOn(m_,number);::port::race::RadialBlurAlpha(m_,number,0,m_.scalar(0x324e94));return;}if(!NitroActive()&&NitroWasActive()){::port::race::RadialBlurOff(m_,number);return;}}
}
// Debug0013C118, E-0099-W63. Original valid timer sentinels and strict active
// interval; a future request is not silently clamped to zero elapsed time.
bool RealDriver::NitroActive()const{const auto request=m_.word(address_+0x278);return request&&request!=0x7fffffffu&&float(std::int32_t(m_.word(0x327cf8)-request))*m_.scalar(0x324ec4)<m_.scalar(0x3271a0);}
// Debug0013C098, E-0099-W63. Previous-frame comparison includes its actual
// nonnegative guard and the separate source timer coefficient.
bool RealDriver::NitroWasActive()const{const auto request=m_.word(address_+0x278);const float previous=float(std::int32_t(m_.word(0x327cf8)-request))*m_.scalar(0x324ec0)-m_.scalar(0x327d08);return request&&request!=0x7fffffffu&&0<=previous&&previous<m_.scalar(0x3271a0);}
// Debug0013C178, E-0099-W63. Actual Player NIS gate, genuine initial/stopped
// timers and inclusive recharge endpoint; no fabricated availability service.
bool RealDriver::NitroAvailable()const{if(const auto player=m_.word(m_.word(address_+0x230)+0x18))if(hp2::debug::Player(m_,player).IsNisPlaying())return false;const auto request=m_.word(address_+0x278);return !request||request==0x7fffffffu||m_.scalar(0x3271a0)+m_.scalar(0x3271a4)<=float(std::int32_t(m_.word(0x327cf8)-request))*m_.scalar(0x324ec8);}
// Debug0013C028/listing, E-0099-W63. Original press, actual cop UsageType,
// BeACop mode and recharge eligibility precede the source word/timer stores.
void RealDriver::JoyHandler_NitroBoost(int,int,std::uint8_t param){const auto car=m_.word(address_+0x230);if(param&&m_.word(m_.word(car+0xc)+0x538)==1&&m_.word(0x3121ec)&&NitroAvailable()){m_.word(address_+0x228,1);m_.word(address_+0x278,m_.word(0x327cf8));}}
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


#include "game_core/port/race/control_construction.hpp"
#include "game_core/indep/src/joystick.hpp"
#include "game_core/port/race/car_render_resources.hpp"
#include "game_core/indep/inc/CarDriver.hpp"
namespace hp2::debug {
// Debug0013B368; E-0099-W24. Byte stores preserve input padding.
void RealDriver::ResetJoyValues(){
 auto& m=m_;const auto a=address_;m.word(a+0x274,0);m.byte(a+0x251,0x80);m.byte(a+0x250,0x80);
 for(auto offset:{0x258u,0x259u,0x25au,0x260u,0x261u,0x262u,0x268u,0x269u,0x270u,0x271u})m.byte(a+offset,0);
 for(auto offset:{0x254u,0x25cu,0x264u,0x26cu})m.word(a+offset,0);
}
// Debug0013AB98; E-0099-W24.
void RealDriver::Sub_0013AB98(std::uint32_t car){
 CarDriver(m_,address_).Sub_00136D10();auto& m=m_;const auto a=address_;
 m.word(a+0x230,car);m.word(a+0x234,0xffffffffu);m.word(a+0x278,0);m.word(a+0x238,0);m.word(a+0x22c,0x002a6e60);
 ResetJoyValues();m.word(a+0x23c,1);m.word(a+0x220,0xffffffffu);m.word(a+0x240,0);m.word(a+0x244,0);
 m.word(a+0x248,0);m.word(a+0x24c,0);m.word(a+0x27c,0);m.word(a+0x278,0);
}
}


namespace hp2::debug {
// Debug0013B058; E-0099-W24. Live queued joystick/event-node disposal remains required.
void RealDriver::RemoveJoyHandlers(){
 if(!m_.word(address_+0x238))return;
 const auto port=m_.signed_word(address_+0x234);
 for(int event:{111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127})hp2::debug::RemoveJoyHandler(m_,port,event);
 const auto entry=m_.word(address_+0x22c)+0x60;
 if(m_.word(entry+4)!=0x13c260)hp2::not_implemented("0x0013B058","RealDriver.StopHonkHorn virtual target");
 hp2::recovered::RealDriver(m_,address_+static_cast<std::uint32_t>(m_.signed_half(entry))).StopHonkHorn();m_.word(address_+0x238,0);
}
// Debug0013AC60; E-0099-W24.
void RealDriver::Sub_0013AC60(int flags){
 m_.word(address_+0x22c,0x002a6e60);
 if(const auto player=m_.word(m_.word(address_+0x230)+0x18))::port::race::RadialBlurOff(m_,m_.signed_word(player+0x10));
 RemoveJoyHandlers();CarDriver(m_,address_).Sub_00136CB0(flags);
}
}

#include "game_core/indep/inc/Replay.hpp"
#include "game_core/indep/inc/CarDriver.hpp"
namespace hp2::debug {
// Debug0013ACC0; E-0099-W74; replay-frame/0013acc0.json/type-RealDriver.json.
void RealDriver::DoSnapshot(std::uint32_t snapshot){
 ReplaySnapshot s(m_,snapshot);s.BeginNamedSection(0x2a6e28);s.MoveData(address_+0x234,1,4,0);
 s.MoveData(address_+0x23c,1,4,1);s.MoveData(address_+0x240,1,4,1);s.MoveData(address_+0x248,1,4,0);
 s.MoveData(address_+0x24c,1,4,1);s.MoveData(address_+0x244,4);
 s.MoveData(address_+0x250,1);s.MoveData(address_+0x251,1);s.MoveData(address_+0x254,4);
 for(unsigned off:{0x258u,0x259u,0x25au})s.MoveData(address_+off,1);
 s.MoveData(address_+0x25c,4);
 for(unsigned off:{0x260u,0x261u,0x262u})s.MoveData(address_+off,1);
 s.MoveData(address_+0x264,4);
 s.MoveData(address_+0x268,1);s.MoveData(address_+0x269,1);s.MoveData(address_+0x26c,4);
 s.MoveData(address_+0x270,1);s.MoveData(address_+0x271,1);s.MoveData(address_+0x274,4);
 s.MoveData(address_+0x278,4);s.MoveData(address_+0x27c,1,4,0);CarDriver(m_,address_).DoSnapshot(snapshot);s.EndNamedSection();
}
}

namespace hp2::debug {
// Debug00153D90 declaration shared with original joystick owner; raw logical code operand.
void AddJoyHandler(::port::driving::Memory&,int,int,std::uint32_t,std::uint32_t);
// Debug0013AE88; TASK-0099 E-0099-W77; snapshot-input/0013ae88{-listing,}.json.
void RealDriver::AddJoyHandlers(){
 if(m_.word(address_+0x238))return;
 // Add registers15 callbacks; Remove additionally removes both REVERSE events.
 for(auto item:{std::pair<int,std::uint32_t>{111,0x13b3b8},{112,0x13b3b8},{113,0x13b7b0},{114,0x13b7b0},{115,0x13b7b0},{116,0x13bb48},{117,0x13bb48},{118,0x13bb48},{121,0x13bbc8},{122,0x13bbc8},{123,0x13bc90},{124,0x13bd70},{125,0x13be30},{126,0x13bf58},{127,0x13c028}})
  AddJoyHandler(m_,m_.signed_word(address_+0x234),item.first,item.second,address_);
 m_.word(address_+0x238,1);
}
}

namespace hp2::recovered {
// Debug0013B360,E-0099-W121; realdriver.cpp, JR/SWC1 delay slot.
void RealDriver::ForceEBrakeStop(float time){m_.scalar(address_+0x244,time);}
}
