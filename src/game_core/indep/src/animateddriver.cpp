// TASK-0099 E-0099-W28. Exact original ..\indep\src\animateddriver.cpp source
// ownership corroborated by read-only MCP FDR/SYMR/source-marker/string captures.
#include "game_core/indep/inc/AnimatedDriver.hpp"
#include "game_core/indep/inc/CameraMover.hpp"
#include "game_core/indep/inc/Car.hpp"
#include "game_core/eAnimation.hpp"
#include "game_core/port/race/character_resources.hpp"
#include "game_core/port/race/car_render_resources.hpp"
#include "game_core/indep/src/fmenus.hpp"
#include "game_core/inc/bSlotPool.hpp"
#include "game_core/port/not_implemented.hpp"
#include <stdexcept>
namespace hp2::debug {
// Debug001A67C8, E-0099-W28. Original AnimationInfoTable stride24hex.
void AnimationPlayer::BeginAnimation(DriverAnimationType type,int begin){
 if(int(type)<DRIVER_ANIMATION_NUM_TYPES){const auto info=0x308908+std::uint32_t(type)*0x24;const auto start=m_.word(0x327d00)+std::uint32_t(begin);
  m_.word(address_+4,start);m_.word(address_,info);m_.word(address_+8,start+m_.word(info+0x10)*2);const auto pause=m_.word(info+0x14);m_.word(address_+0xc,pause);if(pause)m_.word(address_+8,start+100000);}
}
// Debug001A6B90, E-0099-W28. Only original initialized cubic fields are written.
void AnimatedDriver::SetupSteeringFrame(){const auto cubic=m_.allocate(0x30);m_.scalar(cubic+0xc,0);const auto duration=m_.scalar(0x3257b8);
 m_.word(address_+0x1c,cubic);m_.scalar(cubic+0x28,1);m_.half(cubic+0x2e,1);m_.scalar(cubic+0x24,duration);
 m_.scalar(cubic,30);m_.scalar(cubic+4,m_.scalar(cubic+0xc));m_.half(cubic+0x2c,0);m_.scalar(cubic+8,30);}
// Debug001A6CF0, E-0099-W28. Preserve real DriverInfo character pointer.
void AnimatedDriver::Sub_001A6CF0(std::uint32_t car,const std::string&){
 AnimationPlayer(m_,address_+0x70).BeginAnimation(DRIVER_ANIMATION_TYPE_NONE,0);AnimationPlayer(m_,address_+0x80).BeginAnimation(DRIVER_ANIMATION_TYPE_NONE,0);
 m_.byte(address_+0x68,0);m_.byte(address_+0x69,0);m_.word(address_,car);const auto driver=m_.word(car+0x14);
 m_.word(address_+0x60,0);m_.word(address_+0x64,0);m_.word(address_+0x20,0);m_.word(address_+0x6c,DRIVER_ANIMATION_TYPE_NONE);
 if(driver)if(auto character=m_.word(driver+0x14))m_.word(address_+0x20,character);
 for(unsigned i=0;i<3;++i){m_.word(address_+4+4*i,0);m_.word(address_+0x10+4*i,0);}
 InitializeArtwork();SetupSteeringFrame();
}
// Debug001A6F18, E-0099-W28. Actual character/body type and real model/animation
// trees are consumed; a missing source binding never forces a null character.
void AnimatedDriver::InitializeArtwork(){
 for(unsigned i=0;i<3;++i){if(auto animation=m_.word(address_+4+4*i))eAnimation(m_,animation).UnInit();if(auto model=m_.word(address_+0x10+4*i))::port::race::eModel(m_,model).UnInit();}
 const auto character=m_.word(address_+0x20);if(!character)return;
 if(!m_.word(0x328434))hp2::not_implemented("0x001D6030","original character body-type source table is not loaded");
 const auto body=GetBodyTypeInfo(m_,character+0x24);if(!body)return;
 const int body_skin=m_.signed_word(character+0x3c),head_skin=m_.signed_word(character+0x40);
 if(body_skin<0 || body_skin>=16 || head_skin<0 || head_skin>=16)throw std::out_of_range("Original character skin index outside recorded BodyTypeInfo arrays");
 m_.word(address_+0x24,m_.word(body+0x24));m_.word(address_+0x28,m_.word(body+0x34+4*std::uint32_t(body_skin)));
 m_.word(address_+0x40,m_.word(body+0x28));m_.word(address_+0x44,m_.word(body+0x74+4*std::uint32_t(head_skin)));
 int instance=0;
 for(unsigned i=0;i<3;++i){const auto hash=m_.word(body+0x18+4*i);if(!hash)continue;const auto temporary=m_.allocate(24);
  m_.word(temporary+8,0);m_.word(temporary+0xc,0);::port::race::eModel(m_,temporary).Init(hash);const auto solid=m_.word(temporary+0xc);
  if(solid){if(m_.signed_word(solid+0xa0)>0){auto animation=m_.word(address_+4+4*i);
    if(!animation){animation=m_.allocate(0x50);eAnimation(m_,animation).Sub_001A3FD8();m_.word(address_+4+4*i,animation);}
    eAnimation(m_,animation).Init(hash);::port::race::eModel(m_,animation+8).AttachReplacementTextureTable(address_+0x24,2,instance);
   }else{auto model=m_.word(address_+0x10+4*i);if(!model){model=bOMalloc(m_,m_.word(0x326700));m_.word(address_+0x10+4*i,model);m_.word(model+8,0);m_.word(model+0xc,0);}
    ::port::race::eModel(m_,model).Init(hash);::port::race::eModel(m_,model).AttachReplacementTextureTable(address_+0x24,2,instance);}
   ++instance;
  }
  ::port::race::eModel(m_,temporary).UnInit();m_.release(temporary);
 }
 m_.word(address_+0x5c,hp2::frontend::bStringHash("DRIVER_SHIFT01"));
}
// Debug001A6DE8, E-0099-W28. Original construction-state model/animation cleanup.
void AnimatedDriver::Sub_001A6DE8(int flags){
 for(unsigned i=0;i<3;++i){if(auto animation=m_.word(address_+4+4*i))eAnimation(m_,animation).Sub_001A4030(3);
  if(auto model=m_.word(address_+0x10+4*i)){::port::race::eModel(m_,model).UnInit();SlotPool(m_,m_.word(0x326700)).Free(model);}}
 m_.release(m_.word(address_+0x1c));if(flags&1)m_.release(address_);
}
}

namespace hp2::debug {
// Debug001A66A8, E-0099-W28. Exact21/2/8 static blacklist rows, current view
// mode and original ConvertableFlag/character prefix. No generated blacklist.
int IsThisAnimationBlackballed(::port::driving::Memory& m,DriverAnimationType type,int car_type,std::uint32_t character){for(unsigned i=0;i<21;++i){const auto row=0x308b38+i*8;if(m.signed_word(row)==int(type)&&m.signed_word(row+4)==car_type)return 1;}
 const auto info=m.word(0x3278e0);if(::port::race::eGetCurrentViewMode(m)!=1&&!m.byte(info+std::uint32_t(car_type)*0x560+0xe7)){for(unsigned i=0;i<2;++i)if(m.signed_word(0x308be0+i*8)==int(type))return 1;}
 if(character&&m.half(character+0x24)==0x6f43){for(unsigned i=0;i<8;++i){const auto row=0x308bf0+i*8;if(m.signed_word(row)==int(type)&&m.signed_word(row+4)==car_type)return 1;}}return 0;
}
// Debug001A7DD0/001A7240, E-0099-W28. Cancel through actual NONE table row.
void AnimationPlayer::CancelAnimation(){BeginAnimation(DRIVER_ANIMATION_TYPE_NONE,0);}
void AnimatedDriver::CancelAllAnimations(){for(unsigned i=0;i<2;++i)AnimationPlayer(m_,address_+0x70+i*16).CancelAnimation();}
// Debug001A7608, E-0099-W28. Preserve last-free choice and largest remaining
// delay; a pending interval greater than10000 rejects queueing immediately.
int AnimatedDriver::QueueAnimation(DriverAnimationType type){const auto car=m_.word(address_);if(car&&IsThisAnimationBlackballed(m_,type,m_.signed_word(m_.word(car+0x10)),m_.word(address_+0x20)))return 0;int delay=0,free=-1;
 for(int i=0;i<2;++i){const auto remaining=std::int32_t(m_.word(address_+0x78+std::uint32_t(i)*16)-m_.word(0x327d00));if(remaining>0){if(remaining>10000)return 0;if(delay<remaining)delay=remaining;}else free=i;}
 if(free<0)return 0;AnimationPlayer(m_,address_+0x70+std::uint32_t(free)*16).BeginAnimation(type,delay);return 1;
}
// Debug001A7538, E-0099-W28. Active interval is Start<=WorldTime<End. A zero
// allow threshold or priority<=threshold blocks interruption before any cancel.
void AnimatedDriver::BeginCollision(DriverAnimationType type,int priority){const auto now=m_.signed_word(0x327d00);for(unsigned i=0;i<2;++i){const auto player=address_+0x70+i*16;if(now<m_.signed_word(player+8)&&!(now<m_.signed_word(player+4))){const auto threshold=m_.signed_word(m_.word(player)+0x18);if(!threshold||priority<=threshold)return;}}
 CancelAllAnimations();const auto car=m_.word(address_);const auto blocked=IsThisAnimationBlackballed(m_,type,m_.signed_word(m_.word(car+0x10)),m_.word(address_+0x20));QueueAnimation(blocked?DRIVER_ANIMATION_TYPE_COLLISION_LIGHT:type);
}
}

namespace hp2::debug {
// TASK-0099 E-0099-W43. Exact raw animation intervals and byte flag; no
// rendering or host clock substitutes for original WorldTime.
// Debug001A68E8, E-0099-W43.
int AnimationPlayer::GetFrameNumber() const {
 const auto info=m_.word(address_);const auto pause=m_.signed_word(address_+0xc);
 auto frame=std::int32_t(m_.word(info+0xc)+std::uint32_t(std::int32_t(m_.word(0x327d00)-m_.word(address_+4))/2));
 if(pause==-1){const auto count=m_.signed_word(info+0x10);if(frame>=count){if(count<=0)hp2::not_implemented("0x001A68E8","nonpositive looping animation frame count");do{frame-=count;}while(frame>=count);}}
 else if(pause&&pause<frame)frame=pause;return frame;
}
// Debug001A6830, E-0099-W43.
void AnimationPlayer::ReleasePausedFrame(){const auto frame=GetFrameNumber();const auto info=m_.word(address_);const auto count=m_.word(info+0x10);const auto end=m_.word(0x327d00)+(m_.word(info+0xc)+count-std::uint32_t(frame))*2;m_.word(address_+8,end);m_.word(address_+0xc,0);m_.word(address_+4,end-count*2);}
// Debug001A6890, E-0099-W43.
void AnimationPlayer::ReleaseAnimation(){const auto now=m_.signed_word(0x327d00);if(!(now<m_.signed_word(address_+8)&&m_.signed_word(address_+4)<=now)){CancelAnimation();return;}if(std::int32_t(m_.word(address_+8)-std::uint32_t(now))>0&&m_.word(address_+0xc))ReleasePausedFrame();}
// Debug001A72C0, E-0099-W43.
void AnimatedDriver::BeginGearChange(){QueueAnimation(DRIVER_ANIMATION_TYPE_GEAR_CHANGE);}
// Debug001A72C8, E-0099-W43.
void AnimatedDriver::BeginDrivingBackwards(){if(!m_.byte(address_+0x69)){m_.byte(address_+0x69,1);HandleDrivingBackwards();}}
// Debug001A72E8, E-0099-W43.
void AnimatedDriver::EndDrivingBackwards(){if(m_.byte(address_+0x69)){m_.byte(address_+0x69,0);ReleaseAnimation(DRIVER_ANIMATION_TYPE_DRIVING_BACKWARDS);}}
// Debug001A7310, E-0099-W43.
void AnimatedDriver::HandleDrivingBackwards(){if(!FindAnimation(DRIVER_ANIMATION_TYPE_DRIVING_BACKWARDS))QueueAnimation(DRIVER_ANIMATION_TYPE_DRIVING_BACKWARDS);}
// Debug001A7350/001A7378/001A73C8; E-0112-C01. Actual shared animation queues.
void AnimatedDriver::BeginLookingBackwards(){if(!m_.byte(address_+0x68)){m_.byte(address_+0x68,1);m_.word(address_+0x6c,DRIVER_ANIMATION_TYPE_LOOKING_BACKWARDS_RIGHT);HandleLookingBackwards();}}
void AnimatedDriver::EndLookingBackwards(){if(m_.byte(address_+0x68)){m_.byte(address_+0x68,0);ReleaseAnimation(DRIVER_ANIMATION_TYPE_LOOKING_BACKWARDS_RIGHT);ReleaseAnimation(DRIVER_ANIMATION_TYPE_LOOKING_BACKWARDS_LEFT);}}
void AnimatedDriver::HandleLookingBackwards(){
 auto type=static_cast<DriverAnimationType>(m_.signed_word(address_+0x6c));const auto car=m_.word(address_);
 if(car){bool slow=false;const auto follow=GetLookbackCar(m_,car,slow);if(!follow){const auto steer=hp2::recovered::Car(m_,car).GetSteering();if(steer>=0x71d&&steer<=0x7fff)type=DRIVER_ANIMATION_TYPE_LOOKING_BACKWARDS_LEFT;else if(steer>=0x8001&&steer<=0xf8e2)type=DRIVER_ANIMATION_TYPE_LOOKING_BACKWARDS_RIGHT;}
 else{const auto angle=CalculateLookbackAngle(m_,car,follow,m_.matrix(car+0x90));if(!angle)type=DRIVER_ANIMATION_TYPE_NONE;else if(angle==0x8000||angle==0x4000)type=DRIVER_ANIMATION_TYPE_LOOKING_BACKWARDS_RIGHT;else if(angle==0xc000)type=DRIVER_ANIMATION_TYPE_LOOKING_BACKWARDS_LEFT;}}
 if(type!=DRIVER_ANIMATION_TYPE_LOOKING_BACKWARDS_LEFT)ReleaseAnimation(DRIVER_ANIMATION_TYPE_LOOKING_BACKWARDS_LEFT);
 if(type!=DRIVER_ANIMATION_TYPE_LOOKING_BACKWARDS_RIGHT)ReleaseAnimation(DRIVER_ANIMATION_TYPE_LOOKING_BACKWARDS_RIGHT);
 if(type!=DRIVER_ANIMATION_TYPE_NONE&&!FindAnimation(type))QueueAnimation(type);
 m_.word(address_+0x6c,static_cast<std::uint32_t>(type));
}
// Debug001A7700, E-0099-W43.
std::uint32_t AnimatedDriver::FindAnimation(DriverAnimationType type) const {for(unsigned i=0;i<2;++i){const auto player=address_+0x70+i*16;if(m_.signed_word(m_.word(player))==int(type)&&std::int32_t(m_.word(player+8)-m_.word(0x327d00))>=1)return player;}return 0;}
// Debug001A7750, E-0099-W43.
void AnimatedDriver::ReleaseAnimation(DriverAnimationType type){for(unsigned i=0;i<2;++i){const auto player=address_+0x70+i*16;if(m_.signed_word(m_.word(player))==int(type))AnimationPlayer(m_,player).ReleaseAnimation();}}
}

#include "game_core/indep/inc/Replay.hpp"
namespace hp2::debug {
// Debug001A6EA0; E-0099-W74; replay-frame/001a6ea0.json/type-AnimatedDriver.json.
void AnimatedDriver::DoSnapshot(std::uint32_t snapshot){
 ReplaySnapshot s(m_,snapshot);s.BeginSection(0x2add20);if(m_.byte(snapshot+0x3d))CancelAllAnimations();
 s.MoveData(address_+0x68,1);s.MoveData(address_+0x69,1);s.EndSection();
}
}
