// TASK-0042 E-0042-S001: exact Debug source marker(s): ..\indep\src\frontendscreens\pausemenu.cpp.
// Older placement comments below describe the historical host partitions; no retail ABI is asserted.
// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/PauseAudioOpts.hpp"

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/PauseDisplayOpts.hpp"

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/PauseMenu.hpp"

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/PauseOptions.hpp"

// TASK-0028. SLUS_203.62, executable revision unknown; SHA-256 7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2.
// Class spelling: allocator_label; evidence: E-0027-C01.
// Placement: user_requested_root; port-side placement; original path unknown (TASK-0029).
// No original layout, inheritance, exclusive ownership or PS2 ABI is asserted.
// Allocation extent is not sizeof. Implicit C++ lifetime is port-side only.
#include "game_core/PauseRaceObjective.hpp"

#include "game_core/port/not_implemented.hpp"

namespace hp2::recovered {
// ELF 0x001715C0; E-0041-M01; E-0042-S001; E-0043-M01; reverse/functions/sha256-7335b8ed479481082c648958f4b5be4da65b74d042d0ab2f5237df801eafacc2/reference/001715c0.md.
void PauseAudioOpts::FindSlider() {
    hp2::not_implemented("0x001715C0", "PauseAudioOpts::FindSlider (signature and full body unresolved)");
}
} // namespace hp2::recovered

// TASK-0099 E-0099-W62, exact original source owner; Debug authority
//815b60c7f4731ce982805a51fff23858c0ee85be4f6bd4faef5230567f332f1b.
#include "game_core/PauseMenu.hpp"
#include "feanim.hpp"
namespace hp2::debug {
void PauseMenu::Sub_0016F660(){
 tHighlightAnimation(m_,address_+0x18).Sub_00241F40(0x605d2bf1,static_cast<int>(0xab671c9a),m_.signed_word(0x328a28));
 m_.word(address_+0x10,0xffffffff);m_.word(address_,0);m_.word(address_+4,0);
 m_.word(address_+8,0xffffffff);m_.word(address_+0xc,0xffffffff);m_.word(address_+0x14,0);
}
}
namespace hp2::debug {
void PauseMenu::Sub_0016F6C0(int flags){tHighlightAnimation(m_,address_+0x18).Sub_00241F80(2);if(flags&1)m_.release(address_);}
}

#include "game_core/PauseMenu.hpp"
#include "game_core/indep/inc/Replay.hpp"
#include "game_core/indep/inc/Player.hpp"
#include "game_core/indep/inc/RealDriver.hpp"
#include "game_core/indep/src/aiutil.hpp"
#include "game_core/port/race/damage_resources.hpp"
#include "game_core/port/not_implemented.hpp"
namespace hp2::debug {
// Debug0016F4D8; E-0099-W81; replay-events/0016f4d8.json/type-ReplaySpecialEventInfo.json.
// Actual pause-menu consumer shared by ReplayManager's tail-call thunk.
int HandlePauseMenuReplaySpecialEvents(::port::driving::Memory& m,std::uint32_t info){
 const auto event=m.word(info);const auto param1=m.signed_word(info+8),param2=m.signed_word(info+12);
 const auto player=[&](){if(param1<0||param1>=2)hp2::not_implemented("0x0016F4D8","Replay event player number outside original two slots");const auto p=m.word(0x327770+4*param1);if(!p)hp2::not_implemented("0x0016F4D8","Replay event requires actual Player owner");return p;};
 if(event==REPLAY_SPECIAL_EVENT_FORCE_RACE_FINISH){const auto race=m.word(0x327258);if(!race)hp2::not_implemented("0x0016F4D8","Force-finish event requires actual Race owner");m.word(race+0x64c,static_cast<std::uint32_t>(param2));m.word(race+0x648,static_cast<std::uint32_t>(param1));return 1;}
 if(event==REPLAY_SPECIAL_EVENT_DRIVER_MODE){const auto p=player(),car=m.word(p+4);if(!car)hp2::not_implemented("0x0016F4D8","Driver-mode event requires Player's actual Car");m.word(car+0x3dc,static_cast<std::uint32_t>(param2));Player(m,p).SetDrivingStyle(param2);m.word(0x3121cc,static_cast<std::uint32_t>(param2));SetAIDriverModesFromPlayers(m);return 1;}
 if(event==REPLAY_SPECIAL_EVENT_DAMAGE_MODE){m.word(0x311fb8,param1!=0);int index=0;auto world=m.word(0x3272f0);if(!world)hp2::not_implemented("0x0016F4D8","Damage event requires actual World car table");while(index<m.signed_word(world+0x1c)){if(index>=64)hp2::not_implemented("0x0016F4D8","World car count exceeds source table");const auto car=m.word(world+0x2c+4*index);if(const auto damage=m.word(car+0x844)){CarDamage(m,damage).RethinkDamage();world=m.word(0x3272f0);}++index;}return 1;}
 if(event==REPLAY_SPECIAL_EVENT_TRANSMISSION_TYPE){const auto p=player(),car=m.word(p+4);if(!car)hp2::not_implemented("0x0016F4D8","Transmission event requires Player's actual Car");m.byte(p+0x1f,static_cast<std::uint8_t>(param2));hp2::recovered::RealDriver(m,m.word(car+0x2c)).SetManualTransmission(param2);return 1;}
 if(event==REPLAY_SPECIAL_EVENT_JUMP_CAM){m.byte(0x31226a,static_cast<std::uint8_t>(param1));return 1;}
 if(event==REPLAY_SPECIAL_EVENT_ZONE_CAM){m.byte(0x31226b,static_cast<std::uint8_t>(param1));return 1;}
 return 0;
}
}


#include "game_core/port/race/pause_services.hpp"
#include "game_core/indep/src/raceparameters.hpp"
#include "game_core/port/race/flailer_resources.hpp"
#include "game_core/indep/inc/Language.hpp"
namespace hp2::debug {
// Debug0016F718; E-0112-P01. Original by-number ownership, independent device
// stops and seven source screen filters; no fabricated Player or menu pointer.
void PauseMenu::EnterPause(int port,const ::port::race::PauseServices& svc){auto& m=m_;const auto a=address_;m.word(a+0x14,0);m.word(a+8,std::uint32_t(port));const auto first=m.word(0x327770),second=m.word(0x327774);if(first&&m.signed_word(first+8)==port)m.word(a+0x14,first);else if(second&&m.word(second+8)==m.word(a+8))m.word(a+0x14,second);if(first){if(!svc.stop_force)hp2::not_implemented("0x0016F718","original PortStopForce service is absent");svc.stop_force(m.signed_word(first+8));}if(second){if(!svc.stop_force)hp2::not_implemented("0x0016F718","original PortStopForce service is absent");svc.stop_force(m.signed_word(second+8));}if(!svc.set_screen_filter_port)hp2::not_implemented("0x0016F718","original SetScreenFilterPort service is absent");for(auto slot:{0x3276d8u,0x3276dcu,0x3276e0u,0x3276e4u,0x3276e8u,0x3276ecu,0x3276f0u})svc.set_screen_filter_port(m.word(slot),m.signed_word(a+8));}
// Debug0016F7F8; E-0112-P01. Options/replay ports are intentionally untouched.
void PauseMenu::ExitPause(){m_.word(address_,ePAUSE_NOT_PAUSED);m_.word(address_+8,0xffffffff);m_.word(address_+0x14,0);}
// Debug0016FC80; E-0112-P01. Actual pauser's JoyPort, not OptionsPort or
// a host's currently active input device. Null original Pauser is unsupported.
bool PauseMenu::MaybeResume(int port,const ::port::race::PauseServices& svc){if(!svc.all_joysticks_plugged)hp2::not_implemented("0x0016FC80","original AllPlayerJoysticksPluggedIn service is absent");if(!svc.all_joysticks_plugged())return false;const auto player=m_.word(address_+0x14);if(!player)hp2::not_implemented("0x0016FC80","original MaybeResume dereferences null Pauser");if(m_.signed_word(player+8)!=port)return false;if(!svc.request_unpause)hp2::not_implemented("0x0016FC80","original RaceCoordinator.RequestUnPause service is absent");svc.request_unpause(1);return true;}
// Debug0016FA30; E-0112-P01. Original restart is gated by plugged controllers;
// only source-written popup operands are carried across the native service.
void PauseMenu::MaybeRestart(const ::port::race::PauseServices& svc){if(!svc.all_joysticks_plugged)hp2::not_implemented("0x0016FA30","original AllPlayerJoysticksPluggedIn service is absent");if(!svc.all_joysticks_plugged())return;auto& m=m_;const auto a=address_;int message,title;if(RaceParameters(m,0x311f78).IsFreeRun()){message=0x3827f;title=0x38289;}else if(!m.word(0x311f78+600)&&m.word(0x311f78+0x2c)){message=0x38281;title=0x38285;}else{message=0x38271;title=0x38279;}if(!svc.current_screen||!svc.summon_popup)hp2::not_implemented("0x0016FA30","original current screen/SummonPopup services are absent");const auto parent=svc.current_screen();const auto translated=GetTranslatedString(m,message);const ::port::race::PauseConfirmation popup{parent,0x037f3133,translated,m.signed_word(a+8),3,2,1,0,title,{{0x38273,0x38274,0}},{{2,0,0}},0};svc.summon_popup(popup);m.word(a,ePAUSE_WAIT_FOR_RESTART_CONFIRMATION);if(const auto screen=FindFlailerScreen(m,m.word(a+4)))FlailerRenderManager(m,0x2e82b0).SetScreenVisibility(screen,1);}
// Debug0016FB58; E-0112-P01. Unlike restart, source quit does NOT call
// AllPlayerJoysticksPluggedIn. Preserve that observed asymmetry.
void PauseMenu::MaybeQuit(const ::port::race::PauseServices& svc){auto& m=m_;const auto a=address_;int message,title;if(RaceParameters(m,0x311f78).IsFreeRun()){message=0x3827d;title=0x38288;}else if(!m.word(0x311f78+600)){if(!m.word(0x311f78+0x2c)){message=0x38272;title=0x3827b;}else{message=0x38282;title=0x38283;}}else{message=0x3827e;title=0x38284;}const auto translated=GetTranslatedString(m,message);if(!svc.current_screen||!svc.summon_popup)hp2::not_implemented("0x0016FB58","original current screen/SummonPopup services are absent");const ::port::race::PauseConfirmation popup{svc.current_screen(),0x037f3133,translated,m.signed_word(a+8),3,2,1,0,title,{{0x38273,0x38274,0}},{{3,0,0}},0};svc.summon_popup(popup);m.word(a,ePAUSE_WAIT_FOR_QUIT_CONFIRMATION);if(const auto screen=FindFlailerScreen(m,m.word(a+4)))FlailerRenderManager(m,0x2e82b0).SetScreenVisibility(screen,1);}
}

namespace hp2::debug {
// Debug0016F810; E-0112-P01. Original mode-sensitive labels and replay,
// objective/debug item states; no generic menu labels or unlock changes.
void PauseMenu::Activate(std::uint32_t screen,const ::port::race::PauseServices& svc){auto& m=m_;const auto a=address_,replay=m.word(0x3271a8);m.word(a+4,screen);m.word(a,ePAUSE_NORMAL);if(!replay)hp2::not_implemented("0x0016F810","original PauseMenu.Activate requires actual ReplayManager");if(!svc.gray_menu_item)hp2::not_implemented("0x0016F810","original GrayMenuItem service is absent");svc.gray_menu_item(0x52e039f6,m.word(replay+0x88)!=0,screen);svc.gray_menu_item(0x52e039f7,!m.word(0x311f78+80)&&!m.word(0x311f78+0x2c)?2:1,m.word(a+4));svc.gray_menu_item(0x52e039f9,!m.word(0x326e98)&&!m.word(0x2e78e0)?1:2,m.word(a+4));int quit,resume,restart;if(RaceParameters(m,0x311f78).IsFreeRun()){quit=0x30d48;restart=0x30d4e;resume=0x30d4c;}else if(!m.word(0x311f78+600)){if(!m.word(0x311f78+0x2c)){quit=0x30d46;restart=0x30d45;resume=0x30d4f;}else{quit=0x30d52;restart=0x30d51;resume=0x30d50;}}else{quit=0x30d4a;restart=0x30d45;resume=0x30d4b;}FlailerSetLanguageID(m,int(m.word(a+4)),0x52e039f8,quit);FlailerSetLanguageID(m,int(m.word(a+4)),0x52e039f3,resume);FlailerSetLanguageID(m,int(m.word(a+4)),0x52e039f4,restart);}
}

namespace hp2::debug {
// Debug0016F9A8/0016FA28; E-0112-P02: complete instruction-proven empty bodies.
void PauseMenu::DeActivate(){}
void PauseMenu::CheckJoysticks(){}
// Debug0016FCD8; E-0112-P02, reverse/task0112/pause-integration.md.
void PauseMenu::MenuCallback(ScreenMessage msg,ScreenCallbackData& d,const ::port::race::PauseServices& svc){
 auto& m=m_;const auto a=address_;
 tHighlightAnimation(m,a+0x18).HandleEvents(msg,d);
 const auto change=[&](std::uint32_t pointer){if(!svc.change_screen)hp2::not_implemented("0x0016FCD8","original ChangeToScreen service is absent");svc.change_screen(pointer,0);};
 const auto unpause=[&](int reason){if(!svc.request_unpause)hp2::not_implemented("0x0016FCD8","original RequestUnPause service is absent");svc.request_unpause(reason);};
 const auto who=[&](){if(m.signed_word(0x327780)==2)return d.Port;const auto player=m.word(0x327778);if(!player)hp2::not_implemented("0x0016F640","ChooseWhoSelectedMenuItem requires original player0");return m.signed_word(player+8);};
 if(msg==MENUITEM_SELECT){
  if(m.word(a)!=ePAUSE_NORMAL)return;
  switch(d.ItemHash){
   case 0x52e039f3:MaybeResume(d.Port,svc);return;
   case 0x52e039f4:MaybeRestart(svc);return;
   case 0x52e039f5:m.word(a+0xc,std::uint32_t(who()));change(m.word(0x3276dc));return;
   case 0x52e039f6:m.word(a+0x10,std::uint32_t(who()));unpause(4);return;
   case 0x52e039f7:change(m.word(m.signed_word(0x311fa8)==2&&m.word(0x3121ec)?0x3276f0:0x3276ec));return;
   case 0x52e039f8:MaybeQuit(svc);return;
   case 0x52e039f9:change(m.word(0x3276e8));return;
   default:return;
  }
 }
 if(msg==SCREEN_START_BUTTON){if(MaybeResume(d.Port,svc))d.Noise=7;d.Param=1;return;}
 if(msg==SCREEN_BACK_BUTTON){d.Noise=0;d.Param=0;return;}
 if(msg==SCREEN_TICK){CheckJoysticks();return;}
 if(msg!=POPUP_DISMISS)return;
 const auto state=m.word(a);if(state!=ePAUSE_WAIT_FOR_RESTART_CONFIRMATION&&state!=ePAUSE_WAIT_FOR_QUIT_CONFIRMATION)return;
 if(d.Param){
  if(state==ePAUSE_WAIT_FOR_RESTART_CONFIRMATION){
   for(int i=0;i<m.signed_word(0x327780);++i){
    if(i>=2)hp2::not_implemented("0x0016FCD8","source player count exceeds two settings slots");
    const auto player=m.word(0x327778+4*i);const int number=m.signed_word(player+0x10);
    if(number<0||number>=2)hp2::not_implemented("0x0016FCD8","source PlayerNumber exceeds two settings slots");
    // cPlayerSettings has no padding:8 byte fields,3 words,12 name bytes,4 words.
    const auto from=player+0x1c,to=0x311f78+0x28c+48*std::uint32_t(number);
    for(unsigned j=0;j<48;++j)m.byte(to+j,m.byte(from+j));
   }
   unpause(2);
  }else unpause(3);
  return;
 }
 if(const auto screen=FindFlailerScreen(m,m.word(a+4)))FlailerRenderManager(m,0x2e82b0).SetScreenVisibility(screen,0);
 m.word(a,ePAUSE_NORMAL);
}
// Debug00170070; E-0112-P02. Wrapper initialization is distinct from item callback.
void PauseMenu::PauseMenuCallback(ScreenMessage msg,ScreenCallbackData& d,const ::port::race::PauseServices& svc){
 if(msg==SCREEN_ACTIVATE){if(m_.word(address_)==ePAUSE_NOT_PAUSED){const auto rc=m_.word(0x327290);if(!rc)hp2::not_implemented("0x00170070","original RaceCoordinator owner is absent");EnterPause(m_.signed_word(rc+0x14),svc);}Activate(d.ItemHash,svc);d.Param=0;return;}
 if(msg==SCREEN_UNACTIVATE){DeActivate();return;}
 MenuCallback(msg,d,svc);
}
}

#include "game_core/port/race/pause_submenus.hpp"
#include "game_core/indep/inc/EndGameScreens.hpp"
#include "game_core/indep/src/frontendscreens/feanim.hpp"
#include "game_core/indep/src/sound.hpp"
#include "game_core/src/bSound.hpp"
#include "game_core/ReplayManager.hpp"
#include "game_core/indep/inc/point.hpp"
#include "game_core/src/bMath.hpp"
#include "game_core/port/driving/ee_scalar.hpp"
#include <algorithm>
#include <cstdio>
#include "game_core/indep/inc/World.hpp"
namespace hp2::debug {
void SetStormType(int){} //Debug0016F4B8; E-0112-P04.
int GetStormType(){return 1;} //Debug0016F4C0, literal delay-slot instruction.
std::uint32_t GetStormTypeString(int){return 0x2aaf50;} //Debug0016F4C8.
}
namespace port::race {
// E-0112-P03. Port lifetime/state carrier for source pause screen callbacks.
// Each behavior entry below cites its actual source address; fields are decoded
// host storage and never substitute a pointer in the original object graph.
struct PauseSubmenus::Impl {
 driving::Memory& m;std::uint32_t screen=0,player=0,highlight=0;int kind=0,display_port=0,arrow_item=0;std::uint32_t debug_menu=0;float arrow_angle=0;
 std::array<float,7> volumes{};
 struct Slider {std::uint32_t menu,bar,indicator;int category;float coefficient;hp2::debug::tCubic1D spline;};
 struct Objective {std::uint32_t screen,text,object;std::array<float,4> bounds;std::string value;};
 std::vector<Objective> objectives;
 std::vector<Slider> sliders;
 explicit Impl(driving::Memory& memory):m(memory){}
 void language(std::uint32_t hash,int id){hp2::debug::FlailerSetLanguageID(m,int(screen),int(hash),id);}
 void visibility(std::uint32_t hash,bool visible){hp2::debug::FlailerSetVisibility(m,screen,hash,visible);}
 void gray(std::uint32_t hash,int state,const PauseServices& svc){if(!svc.gray_menu_item)hp2::not_implemented("0x001716A0","original GrayMenuItem service is absent");svc.gray_menu_item(hash,state,screen);}
 std::uint32_t object(std::uint32_t hash,int type=0){return hp2::debug::FindFlailerObject(m,screen,hash,type);}
 std::uint32_t settings()const{if(!player)hp2::not_implemented("0x00170B98","original pause options dereferences absent MyPlayer");return player+0x1c;}
 void change(std::uint32_t slot,const PauseServices& svc){if(!svc.change_screen)hp2::not_implemented("0x001711F8","original ChangeToScreen service is absent");svc.change_screen(m.word(slot),0);}
 void replay(int event,int p1,int p2){const auto owner=m.word(0x3271a8);if(!owner)hp2::not_implemented("0x00170EC0","original ReplayManager owner absent");hp2::recovered::ReplayManager(m,owner).AddSpecialEvent(event,p1,p2);}
 // Debug001710A8/001710D8/00171100/00171158/00171178/001711A0/001711C8.
 void Sub_001710A8(){const auto p=settings();language(0x5deb55a3,m.byte(p+1)?0x3345d:0x3345e);language(0xb6791614,m.word(0x311fb8)?0x33459:0x3345a);language(0xa2450f0f,m.byte(p+2)?0xbfa4:0xbf9a);const int t=m.signed_byte(p+3);if(t>=0&&t<3)language(0x6db88809,t==0?0xbf7c:(t==1?0xbf86:0xbf72));language(0x1f1b628b,m.byte(0x31226a)?0x22352:0x22353);language(0x16e95f2b,m.byte(0x31226b)?0x22352:0x22353);language(0xcb3bdd8f,m.word(p+12)?0x1d4c6:0x22356);}
 // Debug00171CB0..001723B0; original language selection, not host labels.
 void Sub_00171CB0(){
  const auto p=settings(),features=m.word(p+16);const auto mode=m.signed_byte(p);const int hud_ids[]={0x33454,0x33456,0x33455,0x33469};language(0x5b472b16,mode>=0&&mode<4?hud_ids[mode]:0);
  language(0x2e931f1b,m.byte(0x312264)?0x22352:0x22353);language(0xffc3dcd2,m.word(p+8)!=1?0x22352:0x22353);const int look=m.signed_word(p+12);language(0x2e018745,look==0?0x22356:(look==1?0x22351:0));
  language(0xe8434eb4,features&0x10?0x22352:0x22353);language(0x8b788a68,features&0x100?0x22352:0x22353);language(0x5d6c141b,features&0x40?0x22352:0x22353);
  const int split=m.signed_byte(p+6);const int split_ids[]={0x2234c,0x2234d,0x2234e,0x2234f,0x22353};language(0x12b1ebd9,split>=0&&split<5?split_ids[split]:0);
  const auto map=m.signed_byte(0x312264+(m.word(0x3121ec)?4:(m.signed_word(0x327780)==1?2:3)));const int map_ids[]={0x22345,0x22346,0x22347,0x223a8,0x22348,0x22349,0x22353,0x22348,0x223a9,0x223aa};language(0x55cb09f2,map>=0&&map<10?map_ids[map]:0);
  const int board=m.signed_byte(0x312269);if(board<0||board>=3)hp2::not_implemented("0x00172320","original leaderboard language index outside three-element table");language(0x71a78ef4,m.signed_word(0x2ab148+4*unsigned(board)));
  language(0x5d2a9432,m.word(p+44)?0x22352:0x22353);language(0x4aff0df3,m.word(p+40)?0x22352:0x22353);
 }
 // Debug001719C0..00171C58,001716A0. Full source predicate gates.
 void Sub_001716A0(const PauseServices& svc){
  const bool normal=m.signed_byte(settings())==0,free=hp2::debug::RaceParameters(m,0x311f78).IsFreeRun(),cop=m.word(0x3121ec)!=0;
  const int time=m.signed_word(0x311f9c);const bool timed=time!=0&&time!=0x7fffffff;const bool trial=m.word(0x311f98)==2;const bool clock_event=!cop&&timed&&(trial||m.word(0x311fa0)!=0);
  const std::array<std::pair<std::uint32_t,bool>,8> gates{{{0x2e931f1b,normal&&m.signed_word(0x327780)==1},{0x55cb09f2,normal},{0x71a78ef4,normal&&!cop&&!free&&!clock_event},{0x8b788a68,normal},{0xe8434eb4,normal&&!cop&&!free&&!clock_event},{0x5d6c141b,normal},{0x12b1ebd9,normal&&!cop},{0x5d2a9432,normal&&!free}}};
  for(unsigned i=0;i<gates.size();++i){gray(0x52e039f4+i,gates[i].second?1:0,svc);visibility(gates[i].first,gates[i].second);}
 }
 void source_player(int port){player=0;for(unsigned i=0;i<2;++i){const auto p=m.word(0x327770+4*i);if(p&&m.signed_word(p+8)==port){player=p;language(0x18b2dbc8,int(0x334b5+i));break;}}}
 // Debug00170B98. Source options initial values and disable/visibility rules.
 void Sub_00170B98(const PauseServices& svc){const auto pause=m.word(0x3276fc);source_player(m.signed_word(pause+0xc));Sub_001710A8();const bool zones=m.signed_word(0x327780)==1&&m.word(0x3121cc)==1;if(zones){visibility(0x1f1b628b,true);visibility(0x16e95f2b,true);}else{gray(0x52e039f9,2,svc);visibility(0x1f1b628b,false);gray(0x52e039fa,2,svc);visibility(0x16e95f2b,false);}if(m.word(0x2e78e0)){gray(0x52e039f6,0,svc);visibility(0xb6791614,false);}else visibility(0xb6791614,true);}
 // Debug00171570. Display settings refer to selected pause options JoyPort.
 void Sub_00171570(const PauseServices& svc){source_player(display_port);Sub_00171CB0();Sub_001716A0(svc);language(0x52e039f7,0x222f0);}
 // Debug001701D8/0016CED8: actual source category/display coefficient and spline.
 void Sub_001701D8(){
  hp2::debug::bSound sound(m,m.word(0x329320));for(unsigned i=0;i<7;++i)volumes[i]=sound.GetCategoryVolume(i);
  const int categories[]={4,0,2,1,6};for(unsigned i=0;i<5;++i){Slider slider{0x52e039f3+i,0xa29f7a16+i,0xc866642b+0x441*i,categories[i],0,{}};const float fallback=hp2::debug::sound::GetVolumeDefault(m,categories[i]);if(fallback==0)hp2::not_implemented("0x0016CED8","zero original default category volume divisor");slider.coefficient=hp2::debug::sound::GetVolumeBalance(m)/fallback;slider.spline.duration=.25f;slider.spline.ValDesired=volumes[unsigned(categories[i])]*slider.coefficient;slider.spline.Val=slider.spline.ValDesired;sliders.push_back(slider);}Sub_00170690();
 }
 // Debug00170690. Three source AudioMode language IDs.
 void Sub_00170690(){const int mode=hp2::debug::sound::GetAudioMode(m);language(0x2932dfca,mode==0?0x1adb9:(mode==1?0x1adb8:(mode==2?0x1adbb:0)));}
 // Debug00170508: source volume application only on leaving Audio options.
 void Sub_00170508(){hp2::debug::bSound sound(m,m.word(0x329320));for(const int c:{5,4,0,6,2,3,1})sound.SetCategoryVolume(unsigned(c),volumes[unsigned(c)]);hp2::debug::sound::SaveVolumesForSpeech(m);sliders.clear();}
 // Debug00140B20/00140AC8/00141238/00141278,00144070 dimensions.
 float extent(std::uint32_t obj,bool height){if(!obj)return 0;if(m.byte(obj+0x28)==1)return height?hp2::debug::FlailerTextChunk(m,obj).GetScaledHeight():hp2::debug::FlailerTextChunk(m,obj).GetScaledWidth();if(m.byte(obj+0x28)!=2)return 0;const auto texture=hp2::debug::FlailerBitmapChunk(m,obj).GetTextureInfo();if(!texture)hp2::not_implemented("0x00141238","bitmap dimensions require original texture");return float(m.signed_half(texture+(height?0x26:0x24)))*m.scalar(obj+(height?0x3c:0x38));}
 // Debug00243D80 +00243C28/00243C60: arrows pulse/track selected text center.
 void Sub_00243D80(){arrow_angle+=10.f*m.scalar(0x327ce4);if(m.scalar(0x326228)<arrow_angle)arrow_angle-=m.scalar(0x326228);const auto target=object(std::uint32_t(arrow_item));for(auto hash:{0xd1cda594u,0x9345e4a7u})if(const auto arrow=object(hash,2)){hp2::debug::FlailerObjectChunk(m,arrow).SetVisibility(arrow_item!=0);if(arrow_item){if(!target)hp2::not_implemented("0x00243D80","source arrow target bounding box absent");const int y=::port::driving::cvt_w_s(m.scalar(target+0x34)),height=::port::driving::cvt_w_s(extent(target,true));const int center=y+height/2;const float alpha=128.f*(hp2::recovered::bSin(m,arrow_angle)+1.f)*.5f;hp2::debug::FlailerBitmapChunk(m,arrow).SetAlpha(::port::driving::cvt_w_s(alpha));hp2::debug::FlailerObjectChunk(m,arrow).SetTopLeftY(float(center)-extent(arrow,true)*.5f);}}}
 // Debug0016CFF0: source slider change is deferred (update=false in pause).
 void Sub_0016CFF0(Slider& slider,float step){const auto c=unsigned(slider.category);const float old=volumes[c],display=std::max(0.f,std::min(1.f,(old+step)*slider.coefficient));if(slider.coefficient==0)hp2::not_implemented("0x0016CFE0","zero original slider display coefficient");volumes[c]=display/slider.coefficient;hp2::debug::sound::SaveVolumesForSpeech(m);if(volumes[c]!=old)hp2::debug::sound::PlayCommonRegionSample(m,"MenuAdjustVolume",display);slider.spline.ValDesired=display;if(display!=slider.spline.Val)slider.spline.state=2;}
 // Debug0016D0E8: original cPoint spline; integer bounding-box projection.
 void Sub_0016D0E8(Slider& slider){hp2::debug::cPoint::SplineSeek(slider.spline,m.scalar(0x327ce4),0,0);const auto bar=object(slider.bar),indicator=object(slider.indicator);if(!bar||!indicator)hp2::not_implemented("0x0016D0E8","source audio slider authored bar/indicator absent");const int left=::port::driving::cvt_w_s(hp2::debug::FlailerObjectChunk(m,bar).GetTopLeft()[0]),width=::port::driving::cvt_w_s(extent(bar,false));hp2::debug::FlailerObjectChunk(m,indicator).SetTopLeftX(float(::port::driving::cvt_w_s(float(left)+slider.spline.Val*float(width))));}
 // Debug00144FD0 source lookup/SetString and actual clipping bounds. The
 // original strip output is transported as copied frame metadata, never GS data.
 void Sub_00144FD0(std::uint32_t text_hash,std::uint32_t rectangle,int who){const auto clip=object(rectangle,2);if(!clip)return;const auto text=object(text_hash,1);if(!text)hp2::not_implemented("0x00144FD0","original text-in-rectangle target is absent");const auto string=hp2::debug::FigureOutRaceObjective(m,who);if(!string)hp2::not_implemented("0x00144FD0","original objective text source is null");hp2::debug::FlailerTextChunk(m,text).SetString(string);Objective result{screen,text_hash,text,{{m.scalar(clip+0x30),m.scalar(clip+0x34),float(::port::driving::cvt_w_s(extent(clip,false))),float(::port::driving::cvt_w_s(extent(clip,true)))}},{}};for(unsigned i=0;;++i){if(i>=4096)hp2::not_implemented("0x00144CC0","objective source text exceeds bounded port observation");const auto c=m.byte(string+i);if(!c)break;result.value+=char(c);}const auto found=std::find_if(objectives.begin(),objectives.end(),[&](const auto& x){return x.screen==screen&&x.text==text_hash;});if(found==objectives.end())objectives.push_back(std::move(result));else *found=std::move(result);}
 // E-0112-P04. Original FlailerPrintf00143C60 shared buffer/SetString.
 std::string Sub_00143C60_text(std::uint32_t at)const{std::string value;for(unsigned i=0;i<4096;++i){const auto c=m.byte(at+i);if(!c)return value;value.push_back(char(c));}throw std::runtime_error("Source debug-menu literal lacks terminator");}
 template<class... A>void Sub_00143C60(std::uint32_t hash,std::uint32_t format,A... args){const auto object=hp2::debug::FindFlailerObject(m,screen,hash,1);if(!object)return;char buffer[128];const auto fmt=Sub_00143C60_text(format);const int count=std::snprintf(buffer,sizeof buffer,fmt.c_str(),args...);if(count<0||count>=128)hp2::not_implemented("0x00143C60","source debug-menu formatted text exceeds original128-byte buffer");for(int i=0;i<=count;++i)m.byte(0x32af00+unsigned(i),std::uint8_t(buffer[i]));hp2::debug::FlailerTextChunk(m,object).SetString(0x32af00);}
 void Sub_001728D8(){
  debug_menu=m.allocate(8);m.word(debug_menu,0);m.word(debug_menu+4,screen);
  hp2::debug::World(m,m.word(0x3272f0)).GetCarByNumber(0);
  const auto race=m.word(0x327258);Sub_00143C60(0x52e039f3,race&&m.word(race)==4?0x2ab1c0:0x2ab1d8,0);
  Sub_00143C60(0x52e039f4,0x2ab1f8);
  Sub_00143C60(0x52e039f5,0x2ab208,Sub_00143C60_text(m.word(0x3276f8)?0x327718:0x327720).c_str());
  Sub_00143C60(0x52e039f6,0x2ab228,Sub_00143C60_text(m.word(0x326f38)?0x327718:0x327720).c_str());
  Sub_00143C60(0x52e039f7,0x2ab238,Sub_00143C60_text(hp2::debug::GetStormTypeString(hp2::debug::GetStormType())).c_str());
  Sub_00143C60(0x52e039f9,0x2ab248,Sub_00143C60_text(m.signed_byte(0x312264)?0x327728:0x327730).c_str());
  Sub_00143C60(0x52e039fa,0x2ab260,Sub_00143C60_text(m.word(0x3276f4)?0x327728:0x327730).c_str());
  Sub_00143C60(0x52e039fb,0x2ab270,Sub_00143C60_text(m.word(0x327884)?0x327728:0x327730).c_str());Sub_00143C60(0x52e039f8,0x327738);m.word(0x327710,debug_menu);
 }
 void Sub_00172AD0(){if(!debug_menu)return;if(const auto position=m.word(debug_menu)){const auto pause=m.word(0x3276fc),pauser=m.word(pause+0x14);if(!pauser)hp2::not_implemented("0x00172AD0","original debug-menu pending finish needs actual Pauser");replay(hp2::debug::REPLAY_SPECIAL_EVENT_FORCE_RACE_FINISH,int(position),m.signed_word(pauser+0x10));}m.release(debug_menu);debug_menu=0;m.word(0x327710,0);}
 void Sub_00172C10(hp2::debug::ScreenMessage msg,hp2::debug::ScreenCallbackData& d){
  using namespace hp2::debug;const auto toggle=[&](std::uint32_t address,std::uint32_t format,std::uint32_t yes,std::uint32_t no){const auto value=m.word(address)==0;m.word(address,value);Sub_00143C60(d.ItemHash,format,Sub_00143C60_text(value?yes:no).c_str());};
  switch(d.ItemHash){
   case 0x52e039f3:{const auto race=m.word(0x327258);if(race&&m.word(race)==4){int position=m.signed_word(debug_menu)+(msg==MENUITEM_PAD_LEFT?-1:1);if(position<0)position=0;if(m.signed_word(race+0x18)<position)position=m.signed_word(race+0x18);m.word(debug_menu,unsigned(position));Sub_00143C60(d.ItemHash,0x2ab1c0,position);}break;}
   case 0x52e039f5:toggle(0x3276f8,0x2ab208,0x327718,0x327720);break;
   case 0x52e039f6:toggle(0x326f38,0x2ab228,0x327718,0x327720);break;
   case 0x52e039f7:{int type=GetStormType()-1;if(type>2)type=0;if(type<0)type=2;SetStormType(type);Sub_00143C60(d.ItemHash,0x2ab238,Sub_00143C60_text(GetStormTypeString(GetStormType())).c_str());const auto world=m.word(0x3272f0);for(auto car=m.word(world);car!=world;car=m.word(car))m.word(car+0x840,GetStormType()!=0);break;}
   case 0x52e039f9:{World(m,m.word(0x3272f0)).GetCarByNumber(0);const bool state=m.signed_byte(0x312264)==0;Sub_00143C60(d.ItemHash,state?0x2ab288:0x2ab2a0);SwitchRearViewMirrorFromPauseMenu(m,state);break;}
   case 0x52e039fa:toggle(0x3276f4,0x2ab260,0x327728,0x327730);break;
   case 0x52e039fb:toggle(0x327884,0x2ab270,0x327728,0x327730);break;
   default:break;
  }
 }
 void Sub_00172B38(hp2::debug::ScreenMessage msg,hp2::debug::ScreenCallbackData& d,const PauseServices& svc){using namespace hp2::debug;if(msg==MENUITEM_PAD_LEFT||msg==MENUITEM_PAD_RIGHT){hp2::frontend::PlayMenuSound_0016E038(1);Sub_00172C10(msg,d);}else if(msg==SCREEN_BACK_BUTTON){change(0x3276d8,svc);d.Param=0;}else if(msg==MENUITEM_SELECT&&d.ItemHash==0xaee77883)m.word(0x328a98,1);}
 void begin(std::uint32_t hash,int type,const PauseServices& svc){screen=hash;kind=type;arrow_item=0;arrow_angle=0;if(kind==4)return;if(kind==5){Sub_001728D8();return;}highlight=m.allocate(0x40);hp2::debug::tHighlightAnimation(m,highlight).Sub_00241F40(int(screen),int(0xab671c9a),m.signed_word(0x328a28));hp2::debug::tHighlightAnimation(m,highlight+0x20).Sub_00241F40(int(screen),0x1042eac3,-1);if(kind==1)Sub_00170B98(svc);else if(kind==2)Sub_00171570(svc);else if(kind==3)Sub_001701D8();}
 void finish(){if(kind==5)Sub_00172AD0();if(kind==3)Sub_00170508();if(highlight){hp2::debug::tHighlightAnimation(m,highlight).Sub_00241F80(2);hp2::debug::tHighlightAnimation(m,highlight+0x20).Sub_00241F80(2);m.release(highlight);}highlight=screen=player=0;kind=0;sliders.clear();objectives.clear();}
 // Debug001711F8: options adjust events and source replay callbacks.
 bool Sub_001711F8(hp2::debug::ScreenMessage msg,hp2::debug::ScreenCallbackData& d,const PauseServices& svc){
  using namespace hp2::debug;
  if(msg==MENUITEM_SELECT){if(d.ItemHash==0x52e039f3){change(0x3276e0,svc);return false;}if(d.ItemHash==0x52e039f4){display_port=d.Port;change(0x3276e4,svc);return false;}d.Noise=0;return false;}
  const std::uint32_t value_hashes[]={0x5deb55a3,0xb6791614,0xa2450f0f,0x6db88809,0x1f1b628b,0x16e95f2b,0xcb3bdd8f};
  if(msg==MENUITEM_HIGHLIGHT){arrow_item=d.ItemHash>=0x52e039f5&&d.ItemHash<=0x52e039fb?int(value_hashes[d.ItemHash-0x52e039f5]):0;tHighlightAnimation(m,highlight+0x20).SetCurrentMenuItem(arrow_item);return false;}
  if(msg!=MENUITEM_PAD_LEFT&&msg!=MENUITEM_PAD_RIGHT)return false;const auto p=settings();
  switch(d.ItemHash){
   case 0x52e039f5:if(!m.byte(p+1)){m.byte(p+1,1);if(!svc.rumble_impact)hp2::not_implemented("0x00170E20","original PortRumbleImpact device output absent");svc.rumble_impact(m.signed_word(player+8),8.f);}else m.byte(p+1,0);break;
   case 0x52e039f6:replay(REPLAY_SPECIAL_EVENT_DAMAGE_MODE,int(m.word(0x311fb8)^1),0);break;
   case 0x52e039f7:replay(REPLAY_SPECIAL_EVENT_DRIVER_MODE,m.signed_word(player+0x10),m.byte(p+2)==0);break;
   case 0x52e039f8:{const int v=m.signed_byte(p+3)+(msg==MENUITEM_PAD_LEFT?-1:1);replay(REPLAY_SPECIAL_EVENT_TRANSMISSION_TYPE,m.signed_word(player+0x10),(v%3+3)%3);break;}
   case 0x52e039f9:replay(REPLAY_SPECIAL_EVENT_JUMP_CAM,m.byte(0x31226a)==0,0);break;
   case 0x52e039fa:replay(REPLAY_SPECIAL_EVENT_ZONE_CAM,m.byte(0x31226b)==0,0);break;
   case 0x52e039fb:PlayerSettingsView(m,p).CycleAlternateCamera();break;
   default:return false;
  }
  Sub_001710A8();return true;
 }
 // Debug001723E0: display adjustment dispatch; source effect callbacks retained.
 bool Sub_001723E0(hp2::debug::ScreenMessage msg,hp2::debug::ScreenCallbackData& d,const PauseServices& svc){
  using namespace hp2::debug;
  const std::uint32_t values[]={0x5b472b16,0x2e931f1b,0x55cb09f2,0x71a78ef4,0x8b788a68,0xe8434eb4,0x5d6c141b,0x12b1ebd9,0x5d2a9432};
  if(msg==MENUITEM_HIGHLIGHT){arrow_item=0;if(d.ItemHash>=0x52e039f3&&d.ItemHash<=0x52e039fb)arrow_item=int(values[d.ItemHash-0x52e039f3]);else if(d.ItemHash==0xaee77883)arrow_item=int(0xffc3dcd2);else if(d.ItemHash==0xaee77884)arrow_item=0x2e018745;else if(d.ItemHash==0xaee77885)arrow_item=0x4aff0df3;tHighlightAnimation(m,highlight+0x20).SetCurrentMenuItem(arrow_item);return false;}
  if(msg==MENUITEM_SELECT){d.Noise=0;return false;}if(msg!=MENUITEM_PAD_LEFT&&msg!=MENUITEM_PAD_RIGHT)return false;
  const auto p=settings();PlayerSettingsView selected(m,p);GlobalDisplaySettingsView global(m,0x312264);const int direction=msg==MENUITEM_PAD_LEFT?-1:1;bool changed=true;
  switch(d.ItemHash){
   case 0x52e039f3:Player(m,player).CycleHud(direction);Sub_001716A0(svc);break;
   case 0x52e039f4:m.byte(0x312264,m.byte(0x312264)==0);break;
   case 0x52e039f5:changed=global.CycleTrackMap(direction);break;
   case 0x52e039f6:changed=global.CycleLeaderboardMode(direction);break;
   case 0x52e039f7:selected.ToggleLapInfo();break;
   case 0x52e039f8:selected.ToggleRacePosition();break;
   case 0x52e039f9:selected.ToggleGauges();break;
   case 0x52e039fa:{const auto old=m.byte(p+6);selected.CycleSplitTime(direction);changed=old!=m.byte(p+6);break;}
   case 0x52e039fb:selected.ToggleScoring();break;
   case 0xaee77883:{selected.ToggleCameraSpring();const auto mover=m.word(player+0xc4);if(mover&&m.word(mover+8)==1)m.word(mover+0x374,m.word(p+8));break;}
   case 0xaee77884:{selected.ToggleLookback();const auto mover=m.word(player+0xc4);if(mover&&m.word(mover+8)==1)m.scalar(mover+0x378,m.scalar(0x3252d4));break;}
   case 0xaee77885:selected.ToggleHudHotkey();break;
   default:changed=false;
  }
  Sub_00171CB0();if(!changed)d.Noise=0;return changed;
 }
 // Debug001706D0,00170608: audio sliders/mode source event dispatch.
 bool Sub_001706D0(hp2::debug::ScreenMessage msg,hp2::debug::ScreenCallbackData& d){using namespace hp2::debug;if(msg==MENUITEM_SELECT){d.Noise=0;return false;}if(msg==MENUITEM_HIGHLIGHT){arrow_item=d.ItemHash==0x52e039f8?0x2932dfca:0;if(arrow_item)tHighlightAnimation(m,highlight+0x20).SetCurrentMenuItem(arrow_item);return false;}if(msg!=MENUITEM_PAD_LEFT&&msg!=MENUITEM_PAD_RIGHT)return false;if(d.ItemHash==0x52e039f8){const int v=(sound::GetAudioMode(m)+(msg==MENUITEM_PAD_LEFT?-1:1))%3;sound::SetAudioMode(m,v<0?2:v);Sub_00170690();return true;}for(auto& slider:sliders)if(slider.menu==d.ItemHash){Sub_0016CFF0(slider,m.scalar(0x3286e8)*(msg==MENUITEM_PAD_LEFT?-1.f:1.f));break;}return false;}
};
PauseSubmenus::PauseSubmenus(driving::Memory& m):impl_(std::make_unique<Impl>(m)){}
PauseSubmenus::~PauseSubmenus(){if(impl_&&impl_->highlight)impl_->m.release(impl_->highlight);}
void PauseSubmenus::close(const PauseServices&){impl_->finish();}
// Source callback wrappers001714C0/00172830/001708A8/00170B00 with host ownership.
bool PauseSubmenus::dispatch(std::uint32_t screen,hp2::debug::ScreenMessage msg,hp2::debug::ScreenCallbackData& d,const PauseServices& svc){
 using namespace hp2::debug;auto& p=*impl_;auto& m=p.m;int kind=0;
 // _GLOBAL__I_PAUSE_MAIN001734A0 registers logical names separately from resource hashes.
 if(screen==0x2b5a03a8)kind=1;else if(screen==0xcc36fab2)kind=2;else if(screen==0x6b321e4e)kind=3;else if(screen==0x887b4d37||screen==0xb5ecd7a0)kind=4;else if(screen==0xd4742cd7)kind=5;else return false;
 if(msg==SCREEN_ACTIVATE){if(p.screen)p.finish();p.begin(screen,kind,svc);return true;}
 if(msg==SCREEN_UNACTIVATE){if(kind==3)d.Noise=6;p.finish();return true;}
 if(kind==5){if(!p.debug_menu||p.screen!=screen)hp2::not_implemented("0x00172B38","original debug pause callback reached before activation");p.Sub_00172B38(msg,d,svc);return true;}
 if(kind==4){
  // Debug00170980. Source objective rendering happens on SCREEN_RENDER only.
  if(msg==SCREEN_START_BUTTON){if(PauseMenu(m,m.word(0x3276fc)).MaybeResume(d.Port,svc)){d.Param=1;d.Noise=7;}}
  else if(msg==MENUITEM_SELECT||msg==SCREEN_BACK_BUTTON){p.change(0x3276d8,svc);d.Param=0;}
  else if(msg==SCREEN_RENDER){if(screen==0xb5ecd7a0){p.Sub_00144FD0(0x8ec51608,0x232320f5,0);p.Sub_00144FD0(0x8ec51609,0x232320f6,1);}else p.Sub_00144FD0(0xaefe36d8,0xcccf9a75,0);}
  return true;
 }
 if(!p.highlight||p.screen!=screen)hp2::not_implemented("0x001711F8","source pause submenu callback reached before activation");
 // Debug00170190 original PauseScreenBase handling order.
 tHighlightAnimation(m,p.highlight).HandleEvents(msg,d);if(msg==SCREEN_TICK)p.Sub_00243D80();
 bool changed=false;if(kind==1)changed=p.Sub_001711F8(msg,d,svc);else if(kind==2)changed=p.Sub_001723E0(msg,d,svc);else changed=p.Sub_001706D0(msg,d);
 if(changed){d.Noise=msg==MENUITEM_PAD_LEFT?1:2;m.word(p.highlight+0x20+8,m.word(0x327cd8));}
 if(msg==SCREEN_START_BUTTON){if(PauseMenu(m,m.word(0x3276fc)).MaybeResume(d.Port,svc))d.Noise=7;}
 else if(msg==SCREEN_BACK_BUTTON){p.change(kind==1?0x3276d8:0x3276dc,svc);d.Param=0;}
 else if(msg==SCREEN_TICK){if(kind==3)for(auto& slider:p.sliders)p.Sub_0016D0E8(slider);tHighlightAnimation(m,p.highlight+0x20).Animate();}
 return true;
}
}

namespace port::race {
void PauseSubmenus::annotate(std::vector<hp2::frontend::FeScreen>& screens)const{
 auto& m=impl_->m;
 for(auto& screen:screens)for(const auto& item:impl_->objectives)if(screen.name_hash_0x08==item.screen){
  auto found=std::find_if(screen.widgets.begin(),screen.widgets.end(),[&](const auto& w){return w.name_hash==item.text;});
  // Source00144FD0 draws its explicitly named target even when ordinary
  // serialized visibility is off. Such a target is absent from visible-only
  // snapshots, so copy its authored draw operands without mutating native flags.
  if(found==screen.widgets.end()){
   const auto object=item.object;if(!object)hp2::not_implemented("0x00144FD0","source explicit rectangle draw target disappeared");
   hp2::frontend::FeWidget w;w.type=1;w.chunk_id=hp2::frontend::kTextWidgetId;w.name_hash=item.text;w.class_hash=m.word(object+0xc);w.layer=m.byte(object+0x2a);w.x=m.scalar(object+0x30);w.y=m.scalar(object+0x34);w.scale_x=m.scalar(object+0x38);w.scale_y=m.scalar(object+0x3c);w.texture_hash=m.word(object+0x50);w.text_capacity=m.word(object+0x54);w.text_align=m.word(object+0x58);w.r=m.byte(object+0x5c);w.g=m.byte(object+0x5d);w.b=m.byte(object+0x5e);w.a=m.byte(object+0x5f);screen.widgets.push_back(std::move(w));found=std::prev(screen.widgets.end());
  }
  found->visible=1;found->port_text_rectangle=item.bounds;found->text=item.value;found->string_id_0x60=0;
 }
}
}
