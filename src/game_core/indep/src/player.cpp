#include "game_core/indep/src/copmgr.hpp"
#include "game_core/port/race/nis_services.hpp"
#include "game_core/indep/src/trackpath.hpp"
#include "game_core/indep/inc/CollisionBody.hpp"
#include "game_core/indep/src/Collision.hpp"
#include "game_core/indep/inc/Smokeable.hpp"
#include "game_core/indep/inc/World.hpp"
// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\player.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// Original translation unit: `..\indep\src\player.cpp` (ELF 0x002B3998; E-0029-P06).
// Directory binding through 0x00174EC0; see reverse/classes/source-path-bindings.json.
// This per-class file is a port-side partition, not the original translation unit or header.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: confirmed original directory (E-0029-P06); per-class basename/header extension port-side.
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/ReplayManager.hpp"
#include "game_core/indep/src/speech/sedwedge.hpp"
#include "game_core/indep/inc/Player.hpp"
#include "game_core/indep/inc/HudAnim.hpp"
#include "game_core/indep/src/raceparameters.hpp"
#include "game_core/indep/inc/AnimatedDriver.hpp"
#include "game_core/port/not_implemented.hpp"
#include "game_core/indep/inc/RealDriver.hpp"
#include "game_core/src/bMath.hpp"
#include <array>
#include <string>
#include "game_core/indep/src/fmenus.hpp"
#include "game_core/indep/src/VisibleDrivingLine.hpp"

#include "game_core/port/race/player_construction.hpp"
#include "game_core/port/race/player_hud_gameplay.hpp"
#include "game_core/indep/src/timer.hpp"
namespace hp2::debug {
// Original inline screen-hash overload emitted at Debug0014C8F8;
// implementation is shared with the source RaceCoordinator translation unit.
void SetFlailerScreenVisibility(::port::driving::Memory&,std::uint32_t,int);
// Debug0017C8E0; E-0110-F02. Original %s stack copy and +/- prefix handling.
// Expanded callee bStrToLong0024CBD8 accepts only leading decimal digits and
// wraps32-bit multiplication/addition; it does not consume whitespace/signs.
// The original16-byte stack overwrite domain remains an explicit port guard.
int Player::ConvertScoreString(std::uint32_t text){
 if(m_.byte(0x3277f8)!='%'||m_.byte(0x3277f9)!='s'||m_.byte(0x3277fa))hp2::not_implemented("0x0017C8E0","original score conversion format differs from reviewed percent-s bytes");
 std::array<std::uint8_t,16> temporary{};
 const auto copy=[&](std::uint32_t source){for(unsigned i=0;i<temporary.size();++i){temporary[i]=m_.byte(source+i);if(!temporary[i])return;}hp2::not_implemented("0x0017C8E0","original score string exceeds16-byte stack buffer");};
 copy(text);std::uint32_t sign=1;
 if(m_.byte(text)==m_.byte(0x327808)){sign=0xffffffff;copy(text+1);}
 if(m_.byte(text)==m_.byte(0x327810))copy(text+1);
 std::uint32_t result=0;for(auto ch:temporary){if(ch<'0'||ch>'9')break;result=result*10u+std::uint32_t(ch-'0');}
 return std::int32_t(sign*result);
}
// Debug0017C990; E-0110-F02. Read the actual DynamicScoringTable score strings;
// no host bonus values are substituted for the original mutable table.
int Player::GetFinishBonus(int position){switch(position){case 1:return ConvertScoreString(0x2f3784);case 2:return ConvertScoreString(0x2f37a8);case 3:return ConvertScoreString(0x2f37cc);default:return 0;}}
// Debug001809A8, E-0099-W51. Actual two cubic tracks. Last update and Busted
// object pointers are omitted by the source and intentionally remain untouched.
void NISOverlayManager::Sub_001809A8(){auto& m=m_;m.word(address_+4,0);m.word(address_+8,0);for(auto field:{0xcu,0x10u}){const auto p=m.allocate(0x30);m.word(address_+field,p);m.scalar(p+0x24,1);m.scalar(p+0x28,1);m.half(p+0x2e,1);for(unsigned k=0;k<4;++k)m.scalar(p+4*k,0);m.half(p+0x2c,0);}m.scalar(address_+0x24,1);m.scalar(address_+0x28,1);m.scalar(address_+0x1c,0);m.scalar(address_+0x20,0);}
// Debug00180A88, E-0099-W51. Ordinary owned cubic deletion, then optional self.
void NISOverlayManager::Sub_00180A88(int flags){m_.release(m_.word(address_+0xc));m_.release(m_.word(address_+0x10));if(flags&1)m_.release(address_);}
}

#include "game_core/indep/inc/Player.hpp"
#include "game_core/indep/inc/Car.hpp"
#include "game_core/indep/src/carstate.hpp"
#include "game_core/indep/src/copmgr.hpp"
#include "game_core/indep/inc/NIS.hpp"
#include "game_core/port/driving/ee_scalar.hpp"
#include "game_core/indep/inc/CameraMover.hpp"
#include "game_core/port/not_implemented.hpp"
namespace hp2::debug {
// Debug00177028, E-0099-W50. Genuine nullable/type/info/enabled/POV gates and
// signed original descriptor adjustments; no synthetic in-car camera setting.
bool Player::IsCameraPOVInCar()const{auto mover=m_.word(address_+0xc4);if(!mover||m_.word(mover+8)!=1)return false;auto table=m_.word(mover+0x28);auto target=m_.word(table+0x34),adjusted=mover+std::int32_t(m_.signed_half(table+0x30));std::uint32_t info;
 if(target==0x15f648)info=DriveCameraMover(m_,adjusted).GetCameraInfo();else if(target==0x15f750)info=CameraMover(m_,adjusted).GetCameraInfo();else hp2::not_implemented("0x00177028","Player camera-info virtual target is not reconstructed");if(!info)return false;mover=m_.word(address_+0xc4);if(!m_.word(mover+0x10))return false;table=m_.word(mover+0x28);target=m_.word(table+0x3c);adjusted=mover+std::int32_t(m_.signed_half(table+0x38));if(target!=0x15bfe0)hp2::not_implemented("0x00177028","Player POV virtual target is not reconstructed");const auto pov=DriveCameraMover(m_,adjusted).GetPOV(m_.signed_byte(address_+0x20));return pov&&m_.word(pov)==1;}
}

#include "game_core/indep/inc/NIS.hpp"
#include "game_core/indep/src/track.hpp"
#include "game_core/indep/src/sound.hpp"
#include "game_core/port/race/control_construction.hpp"
#include "game_core/port/race/physics_construction.hpp"
#include "game_core/port/race/flailer_resources.hpp"
#include "game_core/port/driving/ee_scalar.hpp"
#include "game_core/port/driving/projections.hpp"
namespace hp2::debug {
// Debug00173AB0/listing, E-0099-W55. Original Player-owned resources and global
// publication; untouched fields/padding are not blanket-cleared by this view.
void Player::Sub_00173AB0(int port,int number){
 auto& m=m_;const auto a=address_;const auto count=m.signed_word(0x327780);
 if(number<0||number>=2||count<0||count>=2)throw std::out_of_range("Original Player owner tables contain two entries");
 m.word(a+0x3c,0);m.word(a+0x40,0);for(unsigned i=0;i<8;++i)m.word(a+0x64+i*4,0);
 for(auto offset:{0x94u,0xa0u,0xa8u,0xacu,0xb4u,0x574u})m.word(a+offset,0);
 TopologyCoordinate(m,a+0x6d8).Sub_00151878();NISOverlayManager(m,a+0x9f0).Sub_001809A8();m.word(a+0xa20,0);m.word(a+0xa90,0);
 m.word(0x327780,std::uint32_t(count+1));m.word(0x327778+std::uint32_t(count)*4,a);m.word(0x327770+std::uint32_t(number)*4,a);
 const auto settings=0x311f78u+0x28c+std::uint32_t(number)*48;
 for(unsigned i=0;i<48;++i)m.byte(a+0x1c+i,m.byte(settings+i));
 m.word(a+8,std::uint32_t(port));m.word(a+0xc,std::uint32_t(std::int32_t(std::int8_t(GetJoystickType(m,port)))));m.word(a+0x10,std::uint32_t(number));
 for(auto offset:{0xc0u,0x4cu,0x50u,0x9e8u,0x9ecu,0x54u,0x58u,0x88u,0x8cu,0x90u,0x98u})m.word(a+offset,0);
 for(unsigned i=0;i<8;++i){m.word(a+0x64+i*4,0);m.byte(a+0x5c+i,0);}
 const bool split=m.word(0x311fa8)==2;m.word(a+0x14,split?(number==0?1:2):1);m.word(a+0x18,split?0:3);
 for(auto offset:{0x84u,0xc4u,0xc8u,0xccu,0xd0u,0xd4u,0xd8u,0xdcu,0xe0u,0x2fcu,0x300u,0x304u})m.word(a+offset,0);
 if(!m.word(0x311f90)&&!m.word(0x311f94)){m.word(a+0x300,split?(number==0?0xa8eb7af0:0x6283be92):0xf1f1f8e5);m.word(a+0x304,split?(number==0?0xd4d25ee7:0xc17316c9):0x3ea89b7c);}
 if(m.word(0x3121ec))m.word(a+0x304,0x3ea89b7c);
 m.word(a+0x308,0);m.word(a+0x9f4,m.word(a+0x304));
 for(auto offset:{0x30cu,0x310u,0x314u,0x410u,0x414u,0x3ccu,0x3d0u,0x3d8u,0x3dcu,0x3e0u,0x3e4u,0x3e8u,0x3ecu,0x3f0u,0x3f4u,0x3f8u,0x3fcu,0x400u,0x404u,0x408u,0x40cu,0x418u,0x41cu,0x420u,0x424u,0x42cu,0x430u,0x434u,0x438u,0x43cu,0x440u,0x444u,0x448u,0x474u,0x478u,0x47cu,0x480u,0x488u,0x484u,0x48cu,0x490u,0x470u,0x5e4u,0x5e0u,0x5d8u,0x5e8u,0x654u,0x658u,0x664u,0x5f4u,4u,0x5ecu,0x5f0u})m.word(a+offset,0);
 const auto nis=m.allocate(0x140);NisManager(m,nis).Sub_001AE5C0("Player"+std::to_string(1-number));m.word(a+0xf0,nis);
 m.word(a+0xe4,0xffffffff);m.word(a,0);m.word(a+0xec,0);m.word(a+0xe8,0xffffffff);
 for(unsigned i=0;i<32;++i)for(auto offset:{0xf4u,0x174u,0x1f4u,0x274u})m.word(a+offset+i*4,0);
 m.word(a+0xbc,0);const auto average=m.allocate(16);Average(m,average).Sub_0018DBF8(10);m.word(a+0x9c,average);m.word(a+0xa0,0);m.word(a+0xa4,0);
 for(unsigned i=0;i<20;++i)m.word(a+0x5f8+i*4,0);
 m.word(a+0x648,0);m.word(a+0x9e0,0);m.word(a+0xb0,0);m.word(a+0x668,m.allocate(0xa4));const auto fov=m.allocate(0x200);
 for(auto offset:{0x6d4u,0x2f4u,0x2f8u,0x9a0u,0x99cu})m.word(a+offset,0);
 m.word(a+0x6d0,fov);
 // Both source bSPrintf calls use the actual literal "0" at00327790.
 for(unsigned i=0;;++i){if(i>=32)throw std::out_of_range("Original Player score string exceeds32-byte storage");const auto character=m.byte(0x327790+i);if(character=='%')hp2::not_implemented("0x00173AB0","original score-format varargs are not reconstructed");m.byte(a+0x598+i,character);m.byte(a+0x5b8+i,character);if(!character)break;}
 m.word(a+0x9e4,0);
 for(const auto range:{std::array<unsigned,2>{{0x494,12}},{{0x4c4,10}},{{0x4ec,5}},{{0x500,4}},{{0x454,7}},{{0x558,3}},{{0x564,3}},{{0xa28,8}}})for(unsigned i=0;i<range[1];++i)m.word(a+range[0]+i*4,0);
 const auto beep=GetInGameSample(m,0x2ab7e0);m.word(a+0x574,0);m.word(a+0x64c,0);m.word(a+0x44c,0);m.word(a+0x450,0);
 const float speed=m.scalar(0x327784),time=float(m.signed_word(0x327cf8))*m.scalar(0x3252e4);
 for(auto offset:{0x650u,0xa1cu,0xa24u,0xa48u,0x554u,0xa50u,0xa54u,0xa58u,0xa5cu,0xa64u,0xa68u,0xa6cu})m.word(a+offset,0);
 m.word(a+0x570,beep);m.scalar(a+0x660,speed);m.word(a+0xa20,std::uint32_t(::port::driving::cvt_w_s(time*4000+.5f)));m.word(a+0xa4c,0xffffffff);m.word(a+0xb8,m.word(a+0xb8)&~31u);m.word(a+0x9dc,1);m.word(a+0x65c,1);
 const std::uint32_t names[]={0x464ba8ea,0xd911beac,0xf7f1cfb3,0xf88dd0b1},offsets[]={0xa74,0xa70,0xa78,0xa7c};
 for(unsigned i=0;i<4;++i)m.word(a+offsets[i],FindFlailerObject(m,0x49c15518,names[i],2));
 const auto top=FlailerObjectChunk(m,m.word(a+0xa74)).GetTopLeft(),bottom=FlailerObjectChunk(m,m.word(a+0xa70)).GetTopLeft();m.scalar(a+0xa84,top[1]);m.scalar(a+0xa80,bottom[1]);m.scalar(a+0xa88,FlailerObjectChunk(m,m.word(a+0xa78)).GetTopLeft()[1]-bottom[1]);m.scalar(a+0xa8c,FlailerObjectChunk(m,m.word(a+0xa7c)).GetTopLeft()[1]-top[1]);m.word(a+0xa90,0x7fffffff);m.word(a+0xa94,0);
}
// Debug0017AE98, E-0099-W55. Exact source skip set; no active HudAnimator is
// silently discarded. Constructor-state arrays are genuinely null.
void Player::ClearHudAnimations(){for(unsigned i=0;i<20;++i)if(std::uint32_t(i-12)>2&&i!=18){const auto animation=m_.word(address_+0x5f8+i*4);if(animation){if(i==10&&m_.word(m_.word(address_+0x620)+0x104)){AddToTotalScore(address_+0x5b8);m_.word(m_.word(address_+0x620)+0x104,0);}HudAnimator(m_,animation).FinishAnimation();}}}
// Debug00177E68; E-0112-H03. The no-services overload retains source ownership;
// reached platform consumers require explicit bindings rather than silent defaults.
void Player::SetHudFeatures(std::uint32_t features){SetHudFeatures(features,{});}
// Debug0017A3F0, E-0112-H03. Source compares the currently bound first light,
// rebinds authored RADAR_LIGHT1..12, then applies one +70 group translation
// only when its stored initial Y differs. bRadarOffset is a source word store.
void Player::ResetFlailerMovement(){auto& m=m_;const auto a=address_;if(m.scalar(a+0xa54)==0||m.word(0x327780)!=1)return;
 m.word(a+0xa50,1);const auto top=FlailerObjectChunk(m,m.word(a+0x494)).GetTopLeft();if(top[1]==m.scalar(a+0xa54))m.word(a+0xa50,0);
 const auto hud=m.word(a+0x300);for(unsigned i=0;i<12;++i){const std::string name="RADAR_LIGHT"+std::to_string(i+1);m.word(a+0x494+4*i,FindFlailerObject(m,hud,hp2::frontend::bStringHash(name.c_str()),2));}
 std::uint32_t object=0;while((object=FindFlailerObjectByGroup(m,hud,0x062743f5,0,object)))if(m.word(a+0xa50)){auto p=FlailerObjectChunk(m,object).GetTopLeft();FlailerObjectChunk(m,object).SetTopLeftY(p[1]+70.f);}
}
// Debug001752B8, E-0099-W55. Preserve the actual nullable owned route branch.
void Player::CleanUpJumpCam(){if(m_.word(address_+0x6d4))hp2::not_implemented("0x00164208","actual owned jump-camera PreviewRoute destructor is not reconstructed");m_.word(address_+0x6d4,0);}
// Debug00174448/listing, E-0099-W55. Full constructor-state lifetime. Reached
// active route/zone/line/animation dependencies retain their exact boundary.
void Player::Sub_00174448(int flags){auto& m=m_;const auto a=address_;ResetFlailerMovement();SetHudFeatures(0);
 if(m.word(a+0x84))hp2::not_implemented("0x00164208","actual Player PreviewRoute destructor is not reconstructed");
 if(m.word(a+0x88))hp2::not_implemented("0x00174448","actual Player zone-sample virtual destructor target is not reconstructed");
 if(const auto camera_data=m.word(a+0x668))m.release(camera_data);
 if(const auto fov=m.word(a+0x6d0))m.release(fov);
 CleanUpJumpCam();
 if(m.word(a+0xe0))hp2::not_implemented("0x00174448","actual Player VisibleDrivingLine destructor is not reconstructed");
 if(const auto nis=m.word(a+0xf0))NisManager(m,nis).Sub_001AE6F0(3);
 if(const auto average=m.word(a+0x9c))Average(m,average).Sub_0018DC70(3);
 for(unsigned i=0;i<20;++i)if(const auto animation=m.word(a+0x5f8+i*4))HudAnimator(m,animation).Sub_00234EA8(3);
 const auto count=m.signed_word(0x327780)-1,number=m.signed_word(a+0x10);if(count<0||count>=2||number<0||number>=2)throw std::out_of_range("Original Player close exceeds owner tables");m.word(0x327780,std::uint32_t(count));m.word(a+0x64c,0);m.word(a+0x44c,0);m.word(a+0x450,0);m.word(0x327778+std::uint32_t(count)*4,0);m.word(a+0x650,0);m.word(0x327770+std::uint32_t(number)*4,0);m.word(0x326d3c,1);NISOverlayManager(m,a+0x9f0).Sub_00180A88(2);TopologyCoordinate(m,a+0x6d8).Sub_001518C0(2);if(flags&1)m.release(a);
}
// Debug0017FF80/listing, E-0099-W55. Original bidirectional Car binding and
// signed settings; actual real-driver transmission then actual NIS permutation.
void Player::InitForRace(std::uint32_t car){auto& m=m_;m.word(car+0x18,address_);const auto mode=m.word(car+0x20);const auto style=m.signed_byte(address_+0x1e);m.word(address_+4,car);m.word(car+0x3dc,std::uint32_t(std::int32_t(style)));if(mode==1)hp2::recovered::RealDriver(m,m.word(car+0x2c)).SetManualTransmission(m.signed_byte(address_+0x1f));PredetermineNisNumbers();}
// Debug00180038, E-0099-W55. Preserve ticket-before-arrest global RNG order.
void Player::PredetermineNisNumbers(){CreateRandomNisArray(1);m_.word(address_+0xe8,0xffffffff);CreateRandomNisArray(2);m_.word(address_+0xe4,0xffffffff);}
// Debug00180080, E-0099-W55. Original selection without replacement, followed
// by a separate global bRandom for each authored camera count. No World RNG.
void Player::CreateRandomNisArray(int type){auto& m=m_;const int count=GetNumNisInfo(m,type);if(count>32)hp2::not_implemented("0x00180080","authored NIS contenders exceed original32-entry stack storage");std::array<int,32> contenders;
 for(int i=0;i<count;++i){auto row=GetNisInfo(m,i,type);if(!row)row=GetNisInfo(m,0,1);if(!row)hp2::not_implemented("0x00180080","original fallback first ticket NIS is absent");contenders[std::size_t(i)]=m.signed_word(row+0x28);}
 const auto numbers=address_+(type==1?0xf4u:0x1f4u),cameras=address_+(type==1?0x174u:0x274u);
 for(int i=0;i<count;++i){auto remaining=hp2::recovered::bRandom(m,count-i);int selected=-1;for(int candidate=0;candidate<count;++candidate)if(contenders[std::size_t(candidate)]!=-1){if(!remaining){selected=candidate;break;}--remaining;}if(selected<0)hp2::not_implemented("0x00180080","corrupt original NIS permutation could not select a contender");const auto number=contenders[std::size_t(selected)];m.word(numbers+std::uint32_t(i)*4,std::uint32_t(number));contenders[std::size_t(selected)]=-1;const auto row=GetNisInfo(m,number);if(!row)hp2::not_implemented("0x00180080","original authored NIS number does not index its loaded table");m.word(cameras+std::uint32_t(i)*4,hp2::recovered::bRandom(m,m.signed_word(row+0x30)));}
}
}

namespace hp2::debug {
// Debug0017F1D0, E-0099-W64. Exact source optional-feature/RC/owned-line
// gates; required positive VisibleDrivingLine construction is never skipped.
void Player::TriggerVisibleDrivingLine(int difficulty,float lifetime){auto& m=m_;if(!m.word(0x311fc4))return;const auto coordinator=m.word(0x327290);if(!coordinator)hp2::not_implemented("0x0017F1D0","original driving-line gate requires RaceCoordinator owner");if(m.word(coordinator)!=3||m.word(address_+0xe0))return;if(lifetime==0)lifetime=5;(void)difficulty;(void)lifetime;hp2::not_implemented("0x001737E8","actual VisibleDrivingLine construction/owned route/rendering pipeline is required");}
// Debug00174D78/listing, E-0099-W63. Playing is manager40, distinct from
// Animating5C. Preserve the full original int return rather than a bool cast.
int Player::IsNisPlaying()const{return m_.signed_word(m_.word(address_+0xf0)+0x40);}
}

// TASK-0099 E-0099-W62. Original Player prestart cleanup components.
#include "game_core/indep/src/joystick.hpp"
#include "game_core/port/race/car_render_resources.hpp"
namespace port::race {
// Port dispatch of the exact original destructor descriptors. Active unknown
// derived camera types are required callees, never generic host frees.
static void delete_player_camera(::port::driving::Memory& m,std::uint32_t object){
 const auto table=m.word(object+0x28),target=m.word(table+0xc),self=object+static_cast<std::int32_t>(m.signed_half(table+8));
 if(target==0x15beb8)hp2::debug::DriveCameraMover(m,self).Sub_0015BEB8(3);
 else if(target==0x157280)hp2::debug::CameraMover(m,self).Sub_00157280(3);
 else hp2::not_implemented("0x001752F0","actual derived CameraMover destructor target is not reconstructed");
}
}
namespace hp2::debug {
void Player::AutoPilotOff(){} //001770D0, instruction-proven empty.
void Player::ChangeCameraMover(int type,bool save){
 for(auto offset:{0xd4u,0xd8u})if(m_.word(address_+offset))hp2::not_implemented("0x001752F0","active Demo/CountdownCameraCoordinator destructor");
 if(const auto coordinator=m_.word(address_+0xdc)){NisManager(m_,m_.word(address_+0xf0)).DeleteNisCameraCoordinator(coordinator);m_.word(address_+0xdc,0);}
 const auto current=m_.word(address_+0xc4);if(current&&m_.signed_word(current+8)==type)return;
 if(!m_.word(address_+4))(void)m_.word(m_.word(0x3272f0)); //Actual source CarList fallback read.
 if(current){if(save)hp2::not_implemented("0x001752F0","saved camera Disable descriptor binding");::port::race::delete_player_camera(m_,current);m_.word(address_+0xc4,0);}
 // E-0112-C01: original driving constructor branch, actual PlayerSettings.
 if(type==1){
  const auto mover=m_.allocate(0x3f0);
  auto car=m_.word(address_+4);if(!car)car=m_.word(m_.word(0x3272f0));
  DriveCameraMover(m_,mover).Sub_0015BD10(m_.signed_word(address_+0x14),car,m_.signed_byte(address_+0x20),m_.scalar(0x3252e8),m_.signed_word(address_+0x24),0,1);
  m_.word(address_+0xc4,mover);m_.scalar(mover+0x7c,m_.word(address_+0x2f8)?1.f:0.f);
  m_.byte(address_+0x20,std::uint8_t(CycleDriveCameraPOVIndex(m_.signed_byte(address_+0x20))));
 }else if(type!=0)hp2::not_implemented("0x001752F0","required original non-driving camera constructor branch is not reconstructed");
 if(const auto camera=m_.word(address_+0xc4))m_.word(camera+0x1c,address_);
}
// Debug00176EE0; E-0112-C01. Input is the requested index, not an implicit increment.
int Player::CycleDriveCameraPOVIndex(int index){
 int selected=index%4;const auto mover=m_.word(address_+0xc4);
 if(mover&&m_.signed_word(mover+8)==1){
  const auto table=m_.word(mover+0x28);
  if(m_.word(table+0x34)!=0x15f648||m_.word(table+0x3c)!=0x15bfe0)hp2::not_implemented("0x00176EE0","derived camera POV dispatch not reconstructed");
  DriveCameraMover info(m_,mover+std::int32_t(m_.signed_half(table+0x30)));
  DriveCameraMover pov_view(m_,mover+std::int32_t(m_.signed_half(table+0x38)));
  if(info.GetCameraInfo()){
   unsigned attempts=0;for(;attempts<4;++attempts,selected=(selected+1)%4){const auto pov=pov_view.GetPOV(selected);if(!pov)continue;if(m_.word(0x326e9c))break;const auto kind=m_.signed_word(pov);const auto ride=m_.word(m_.word(address_+4)+0x10);if(!ride)hp2::not_implemented("0x00176EE0","actual Player car RideInfo absent");if(m_.byte(ride+6)?kind==1:(kind==0||kind==2))break;}
   if(attempts==4)selected=0;
   DriveCameraMover(m_,mover).SetPOVIndex(selected);
  }
 }return selected;
}
void Player::RemoveCameraMovers(){ChangeCameraMover(0,false);for(auto offset:{0xc8u,0xccu})if(const auto camera=m_.word(address_+offset)){::port::race::delete_player_camera(m_,camera);m_.word(address_+offset,0);}}
// Debug00177DB0; E-0112-C01. Preserve signed virtual this adjustments and actual POV.
int Player::GetGaugeMode()const{
 const auto mover=m_.word(address_+0xc4);if(!mover)return 0;const auto table=m_.word(mover+0x28);
 if(m_.word(table+0x3c)!=0x15bfe0||m_.word(table+0x44)!=0x15c218)hp2::not_implemented("0x00177DB0","derived GetPOV/OutsidePOV dispatch is not reconstructed");
 const auto pov=DriveCameraMover(m_,mover+std::int32_t(m_.signed_half(table+0x38))).GetPOV(m_.signed_byte(address_+0x20));if(!pov)return 0;
 if(DriveCameraMover(m_,mover+std::int32_t(m_.signed_half(table+0x40))).OutsidePOV()||m_.word(0x311fa8)==2)return 0x40;
 if(m_.word(pov)==0&&m_.word(0x311fa8)==1)return 0x20;
 return m_.word(pov)==1?0x10000000:0;
}
// Debug00177198; E-0112-C01. Retain original timer/mode gates and animation side effects.
void Player::RacingJoyHandler(int,int event,std::uint8_t param){
 if(event==93){ // JOY_EVENT_CAMERA_POV_CHANGE, original Debug enum.
  if(1.f<float(std::int32_t(m_.word(0x327cd8)-m_.word(address_+0x94)))*m_.scalar(0x325330)){
   const auto mover=m_.word(address_+0xc4);if(m_.word(address_+0xc0)!=2||!mover||m_.word(mover+8)!=1)return;
   m_.byte(address_+0x20,std::uint8_t(CycleDriveCameraPOVIndex(m_.signed_byte(address_+0x20)+1)));
  }return;
 }
 if(event==94){ // JOY_EVENT_CAMERA_LOOKBACK.
  m_.word(address_+0x2f8,param!=0);if(!m_.word(0x3272f4))m_.word(address_+0x64,m_.word(0x327cf8));
  auto mover=m_.word(address_+0xc4);if(!mover||m_.word(mover+8)!=1)mover=m_.word(address_+0xc8);
  if(mover&&m_.word(mover+8)==1)DriveCameraMover(m_,mover).SetLookBack(param!=0);
  const auto driver=m_.word(m_.word(address_+4)+0x4b4);if(driver){AnimatedDriver view(m_,driver);if(param)view.BeginLookingBackwards();else view.EndLookingBackwards();}return;
 }
 hp2::not_implemented("0x00177198","non-camera RacingJoyHandler branch is outside the recovered binding");
}
// Debug00175EB8; TASK-0099 E-0099-W77; snapshot-input/00175eb8{-listing,}.json.
// Port dispatch preserves raw source descriptors/callback addresses; no device is invented.
void Player::SetInputMode(int mode){
 auto& m=m_;const auto a=address_;const int old=m.signed_word(a+0xc0),port=m.signed_word(a+8);
 const auto camera_handlers=[&](std::uint32_t camera,bool add){
  if(!camera)return;const auto entry=m.word(camera+0x28)+(add?0x20u:0x28u),target=m.word(entry+4);
  // Exact CameraMover base handlers0015F760/0015F758 are JR RA/NOP.
  if(target!=(add?0x15f760u:0x15f758u))hp2::not_implemented("0x00175EB8","Derived camera joystick-handler virtual target");
 };
 const auto driver_handlers=[&](std::uint32_t driver,bool add){
  if(!driver)return;const auto entry=m.word(driver+0x22c)+(add?0x20u:0x28u),target=m.word(entry+4),self=driver+static_cast<std::uint32_t>(m.signed_half(entry));
  if(add&&target==0x13ae88)RealDriver(m,self).AddJoyHandlers();
  else if(!add&&target==0x13b058)RealDriver(m,self).RemoveJoyHandlers();
  else if(target!=(add?0x136ca0u:0x136c98u))hp2::not_implemented("0x00175EB8","CarDriver joystick-handler virtual target"); //Base methods are proven empty.
 };
 const auto cop_owns_car=[&](){const auto manager=m.word(0x328c1c);if(!manager)hp2::not_implemented("0x00175EB8","BeACop input mode requires actual CopManager owner");const auto index=m.signed_word(manager+12);if(index<0||index>=16)hp2::not_implemented("0x00175EB8","Source BeACop player index would not identify a real cop");const auto cop=m.word(manager+0x10+4*index);if(!cop)hp2::not_implemented("0x00175EB8","Source player cop is absent");return m.word(cop+4)==m.word(a+4);};
 if(old==2||old==7){
  if(old==2){RemoveJoyHandler(m,port,131);RemoveJoyHandler(m,port,93);RemoveJoyHandler(m,port,130);}RemoveJoyHandler(m,port,94);
  if(!m.word(0x3121ec)){for(int event:{95,96,97,98})RemoveJoyHandler(m,port,event);}
  else if(cop_owns_car()){RemoveJoyHandler(m,port,128);RemoveJoyHandler(m,port,129);}
  if(const auto car=m.word(a+4))driver_handlers(m.word(car+0x2c),false);
  for(unsigned off:{0xc4u,0xc8u,0xccu})camera_handlers(m.word(a+off),false);
 }else if(old==5){RemoveJoyHandler(m,port,132);RemoveJoyHandler(m,port,131);}
 else if(old==3)RemoveJoyHandler(m,port,131);
 else if(old==6){RemoveJoyHandler(m,port,132);RemoveJoyHandler(m,port,131);}
 if(m.signed_word(a+0x10)==0&&IsJoyHandlerAdded(m,1,45))RemoveJoyHandler(m,1,45);
 m.word(a+0xc0,static_cast<std::uint32_t>(mode));
 if(mode==2||mode==7){
  if(mode==2)for(int event:{131,93,130})AddJoyHandler(m,port,event,0x181820,a);
  AddJoyHandler(m,port,94,0x181820,a);
  if(!m.word(0x3121ec)){for(int event:{95,96,97,98})AddJoyHandler(m,port,event,0x181820,a);}
  else if(cop_owns_car()){AddJoyHandler(m,port,128,0x181820,a);AddJoyHandler(m,port,129,0x181820,a);}
  if(const auto car=m.word(a+4))if(m.word(car+0x20)==1)if(const auto driver=m.word(car+0x2c)){
   // Actual original SetJoystickPort inline store precedes AddJoyHandlers.
   m.word(driver+0x234,m.word(a+8));driver_handlers(driver,true);
  }
  for(unsigned off:{0xc4u,0xc8u,0xccu})camera_handlers(m.word(a+off),true);
 }else if(mode==3)AddJoyHandler(m,port,131,0x181820,a);
 else if(mode==5){AddJoyHandler(m,port,132,0x176390,a);AddJoyHandler(m,port,131,0x176390,a);}
 else if(mode==6){AddJoyHandler(m,port,132,0x1763e8,a);AddJoyHandler(m,port,131,0x1763e8,a);}
 if((mode==2||mode==4)&&m.signed_word(0x327780)<2&&m.signed_word(a+0x10)==0)AddJoyHandler(m,1,45,0x181800,a);
}
// Debug00175E10; E-0099-W77. RC state mapping then finish/frozen overrides.
void Player::DetermineInputMode(){
 const auto coordinator=m_.word(0x327290);if(!coordinator)hp2::not_implemented("0x00175E10","Source input mode requires actual coordinator owner");
 const auto state=m_.word(coordinator);int mode=0;
 switch(state){case 1:case 2:mode=5;break;case 3:mode=2;break;case 4:case 5:case 7:case 9:mode=7;break;case 6:mode=1;break;case 8:mode=4;break;default:break;}
 if(m_.word(address_+0x4c))mode=6;
 if(m_.byte(0x3277e0+static_cast<std::uint32_t>(m_.signed_word(address_+0x54))))mode=3;
 if(m_.signed_word(address_+0xc0)!=mode)SetInputMode(mode);
}
// Debug00180280; E-0099-W77; actual source live Player index order.
void DetermineAllPlayersInputModes(::port::driving::Memory& m){
 for(int i=0;i<m.signed_word(0x327780);++i){if(i>=2)hp2::not_implemented("0x00180280","Live player index exceeds source two slots");Player(m,m.word(0x327778+4*i)).DetermineInputMode();}
}

void Player::CleanupAfterRace(){AutoPilotOff();const auto car=m_.word(address_+4);m_.word(address_+4,0);const int number=m_.signed_word(address_+0x10);m_.word(car+0x18,0);::port::race::RadialBlurOff(m_,number);RemoveCameraMovers();if(IsNisPlaying()){NisManager(m_,m_.word(address_+0xf0)).Stop();NISOverlayManager(m_,address_+0x9f0).Finish();}SetInputMode(0);}
}

namespace hp2::debug {
// TASK-0099 E-0099-W69. Source true inactive/ongoing branches. AllowDebugLooping
// is a proven return0, not an injected flag. Completed playback needs its real
// camera replacement, Stop, callback and overlay tail; never discard them.
void Player::ServiceNis(){const auto manager=m_.word(address_+0xf0);if(!manager)hp2::not_implemented("0x00174D88","original Player requires NisManager owner");if(!m_.word(manager+0x40))return;
 if(NisManager(m_,manager).AllowDebugLooping())hp2::not_implemented("0x001AE9C8","unexpected branch from instruction-proven zero helper");
 if(m_.signed_word(manager+0x48)>m_.signed_word(0x327d04))return;
 const auto coordinator=m_.word(0x327290);if(!coordinator)hp2::not_implemented("0x00174D88","completed NIS requires original RaceCoordinator");const auto state=m_.word(coordinator);
 if(state!=4&&state!=7&&state!=5&&state!=9)ChangeCameraMover(1,false);
 NisManager(m_,manager).Stop();if(const auto callback=m_.word(address_)){auto& services=::port::race::require_nis_services(m_);if(!services.finished_callback)hp2::not_implemented("0x00174D88","actual captured NIS completion callback dispatcher is unbound");services.finished_callback(callback,m_.signed_word(address_+0xec));m_.word(address_,0);m_.word(address_+0xec,0);}NISOverlayManager(m_,address_+0x9f0).Finish();
}
// Debug0017F6B8: unsigned16 angle folding and hysteresis precede velocity and
// grounded-wheel predicates. No route-direction/velocity normalization added.
int Player::DetectWrongWay(int previous){const auto car=m_.word(address_+4);const auto direction=std::uint16_t(::port::driving::bATan(m_,m_.scalar(car+0x7f8),m_.scalar(car+0x7fc)));
 const auto separation=[&](std::uint16_t angle){auto delta=std::uint16_t(direction-angle);if(delta>0x8000)delta=std::uint16_t(-delta);if(previous)delta=std::uint16_t(delta+0x71c);return delta;};
 if(separation(m_.half(car+0x3a4))<=0x4e37)return 0;
 if(10.f<std::fabs(m_.scalar(car+0x3c0)*m_.scalar(0x3253bc))){if(separation(std::uint16_t(::port::driving::bATan(m_,m_.scalar(car+0x70),m_.scalar(car+0x74))))<0x4e38)return 0;}
 else if(!previous)return 0;
 return previous?1:int(m_.signed_word(car+0x3bc)>3);
}
// Debug001804F0: actual zone table, NIS, speed and early BeACop race gate.
bool Player::IsAllowedToReset(){const int zone=m_.signed_word(address_+0x54);if(m_.byte(0x3277e0+std::uint32_t(zone))&&zone!=6&&zone!=7)return false;if(IsNisPlaying())return false;
 if(!(m_.scalar(m_.word(address_+4)+0x3c4)*m_.scalar(0x3253c8)<=60.f))return false;
 if(m_.word(0x3121ec)){const auto race=m_.word(0x327258);if(!race)hp2::not_implemented("0x001804F0","original BeACop reset gate requires Race owner");if(float(m_.signed_word(race+0x10))*m_.scalar(0x3253cc)<5.f)return false;}return true;
}
// Debug001805C0. This manages source timers; it does not invent a reset prompt
// display operation. Existing pursuit lookup is called only at its source gate.
void Player::ChooseToDisplayReset(){const auto car=m_.word(address_+4),body=m_.word(car+0x34),now=m_.word(0x327cf8);if(!body)hp2::not_implemented("0x001805C0","original reset diagnostic requires CarCollisionBody");
 const auto elapsed=[&](std::uint32_t time,float scale){return float(std::int32_t(now-time))*scale;};
 const float speed=m_.scalar(car+0x3c4)*m_.scalar(0x3253d0),collision_age=elapsed(m_.word(body+0x104),m_.scalar(0x3253d4));
 if(60.f<speed&&collision_age<3.f&&50.f<std::fabs(hp2::recovered::CarState(m_,car+0x40).GetYawAlways()*m_.scalar(0x3253d8)))m_.word(address_+0xa8,now);
 const float interval=elapsed(m_.word(address_+0xb4),m_.scalar(0x3253dc));if(.5f<interval){const float previous=m_.scalar(address_+0xb0);if(60.f<previous&&collision_age<3.f&&(speed-previous)/interval< -70.f)m_.word(address_+0xa8,now);m_.word(address_+0xb4,now);m_.scalar(address_+0xb0,speed);}
 if(speed<10.f&&m_.scalar(car+0xb8)<0)m_.word(address_+0xa8,now);
 if(std::int32_t(now-m_.word(address_+0xa8))>20000)m_.word(address_+0xa8,0);
 const auto trouble=m_.word(address_+0xa8);if(trouble&&trouble!=0x7fffffffu&&IsAllowedToReset()){
  const auto cops=m_.word(0x328c1c);if(!cops||!tCopManager(m_,cops).IsBeingPursued(car)){const auto prompt=m_.word(address_+0xac);if(prompt&&prompt!=0x7fffffffu)return;m_.word(address_+0xa8,0);return;}}
 const auto prompt=m_.word(address_+0xac);if(prompt&&prompt!=0x7fffffffu&&std::int32_t(now-prompt)>12000)m_.word(address_+0xac,0);
}
// Debug0017F280. Source max.S is a scalar max, not the decompiler's spurious
// float-to-integer expression. Original Average uses its actual allocated ring.
void Player::UpdateGameState(const ::port::race::AiDriveTargetServices* ai){ServiceNis();m_.word(address_+0x2f4,DetectWrongWay(m_.signed_word(address_+0x2f4))!=0);ChooseToDisplayReset();
 const auto car=m_.word(address_+4),racing=m_.word(car+0x1c);if(!racing)hp2::not_implemented("0x0017F280","original Player frame requires RacingCar owner");(void)m_.signed_half(racing+0x12);
 const auto coordinator=m_.word(0x327290);if(!coordinator)hp2::not_implemented("0x0017F280","original Player frame requires RaceCoordinator");const auto state=m_.word(coordinator);const float dt=m_.scalar(0x327d08);
 if(state==3||state==4||state==7||state==5||state==9){const auto packed=std::uint32_t(::port::driving::cvt_w_s(dt*4000.f+.5f));const auto remaining=m_.word(address_+0xa0)-packed;m_.word(address_+0xa0,remaining);
  if(float(std::int32_t(remaining))*m_.scalar(0x3253b0)<0){hp2::recovered::Car subject(m_,car);const auto route=ai?subject.GetRoute(*ai):subject.GetRoute();if(route){const int car_class=m_.signed_byte(m_.word(car+0xc)+0xe6);const auto point=ai?subject.GetRoutePoint(*ai):subject.GetRoutePoint();float ratio=1;
    if(point){if(car_class<0||car_class>10)hp2::not_implemented("0x0017F3A8","original class-selected driving line outside reviewed21 entries");if(m_.word(route+0x28+0x60*std::uint32_t(car_class))&3){const float authored_speed=float(std::uint32_t(m_.byte(point+0x5a+2*car_class))<<16)*0x1p-16f;const float velocity=m_.scalar(car+0x3c4);ratio=authored_speed==0?::port::driving::ee_div_s_zero(velocity,authored_speed,"0x0017F404"):velocity/authored_speed;}}
    const auto average=m_.word(address_+0x9c);Average(m_,average).Record(ratio);const float recent=m_.scalar(average+8),maximum=m_.scalar(address_+0xa4);if(!std::isfinite(recent)||!std::isfinite(maximum))hp2::not_implemented("0x0017F41C","exceptional EE MAX.S speed ratio");m_.scalar(address_+0xa4,maximum<recent?recent:maximum);
   }m_.word(address_+0xa0,std::uint32_t(::port::driving::cvt_w_s(m_.scalar(0x3253b4))));}}
 const float delay=m_.scalar(address_+0x98);if(0<delay){const float left=delay-dt;m_.scalar(address_+0x98,left);if(left<=0){m_.scalar(address_+0x98,0);hp2::not_implemented("0x00176490","expired freeze delay requires original RequestZone(1,false,true,true) and successful bust sound");}}
}
// Debug00180440: reload NumPlayers after each source call, as the machine does.
void UpdatePlayersGameState(::port::driving::Memory& m,const ::port::race::AiDriveTargetServices* ai){for(int i=0;i<m.signed_word(0x327780);++i){if(i>=2)hp2::not_implemented("0x00180440","Player live owner table exceeds its two source entries");const auto player=m.word(0x327778+4*std::uint32_t(i));if(!player)hp2::not_implemented("0x00180440","live Player index table contains no original owner");Player(m,player).UpdateGameState(ai);}}
}

// E-0099-W70, original00174EA0. Real original finish state/nearby replay event;
// active CopSED still requires its original event-queue operation.
#include "game_core/indep/inc/Race.hpp"
#include "game_core/ReplayManager.hpp"
#include "game_core/indep/src/sound.hpp"
namespace hp2::debug {
void Player::SetFinishedRacing(){
 if(m_.word(address_+0x4c))return;
 m_.word(address_+0x4c,m_.word(0x327d04));const auto car=m_.word(address_+4),racing=m_.word(car+0x1c),race=m_.word(0x327258);
 if(m_.signed_byte(racing+9)==m_.signed_word(race+0x644)){
  if(!m_.word(0x3121ec)){int close=0;for(int i=0;i<m_.signed_word(race+0x18);++i){const auto other=m_.word(m_.word(race+0x624+4*i));const auto own=m_.word(racing);const float x=m_.scalar(own+0x60)-m_.scalar(other+0x60),y=m_.scalar(own+0x64)-m_.scalar(other+0x64),z=m_.scalar(own+0x68)-m_.scalar(other+0x68);const float square=(x*x+y*y)+z*z;if(!std::isfinite(square)||square<0)hp2::not_implemented("0x00174EA0","exceptional EE finish proximity sqrt operand");if(std::sqrt(square)<30)++close;}
   const auto name=::port::race::render_resource_text(m_,0x2ab7f0);hp2::recovered::ReplayManager(m_,m_.word(0x3271a8)).RegisterInterestingEvent((float(close)-1.f)*7.5f+10.f,name.c_str());
  }
  if(Race(m_,race).AreAllPlayersFinishedRacing())if(const auto sed=m_.word(0x327f20))cSEDWedge(m_,sed).ClearEventQueue();
  TriggerFinishLineSound(m_);
 }
 m_.word(address_+0x50,0x168);
}
}

#include "game_core/indep/inc/Replay.hpp"
namespace hp2::debug {
// Debug00174608; E-0099-W74; replay-frame/00174608.json/type-Player.json.
void Player::DoSnapshot(std::uint32_t snapshot){
 ReplaySnapshot s(m_,snapshot);s.BeginNamedSection(0x327798);s.MoveData(address_+0x4c,4,4,1);
 s.MoveData(address_+0x50,2,4,1);s.MoveData(address_+0x54,1,4,1);s.MoveData(address_+0x5c,8,8,0);s.MoveData(address_+0x64,32);
 s.MoveData(address_+0xbc,1,4,1);Average(m_,m_.word(address_+0x9c)).DoSnapshot(snapshot);
 for(unsigned off:{0xa0u,0xa4u,0xa8u,0xacu,0xb0u,0xb4u,0xb8u,0xe4u,0xe8u,0u,0xecu})s.MoveData(address_+off,4);
 NisManager(m_,m_.word(address_+0xf0)).DoSnapshot(snapshot);
 s.MoveData(address_+0x2f4,1,4,0);s.MoveData(address_+0x2f8,1,4,0);s.MoveData(address_+0x8c,1,4,0);
 if(m_.byte(snapshot+0x3d))SetInputMode(0);
 s.MoveData(address_+0x98,4);s.EndNamedSection();
}
}

namespace hp2::debug {
// Debug001804A0; E-0099-W81; replay-events/001804a0.json, PlayerSettings+1C/DrivingStyle+2.
void Player::SetDrivingStyle(int mode){m_.byte(address_+0x1e,static_cast<std::uint8_t>(mode));}
}

namespace hp2::debug {
// Debug0017F848, E-0099-W92. Original Average value, not max-speed ratio.
float Player::GetSpeedRatio()const{return m_.scalar(m_.word(address_+0x9c)+8);}
// Debug0017F858, E-0099-W92. Exact byte counts and strict ratio boundary.
bool Player::CatchupAllowed(float minimum)const{const auto average=m_.word(address_+0x9c);return m_.byte(average+2)==m_.byte(average)&&minimum<m_.scalar(address_+0xa4);}
}

#include "game_core/port/race/player_hud_gameplay.hpp"
#include "game_core/port/runtime_warnings.hpp"
#include "game_core/indep/src/raceparameters.hpp"
#include <string>
namespace port::race::hud_detail {
static void source_start_score(driving::Memory&,std::uint32_t,int,bool,const PlayerHudPresentationServices&);
static void update_total_text(driving::Memory&,std::uint32_t);
}
namespace port::race::detail {
// Port presentation boundary, not an original helper. Strings are source Player fields.
static std::string scoring_string(::port::driving::Memory& m,std::uint32_t a){std::string text;for(unsigned i=0;i<32;++i){const auto c=m.byte(a+i);if(!c)return text;text+=static_cast<char>(c);}hp2::not_implemented("0x0017BA40","original scoring text exceeds32-byte field");}
static void scoring_write(::port::driving::Memory& m,std::uint32_t a,const std::string& text){if(text.size()>=32)hp2::not_implemented("0x0017BA40","original score text exceeds32-byte field");for(unsigned i=0;i<text.size();++i)m.byte(a+i,static_cast<std::uint8_t>(text[i]));m.byte(a+static_cast<std::uint32_t>(text.size()),0);}
static void score_animation_start(::port::driving::Memory& m,std::uint32_t player,PlayerHudGameplayState& state){if(state.score_active)hp2::debug::Player(m,player).AddToTotalScore(player+0x5b8);state.score_active=true;state.score_phase=0;state.score_start_time=static_cast<float>(m.signed_word(0x327cf8))*m.scalar(0x32611c);scoring_write(m,player+0x5b8,scoring_string(m,player+0x598));m.word(player+0xa90,m.word(0x327cf8));}
static void score_animation_tick(::port::driving::Memory& m,std::uint32_t player,PlayerHudGameplayState& state){if(!state.score_active)return;const float duration=state.score_phase==2?1.f:.5f;float fraction=(static_cast<float>(m.signed_word(0x327cf8))*m.scalar(0x326134)-state.score_start_time)/duration;if(1<fraction)fraction=1;if(1<=fraction){if(state.score_phase==2){state.score_active=false;hp2::debug::Player(m,player).AddToTotalScore(player+0x598);}else{state.score_start_time+=duration;++state.score_phase;}}}
// Port logical HUD-present correspondence: evaluate only source bits which
// gate non-render producers. Original screen/text pointers are never fabricated.
static std::uint32_t logical_gauge_mode(::port::driving::Memory& m,std::uint32_t player,const PlayerHudGameplayState& state){if(m.word(player+0xc4))return static_cast<std::uint32_t>(hp2::debug::Player(m,player).GetGaugeMode());if(state.camera){::port::diagnostics::warn("0x00177DB0","original camera presentation object disabled; explicit PC selected-POV semantic context supplied","evaluating original GetGaugeMode predicates with actual PlayerSettings feature gates; no fake camera pointer or gauge override");const auto& camera=*state.camera;if(!camera.selected_pov_available)return 0;if(camera.outside_pov||m.word(0x311fa8)==2)return 0x40;if(camera.selected_pov_kind==0&&m.word(0x311fa8)==1)return 0x20;if(camera.selected_pov_kind==1)return 0x10000000;return 0;}if(m.word(player+0xc4))hp2::not_implemented("0x00177DB0","original nonnull camera POV/outside-POV gauge dispatch remains required");return 0;}
// Exact full DetermineHudFeatures mask for source one-player non-BeACop domain.
// Logical HUD presence replaces disabled screen binding only; every true mask
// transition retains the source ClearHudAnimations pending-score flush.
static std::uint32_t logical_hud_data_features(::port::driving::Memory& m,std::uint32_t player,const PlayerHudGameplayState& state){
 if(!m.word(player+4))return 0;if(m.signed_word(0x327780)!=1||m.word(0x3121ec))hp2::not_implemented("0x00177570","logical HUD gameplay currently requires original single-player racer domain");
 constexpr std::uint32_t parameters=0x311f78;const auto coordinator=m.word(0x327290),race=m.word(0x327258);if(!coordinator||!race)hp2::not_implemented("0x00177570","actual RaceCoordinator and Race required for logical HUD features");const auto phase=m.word(coordinator);std::uint32_t features=0x40000;if(m.signed_word(player+0x54))return features;if(phase==4||phase==7||phase-8u<2)return 0;
 const auto mode=m.signed_byte(player+0x1c);const auto preferred=m.word(player+0x2c);const bool alone=m.signed_word(parameters+0x30)+m.signed_word(parameters+0x34)==1;const auto free_run=[&]{return hp2::debug::RaceParameters(m,parameters).IsFreeRun();};
 if(hp2::debug::Player(m,player).IsNisPlaying()){if(m.word(parameters+0x25c)&&phase!=6)features=0x8040000;if(alone||free_run()||mode==1||mode==3)features&=0xf7fffdff;if(phase!=6)features|=0x200000;return features|0x2000000;}
 if(const auto manager=m.word(0x328c1c);manager&&hp2::debug::tCopManager(m,manager).ShouldDisplayBustedText())return phase==6?features:features|0x200000;
 const auto stage=m.word(race);if(stage==1||stage==2)return 0x2000000;if(stage==3)return features|0x2000000;if(m.word(player+0x4c)&&phase==3)return 0x40004;
 if(phase==6){if(m.word(coordinator+0x18)-1u<2)features=0x2040000;const auto changed=m.word(player+0x5f0);if(!changed)return features;if(59<static_cast<std::int32_t>(m.word(0x327d04)-changed)){m.word(player+0x5f0,0);return features;}return features|1;}
 if(m.signed_word(player+0x54)==1||m.signed_word(player+0x54)==2)return 0x42000;
 if(mode==1){features=m.word(parameters+0x25c)?0x4040001:0x40001;}
 else if(mode==0){features=m.signed_byte(parameters+0x2ee)==6?0x140000:0x140001;if(preferred&0x100)features|=0x508;if(preferred&4)features|=0x80;if(preferred&0x10)features|=0x200;const auto time=m.word(parameters+0x24);if((preferred&0x200)&&((time==0||(time==0x7fffffff&&!free_run()))&&m.signed_byte(parameters+0x2f1)!=0))features|=0x800000;if(m.signed_byte(parameters+0x2ec)!=0&&!m.word(player+0x2f8))features|=0x1000;if(m.word(parameters+0x20)==2&&!m.word(parameters+0x28))features&=0xfffffdff;if(alone||free_run())features&=0xfffffdff;if(m.word(parameters+0x25c)){features|=(preferred&0x100)?0x8000:0x4000000;if(!m.word(player+0x2f8))features|=0x80000;}if(!free_run()&&m.word(player+0x48))features|=0x20000;if(preferred&0x40)features|=logical_gauge_mode(m,player,state);}
 else if(mode==3){if(m.word(parameters+0x25c))features=0x4040000;features|=logical_gauge_mode(m,player,state);}
 else if(mode==2&&m.word(parameters+0x25c)&&!m.word(player+0x4c))features=0x4040000;
 if(m.signed_byte(parameters+0x2ee)==6&&mode==0)features&=0xfffffffe;if(m.word(player+0x2f4))features|=0x4000;const auto reset=m.word(player+0xac);if(reset&&reset!=0x7fffffff)features|=0x10000;return features;
}

static void render_hud_data(::port::driving::Memory& m,std::uint32_t player,std::uint32_t features){if(!features)return;const auto own=m.word(player+4);auto car=own;if(!m.word(0x32753c))hp2::not_implemented("0x0014D2C0","original debug alternate followed-car lookup required by HUD data");const auto racing=hp2::debug::Race(m,m.word(0x327258)).GetRacingCar(car);if(!(features&0x10000060))m.word(player+0x64c,999);else{const bool metric=m.signed_byte(0x312265)==1;const auto hud=m.word(own+0xc)+0x4d0;const float maximum=m.scalar(hud+(metric?0x34:0x2c)),speed=m.scalar(car+0x3c0)*m.scalar(metric?0x32537c:0x325380);if(!m.word(player+0x654))m.word(player+0x654,static_cast<std::uint32_t>(::port::driving::cvt_w_s(m.scalar(player+0x660)*maximum)));if(maximum==0)hp2::not_implemented("0x0017D858","original speed HUD maximum divisor requires exact EE carrier");const float ratio=speed/maximum;const int current=::port::driving::cvt_w_s(speed);m.word(player+0x658,0);if(ratio<m.scalar(0x327788))m.word(player+0x65c,1);if(m.word(player+0x65c)&&current<m.signed_word(player+0x64c)&&m.signed_word(player+0x654)<m.signed_word(player+0x64c)&&m.scalar(player+0x660)<ratio){const float next=m.scalar(player+0x660)+m.scalar(0x325384);m.scalar(player+0x660,1<next?1:next);m.word(player+0x65c,0);m.word(player+0x658,1);}m.word(player+0x64c,static_cast<std::uint32_t>(current));}if(racing){const int position=m.signed_byte(racing+0xa),previous=m.signed_word(player+0x648);if(position!=previous){m.scalar(player+0x5e0,static_cast<float>(m.signed_word(0x327cf8))*m.scalar(0x32539c));m.word(player+0x5e4,position<previous?1:0xffffffff);}m.word(player+0x648,static_cast<std::uint32_t>(position));if((features&0x200)&&m.word(player+0x5e4))m.scalar(player+0x5e0,static_cast<float>(m.signed_word(0x327cf8))*m.scalar(0x3253a0));}}
static float scoring_mass(::port::driving::Memory& m,std::uint32_t body){const auto table=m.word(body+0x50),self=body+static_cast<std::int32_t>(m.signed_half(table+0x98)),target=m.word(table+0x9c);if(target==0x1d4ed0)return hp2::debug::CarCollisionBody(m,self).GetMassInverse();if(target==0x1d5be8)return hp2::debug::DynamicCollisionBody(m,self).GetMassInverse();if(target==0x1d5cf8)return hp2::debug::StaticCollisionBody(m,self).GetMassInverse();if(target==0x21c880)return hp2::debug::Smokeable(m,self).GetMassInverse();hp2::not_implemented("0x0017D0E4","actual scoring witness mass-inverse virtual target required");}
}
namespace hp2::debug {
// Debug0017CA10,E-0110-F09. Source signed32 sum/clamp; UI string sink disabled before entry.
void Player::AddToTotalScore(std::uint32_t text){const auto value=m_.word(address_+0x5e8)+static_cast<std::uint32_t>(ConvertScoreString(text));m_.word(address_+0x5e8,static_cast<std::int32_t>(value)<0?0:value);::port::race::hud_detail::update_total_text(m_,address_);}
// Debug0017CA70/0017CC18,E-0110-F09. Source score text and pending replacement order.
void Player::StartScore(int event,::port::race::PlayerHudGameplayState& state){if(state.source_hud){if(!state.presentation)hp2::not_implemented("0x0017CA70","source HUD presentation services absent");::port::race::hud_detail::source_start_score(m_,address_,event,false,*state.presentation);return;}if(event<0||event>30)hp2::not_implemented("0x0017CA70","original dynamic score event outside recovered table");auto& m=m_;::port::diagnostics::warn("0x0017CA70","original score sound/title/Flailer animation presentation disabled by explicit PC policy","retaining score text, original pending replacement, phase durations and total-score mutations");const auto text=::port::race::detail::scoring_string(m,0x2f3448+36*event);if(text.find('%')!=std::string::npos)hp2::not_implemented("0x0017CA70","original no-argument score format requires varargs recovery");::port::race::detail::scoring_write(m,address_+0x598,text);::port::race::detail::score_animation_start(m,address_,state);}
void Player::StartScore(int event,int,::port::race::PlayerHudGameplayState& state){if(state.source_hud){if(!state.presentation)hp2::not_implemented("0x0017CC18","source HUD presentation services absent");::port::race::hud_detail::source_start_score(m_,address_,event,true,*state.presentation);return;}if(event<0||event>30)hp2::not_implemented("0x0017CC18","original dynamic score event outside recovered table");::port::diagnostics::warn("0x0017CC18","original score sound/title/Flailer animation presentation disabled by explicit PC policy","retaining caller-formatted score text and original pending score lifecycle");::port::race::detail::score_animation_start(m_,address_,state);}
// Debug0017CEE0,E-0110-F09. Collision virtual children outside recovered domain remain mandatory.
void Player::UpdateScoring(::port::race::PlayerHudGameplayState& state){auto& m=m_;const auto a=address_,car=m.word(a+4),race=m.word(0x327258);if(!race)hp2::not_implemented("0x0017CEE0","actual Race required by Player scoring");const auto racing=Race(m,race).GetRacingCar(car);if(hp2::recovered::Car(m,car).IsBlinking()){m.scalar(a+0x5d8,0);m.word(0x326d3c,1);return;}
 if(racing&&!m.signed_half(racing+0x12)&&m.signed_byte(racing+0xd)){const auto p=0x311f78u;const auto time=m.word(p+0x24);const bool timed=m.word(p+0x20)==2&&time&&time!=0x7fffffff&&!m.word(p+0x274);if(timed||m.word(p+0x28)){if(!m.word(a+0xa24))StartScore(7,state);m.word(a+0xa24,0);}else{if(!m.word(a+0x9e0)){StartScore(3,state);m.word(a+0xa64,m.word(a+0xa64)+1);}else if(m.signed_byte(racing+0xa)==1)StartScore(m.signed_byte(racing+9)==1?4:5,state);else if(!m.word(a+0xa24)&&0<m.signed_byte(racing+9))StartScore(7,state);m.word(a+0xa24,0);m.word(a+0x9e0,0);}}
 if(m.word(a+0x658))StartScore(6,state);
 const auto collision=CollisionBody(m,m.word(car+0x34)).GetCollision(0);if(collision){Collision contact(m,collision);std::uint32_t first=0,second=0;contact.GetCollidingCars(first,second);int side=-1;if(first&&m.word(first+0x18)==a)side=0;if(second&&m.word(second+0x18)==a)side=1;if(side<0)m.word(a+0x9e4,0);else{const auto witness=m.word(collision+0x54);const float mass1=::port::race::detail::scoring_mass(m,m.word(witness+0x60)),mass2=::port::race::detail::scoring_mass(m,m.word(witness+0x64)),mass=side==1?mass1:mass2;if(!(500.f<=mass)&&m.scalar(a+0x5d8)==0){const float x=m.scalar(collision+0x60)*m.scalar(car+0xb0),y=m.scalar(collision+0x64)*m.scalar(car+0xb4),z=m.scalar(collision+0x68)*m.scalar(car+0xb8),dot=(x+y)+z;if(dot<m.scalar(0x325368)&&m.scalar(0x32536c)<dot&&dot!=0&&m.scalar(0x325370)<m.scalar(collision+0x50)){m.word(a+0x9e0,1);m.word(a+0xa24,1);m.word(a+0x9dc,0);if(contact.IsSmokeableInvolved()){if(!m.word(a+0x9e4))m.word(a+0x9e4,1);}else m.word(a+0x9e4,0);}}}}
 const float hang=m.scalar(a+0x5d8);if(m.signed_word(car+0x3bc)==0){if(hang==0)m.scalar(a+0x5d8,static_cast<float>(m.signed_word(0x327cf8))*m.scalar(0x325374));}else if(hang==0)m.scalar(a+0x5d8,0);else{const float elapsed=static_cast<float>(m.signed_word(0x327cf8))*m.scalar(0x325378)-hang;const int raw=::port::driving::cvt_w_s(elapsed*250.f),score=(raw/50)*50;auto air=[&](int event){::port::race::detail::scoring_write(m,a+0x598,std::to_string(score));StartScore(event,score,state);};if(static_cast<std::uint32_t>(score-100)<100)air(11);if(static_cast<std::uint32_t>(score-200)<150)air(12);if(score>=350)air(13);m.scalar(a+0x5d8,0);}
 if(m.signed_word(a+0x5e4)>0){m.word(a+0x5e4,0);const int position=m.signed_word(a+0x648);if(position<1||position>8)hp2::not_implemented("0x0017D350","original reached position outside8 word entries");const auto flag=a+0xa28+4*(position-1);if(!m.word(flag)&&!m.word(0x3121ec)){StartScore(position+15,state);m.word(flag,1);}}
}
void Player::UpdateHud(::port::race::PlayerHudGameplayState& state,bool minimal){if(state.source_hud){UpdateHudPresentation(state);return;}if(!minimal)hp2::not_implemented("0x00177570","strict original HUD feature/animation/render pipeline required");auto& m=m_;const auto a=address_;::port::diagnostics::warn("0x0017D3B0","original HUD feature/title/render presentation disabled by explicit PC policy","running original score producer gates and logical score animation lifecycle before snapshot publication");const auto data_features=::port::race::detail::logical_hud_data_features(m,a,state);if(data_features!=state.hud_features){if(state.score_active){AddToTotalScore(a+0x5b8);state.score_active=false;}state.hud_features=data_features;}::port::race::detail::score_animation_tick(m,a,state);const auto race=m.word(0x327258),rc=m.word(0x327290);if(!race||!rc)hp2::not_implemented("0x0017D3B0","actual Race and RaceCoordinator required by HUD gameplay");const int phase=m.signed_word(rc);const bool eligible=m.word(race)==4&&phase!=4&&phase!=5&&phase!=7&&phase!=9;const auto racing=Race(m,race).GetRacingCar(m.word(a+4));if(eligible){if(!m.word(a+0xa94)){if(m.signed_word(0x327780)==2&&RaceParameters(m,0x311f78).GetNumEliminated()==0){if(racing){int place=m.signed_byte(racing+0xa)-1;const int last=m.signed_word(race+0x18)-1;if(place==last)--place;for(int i=0;i<last-place;++i)m.word(a+0xa28+4*(last-i-1),1);}}else if(racing){const int count=m.signed_word(race+0x18),places=count-m.signed_byte(racing+0xa);std::uint32_t sum=0;for(int i=0;i<places;++i){sum+=static_cast<std::uint32_t>(ConvertScoreString(0x2f3448+36*(count+14-i)));m.word(a+0xa28+4*(count-i-2),1);}if(sum){::port::race::detail::scoring_write(m,a+0x598,std::to_string(static_cast<std::int32_t>(sum)));StartScore(15,static_cast<int>(sum),state);}}m.word(a+0xa94,1);}UpdateScoring(state);}else{if(racing){const int index=(m.signed_half(racing+6)+5)%5;const auto crossing=m.word(racing+0x24+4*index);if(crossing&&crossing!=0x7fffffff)m.word(a+0xa20,crossing);}m.word(a+0x5e4,0);}::port::race::detail::render_hud_data(m,a,data_features);}
}


namespace port::race::detail {
// Native lookup carrier for the source twenty-slot owner. It does not create
// animations proactively: original lazy allocation order is retained.
static std::uint32_t owned_hud_animation(driving::Memory& m,std::uint32_t player,int id,const char* pc){
 if(id<0||id>=20)hp2::not_implemented(pc,"original HudAnimator index outside twenty-slot owner");
 const auto slot=player+0x5f8+4*std::uint32_t(id);auto animation=m.word(slot);
 if(!animation){animation=m.allocate(0x120);hp2::debug::HudAnimator(m,animation).Sub_00234E38();m.word(slot,animation);}return animation;
}
}
namespace hp2::debug {
// Debug0017B688/0017B800; E-0112-H02. Endpoint deltas are formed by source
// scalar subtraction, and alpha subtraction retains original word wrapping.
void Player::HudStartAnimation(int id,FlailerTextChunk text,const std::array<float,2>& pos,const std::array<float,2>& end,const std::array<float,2>& scale,const std::array<float,2>& endscale,int alpha,int endalpha,float duration,std::uint32_t timer){
 const auto animation=::port::race::detail::owned_hud_animation(m_,address_,id,"0x0017B688");std::array<float,2> delta,scaledelta;for(unsigned k=0;k<2;++k){delta[k]=end[k]-pos[k];scaledelta[k]=endscale[k]-scale[k];}
 HudAnimator(m_,animation).StartAnimation(text,pos,delta,scale,scaledelta,alpha,float(std::int32_t(std::uint32_t(endalpha)-std::uint32_t(alpha))),duration,timer);
}
void Player::HudStartAnimation(int id,FlailerBitmapChunk bitmap,const std::array<float,2>& pos,const std::array<float,2>& end,const std::array<float,2>& scale,const std::array<float,2>& endscale,int alpha,int endalpha,float duration,std::uint32_t timer,std::uint32_t visible){
 const auto animation=::port::race::detail::owned_hud_animation(m_,address_,id,"0x0017B800");std::array<float,2> delta,scaledelta;for(unsigned k=0;k<2;++k){delta[k]=end[k]-pos[k];scaledelta[k]=endscale[k]-scale[k];}
 HudAnimator(m_,animation).StartAnimation(bitmap,pos,delta,scale,scaledelta,alpha,float(std::int32_t(std::uint32_t(endalpha)-std::uint32_t(alpha))),duration,timer,visible);
}
// Debug0017B980; E-0112-H02. Original VU colour subtraction, scalar rate delta.
void Player::HudStartBlink(int id,FlailerBitmapChunk bitmap,const std::array<float,4>& colour,const std::array<float,4>& end,float minimum,float maximum){const auto animation=::port::race::detail::owned_hud_animation(m_,address_,id,"0x0017B980");std::array<float,4> delta;for(unsigned k=0;k<4;++k)delta[k]=::port::driving::ee_vsub_finite(end[k],colour[k],"0x0017B980");HudAnimator(m_,animation).StartBlink(bitmap,colour,delta,minimum,maximum-minimum);}
// Debug0017B1E8; E-0112-H02. Original alpha is sign-extended from char;
// alpha128 therefore becomes -128 before the source animation math.
void Player::HudStartGrowAnimation(FlailerTextChunk text,FlailerTextChunk master,std::uint32_t timer,bool bigger){const auto at=master.address();const std::array<float,2> pos{{m_.scalar(at+0x30),m_.scalar(at+0x34)}},scale{{m_.scalar(at+0x38),m_.scalar(at+0x3c)}};const int alpha=m_.signed_byte(at+0x5f);HudStartAnimation(0,text,pos,{{pos[0]+0.f,pos[1]+(bigger?-30.f:100.f)}},scale,bigger?std::array<float,2>{{2,2}}:std::array<float,2>{{.5f,.5f}},bigger?alpha:0,bigger?0:alpha,bigger?.5f:1.f,timer);}
// Debug0017B400; E-0112-H02. Only enters with the actual bound speed string.
void Player::HudStartSpeedAnimation(FlailerTextChunk text,FlailerTextChunk master,std::uint32_t timer,bool bigger){if(!m_.word(address_+0x404))return;const auto at=master.address();const std::array<float,2> pos{{m_.scalar(at+0x30),m_.scalar(at+0x34)}},scale{{m_.scalar(at+0x38),m_.scalar(at+0x3c)}};const int alpha=m_.signed_byte(at+0x5f);std::array<float,2> end{{pos[0]+0.f,pos[1]+100.f}};if(bigger){const float width=master.GetScaledWidth(),height=master.GetScaledHeight();end={{(width*.5f+width*.5f*-2.f)+pos[0],(height*.5f+height*.5f*-2.f)+pos[1]}};}HudStartAnimation(1,text,pos,end,scale,bigger?std::array<float,2>{{2,2}}:std::array<float,2>{{.5f,.5f}},alpha,0,1,timer);}
// Debug0017AF58; E-0112-H02. Original free-run/live-split/split-player layout.
void Player::HudStartWrongWay(){std::array<float,2> pos{{258,119}},end{{-105,-80}},scale{{1,1}};float endscale=m_.scalar(0x325348);if(RaceParameters(m_,0x311f78).IsFreeRun())end={{-240,-100}};if((m_.word(address_+0x2fc)&0x800200)==0x800000)end={{-95,-80}};end[0]+=258;end[1]+=119;if(m_.signed_word(0x327780)>1){scale={{.5f,.5f}};endscale=.25f;const auto number=m_.signed_word(address_+0x10);if(number==0){pos={{258,125}};end={{128,125}};}if(number==1){pos={{258,350}};end={{128,350}};}}
 const auto bitmap=FindFlailerObject(m_,m_.word(address_+0x300),0x89af1a1e,2);HudStartAnimation(15,FlailerBitmapChunk(m_,bitmap),pos,end,scale,{{endscale,endscale}},64,64,1,m_.word(0x327cf8),1);
}
}

#include "game_core/indep/src/fmenus.hpp"
namespace hp2::debug {
// Debug00174900,E-0100-PA01. Pure NIS selection; source arrays were randomized
// earlier by PredetermineNisNumbers, never shuffled or synthesized here.
int Player::FindNextNisIndex(std::uint32_t chased,std::uint32_t cop,int type){
 auto& m=m_;auto body_hash=[&](std::uint32_t car){const auto animation=m.word(car+0x4b4),character=animation?m.word(animation+0x20):0;if(!character)return std::uint32_t(0);std::string name;for(unsigned i=0;i<24;++i){const auto ch=m.byte(character+0x24+i);if(!ch)return hp2::frontend::bStringHash(name.c_str());name.push_back(char(ch));}hp2::not_implemented("0x00174900","original CharacterInfo body name lacks a bound terminator");};
 const auto player_hash=body_hash(chased),other_hash=body_hash(cop);int index=m.signed_word(address_+(type==2?0xe4:0xe8));const auto numbers=address_+(type==2?0x1f4:0xf4);const auto other=m.word(0x327778)==address_?m.word(0x32777c):m.word(0x327778);const auto other_manager=other?m.word(other+0xf0):0;const int other_number=other_manager?NisManager(m,other_manager).GetCurrentNisNumber():-1;int attempts=-1;
 do{if(index==0x7fffffff)hp2::not_implemented("0x00174900","NIS source cyclic index outside reviewed signed increment domain");++index;const int count=GetNumNisInfo(m,type);if(count>32)hp2::not_implemented("0x00174900","NIS source selection exceeds original32-number array");if(count<=index)index=0;++attempts;if(attempts>=0x21)break;if(index<0||index>=32)hp2::not_implemented("0x00174900","NIS source selection index lacks original table memory");}while(!IsNisSuitable(m,m.signed_word(numbers+4*unsigned(index)),player_hash,other_hash,other_number));return index;
}
// Debug00174A40,E-0100-PA01. Preserve chosen source index; optional PC policy
// disables only the subsequent NisManager.Load001AEB18 resource operation.
void Player::LoadNextNis(std::uint32_t chased,std::uint32_t cop,int tickets,bool disable_asset_load){auto& m=m_;const bool arrest=!RaceParameters(m,0x311f78).IsFreeRun()&&m.signed_word(0x3121f4)<=tickets;const int index=FindNextNisIndex(chased,cop,arrest?2:1);m.word(address_+(arrest?0xe4:0xe8),std::uint32_t(index));if(disable_asset_load){::port::diagnostics::warn("0x001AEB18","pursuit NIS/CAM preload disabled by explicit PC presentation policy","retaining original NIS suitability selection and stored index; actual capture Start still requires real NIS/CAM load");return;}NisManager(m,m.word(address_+0xf0)).Load(m.signed_word(address_+(arrest?0x1f4:0xf4)+unsigned(index)*4));}
}

#include "game_core/port/race/player_hud_presentation.inc"

// Debug001803A0; E-0112-P04. Original loop repeats only this byte store.
namespace hp2::debug {void SwitchRearViewMirrorFromPauseMenu(::port::driving::Memory& m,int state){for(int i=0;i<m.signed_word(0x327780);++i)m.byte(0x312264,state!=0);}}
#include "game_core/port/race/nis_services.hpp"
#include "game_core/port/race/physics_construction.hpp"
namespace hp2::debug {
// Debug00174B60. Native source arguments plus an optional explicit port
// dependency identity; every transition uses the actual same-Memory owner.
int Player::PlayNextNis(std::uint32_t ticketed,std::uint32_t cop,std::uint32_t callback,int parameter,const ::port::race::PhysicsConstructionServices* supplied){
 auto& m=m_;const auto a=address_;auto& services=::port::race::require_nis_services(m);if(supplied&&services.physics!=supplied)hp2::not_implemented("0x00174B60","capture NIS physics owner differs from current native service scope");m.word(a,callback);m.word(a+0xec,unsigned(parameter));const auto manager=m.word(0x328c1c);if(!manager)hp2::not_implemented("0x00174B60","actual CopManager required by capture NIS");const int tickets=tCopManager(m,manager).GetNumTickets(ticketed);ClearZone(true);if(const auto player=m.word(cop+0x18))::port::race::RadialBlurOff(m,m.signed_word(player+0x10));hp2::recovered::Car(m,ticketed).StopBlinking();m.word(0x326d3c,1);for(auto field:{0xe4u,0xe8u})if(m.signed_word(a+field)<0)m.word(a+field,0);
 const auto other=m.word(0x327778)==a?m.word(0x32777c):m.word(0x327778);int other_camera=-1;if(other)if(const auto nis=m.word(other+0xf0))if(m.word(nis+0x40))other_camera=m.signed_word(nis+0x50);
 const bool ticket=RaceParameters(m,0x311f78).IsFreeRun()||tickets<m.signed_word(0x3121f4);
 // MIPS00174C40..68 explicitly indexes BOTH arrest arrays with TicketNisIndex.
 const int index=m.signed_word(a+0xe8);if(index<0||index>=32)hp2::not_implemented("0x00174B60","original selected NIS index exceeds32 source slots");const auto numbers=a+(ticket?0xf4:0x1f4),cameras=a+(ticket?0x174:0x274);const int number=m.signed_word(numbers+4*unsigned(index));int camera=m.signed_word(cameras+4*unsigned(index));if(camera==other_camera){const auto info=GetNisInfo(m,number);if(!info||!m.word(info+0x30))hp2::not_implemented("0x00174B60","original NIS camera modulo trap: no authored cameras");camera=(camera+1)%m.signed_word(info+0x30);}const auto nis=m.word(a+0xf0);NisManager(m,nis).Start(number,camera,ticketed,cop);if(ticket)NISOverlayManager(m,a+0x9f0).StartTicket();else NISOverlayManager(m,a+0x9f0).StartArrest();const auto state=m.word(m.word(0x327290));if(state!=4&&state!=7&&state!=5&&state!=9){ChangeCameraMover(0,false);m.word(a+0xdc,NisManager(m,nis).CreateNisCameraCoordinator(m.signed_word(a+0x14)));}return 1;
}
}
namespace hp2::debug {
void Player::ClearZoneSound(){auto& m=m_;if(m.word(0x326f40))if(const auto sample=m.word(address_+0x88)){const auto table=m.word(sample+0xb8),target=m.word(table+0xc);if(target!=0x26b3f8)hp2::not_implemented("0x00176C80","derived zone-sample destructor target not reconstructed");bSample(m,sample+std::int32_t(m.signed_half(table+8))).Sub_0026B3F8(3);m.word(address_+0x88,0);}}
void Player::RestoreCameraMover(){auto& m=m_;const auto a=address_;if(const auto current=m.word(a+0xc4))::port::race::delete_player_camera(m,current);const auto saved=m.word(a+0xc8);m.word(a+0xc4,saved);m.word(a+0xc8,m.word(a+0xcc));m.word(a+0xcc,0);if(saved&&m.word(saved+8)==1)m.scalar(saved+0x7c,m.word(a+0x2f8)?1.f:0.f);if(!saved)hp2::not_implemented("0x00175198","source restored camera virtual call has null owner");const auto table=m.word(saved+0x28);if(m.word(table+0x54)!=0x1572d8)hp2::not_implemented("0x00175198","source restored camera Enable virtual target not reconstructed");CameraMover(m,saved+std::int32_t(m.signed_half(table+0x50))).Enable();}
void Player::ClearZone(bool sound){auto& m=m_;const auto a=address_;const int zone=m.signed_word(a+0x54);if(!zone)return;::port::race::RadialBlurOff(m,m.signed_word(a+0x10));if(zone==1||zone==2||zone==6||zone==7){if((zone==1||zone==2)&&!m.word(0x3272f4))m.word(a+0x64,m.word(0x327cf8));const auto rc=m.word(0x327290);const auto state=m.word(rc);if(state!=4&&state!=7&&state!=5&&state!=9)RestoreCameraMover();}World(m,m.word(0x3272f0)).ResetTimeScale();m.word(a+0x54,0);m.word(a+0x58,unsigned(zone));m.word(a+0x8c,0);m.word(a+0x90,0);if(sound)ClearZoneSound();}
void NISOverlayManager::UpdateDateAndTime(){auto& m=m_;auto& services=::port::race::require_nis_services(m);if(!services.read_local_clock)hp2::not_implemented("0x00180D70","actual RTC/local-time device provider is required");const auto value=services.read_local_clock();if(!value)return;const int month=value->month>=1&&value->month<=12?value->month:1;const auto translated=::port::race::hud_detail::string(m,GetTranslatedString(m,0xe86c+month-1));const auto date_format=::port::race::hud_detail::string(m,0x2abde0),time_format=::port::race::hud_detail::string(m,0x2abdf0);char buffer[128];int n=std::snprintf(buffer,sizeof buffer,date_format.c_str(),translated.c_str(),value->day,value->year);if(n<0||n>=128)hp2::not_implemented("0x00180D70","source RTC date formatting overflow");::port::race::hud_detail::text(m,FindFlailerObject(m,m.word(address_+4),0x1450dd,1),buffer);n=std::snprintf(buffer,sizeof buffer,time_format.c_str(),value->hour,value->minute,value->second);if(n<0||n>=128)hp2::not_implemented("0x00180D70","source RTC clock formatting overflow");::port::race::hud_detail::text(m,FindFlailerObject(m,m.word(address_+4),0x1d380e,1),buffer);m.word(address_,m.word(0x327ce0));}
void NISOverlayManager::Start(){auto& m=m_;const auto a=address_,screen=m.word(a+4);if(!screen)return;m.word(a,0);UpdateDateAndTime();SetFlailerScreenVisibility(m,screen,0);const auto display=FindFlailerObject(m,screen,0x76494c5b,1),score=FindFlailerObject(m,screen,0x233dfbe1,1);m.word(a+0x14,display);m.word(a+0x18,score);if(!display)hp2::not_implemented("0x00180AE0","actual NIS overlay BustedDisplay chunk is absent");m.scalar(a+0x1c,m.scalar(display+0x30));m.scalar(a+0x20,m.scalar(display+0x34));m.scalar(a+0x24,m.scalar(display+0x38));m.scalar(a+0x28,m.scalar(display+0x3c));const auto scale=m.word(a+0xc),alpha=m.word(a+0x10);auto setup=[&](std::uint32_t cubic,float start,float finish){m.half(cubic+0x2e,0);m.scalar(cubic+0x24,m.scalar(0x32789c));m.scalar(cubic+8,start);if(start!=m.scalar(cubic))m.half(cubic+0x2c,2);m.scalar(cubic,m.scalar(cubic+8));m.scalar(cubic+4,m.scalar(cubic+12));m.half(cubic+0x2c,0);m.scalar(cubic+8,finish);if(finish!=start)m.half(cubic+0x2c,2);};setup(scale,m.scalar(0x327890),m.scalar(a+0x24));setup(alpha,m.scalar(0x327894),m.scalar(0x327898));}
void NISOverlayManager::StartTicket(){m_.word(address_+8,1);Start();} //00181850.
void NISOverlayManager::StartArrest(){m_.word(address_+8,2);Start();} //00181840.
void NISOverlayManager::Finish(){if(const auto screen=m_.word(address_+4))SetFlailerScreenVisibility(m_,screen,1);} //00180F88.
}
